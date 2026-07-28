#include "pet/AtlasComposer.h"
#include "pet/PetAtlas.h"
#include "resources/PackagePolicy.h"
#include "resources/PetPackage.h"
#include "resources/PetPackageValidator.h"
#include "resources/PetPackageWriter.h"
#include "resources/PetStore.h"

#include <QColorSpace>
#include <QDir>
#include <QFile>
#include <QImageWriter>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QTemporaryDir>
#include <QTest>

namespace {

QImage makeStrip(int count)
{
    const int cellH = PetAtlas::CellHeight;
    const int cellW = PetAtlas::CellWidth;
    QImage strip(cellW * count, cellH, QImage::Format_ARGB32);
    strip.fill(QColor(0x00, 0xB1, 0x40));
    QPainter painter(&strip);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    for (int i = 0; i < count; ++i) {
        painter.fillRect(i * cellW + (cellW - 60) / 2, cellH - 100, 60, 90, QColor(200, 40, 40));
    }
    painter.end();
    return strip;
}

// The composed article, so these tests exercise the real pipeline output rather than
// a hand-built image that might satisfy rules the composer does not.
QImage composedAtlas()
{
    QVector<AtlasComposer::RowInput> inputs;
    for (const AtlasComposer::RowSpec &spec : AtlasComposer::rows()) {
        inputs.append({spec.row, makeStrip(AtlasComposer::frameCount(spec.row))});
    }
    const AtlasComposer::Result result = AtlasComposer::compose(inputs, {});
    return result.isInstallable() ? result.atlas : QImage();
}

PetPackageWriter::PetInfo info()
{
    return {QStringLiteral("my-pet"), QStringLiteral("My Pet"), QStringLiteral("A short description.")};
}

}  // namespace

class PetPackageWriterTest final : public QObject
{
    Q_OBJECT

private slots:
    // Throwaway: round-trip the real shipped atlas through the composer.
    void writesExactlyTheTwoFilesTheManifestReferences()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QString error;
        QVERIFY2(PetPackageWriter::write(temp.path(), composedAtlas(), info(),
                                         PetPackageWriter::preferredFormat(), &error),
                 qPrintable(error));

