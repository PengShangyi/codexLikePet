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
#include <QSet>

#include <algorithm>

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

PackageValidationResult PetPackageValidator::validateDirectory(const QString &directoryPath,
                                                               ValidationDepth depth) const
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
    const QJsonValue spriteSheetValue = petObject.value(QStringLiteral("spritesheetPath"));
    if (!spriteSheetValue.isUndefined() && !spriteSheetValue.isString()) {
        addIssue(&result,
                 QStringLiteral("pet.spritesheetPath"),
                 QStringLiteral("spritesheetPath must be a string"));
    }
    result.package.spriteSheetPath = spriteSheetValue.toString();
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
    validateAtlas(rootPath, result.package.spriteSheetPath, &result, QStringLiteral("base"), depth);

    const QString potatoPath = QDir(rootPath).filePath(QStringLiteral("potato.json"));
    const bool hasPotatoManifest = QFileInfo::exists(potatoPath);
    if (hasPotatoManifest) {
        QJsonObject potatoObject;
        if (!readJsonObject(potatoPath, &potatoObject, &error)) {
            addIssue(&result, QStringLiteral("potato.manifest"), error, QStringLiteral("potato.json"));
        } else {
            parsePotatoManifest(rootPath, potatoObject, &result, depth);
        }
    }
    validateKnownFiles(rootPath, result.package, hasPotatoManifest, &result);
    return result;
}

