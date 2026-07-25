#include "pet/PetAtlas.h"
#include "resources/PetPackageImporter.h"
#include "resources/PetPackageValidator.h"
#include "resources/ArchiveExtractor.h"
#include "support/AtlasFixture.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>

#include <miniz.h>

class PackageImporterTest final : public QObject
{
    Q_OBJECT

    static bool createPackage(const QString &root)
    {
        if (!TestAtlas::writeValid(QDir(root).filePath(QStringLiteral("spritesheet.png")))) {
            return false;
        }
        QFile manifest(QDir(root).filePath(QStringLiteral("pet.json")));
        if (!manifest.open(QIODevice::WriteOnly)) {
            return false;
        }
        const QJsonObject object{{QStringLiteral("id"), QStringLiteral("safe-pet")},
                                 {QStringLiteral("displayName"), QStringLiteral("Safe Pet")},
                                 {QStringLiteral("description"), QStringLiteral("Test")},
                                 {QStringLiteral("spriteVersionNumber"), 2},
                                 {QStringLiteral("spritesheetPath"), QStringLiteral("spritesheet.png")}};
        return manifest.write(QJsonDocument(object).toJson()) > 0;
    }

    static bool writeArchive(const QString &archivePath,
                             const QString &packageRoot,
                             const QString &unsafeEntry = {})
    {
        mz_zip_archive archive{};
        const QByteArray encoded = QFile::encodeName(archivePath);
        if (!mz_zip_writer_init_file(&archive, encoded.constData(), 0)) {
            return false;
        }
        const QByteArray manifestPath = QFile::encodeName(QDir(packageRoot).filePath(QStringLiteral("pet.json")));
        const QByteArray atlasPath = QFile::encodeName(QDir(packageRoot).filePath(QStringLiteral("spritesheet.png")));
        bool ok = mz_zip_writer_add_file(&archive, "pet.json", manifestPath.constData(), nullptr, 0, MZ_BEST_SPEED)
            && mz_zip_writer_add_file(&archive,
                                      "spritesheet.png",
                                      atlasPath.constData(),
                                      nullptr,
                                      0,
                                      MZ_BEST_SPEED);
        if (ok && !unsafeEntry.isEmpty()) {
            static constexpr char payload[] = "bad";
            const QByteArray entryName = unsafeEntry.toUtf8();
            ok = mz_zip_writer_add_mem(&archive,
                                       entryName.constData(),
                                       payload,
                                       sizeof(payload),
                                       MZ_BEST_SPEED);
        }
        ok = ok && mz_zip_writer_finalize_archive(&archive);
        mz_zip_writer_end(&archive);
        return ok;
    }

private slots:
    void importsAValidatedArchiveAtomically()
    {
        QTemporaryDir source;
        QTemporaryDir destination;
        QVERIFY(source.isValid());
        QVERIFY(destination.isValid());
        QVERIFY(createPackage(source.path()));
        const QString archive = source.filePath(QStringLiteral("safe.potatopet"));
        QVERIFY(writeArchive(archive, source.path()));

        PetPackageImporter importer(PetStore(destination.path()));
        const PetImportResult result = importer.importPath(archive);
        QVERIFY2(result.success, qPrintable(result.error));
        QVERIFY(QFileInfo::exists(QDir(result.installedPath).filePath(QStringLiteral("pet.json"))));
        QVERIFY(QFileInfo::exists(QDir(result.installedPath).filePath(QStringLiteral("spritesheet.png"))));
    }

    void rejectsArchiveTraversalWithoutInstalling()
    {
        QTemporaryDir source;
        QTemporaryDir destination;
        QVERIFY(createPackage(source.path()));
        const QString archive = source.filePath(QStringLiteral("unsafe.potatopet"));
        QVERIFY(writeArchive(archive, source.path(), QStringLiteral("../escape.txt")));

        PetPackageImporter importer(PetStore(destination.path()));
        const PetImportResult result = importer.importPath(archive);
        QVERIFY(!result.success);
        QVERIFY(result.error.contains(QStringLiteral("Unsafe archive path")));
        QCOMPARE(QDir(destination.path()).entryList(QDir::Dirs | QDir::NoDotAndDotDot).size(), 0);
    }

    void rejectsWindowsStyleAbsoluteArchivePaths()
    {
        QTemporaryDir source;
        QTemporaryDir destination;
        QVERIFY(createPackage(source.path()));
        const QString archive = source.filePath(QStringLiteral("drive.potatopet"));
        QVERIFY(writeArchive(archive, source.path(), QStringLiteral("C:/escape.txt")));

        const PetImportResult result = PetPackageImporter(PetStore(destination.path()))
                                           .importPath(archive);
        QVERIFY(!result.success);
        QVERIFY(result.error.contains(QStringLiteral("Unsafe archive path")));
    }

    void rejectsArchivePathsWithControlCharacters()
    {
        QTemporaryDir source;
        QTemporaryDir destination;
        QVERIFY(createPackage(source.path()));
        const QString archive = source.filePath(QStringLiteral("control.potatopet"));
        QVERIFY(writeArchive(archive, source.path(), QStringLiteral("bad\nname.txt")));

        const PetImportResult result = PetPackageImporter(PetStore(destination.path()))
                                           .importPath(archive);
        QVERIFY(!result.success);
        QVERIFY(result.error.contains(QStringLiteral("Unsafe archive path")));
    }

