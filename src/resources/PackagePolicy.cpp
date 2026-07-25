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

}  // namespace PackagePolicy