bool PetPackageValidator::readJsonObject(const QString &path, QJsonObject *object, QString *error)
{
    const QFileInfo info(path);
    if (info.size() > MaximumManifestBytes) {
        *error = QStringLiteral("%1 exceeds the 1MiB manifest limit").arg(info.fileName());
        return false;
    }
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
    const bool windowsDrivePath = relativePath.size() >= 2
        && relativePath.at(0).isLetter() && relativePath.at(1) == QLatin1Char(':');
    const bool hasControlCharacter = std::any_of(relativePath.cbegin(),
                                                  relativePath.cend(),
                                                  [](QChar character) {
                                                      const ushort value = character.unicode();
                                                      return value < 0x20 || value == 0x7f;
                                                  });
    if (relativePath.isEmpty() || relativePath.contains(QChar::Null)
        || hasControlCharacter || relativePath.contains(QLatin1Char('\\')) || windowsDrivePath
        || QDir::isAbsolutePath(relativePath)) {
        *error = QStringLiteral("Path must be a portable, non-empty relative path");
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
                                              PackageValidationResult *result,
                                              ValidationDepth depth) const
{
    if (object.value(QStringLiteral("schemaVersion")).toInt() != 1) {
        addIssue(result, QStringLiteral("potato.version"), QStringLiteral("potato.json schemaVersion must be 1"));
    }

    const QJsonValue renderModeValue = object.value(QStringLiteral("renderMode"));
    if (!renderModeValue.isUndefined() && !renderModeValue.isString()) {
        addIssue(result,
                 QStringLiteral("potato.renderMode"),
                 QStringLiteral("renderMode must be a string"));
    }
    const QString renderMode = renderModeValue.toString(QStringLiteral("smooth"));
    if (renderMode == QStringLiteral("nearest")) {
        result->package.renderMode = RenderMode::Nearest;
    } else if (renderMode == QStringLiteral("smooth")) {
        result->package.renderMode = RenderMode::Smooth;
    } else {
        addIssue(result, QStringLiteral("potato.renderMode"), QStringLiteral("renderMode must be smooth or nearest"));
    }

    const QJsonValue variantsValue = object.value(QStringLiteral("variants"));
    if (!variantsValue.isUndefined() && !variantsValue.isObject()) {
        addIssue(result,
                 QStringLiteral("potato.variants"),
                 QStringLiteral("variants must be an object"));
    }
    const QJsonObject variants = variantsValue.toObject();
    for (auto iterator = variants.begin(); iterator != variants.end(); ++iterator) {
        if (!isKnownVariantKey(iterator.key())) {
            addIssue(result,
                     QStringLiteral("variant.key"),
                     QStringLiteral("Unknown variant key: %1").arg(iterator.key()));
            continue;
        }
        if (!iterator.value().isString()) {
            addIssue(result,
                     QStringLiteral("variant.path"),
                     QStringLiteral("Variant paths must be strings"),
                     iterator.key());
            continue;
        }
        const QString path = iterator.value().toString();
        result->package.variants.insert(iterator.key(), path);
        validateAtlas(rootPath, path, result, QStringLiteral("variant %1").arg(iterator.key()), depth);
    }

    const QJsonValue clipsValue = object.value(QStringLiteral("clips"));
    if (!clipsValue.isUndefined() && !clipsValue.isObject()) {
        addIssue(result,
                 QStringLiteral("potato.clips"),
                 QStringLiteral("clips must be an object"));
    }
    parseClips(rootPath,
               clipsValue.toObject(),
               &result->package.clips,
               result,
               QStringLiteral("base"),
               depth);

    const QJsonValue variantClipsValue = object.value(QStringLiteral("variantClips"));
    if (!variantClipsValue.isUndefined() && !variantClipsValue.isObject()) {
        addIssue(result,
                 QStringLiteral("potato.variantClips"),
                 QStringLiteral("variantClips must be an object"));
    }
    const QJsonObject variantClips = variantClipsValue.toObject();
    for (auto iterator = variantClips.begin(); iterator != variantClips.end(); ++iterator) {
        if (!isKnownVariantKey(iterator.key())) {
            addIssue(result,
                     QStringLiteral("variantClip.key"),
                     QStringLiteral("Unknown variant clip key: %1").arg(iterator.key()));
            continue;
        }
        if (!iterator.value().isObject()) {
            addIssue(result,
                     QStringLiteral("variantClip.value"),
                     QStringLiteral("Each variantClips value must be an object"),
                     iterator.key());
            continue;
        }
        auto &destination = result->package.variantClips[iterator.key()];
        parseClips(rootPath,
                   iterator.value().toObject(),
                   &destination,
                   result,
                   QStringLiteral("variant %1").arg(iterator.key()),
                   depth);
    }
}

void PetPackageValidator::parseClips(const QString &rootPath,
                                     const QJsonObject &object,
                                     QHash<QString, ClipDefinition> *clips,
                                     PackageValidationResult *result,
                                     const QString &context,
                                     ValidationDepth depth) const
{
    for (auto iterator = object.begin(); iterator != object.end(); ++iterator) {
        if (!isKnownClipKey(iterator.key())) {
            addIssue(result,
                     QStringLiteral("clip.key"),
                     QStringLiteral("Unknown clip key: %1").arg(iterator.key()));
            continue;
        }
        if (!iterator.value().isObject()) {
            addIssue(result,
                     QStringLiteral("clip.definition"),
                     QStringLiteral("Clip definitions must be objects"),
                     iterator.key());
            continue;
        }
        const QJsonObject definition = iterator.value().toObject();
        if (!definition.value(QStringLiteral("path")).isString()
            || !definition.value(QStringLiteral("durationsMs")).isArray()) {
            addIssue(result,
                     QStringLiteral("clip.definition"),
                     QStringLiteral("Clip definitions require a string path and durationsMs array"),
                     iterator.key());
            continue;
        }
        ClipDefinition clip{definition.value(QStringLiteral("path")).toString(),
                            parseDurations(definition.value(QStringLiteral("durationsMs")))};
        clips->insert(iterator.key(), clip);
        validateClip(rootPath, iterator.key(), clip, result, context, depth);
    }
}

void PetPackageValidator::validateAtlas(const QString &rootPath,
                                        const QString &relativePath,
                                        PackageValidationResult *result,
                                        const QString &context,
                                        ValidationDepth depth) const
{
    QString error;
    if (!isSafeRelativePath(rootPath, relativePath, &error)) {
        addIssue(result,
                 QStringLiteral("atlas.path"),
                 QStringLiteral("%1 atlas: %2").arg(context, error),
                 relativePath);
        return;
    }
    if (depth == ValidationDepth::Metadata) return;  // decoding is the Full-only part
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
                                       const QString &context,
                                       ValidationDepth depth) const
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
    reader.setAutoTransform(false);
    const QSize size = reader.size();
    const int frames = size.width() / PetAtlas::CellWidth;
    if (!size.isValid() || size.height() != PetAtlas::CellHeight
        || size.width() % PetAtlas::CellWidth != 0 || frames < 1 || frames > 8) {
        addIssue(result,
                 QStringLiteral("clip.geometry"),
                 QStringLiteral("%1 %2 clip must contain 1 to 8 transparent 192x208 cells")
                     .arg(context, name),
                 clip.path);
        return;
    }
    // Decoding is the Full-only part; the geometry above came from the header.
    if (depth == ValidationDepth::Full) {
        const QImage image = reader.read();
        if (image.isNull() || !image.hasAlphaChannel()) {
            addIssue(result,
                     QStringLiteral("clip.geometry"),
                     QStringLiteral("%1 %2 clip must be transparent and use 192x208 cells")
                         .arg(context, name),
                     clip.path);
            return;
        }
    }
    if (clip.durationsMs.size() != frames) {
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

void PetPackageValidator::validateKnownFiles(const QString &rootPath,
                                             const PetPackage &package,
                                             bool hasPotatoManifest,
                                             PackageValidationResult *result) const
{
    const auto comparisonKey = [](const QString &path) {
        return QDir::cleanPath(path).normalized(QString::NormalizationForm_C).toCaseFolded();
    };
    QSet<QString> knownPaths{comparisonKey(QStringLiteral("pet.json")),
                             comparisonKey(package.spriteSheetPath)};
    if (hasPotatoManifest) knownPaths.insert(comparisonKey(QStringLiteral("potato.json")));
    for (const QString &path : package.variants) knownPaths.insert(comparisonKey(path));
    for (const ClipDefinition &clip : package.clips) knownPaths.insert(comparisonKey(clip.path));
    for (const auto &clips : package.variantClips) {
        for (const ClipDefinition &clip : clips) knownPaths.insert(comparisonKey(clip.path));
    }

    QDirIterator iterator(rootPath, QDir::Files | QDir::NoDotAndDotDot,
                          QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        iterator.next();
        const QFileInfo info = iterator.fileInfo();
        if (info.isSymLink()) continue;
        const QString relative = QDir(rootPath).relativeFilePath(info.filePath());
        if (knownPaths.contains(comparisonKey(relative))) continue;

        const QString base = info.fileName().toLower();
        const QString relativeLower = relative.toLower();
        const QString suffix = info.suffix().toLower();
        const bool textDocument = suffix.isEmpty() || suffix == QStringLiteral("md")
            || suffix == QStringLiteral("txt");
        const bool documentation = textDocument
            && (base == QStringLiteral("readme.md")
                || base == QStringLiteral("readme.txt")
                || base.startsWith(QStringLiteral("license"))
                || base.startsWith(QStringLiteral("notice"))
                || relativeLower.startsWith(QStringLiteral("licenses/")));
        if (!documentation) {
            addIssue(result,
                     QStringLiteral("package.unknownFile"),
                     QStringLiteral("Unreferenced file is not allowed in a pet package"),
                     relative);
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
        const QString relative = QDir(rootPath).relativeFilePath(info.filePath());
        ++entries;
        if (entries > MaximumEntries) {
            addIssue(result, QStringLiteral("package.entries"), QStringLiteral("Package has more than 256 entries"));
            return;
        }
        if (info.isSymLink()) {
            addIssue(result,
                     QStringLiteral("package.symlink"),
                     QStringLiteral("Symbolic links are not allowed"),
                     relative);
        }
        const bool hasControlCharacter = std::any_of(relative.cbegin(),
                                                      relative.cend(),
                                                      [](QChar character) {
                                                          const ushort value = character.unicode();
                                                          return value < 0x20 || value == 0x7f;
                                                      });
        if (hasControlCharacter) {
            addIssue(result,
                     QStringLiteral("package.path"),
                     QStringLiteral("Control characters are not allowed in package paths"));
        }
        if (info.isFile()) {
            totalBytes += info.size();
            const QString suffix = info.suffix().toLower();
            const QString base = info.fileName().toLower();
            const bool allowed = suffix == QStringLiteral("json") || suffix == QStringLiteral("png")
                || suffix == QStringLiteral("webp") || suffix == QStringLiteral("txt")
                || suffix == QStringLiteral("md") || base.startsWith(QStringLiteral("license"));
            if (!allowed) {
                addIssue(result,
                         QStringLiteral("package.fileType"),
                         QStringLiteral("Unsupported file in package"),
                         relative);
            }
            const QFileDevice::Permissions executableBits = QFileDevice::ExeOwner
                | QFileDevice::ExeGroup | QFileDevice::ExeOther;
            if (info.permissions() & executableBits) {
                addIssue(result,
                         QStringLiteral("package.executable"),
                         QStringLiteral("Executable files are not allowed"),
                         relative);
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
