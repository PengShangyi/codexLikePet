#include "resources/PetPackageValidator.h"

#include "pet/PetAtlas.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>

namespace {
void addIssue(PackageValidationResult *result,
              QString code,
              QString message,
              QString path = {},
              PackageIssueSeverity severity = PackageIssueSeverity::Error)
{
    result->issues.append({severity, std::move(code), std::move(message), std::move(path)});
}

QVector<int> parseDurations(const QJsonValue &value)
{
    QVector<int> durations;
    for (const QJsonValue &entry : value.toArray()) {
        durations.append(entry.toInt(-1));
    }
    return durations;
}
}

PackageValidationResult PetPackageValidator::validateDirectory(const QString &directoryPath) const
{
    PackageValidationResult result;
    const QFileInfo rootInfo(directoryPath);
    if (!rootInfo.exists() || !rootInfo.isDir() || rootInfo.isSymLink()) {
        addIssue(&result, QStringLiteral("root.invalid"), QStringLiteral("Pet package root must be a real directory"));
        return result;
    }
    const QString rootPath = rootInfo.canonicalFilePath();
    result.package.rootPath = rootPath;
    validateDirectoryEnvelope(rootPath, &result);

    QJsonObject petObject;
    QString error;
    const QString petManifestPath = QDir(rootPath).filePath(QStringLiteral("pet.json"));
    if (!readJsonObject(petManifestPath, &petObject, &error)) {
        addIssue(&result, QStringLiteral("pet.manifest"), error, QStringLiteral("pet.json"));
        return result;
    }

    result.package.id = petObject.value(QStringLiteral("id")).toString();
    result.package.displayName = petObject.value(QStringLiteral("displayName")).toString().trimmed();
    result.package.description = petObject.value(QStringLiteral("description")).toString().trimmed();
    result.package.spriteSheetPath = petObject.value(QStringLiteral("spritesheetPath")).toString();
    if (result.package.spriteSheetPath.isEmpty()) {
        result.package.spriteSheetPath = QStringLiteral("spritesheet.webp");
    }

    static const QRegularExpression idPattern(QStringLiteral("^[a-z0-9][a-z0-9-]{0,63}$"));
    if (!idPattern.match(result.package.id).hasMatch()) {
        addIssue(&result, QStringLiteral("pet.id"), QStringLiteral("Pet id must use lowercase letters, digits, and hyphens"));
    }
    if (result.package.displayName.isEmpty()) {
        addIssue(&result, QStringLiteral("pet.displayName"), QStringLiteral("displayName must not be empty"));
    }
    if (petObject.value(QStringLiteral("spriteVersionNumber")).toInt() != 2) {
        addIssue(&result, QStringLiteral("pet.version"), QStringLiteral("spriteVersionNumber must be 2"));
    }
    validateAtlas(rootPath, result.package.spriteSheetPath, &result, QStringLiteral("base"));

    const QString potatoPath = QDir(rootPath).filePath(QStringLiteral("potato.json"));
    if (QFileInfo::exists(potatoPath)) {
        QJsonObject potatoObject;
        if (!readJsonObject(potatoPath, &potatoObject, &error)) {
            addIssue(&result, QStringLiteral("potato.manifest"), error, QStringLiteral("potato.json"));
        } else {
            parsePotatoManifest(rootPath, potatoObject, &result);
        }
    }
    return result;
}

bool PetPackageValidator::readJsonObject(const QString &path, QJsonObject *object, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        *error = QStringLiteral("Unable to read %1").arg(QFileInfo(path).fileName());
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        *error = QStringLiteral("Invalid JSON in %1: %2")
                     .arg(QFileInfo(path).fileName(), parseError.errorString());
        return false;
    }
    *object = document.object();
    return true;
}

