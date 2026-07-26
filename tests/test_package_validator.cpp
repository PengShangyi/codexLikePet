#include "pet/PetAtlas.h"
#include "resources/PetPackageValidator.h"
#include "support/AtlasFixture.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>

class PackageValidatorTest final : public QObject
{
    Q_OBJECT

    static bool writeJson(const QString &path, const QJsonObject &object)
    {
        QFile file(path);
        return file.open(QIODevice::WriteOnly)
            && file.write(QJsonDocument(object).toJson()) > 0;
    }

    static bool writeValidAtlas(const QString &path) { return TestAtlas::writeValid(path); }

    static QJsonObject validPetManifest()
    {
        return {{QStringLiteral("id"), QStringLiteral("potato-test")},
                {QStringLiteral("displayName"), QStringLiteral("Potato Test")},
                {QStringLiteral("description"), QStringLiteral("A test pet")},
                {QStringLiteral("spriteVersionNumber"), 2},
                {QStringLiteral("spritesheetPath"), QStringLiteral("spritesheet.png")}};
    }

private slots:
    void acceptsAValidExtendedPackage()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(writeValidAtlas(temp.filePath(QStringLiteral("spritesheet.png"))));
        QVERIFY(writeValidAtlas(temp.filePath(QStringLiteral("winter-night.png"))));
        QImage clip(384, 208, QImage::Format_RGBA8888);
        clip.fill(Qt::transparent);
        clip.setPixelColor(10, 10, Qt::white);
        clip.setPixelColor(202, 10, Qt::white);
        QVERIFY(clip.save(temp.filePath(QStringLiteral("click.png"))));
        QVERIFY(writeJson(temp.filePath(QStringLiteral("pet.json")), validPetManifest()));
        QVERIFY(writeJson(temp.filePath(QStringLiteral("potato.json")),
                          {{QStringLiteral("schemaVersion"), 1},
                           {QStringLiteral("renderMode"), QStringLiteral("smooth")},
                           {QStringLiteral("variants"),
                            QJsonObject{{QStringLiteral("winter-night"),
                                         QStringLiteral("winter-night.png")}}},
                           {QStringLiteral("clips"),
                            QJsonObject{{QStringLiteral("click"),
                                         QJsonObject{{QStringLiteral("path"), QStringLiteral("click.png")},
                                                     {QStringLiteral("durationsMs"),
                                                      QJsonArray{120, 220}}}}}}}));

