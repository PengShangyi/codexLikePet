#include "resources/PackagePolicy.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

#include <algorithm>

namespace PackagePolicy {

bool hasControlCharacters(QStringView text)
{
    return std::any_of(text.cbegin(), text.cend(), [](QChar character) {
        const ushort value = character.unicode();
        return value < 0x20 || value == 0x7f;
    });
}

bool isPortableRelativePath(const QString &path, QString *cleanPath)
{
    // An embedded NUL is already covered by hasControlCharacters (0x00 < 0x20).
    const bool windowsDrivePath = path.size() >= 2 && path.at(0).isLetter()
        && path.at(1) == QLatin1Char(':');
    if (path.isEmpty() || hasControlCharacters(path) || path.contains(QLatin1Char('\\'))
        || windowsDrivePath || QDir::isAbsolutePath(path)) {
        return false;
    }
    if (cleanPath) *cleanPath = QDir::cleanPath(path);
    return true;
}

bool escapesRoot(const QString &cleanPath)
{
    return cleanPath == QStringLiteral("..") || cleanPath.startsWith(QStringLiteral("../"));
}

bool isAllowedPackageFileName(const QString &path)
{
    const QFileInfo info(path);
    const QString suffix = info.suffix().toLower();
    const QString base = info.fileName().toLower();
    return suffix == QStringLiteral("json") || suffix == QStringLiteral("png")
        || suffix == QStringLiteral("webp") || suffix == QStringLiteral("txt")
        || suffix == QStringLiteral("md") || base.startsWith(QStringLiteral("license"));
}

bool isValidPetId(const QString &id)
{
    static const QRegularExpression pattern(QStringLiteral("^[a-z0-9][a-z0-9-]{0,63}$"));
    return pattern.match(id).hasMatch();
}

QString suggestPetId(const QString &displayName)
{
    QString id;
    id.reserve(displayName.size());
    for (const QChar ch : displayName.toLower()) {
        if ((ch >= QLatin1Char('a') && ch <= QLatin1Char('z'))
            || (ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))) {
            id.append(ch);
        } else if (!id.isEmpty() && !id.endsWith(QLatin1Char('-'))) {
            // One hyphen per run of anything else, and never a leading one -- the id
            // pattern rejects both a leading hyphen and a doubled separator would
            // only read as a typo.
            id.append(QLatin1Char('-'));
        }
    }
    while (id.endsWith(QLatin1Char('-'))) id.chop(1);
    // The pattern allows 64 characters total.
    if (id.size() > 64) {
        id.truncate(64);
        while (id.endsWith(QLatin1Char('-'))) id.chop(1);
    }
    return id;
}

}  // namespace PackagePolicy
