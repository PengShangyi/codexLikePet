#pragma once

#include "resources/PetPackage.h"

#include <QJsonObject>

#include <optional>

// How much of a package to check. Both depths enforce every *safety* rule --
// the directory envelope (file-type allowlist, symlinks, executable bits, entry
// and size caps), manifest shape, relative-path safety including symlink escape,
// unreferenced files, and clip geometry/durations read from image headers.
//
// Full additionally decodes every atlas and clip to check pixel content
// (v2 cell occupancy, alpha channel). That decode is the entire cost of
// validation: for the built-in potato pet, 8 atlases at 23ms each against 0.18ms
// for the occupancy scan that follows one. So it runs at the trust boundary
// (import, and the post-copy recheck) rather than on every launch. Listing an
// already-installed pet uses Metadata; a decode problem that slips past it still
// surfaces at load time through PetAtlas/AnimationClip::load, which report
// through AppController's pet-load error path.
enum class ValidationDepth {
    Metadata,
    Full,
};

class PetPackageValidator final
{
public:
    static constexpr int MaximumEntries = 256;
    static constexpr qint64 MaximumExpandedBytes = 512LL * 1024 * 1024;
    static constexpr qint64 MaximumManifestBytes = 1024 * 1024;

    PackageValidationResult validateDirectory(const QString &directoryPath,
                                              ValidationDepth depth = ValidationDepth::Full) const;

private:
    // One deferred pixel-level check.
    //
    // The sequential walk records these instead of running them, for two reasons.
    // Decoding is ~99% of Full validation and the files are independent and only
    // read, so the decodes can run concurrently. And recording them in walk order
    // means the report is assembled in that order afterwards, so which thread
    // finished first cannot change the messages a user sees.
    struct DeepCheck {
        bool isAtlas = false;
        QString absolutePath;
        QString relativePath;
        QString context;    // "base", "variant winter-day"
        QString clipName;   // empty for atlases
    };

    // Where the walk records its deferred checks. Null means Metadata depth --
    // nowhere to put a decode job, so the walk structurally cannot run one.
    using DeepCheckSink = QList<DeepCheck>;

    static bool readJsonObject(const QString &path, QJsonObject *object, QString *error);
    static bool isSafeRelativePath(const QString &rootPath,
                                   const QString &relativePath,
                                   QString *error);
    static bool isKnownVariantKey(const QString &key);
    static bool isKnownClipKey(const QString &key);

    static std::optional<PackageIssue> runDeepCheck(const DeepCheck &check);
    static void runDeepChecks(const DeepCheckSink &checks, PackageValidationResult *result);

    void parsePotatoManifest(const QString &rootPath,
                             const QJsonObject &object,
                             PackageValidationResult *result,
                             DeepCheckSink *deep) const;
    void parseClips(const QString &rootPath,
                    const QJsonObject &object,
                    QHash<QString, ClipDefinition> *clips,
                    PackageValidationResult *result,
                    const QString &context,
                    DeepCheckSink *deep) const;
    void validateAtlas(const QString &rootPath,
                       const QString &relativePath,
                       PackageValidationResult *result,
                       const QString &context,
                       DeepCheckSink *deep) const;
    void validateClip(const QString &rootPath,
                      const QString &name,
                      const ClipDefinition &clip,
                      PackageValidationResult *result,
                      const QString &context,
                      DeepCheckSink *deep) const;
    void validateDirectoryEnvelope(const QString &rootPath,
                                   PackageValidationResult *result) const;
    void validateKnownFiles(const QString &rootPath,
                            const PetPackage &package,
                            bool hasPotatoManifest,
                            PackageValidationResult *result) const;
};