    void rejectsAnOversizedArchiveBeforeOpeningIt()
    {
        QTemporaryDir source;
        QTemporaryDir destination;
        const QString archive = source.filePath(QStringLiteral("huge.potatopet"));
        QFile file(archive);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QVERIFY(file.resize(ArchiveExtractor::MaximumArchiveBytes + 1));
        file.close();

        const PetImportResult result = PetPackageImporter(PetStore(destination.path()))
                                           .importPath(archive);
        QVERIFY(!result.success);
        QVERIFY(result.error.contains(QStringLiteral("256MiB")));
    }

    void rejectsArchivesWithTooManyEntries()
    {
        QTemporaryDir source;
        QTemporaryDir destination;
        const QString archivePath = source.filePath(QStringLiteral("many.potatopet"));
        mz_zip_archive archive{};
        const QByteArray encoded = QFile::encodeName(archivePath);
        QVERIFY(mz_zip_writer_init_file(&archive, encoded.constData(), 0));
        static constexpr char payload[] = "x";
        bool ok = true;
        for (int index = 0; index <= ArchiveExtractor::MaximumEntries && ok; ++index) {
            const QByteArray name = QStringLiteral("entry-%1.txt").arg(index).toUtf8();
            ok = mz_zip_writer_add_mem(&archive,
                                       name.constData(),
                                       payload,
                                       sizeof(payload),
                                       MZ_BEST_SPEED);
        }
        ok = ok && mz_zip_writer_finalize_archive(&archive);
        mz_zip_writer_end(&archive);
        QVERIFY(ok);

        const PetImportResult result = PetPackageImporter(PetStore(destination.path()))
                                           .importPath(archivePath);
        QVERIFY(!result.success);
        QVERIFY(result.error.contains(QStringLiteral("more than 256 entries")));
    }

    void restoresTheExistingPetWhenActivationFails()
    {
        QTemporaryDir source;
        QTemporaryDir destination;
        QVERIFY(createPackage(source.path()));
        const PackageValidationResult validation = PetPackageValidator().validateDirectory(source.path());
        QVERIFY(validation.isValid());

        QString installedPath;
        QString error;
        PetStore initialStore(destination.path());
        QVERIFY(initialStore.install(validation, &installedPath, &error));
        QFile marker(QDir(installedPath).filePath(QStringLiteral("existing.txt")));
        QVERIFY(marker.open(QIODevice::WriteOnly));
        QVERIFY(marker.write("keep") > 0);
        marker.close();

        PetStore failingStore(destination.path(), [] { return false; });
        QVERIFY(!failingStore.install(validation, nullptr, &error));
        QVERIFY(error.contains(QStringLiteral("previous pet restored")));
        QVERIFY(QFileInfo::exists(QDir(installedPath).filePath(QStringLiteral("existing.txt"))));
        const QStringList leftovers = QDir(destination.path()).entryList(
            {QStringLiteral(".install-*"), QStringLiteral(".backup-*")},
            QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot);
        QVERIFY(leftovers.isEmpty());
    }

    void neverFollowsAStoreSymlinkDuringUpdateOrRemoval()
    {
        QTemporaryDir source;
        QTemporaryDir destination;
        QTemporaryDir outside;
        QVERIFY(createPackage(source.path()));
        const PackageValidationResult validation = PetPackageValidator().validateDirectory(source.path());
        QVERIFY(validation.isValid());

        QFile marker(outside.filePath(QStringLiteral("keep.txt")));
        QVERIFY(marker.open(QIODevice::WriteOnly));
        QVERIFY(marker.write("keep") > 0);
        marker.close();
        const QString target = destination.filePath(QStringLiteral("safe-pet"));
        QVERIFY(QFile::link(outside.path(), target));

        QString installedPath;
        QString error;
        PetStore store(destination.path());
        QVERIFY2(store.install(validation, &installedPath, &error), qPrintable(error));
        QVERIFY(QFileInfo(installedPath).isDir());
        QVERIFY(!QFileInfo(installedPath).isSymLink());
        QVERIFY(QFileInfo::exists(outside.filePath(QStringLiteral("keep.txt"))));

        QVERIFY(QDir(installedPath).removeRecursively());
        QVERIFY(QFile::link(outside.path(), target));
        QVERIFY2(store.remove(QStringLiteral("safe-pet"), &error), qPrintable(error));
        QVERIFY(!QFileInfo(target).isSymLink());
        QVERIFY(QFileInfo::exists(outside.filePath(QStringLiteral("keep.txt"))));
    }

    void rejectsAStoreRootNestedInsideTheImportSource()
    {
        QTemporaryDir source;
        QVERIFY(source.isValid());
        QVERIFY(createPackage(source.path()));
        const PackageValidationResult validation = PetPackageValidator().validateDirectory(source.path());
        QVERIFY(validation.isValid());

        QString error;
        const QString nestedStore = source.filePath(QStringLiteral("nested-store"));
        QVERIFY(!PetStore(nestedStore).install(validation, nullptr, &error));
        QVERIFY(error.contains(QStringLiteral("inside the imported package")));
        QVERIFY(QDir(nestedStore).entryList(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty());
    }
};

// Runs inside a grouped binary; see tests/support/TestRunner.h.
QObject *createPackageImporterTest() { return new PackageImporterTest; }

#include "test_package_importer.moc"
