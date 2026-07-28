#include "resources/PetPackageWriter.h"

#include "resources/PackagePolicy.h"

#include <QDir>
#include <QFile>
#include <QImageWriter>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

const auto kManifestName = QStringLiteral("pet.json");

bool fail(QString *error, const QString &message)
{
    if (error) *error = message;
    return false;
}

}  // namespace

namespace PetPackageWriter {

SpritesheetFormat preferredFormat()
{
    return QImageWriter::supportedImageFormats().contains(QByteArrayLiteral("webp"))
        ? SpritesheetFormat::Webp
        : SpritesheetFormat::Png;
}

QString spritesheetName(SpritesheetFormat format)
{
    return format == SpritesheetFormat::Webp ? QStringLiteral("spritesheet.webp")
                                             : QStringLiteral("spritesheet.png");
}

bool write(const QString &directory,
           const QImage &atlas,
           const PetInfo &info,
           SpritesheetFormat format,
           QString *error)
{
    if (atlas.isNull()) return fail(error, QStringLiteral("Cannot write an empty atlas"));
    if (!PackagePolicy::isValidPetId(info.id)) {
        return fail(error, QStringLiteral("Pet id must use lowercase letters, digits, and hyphens"));
    }
    if (info.displayName.trimmed().isEmpty()) {
        return fail(error, QStringLiteral("displayName must not be empty"));
    }

    QDir dir(directory);
    if (!dir.exists()) return fail(error, QStringLiteral("Destination directory does not exist"));
    if (!dir.isEmpty(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System)) {
        // See the header: anything already here becomes an unreferenced file the
        // validator refuses, so failing now beats failing at import with a message
        // about a file the user never chose.
        return fail(error, QStringLiteral("Destination directory is not empty"));
    }

    const QString sheetName = spritesheetName(format);
    const QString sheetPath = dir.filePath(sheetName);
    QImageWriter writer(sheetPath, format == SpritesheetFormat::Webp ? QByteArrayLiteral("webp")
                                                                    : QByteArrayLiteral("png"));
    // Quality 100 is what makes Qt's WebP handler select the lossless encoder; below
    // it, the atlas is re-encoded lossily and the cell edges the composer just
    // cleaned come back fringed. Harmless for PNG, which is always lossless.
    writer.setQuality(100);
    if (!writer.write(atlas)) {
        // QImageWriter over QImage::save purely so this message exists.
        return fail(error,
                    QStringLiteral("Could not write %1: %2").arg(sheetName, writer.errorString()));
    }

    QJsonObject manifest;
    manifest.insert(QStringLiteral("id"), info.id);
    manifest.insert(QStringLiteral("displayName"), info.displayName);
    if (!info.description.trimmed().isEmpty()) {
        manifest.insert(QStringLiteral("description"), info.description);
    }
    // A literal 2: the validator compares exactly, and this is the version of the
    // grid AtlasComposer lays out.
    manifest.insert(QStringLiteral("spriteVersionNumber"), 2);
    manifest.insert(QStringLiteral("spritesheetPath"), sheetName);
    // No potato.json. Absent means renderMode defaults to Smooth, which is right for
    // the non-pixel art an image model returns, and it keeps the package to the two
    // files the manifest references.

    QFile file(dir.filePath(kManifestName));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return fail(error, QStringLiteral("Could not write %1: %2").arg(kManifestName, file.errorString()));
    }
    // QJsonObject is key-sorted, so the emitted order is alphabetical rather than the
    // logical order of the sample in the pet guide. That is not worth hand-rolling
    // JSON to fix. toJson() already ends with a newline.
    if (file.write(QJsonDocument(manifest).toJson(QJsonDocument::Indented)) < 0) {
        return fail(error, QStringLiteral("Could not write %1: %2").arg(kManifestName, file.errorString()));
    }
    file.close();
    return true;
}

}  // namespace PetPackageWriter
