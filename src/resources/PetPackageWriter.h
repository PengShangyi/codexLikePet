#pragma once

#include <QImage>
#include <QMetaType>
#include <QString>

// Writes a minimal pet package that PetPackageValidator accepts: one manifest and
// one spritesheet, and nothing else.
//
// The write-side twin of PetPackageValidator, and in the same library on purpose --
// the manifest key spellings belong next to the code that reads them, or they drift.
namespace PetPackageWriter {

// A distinct type rather than a bool, because the call already carries an image and
// a struct and `write(dir, atlas, info, true, &error)` says nothing.
enum class SpritesheetFormat { Webp, Png };

struct PetInfo {
    QString id;
    QString displayName;
    QString description;  // optional; omitted from the manifest when empty
};

// Webp unless the deployed bundle is missing the qwebp plugin, in which case Png.
// Both validate and install identically -- PackagePolicy allows either suffix and
// the validator reads the path out of the manifest -- so the fallback costs only
// file size. scripts/package-local.sh documents a plugin-prune insertion point; if
// anyone implements it, imageformats/libqwebp.dylib has to survive.
SpritesheetFormat preferredFormat();

QString spritesheetName(SpritesheetFormat format);

// Writes exactly two files into `directory`, which must exist and be empty.
//
// Empty is a hard requirement rather than politeness. Anything already in there is
// an unreferenced file, and the validator rejects those outright
// (package.unknownFile) -- so pointing this at the folder holding the eleven source
// strips would produce a package that cannot install. A .DS_Store is worse still:
// it has no suffix, so it trips the file-type rule instead. Callers should write
// into a QTemporaryDir and hand that straight to the importer.
bool write(const QString &directory,
           const QImage &atlas,
           const PetInfo &info,
           SpritesheetFormat format,
           QString *error);

}  // namespace PetPackageWriter

// Travels through AtlasAssemblerWindow::installRequested, and QSignalSpy stores signal
// arguments as QVariants -- without this the assembler's test records an empty one.
Q_DECLARE_METATYPE(PetPackageWriter::PetInfo)