        const PackageValidationResult result = PetPackageValidator().validateDirectory(temp.path());
        QVERIFY2(result.isValid(), qPrintable(result.errorMessages().join('\n')));
        QCOMPARE(result.package.id, QStringLiteral("potato-test"));
        QCOMPARE(result.package.variants.value(QStringLiteral("winter-night")),
                 QStringLiteral("winter-night.png"));
    }

    void rejectsTraversalAndWrongVersion()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QJsonObject manifest = validPetManifest();
        manifest[QStringLiteral("spriteVersionNumber")] = 1;
        manifest[QStringLiteral("spritesheetPath")] = QStringLiteral("../outside.png");
        QVERIFY(writeJson(temp.filePath(QStringLiteral("pet.json")), manifest));
        const PackageValidationResult result = PetPackageValidator().validateDirectory(temp.path());
        QVERIFY(!result.isValid());
        QVERIFY(result.errorMessages().join(' ').contains(QStringLiteral("spriteVersionNumber")));
        QVERIFY(result.errorMessages().join(' ').contains(QStringLiteral("escapes")));
    }

    void rejectsOpaqueUnusedCells()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(writeValidAtlas(temp.filePath(QStringLiteral("spritesheet.png"))));
        QImage image(temp.filePath(QStringLiteral("spritesheet.png")));
        image.setPixelColor(7 * PetAtlas::CellWidth + 5, 5, Qt::red);
        QVERIFY(image.save(temp.filePath(QStringLiteral("spritesheet.png"))));
        QVERIFY(writeJson(temp.filePath(QStringLiteral("pet.json")), validPetManifest()));
        const PackageValidationResult result = PetPackageValidator().validateDirectory(temp.path());
        QVERIFY(!result.isValid());
        QVERIFY(result.errorMessages().join(' ').contains(QStringLiteral("Unused cell")));
    }

    void acceptsTheOptionalExtendedNeutralCell()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString atlasPath = temp.filePath(QStringLiteral("spritesheet.png"));
        QVERIFY(writeValidAtlas(atlasPath));
        QImage image(atlasPath);
        image.setPixelColor(6 * PetAtlas::CellWidth + 5, 5, Qt::red);
        QVERIFY(image.save(atlasPath));
        QVERIFY(writeJson(temp.filePath(QStringLiteral("pet.json")), validPetManifest()));

        const PackageValidationResult result = PetPackageValidator().validateDirectory(temp.path());
        QVERIFY2(result.isValid(), qPrintable(result.errorMessages().join('\n')));
    }

    void rejectsUnsupportedExecutableAndSymbolicLinkEntries()
    {
        QTemporaryDir temp;
        QVERIFY(writeValidAtlas(temp.filePath(QStringLiteral("spritesheet.png"))));
        QVERIFY(writeJson(temp.filePath(QStringLiteral("pet.json")), validPetManifest()));
        QFile payload(temp.filePath(QStringLiteral("payload.bin")));
        QVERIFY(payload.open(QIODevice::WriteOnly));
        QVERIFY(payload.write("bad") > 0);
        payload.close();
        QVERIFY(payload.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner
                                       | QFileDevice::ExeOwner));
        QVERIFY(QFile::link(temp.filePath(QStringLiteral("spritesheet.png")),
                            temp.filePath(QStringLiteral("linked.png"))));

        const PackageValidationResult result = PetPackageValidator().validateDirectory(temp.path());
        QVERIFY(!result.isValid());
        const QString errors = result.errorMessages().join(QLatin1Char(' '));
        QVERIFY(errors.contains(QStringLiteral("Unsupported file")));
        QVERIFY(errors.contains(QStringLiteral("Executable")));
        QVERIFY(errors.contains(QStringLiteral("Symbolic links")));
    }

    void rejectsPortableAbsoluteAndUnreferencedResources()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(writeValidAtlas(temp.filePath(QStringLiteral("spritesheet.png"))));
        QJsonObject manifest = validPetManifest();
        manifest[QStringLiteral("spritesheetPath")] = QStringLiteral("C:/spritesheet.png");
        QVERIFY(writeJson(temp.filePath(QStringLiteral("pet.json")), manifest));
        QVERIFY(writeJson(temp.filePath(QStringLiteral("unused.json")), QJsonObject{}));

        PackageValidationResult result = PetPackageValidator().validateDirectory(temp.path());
        QVERIFY(!result.isValid());
        const QString errors = result.errorMessages().join(QLatin1Char(' '));
        QVERIFY(errors.contains(QStringLiteral("portable")));
        QVERIFY(errors.contains(QStringLiteral("Unreferenced file")));

        manifest[QStringLiteral("spritesheetPath")] = QStringLiteral("spritesheet.png");
        QVERIFY(writeJson(temp.filePath(QStringLiteral("pet.json")), manifest));
        QVERIFY(QFile::remove(temp.filePath(QStringLiteral("unused.json"))));
        QFile readme(temp.filePath(QStringLiteral("README.md")));
        QVERIFY(readme.open(QIODevice::WriteOnly));
        QVERIFY(readme.write("author notes") > 0);
        readme.close();
        result = PetPackageValidator().validateDirectory(temp.path());
        QVERIFY2(result.isValid(), qPrintable(result.errorMessages().join('\n')));
    }

    void rejectsClipsLargerThanEightFrames()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(writeValidAtlas(temp.filePath(QStringLiteral("spritesheet.png"))));
        QVERIFY(writeJson(temp.filePath(QStringLiteral("pet.json")), validPetManifest()));
        QImage clip(9 * PetAtlas::CellWidth,
                    PetAtlas::CellHeight,
                    QImage::Format_RGBA8888);
        clip.fill(Qt::transparent);
        QVERIFY(clip.save(temp.filePath(QStringLiteral("too-wide.png"))));
        QJsonArray durations;
        for (int index = 0; index < 9; ++index) durations.append(100);
        QVERIFY(writeJson(temp.filePath(QStringLiteral("potato.json")),
                          {{QStringLiteral("schemaVersion"), 1},
                           {QStringLiteral("clips"),
                            QJsonObject{{QStringLiteral("click"),
                                         QJsonObject{{QStringLiteral("path"),
                                                      QStringLiteral("too-wide.png")},
                                                     {QStringLiteral("durationsMs"), durations}}}}}}));

        const PackageValidationResult result = PetPackageValidator().validateDirectory(temp.path());
        QVERIFY(!result.isValid());
        QVERIFY(result.errorMessages().join(QLatin1Char(' ')).contains(QStringLiteral("1 to 8")));
    }

    void rejectsWrongManifestTypesAndOversizedJson()
    {
        QTemporaryDir wrongTypes;
        QVERIFY(wrongTypes.isValid());
        QVERIFY(writeValidAtlas(wrongTypes.filePath(QStringLiteral("spritesheet.png"))));
        QJsonObject pet = validPetManifest();
        pet[QStringLiteral("spritesheetPath")] = 42;
        QVERIFY(writeJson(wrongTypes.filePath(QStringLiteral("pet.json")), pet));
        QVERIFY(writeJson(wrongTypes.filePath(QStringLiteral("potato.json")),
                          {{QStringLiteral("schemaVersion"), 1},
                           {QStringLiteral("variants"), QJsonArray{QStringLiteral("bad")}}}));
        PackageValidationResult result = PetPackageValidator().validateDirectory(wrongTypes.path());
        QVERIFY(!result.isValid());
        const QString errors = result.errorMessages().join(QLatin1Char(' '));
        QVERIFY(errors.contains(QStringLiteral("spritesheetPath must be a string")));
        QVERIFY(errors.contains(QStringLiteral("variants must be an object")));

        QTemporaryDir oversized;
        QVERIFY(oversized.isValid());
        QFile manifest(oversized.filePath(QStringLiteral("pet.json")));
        QVERIFY(manifest.open(QIODevice::WriteOnly));
        QVERIFY(manifest.resize(PetPackageValidator::MaximumManifestBytes + 1));
        manifest.close();
        result = PetPackageValidator().validateDirectory(oversized.path());
        QVERIFY(!result.isValid());
        QVERIFY(result.errorMessages().join(QLatin1Char(' ')).contains(QStringLiteral("1MiB")));
    }

    // Metadata depth skips only the pixel-level decode. A package whose atlas is
    // structurally referenced but pixel-invalid lists fine (the failure resurfaces
    // at load time) while the Full pass still rejects it.
    void metadataDepthSkipsOnlyThePixelDecode()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString atlasPath = temp.filePath(QStringLiteral("spritesheet.png"));
        QVERIFY(writeValidAtlas(atlasPath));
        QImage image(atlasPath);
        image.setPixelColor(7 * PetAtlas::CellWidth + 5, 5, Qt::red); // opaque unused cell
        QVERIFY(image.save(atlasPath));
        QVERIFY(writeJson(temp.filePath(QStringLiteral("pet.json")), validPetManifest()));

        const PetPackageValidator validator;
        const PackageValidationResult full =
            validator.validateDirectory(temp.path(), ValidationDepth::Full);
        QVERIFY(!full.isValid());
        QVERIFY(full.errorMessages().join(QLatin1Char(' ')).contains(QStringLiteral("Unused cell")));

        const PackageValidationResult metadata =
            validator.validateDirectory(temp.path(), ValidationDepth::Metadata);
        QVERIFY2(metadata.isValid(), qPrintable(metadata.errorMessages().join('\n')));
        // The manifest is still fully parsed, so the library can list the pet.
        QCOMPARE(metadata.package.id, QStringLiteral("potato-test"));
        QCOMPARE(metadata.package.spriteSheetPath, QStringLiteral("spritesheet.png"));
    }

    // Every *safety* rule must hold at Metadata depth too -- this is the guard
    // against the depth split quietly becoming a way to bypass the envelope.
    void metadataDepthStillEnforcesEverySafetyRule()
    {
        const PetPackageValidator validator;

        QTemporaryDir traversal;
        QVERIFY(traversal.isValid());
        QJsonObject escaping = validPetManifest();
        escaping[QStringLiteral("spritesheetPath")] = QStringLiteral("../outside.png");
        QVERIFY(writeJson(traversal.filePath(QStringLiteral("pet.json")), escaping));
        QString errors = validator.validateDirectory(traversal.path(), ValidationDepth::Metadata)
                             .errorMessages()
                             .join(QLatin1Char(' '));
        QVERIFY(errors.contains(QStringLiteral("escapes")));

        QTemporaryDir envelope;
        QVERIFY(envelope.isValid());
        QVERIFY(writeValidAtlas(envelope.filePath(QStringLiteral("spritesheet.png"))));
        QVERIFY(writeJson(envelope.filePath(QStringLiteral("pet.json")), validPetManifest()));
        QFile payload(envelope.filePath(QStringLiteral("payload.bin")));
        QVERIFY(payload.open(QIODevice::WriteOnly));
        QVERIFY(payload.write("bad") > 0);
        payload.close();
        QVERIFY(payload.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner
                                       | QFileDevice::ExeOwner));
        QVERIFY(QFile::link(envelope.filePath(QStringLiteral("spritesheet.png")),
                            envelope.filePath(QStringLiteral("linked.png"))));
        QVERIFY(writeJson(envelope.filePath(QStringLiteral("unused.json")), QJsonObject{}));
        errors = validator.validateDirectory(envelope.path(), ValidationDepth::Metadata)
                     .errorMessages()
                     .join(QLatin1Char(' '));
        QVERIFY(errors.contains(QStringLiteral("Unsupported file")));
        QVERIFY(errors.contains(QStringLiteral("Executable")));
        QVERIFY(errors.contains(QStringLiteral("Symbolic links")));
        QVERIFY(errors.contains(QStringLiteral("Unreferenced file")));

        // Clip geometry and durations come from the image header, not a decode,
        // so they stay enforced at Metadata depth.
        QTemporaryDir clips;
        QVERIFY(clips.isValid());
        QVERIFY(writeValidAtlas(clips.filePath(QStringLiteral("spritesheet.png"))));
        QVERIFY(writeJson(clips.filePath(QStringLiteral("pet.json")), validPetManifest()));
        QImage wide(9 * PetAtlas::CellWidth, PetAtlas::CellHeight, QImage::Format_RGBA8888);
        wide.fill(Qt::transparent);
        QVERIFY(wide.save(clips.filePath(QStringLiteral("too-wide.png"))));
        QJsonArray durations;
        for (int index = 0; index < 9; ++index) durations.append(100);
        QVERIFY(writeJson(clips.filePath(QStringLiteral("potato.json")),
                          {{QStringLiteral("schemaVersion"), 1},
                           {QStringLiteral("clips"),
                            QJsonObject{{QStringLiteral("click"),
                                         QJsonObject{{QStringLiteral("path"),
                                                      QStringLiteral("too-wide.png")},
                                                     {QStringLiteral("durationsMs"), durations}}}}}}));
        errors = validator.validateDirectory(clips.path(), ValidationDepth::Metadata)
                     .errorMessages()
                     .join(QLatin1Char(' '));
        QVERIFY(errors.contains(QStringLiteral("1 to 8")));
    }

    void rejectsDirectoryEntryAndExpandedSizeLimits()
    {
        QTemporaryDir entries;
        for (int index = 0; index <= PetPackageValidator::MaximumEntries; ++index) {
            QVERIFY(QDir().mkpath(entries.filePath(QStringLiteral("entry-%1").arg(index))));
        }
        PackageValidationResult result = PetPackageValidator().validateDirectory(entries.path());
        QVERIFY(!result.isValid());
        QVERIFY(result.errorMessages().join(QLatin1Char(' ')).contains(QStringLiteral("more than 256 entries")));

        QTemporaryDir expanded;
        QFile huge(expanded.filePath(QStringLiteral("huge.txt")));
        QVERIFY(huge.open(QIODevice::WriteOnly));
        QVERIFY(huge.resize(PetPackageValidator::MaximumExpandedBytes + 1));
        huge.close();
        result = PetPackageValidator().validateDirectory(expanded.path());
        QVERIFY(!result.isValid());
        QVERIFY(result.errorMessages().join(QLatin1Char(' ')).contains(QStringLiteral("512MiB")));
    }

    // The pixel-level checks run concurrently, so the order they finish in is not
    // the order they were queued in. The report must not show it: a user
    // comparing two runs of the same broken package has to see the same list,
    // and a test asserting on errorMessages().first() must not flake.
    void concurrentPixelChecksStillReportInAStableOrder()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());

        // Three atlases, each broken the same way (an opaque pixel in a cell the
        // v2 contract requires to be empty), so the report has three
        // indistinguishable-except-for-order entries to get wrong.
        const auto writeBrokenAtlas = [](const QString &path) {
            QImage image(TestAtlas::validImage());
            image.setPixelColor(7 * PetAtlas::CellWidth + 5, 5, Qt::red);
            return image.save(path);
        };
        QVERIFY(writeBrokenAtlas(temp.filePath(QStringLiteral("spritesheet.png"))));
        QVERIFY(writeBrokenAtlas(temp.filePath(QStringLiteral("summer-day.png"))));
        QVERIFY(writeBrokenAtlas(temp.filePath(QStringLiteral("winter-night.png"))));
        QVERIFY(writeJson(temp.filePath(QStringLiteral("pet.json")), validPetManifest()));
        QVERIFY(writeJson(temp.filePath(QStringLiteral("potato.json")),
                          {{QStringLiteral("schemaVersion"), 1},
                           {QStringLiteral("variants"),
                            QJsonObject{{QStringLiteral("summer-day"), QStringLiteral("summer-day.png")},
                                        {QStringLiteral("winter-night"), QStringLiteral("winter-night.png")}}}}));

        const QStringList expected =
            PetPackageValidator().validateDirectory(temp.path()).errorMessages();
        QCOMPARE(expected.size(), 3);
        for (const QString &message : expected) {
            QVERIFY2(message.contains(QStringLiteral("Unused cell")), qPrintable(message));
        }
        // Walk order: the base atlas first, then the variants as the manifest
        // iteration produced them.
        QVERIFY2(expected.first().contains(QStringLiteral("base atlas")),
                 qPrintable(expected.first()));

        for (int run = 0; run < 20; ++run) {
            QCOMPARE(PetPackageValidator().validateDirectory(temp.path()).errorMessages(), expected);
        }
    }
};

// Runs inside a grouped binary; see tests/support/TestRunner.h.
QObject *createPackageValidatorTest() { return new PackageValidatorTest; }

#include "test_package_validator.moc"