        QStringList names = QDir(temp.path()).entryList(QDir::AllEntries | QDir::NoDotAndDotDot
                                                        | QDir::Hidden | QDir::System);
        names.sort();
        QCOMPARE(names, QStringList({QStringLiteral("pet.json"),
                                     PetPackageWriter::spritesheetName(
                                         PetPackageWriter::preferredFormat())}));
    }

    // The point of the whole stage: compose, write, and have the real validator
    // accept it at the depth import uses. If this passes, the pipeline works with no
    // UI attached to it at all.
    void theWrittenPackagePassesFullValidationAndInstalls()
    {
        QTemporaryDir source;
        QTemporaryDir petsRoot;
        QVERIFY(source.isValid());
        QVERIFY(petsRoot.isValid());

        QString error;
        QVERIFY2(PetPackageWriter::write(source.path(), composedAtlas(), info(),
                                         PetPackageWriter::preferredFormat(), &error),
                 qPrintable(error));

        const PackageValidationResult validation =
            PetPackageValidator().validateDirectory(source.path(), ValidationDepth::Full);
        QVERIFY2(validation.isValid(),
                 qPrintable(validation.errorMessages().join(QLatin1Char('\n'))));
        QCOMPARE(validation.package.id, QStringLiteral("my-pet"));
        QCOMPARE(validation.package.displayName, QStringLiteral("My Pet"));

        // And through the store, which re-validates at Full depth after copying.
        PetStore store(petsRoot.path());
        QString installedPath;
        QVERIFY2(store.install(validation, &installedPath, &error), qPrintable(error));
        QVERIFY(QFile::exists(QDir(installedPath).filePath(QStringLiteral("pet.json"))));

        // The installed copy loads as a real atlas, occupancy and all.
        PetAtlas atlas;
        const QString sheet = QDir(installedPath).filePath(
            PetPackageWriter::spritesheetName(PetPackageWriter::preferredFormat()));
        QVERIFY2(atlas.load(sheet), qPrintable(atlas.errorString()));
        QVERIFY2(atlas.validateV2Occupancy(&error), qPrintable(error));
    }

    void refusesADestinationThatWouldProduceUnreferencedFiles()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        // A stray file is exactly what tripping package.unknownFile looks like at
        // import, so it is refused here where the message can name the cause.
        QFile stray(QDir(temp.path()).filePath(QStringLiteral("idle.png")));
        QVERIFY(stray.open(QIODevice::WriteOnly));
        stray.write("x");
        stray.close();

        QString error;
        QVERIFY(!PetPackageWriter::write(temp.path(), composedAtlas(), info(),
                                        PetPackageWriter::preferredFormat(), &error));
        QVERIFY(error.contains(QStringLiteral("not empty")));

        // And a destination that does not exist at all.
        QVERIFY(!PetPackageWriter::write(QDir(temp.path()).filePath(QStringLiteral("nope")),
                                        composedAtlas(), info(),
                                        PetPackageWriter::preferredFormat(), &error));
        QVERIFY(!error.isEmpty());
    }

    void refusesTheThreeThingsTheValidatorWouldRejectAnyway()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QImage atlas = composedAtlas();
        QString error;

        for (const QString &badId : {QStringLiteral(""), QStringLiteral("-lead"),
                                     QStringLiteral("Caps"), QStringLiteral("has space")}) {
            PetPackageWriter::PetInfo bad = info();
            bad.id = badId;
            QVERIFY2(!PetPackageWriter::write(temp.path(), atlas, bad,
                                             PetPackageWriter::preferredFormat(), &error),
                     qPrintable(QStringLiteral("accepted id \"%1\"").arg(badId)));
        }

        PetPackageWriter::PetInfo noName = info();
        noName.displayName = QStringLiteral("   ");
        QVERIFY(!PetPackageWriter::write(temp.path(), atlas, noName,
                                        PetPackageWriter::preferredFormat(), &error));

        QVERIFY(!PetPackageWriter::write(temp.path(), QImage(), info(),
                                        PetPackageWriter::preferredFormat(), &error));

        // Nothing was left behind by any of those failures.
        QVERIFY(QDir(temp.path()).isEmpty());
    }

    void theManifestParsesWithSpriteVersionTwo()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QString error;
        QVERIFY(PetPackageWriter::write(temp.path(), composedAtlas(), info(),
                                        PetPackageWriter::preferredFormat(), &error));

        QFile file(QDir(temp.path()).filePath(QStringLiteral("pet.json")));
        QVERIFY(file.open(QIODevice::ReadOnly));
        QJsonParseError parse{};
        const QJsonObject manifest = QJsonDocument::fromJson(file.readAll(), &parse).object();
        QCOMPARE(parse.error, QJsonParseError::NoError);

        QCOMPARE(manifest.value(QStringLiteral("spriteVersionNumber")).toInt(), 2);
        QCOMPARE(manifest.value(QStringLiteral("id")).toString(), QStringLiteral("my-pet"));
        QCOMPARE(manifest.value(QStringLiteral("displayName")).toString(), QStringLiteral("My Pet"));
        QCOMPARE(manifest.value(QStringLiteral("spritesheetPath")).toString(),
                 PetPackageWriter::spritesheetName(PetPackageWriter::preferredFormat()));
        // No potato.json is written, so nothing claims a schema version for one.
        QVERIFY(!QFile::exists(QDir(temp.path()).filePath(QStringLiteral("potato.json"))));
    }

    void anEmptyDescriptionIsOmittedRatherThanWrittenBlank()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        PetPackageWriter::PetInfo bare = info();
        bare.description = QStringLiteral("  ");
        QString error;
        QVERIFY(PetPackageWriter::write(temp.path(), composedAtlas(), bare,
                                        PetPackageWriter::preferredFormat(), &error));

        QFile file(QDir(temp.path()).filePath(QStringLiteral("pet.json")));
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QJsonObject manifest = QJsonDocument::fromJson(file.readAll()).object();
        QVERIFY(!manifest.contains(QStringLiteral("description")));
        // Still valid without it: the validator reads description and never requires it.
        QVERIFY(PetPackageValidator().validateDirectory(temp.path(), ValidationDepth::Full).isValid());
    }

    // WebP at quality 100 must be genuinely lossless, or the composer's cleaned cell
    // edges come back fringed. Asserted two ways, because a round trip alone could
    // pass by luck on a simple image: the file's fourth chunk tag is VP8L, which *is*
    // the lossless bitstream, and the pixels come back identical.
    void webpAtQualityOneHundredIsLossless()
    {
        if (!QImageWriter::supportedImageFormats().contains(QByteArrayLiteral("webp"))) {
            QSKIP("the qwebp plugin is not available in this build");
        }

        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QImage atlas = composedAtlas();
        QVERIFY(!atlas.isNull());
        QString error;
        QVERIFY2(PetPackageWriter::write(temp.path(), atlas, info(),
                                         PetPackageWriter::SpritesheetFormat::Webp, &error),
                 qPrintable(error));

        const QString path = QDir(temp.path()).filePath(QStringLiteral("spritesheet.webp"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QByteArray header = file.read(16);
        file.close();
        QCOMPARE(header.mid(0, 4), QByteArrayLiteral("RIFF"));
        QCOMPARE(header.mid(8, 4), QByteArrayLiteral("WEBP"));
        QCOMPARE(header.mid(12, 4), QByteArrayLiteral("VP8L"));

        QImage reloaded(path);
        QVERIFY(!reloaded.isNull());
        QCOMPARE(reloaded.size(), atlas.size());
        QCOMPARE(reloaded.convertedTo(QImage::Format_ARGB32), atlas);
    }

    // The fallback has to be a real option, not a theoretical one.
    void thePngFallbackAlsoValidatesAndInstalls()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QString error;
        QVERIFY2(PetPackageWriter::write(temp.path(), composedAtlas(), info(),
                                         PetPackageWriter::SpritesheetFormat::Png, &error),
                 qPrintable(error));

        QVERIFY(QFile::exists(QDir(temp.path()).filePath(QStringLiteral("spritesheet.png"))));
        const PackageValidationResult validation =
            PetPackageValidator().validateDirectory(temp.path(), ValidationDepth::Full);
        QVERIFY2(validation.isValid(),
                 qPrintable(validation.errorMessages().join(QLatin1Char('\n'))));
        QCOMPARE(validation.package.spriteSheetPath, QStringLiteral("spritesheet.png"));

        QImage reloaded(QDir(temp.path()).filePath(QStringLiteral("spritesheet.png")));
        QCOMPARE(reloaded.convertedTo(QImage::Format_ARGB32), composedAtlas());
    }

    // A tagged atlas renders differently from the untagged built-in sprites, so the
    // written file must not acquire a colour space on the way out either.
    void theWrittenAtlasStaysUntagged()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QString error;
        QVERIFY(PetPackageWriter::write(temp.path(), composedAtlas(), info(),
                                        PetPackageWriter::preferredFormat(), &error));

        QImage reloaded(QDir(temp.path()).filePath(
            PetPackageWriter::spritesheetName(PetPackageWriter::preferredFormat())));
        QVERIFY(!reloaded.isNull());
        QVERIFY(!reloaded.colorSpace().isValid());
    }

    void suggestPetIdProducesSomethingTheIdRuleAccepts()
    {
        using PackagePolicy::isValidPetId;
        using PackagePolicy::suggestPetId;

        QCOMPARE(suggestPetId(QStringLiteral("My Pet")), QStringLiteral("my-pet"));
        QCOMPARE(suggestPetId(QStringLiteral("  Spud   the   Potato ")),
                 QStringLiteral("spud-the-potato"));
        QCOMPARE(suggestPetId(QStringLiteral("Mr. Fluffy!!")), QStringLiteral("mr-fluffy"));
        QCOMPARE(suggestPetId(QStringLiteral("--leading")), QStringLiteral("leading"));
        QCOMPARE(suggestPetId(QStringLiteral("Robot 9000")), QStringLiteral("robot-9000"));

        for (const QString &name : {QStringLiteral("My Pet"), QStringLiteral("Mr. Fluffy!!"),
                                    QStringLiteral("Robot 9000"), QString(70, QLatin1Char('a')),
                                    QStringLiteral("a b c d e f g h i j k l m n o p q r s t u v w x y"
                                                   " z a b c d e f g h i j")}) {
            const QString id = suggestPetId(name);
            QVERIFY2(isValidPetId(id),
                     qPrintable(QStringLiteral("\"%1\" produced invalid id \"%2\"").arg(name, id)));
        }

        // Nothing usable survives a non-Latin name, and the caller has to ask rather
        // than be handed a guess.
        QCOMPARE(suggestPetId(QStringLiteral("土豆")), QString());
        QCOMPARE(suggestPetId(QStringLiteral("!!!")), QString());
        QVERIFY(!isValidPetId(QString()));
    }
};

// Runs inside a grouped binary; see tests/support/TestRunner.h.
QObject *createPetPackageWriterTest() { return new PetPackageWriterTest; }
#include "test_pet_package_writer.moc"
