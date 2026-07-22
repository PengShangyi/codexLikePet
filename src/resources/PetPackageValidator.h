#pragma once

#include "resources/PetPackage.h"

#include <QJsonObject>

class PetPackageValidator final
{
public:
    static constexpr int MaximumEntries = 256;
    static constexpr qint64 MaximumExpandedBytes = 512LL * 1024 * 1024;
    static constexpr qint64 MaximumManifestBytes = 1024 * 1024;

    PackageValidationResult validateDirectory(const QString &directoryPath) const;

private:
    static bool readJsonObject(const QString &path, QJsonObject *object, QString *error);
    static bool isSafeRelativePath(const QString &rootPath,
                                   const QString &relativePath,
                                   QString *error);
    static bool isKnownVariantKey(const QString &key);
    static bool isKnownClipKey(const QString &key);

    void parsePotatoManifest(const QString &rootPath,
                             const QJsonObject &object,
                             PackageValidationResult *result) const;
    void parseClips(const QString &rootPath,
                    const QJsonObject &object,
                    QHash<QString, ClipDefinition> *clips,
                    PackageValidationResult *result,
                    const QString &context) const;
    void validateAtlas(const QString &rootPath,
                       const QString &relativePath,
                       PackageValidationResult *result,
                       const QString &context) const;
    void validateClip(const QString &rootPath,
                      const QString &name,
                      const ClipDefinition &clip,
                      PackageValidationResult *result,
                      const QString &context) const;
    void validateDirectoryEnvelope(const QString &rootPath,
                                   PackageValidationResult *result) const;
    void validateKnownFiles(const QString &rootPath,
                            const PetPackage &package,
                            bool hasPotatoManifest,
                            PackageValidationResult *result) const;
};
