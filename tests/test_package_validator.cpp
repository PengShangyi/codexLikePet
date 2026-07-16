#include "pet/PetAtlas.h"
#include "resources/PetPackageValidator.h"

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

    static bool writeValidAtlas(const QString &path)
    {
        QImage image(PetAtlas::Width, PetAtlas::Height, QImage::Format_RGBA8888);
        image.fill(Qt::transparent);
        for (int row = 0; row < PetAtlas::Rows; ++row) {
            const int columns = row <= 8
                ? PetAtlas::animationSpec(static_cast<V2AnimationState>(row)).frameCount
                : 8;
            for (int column = 0; column < columns; ++column) {
                image.setPixelColor(column * PetAtlas::CellWidth + 10,
                                    row * PetAtlas::CellHeight + 10,
                                    QColor(100, 70, 40, 255));
            }
        }
        return image.save(path);
    }

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
};

QTEST_GUILESS_MAIN(PackageValidatorTest)

#include "test_package_validator.moc"
