#include "pet/PetAtlas.h"
#include "resources/PetPackageImporter.h"

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
        QImage image(PetAtlas::Width, PetAtlas::Height, QImage::Format_RGBA8888);
        image.fill(Qt::transparent);
        for (int row = 0; row < PetAtlas::Rows; ++row) {
            const int columns = row <= 8
                ? PetAtlas::animationSpec(static_cast<V2AnimationState>(row)).frameCount
                : 8;
            for (int column = 0; column < columns; ++column) {
                image.setPixelColor(column * PetAtlas::CellWidth + 1,
                                    row * PetAtlas::CellHeight + 1,
                                    Qt::white);
            }
        }
        if (!image.save(QDir(root).filePath(QStringLiteral("spritesheet.png")))) {
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
                             bool includeTraversal)
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
        if (ok && includeTraversal) {
            static constexpr char payload[] = "bad";
            ok = mz_zip_writer_add_mem(&archive, "../escape.txt", payload, sizeof(payload), MZ_BEST_SPEED);
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
        QVERIFY(writeArchive(archive, source.path(), false));

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
        QVERIFY(writeArchive(archive, source.path(), true));

        PetPackageImporter importer(PetStore(destination.path()));
        const PetImportResult result = importer.importPath(archive);
        QVERIFY(!result.success);
        QVERIFY(result.error.contains(QStringLiteral("Unsafe archive path")));
        QCOMPARE(QDir(destination.path()).entryList(QDir::Dirs | QDir::NoDotAndDotDot).size(), 0);
    }
};

QTEST_GUILESS_MAIN(PackageImporterTest)

#include "test_package_importer.moc"