bool PetPackageValidator::isSafeRelativePath(const QString &rootPath,
                                             const QString &relativePath,
                                             QString *error)
{
    if (relativePath.isEmpty() || QDir::isAbsolutePath(relativePath)) {
        *error = QStringLiteral("Path must be non-empty and relative");
        return false;
    }
    const QString clean = QDir::cleanPath(relativePath);
    if (clean == QStringLiteral("..") || clean.startsWith(QStringLiteral("../"))) {
        *error = QStringLiteral("Path escapes the package root");
        return false;
    }
    const QFileInfo info(QDir(rootPath).filePath(clean));
    if (!info.exists() || !info.isFile() || info.isSymLink()) {
        *error = QStringLiteral("Referenced file does not exist or is a symbolic link");
        return false;
    }
    const QString canonical = info.canonicalFilePath();
    const QString rootPrefix = rootPath + QDir::separator();
    if (!canonical.startsWith(rootPrefix)) {
        *error = QStringLiteral("Referenced file resolves outside the package root");
        return false;
    }
    return true;
}

bool PetPackageValidator::isKnownVariantKey(const QString &key)
{
    static const QRegularExpression pattern(
        QStringLiteral("^(spring|summer|autumn|winter|day|night|(spring|summer|autumn|winter)-(day|night))$"));
    return pattern.match(key).hasMatch();
}

bool PetPackageValidator::isKnownClipKey(const QString &key)
{
    return key == QStringLiteral("click") || key == QStringLiteral("typing")
        || key == QStringLiteral("edge-left") || key == QStringLiteral("edge-right")
        || key == QStringLiteral("edge-bottom");
}

void PetPackageValidator::parsePotatoManifest(const QString &rootPath,
                                              const QJsonObject &object,
                                              PackageValidationResult *result) const
{
    if (object.value(QStringLiteral("schemaVersion")).toInt() != 1) {
        addIssue(result, QStringLiteral("potato.version"), QStringLiteral("potato.json schemaVersion must be 1"));
    }

    const QString renderMode = object.value(QStringLiteral("renderMode")).toString(QStringLiteral("smooth"));
    if (renderMode == QStringLiteral("nearest")) {
        result->package.renderMode = RenderMode::Nearest;
    } else if (renderMode == QStringLiteral("smooth")) {
        result->package.renderMode = RenderMode::Smooth;
    } else {
        addIssue(result, QStringLiteral("potato.renderMode"), QStringLiteral("renderMode must be smooth or nearest"));
    }

    const QJsonObject variants = object.value(QStringLiteral("variants")).toObject();
    for (auto iterator = variants.begin(); iterator != variants.end(); ++iterator) {
        if (!isKnownVariantKey(iterator.key())) {
            addIssue(result,
                     QStringLiteral("variant.key"),
                     QStringLiteral("Unknown variant key: %1").arg(iterator.key()));
            continue;
        }
        const QString path = iterator.value().toString();
        result->package.variants.insert(iterator.key(), path);
        validateAtlas(rootPath, path, result, QStringLiteral("variant %1").arg(iterator.key()));
    }

    parseClips(rootPath,
               object.value(QStringLiteral("clips")).toObject(),
               &result->package.clips,
               result,
               QStringLiteral("base"));

    const QJsonObject variantClips = object.value(QStringLiteral("variantClips")).toObject();
    for (auto iterator = variantClips.begin(); iterator != variantClips.end(); ++iterator) {
        if (!isKnownVariantKey(iterator.key())) {
            addIssue(result,
                     QStringLiteral("variantClip.key"),
                     QStringLiteral("Unknown variant clip key: %1").arg(iterator.key()));
            continue;
        }
        auto &destination = result->package.variantClips[iterator.key()];
        parseClips(rootPath,
                   iterator.value().toObject(),
                   &destination,
                   result,
                   QStringLiteral("variant %1").arg(iterator.key()));
    }
}

void PetPackageValidator::parseClips(const QString &rootPath,
                                     const QJsonObject &object,
                                     QHash<QString, ClipDefinition> *clips,
                                     PackageValidationResult *result,
                                     const QString &context) const
{
    for (auto iterator = object.begin(); iterator != object.end(); ++iterator) {
        if (!isKnownClipKey(iterator.key())) {
            addIssue(result,
                     QStringLiteral("clip.key"),
                     QStringLiteral("Unknown clip key: %1").arg(iterator.key()));
            continue;
        }
        const QJsonObject definition = iterator.value().toObject();
        ClipDefinition clip{definition.value(QStringLiteral("path")).toString(),
                            parseDurations(definition.value(QStringLiteral("durationsMs")))};
        clips->insert(iterator.key(), clip);
        validateClip(rootPath, iterator.key(), clip, result, context);
    }
}

void PetPackageValidator::validateAtlas(const QString &rootPath,
                                        const QString &relativePath,
                                        PackageValidationResult *result,
                                        const QString &context) const
{
    QString error;
    if (!isSafeRelativePath(rootPath, relativePath, &error)) {
        addIssue(result,
                 QStringLiteral("atlas.path"),
                 QStringLiteral("%1 atlas: %2").arg(context, error),
                 relativePath);
        return;
    }
    PetAtlas atlas;
    if (!atlas.load(QDir(rootPath).filePath(relativePath))) {
        addIssue(result,
                 QStringLiteral("atlas.decode"),
                 QStringLiteral("%1 atlas: %2").arg(context, atlas.errorString()),
                 relativePath);
        return;
    }
    if (!atlas.validateV2Occupancy(&error)) {
        addIssue(result,
                 QStringLiteral("atlas.cells"),
                 QStringLiteral("%1 atlas: %2").arg(context, error),
                 relativePath);
    }
}

void PetPackageValidator::validateClip(const QString &rootPath,
                                       const QString &name,
                                       const ClipDefinition &clip,
                                       PackageValidationResult *result,
                                       const QString &context) const
{
    QString error;
    if (!isSafeRelativePath(rootPath, clip.path, &error)) {
        addIssue(result,
                 QStringLiteral("clip.path"),
                 QStringLiteral("%1 %2 clip: %3").arg(context, name, error),
                 clip.path);
        return;
    }
    QImageReader reader(QDir(rootPath).filePath(clip.path));
    const QImage image = reader.read();
    if (image.isNull() || !image.hasAlphaChannel() || image.height() != PetAtlas::CellHeight
        || image.width() % PetAtlas::CellWidth != 0) {
        addIssue(result,
                 QStringLiteral("clip.geometry"),
                 QStringLiteral("%1 %2 clip must be transparent and use 192x208 cells")
                     .arg(context, name),
                 clip.path);
        return;
    }
    const int frames = image.width() / PetAtlas::CellWidth;
    if (frames < 1 || frames > 8 || clip.durationsMs.size() != frames) {
        addIssue(result,
                 QStringLiteral("clip.frames"),
                 QStringLiteral("%1 %2 clip frame count and durations do not match")
                     .arg(context, name),
                 clip.path);
        return;
    }
    for (int duration : clip.durationsMs) {
        if (duration < 50 || duration > 2000) {
            addIssue(result,
                     QStringLiteral("clip.duration"),
                     QStringLiteral("%1 %2 clip durations must be 50–2000ms").arg(context, name),
                     clip.path);
            return;
        }
    }
}

void PetPackageValidator::validateDirectoryEnvelope(const QString &rootPath,
                                                    PackageValidationResult *result) const
{
    int entries = 0;
    qint64 totalBytes = 0;
    QDirIterator iterator(rootPath, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
                          QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        iterator.next();
        const QFileInfo info = iterator.fileInfo();
        ++entries;
        if (entries > MaximumEntries) {
            addIssue(result, QStringLiteral("package.entries"), QStringLiteral("Package has more than 256 entries"));
            return;
        }
        if (info.isSymLink()) {
            addIssue(result,
                     QStringLiteral("package.symlink"),
                     QStringLiteral("Symbolic links are not allowed"),
                     QDir(rootPath).relativeFilePath(info.filePath()));
        }
        if (info.isFile()) {
            totalBytes += info.size();
            if (info.permission(QFileDevice::ExeOwner | QFileDevice::ExeGroup | QFileDevice::ExeOther)) {
                addIssue(result,
                         QStringLiteral("package.executable"),
                         QStringLiteral("Executable files are not allowed"),
                         QDir(rootPath).relativeFilePath(info.filePath()));
            }
        }
        if (totalBytes > MaximumExpandedBytes) {
            addIssue(result,
                     QStringLiteral("package.size"),
                     QStringLiteral("Package expands beyond 512MiB"));
            return;
        }
    }
}
