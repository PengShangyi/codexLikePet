#include "resources/PetPackageValidator.h"

#include <QDir>
#include <QTest>

class BuiltInPetTest final : public QObject
{
    Q_OBJECT

private slots:
    void validatesPackagedPotato()
    {
        const QString root = QDir(QStringLiteral(POTATO_SOURCE_DIR))
                                 .filePath(QStringLiteral("assets/Pets/potato"));
        const PackageValidationResult result = PetPackageValidator().validateDirectory(root);

        QVERIFY2(result.isValid(), qPrintable(result.errorMessages().join('\n')));
        QCOMPARE(result.package.id, QStringLiteral("potato"));
        QCOMPARE(result.package.variants.value(QStringLiteral("spring-day")),
                 QStringLiteral("spritesheet.webp"));
        QCOMPARE(result.package.variants.value(QStringLiteral("spring-night")),
                 QStringLiteral("variants/spring-night.webp"));
        QCOMPARE(result.package.variants.value(QStringLiteral("summer-day")),
                 QStringLiteral("variants/summer-day.webp"));
        QCOMPARE(result.package.variants.value(QStringLiteral("summer-night")),
                 QStringLiteral("variants/summer-night.webp"));
        QCOMPARE(result.package.variants.value(QStringLiteral("autumn-day")),
                 QStringLiteral("variants/autumn-day.webp"));
        QCOMPARE(result.package.variants.value(QStringLiteral("autumn-night")),
                 QStringLiteral("variants/autumn-night.webp"));
        QCOMPARE(result.package.variants.value(QStringLiteral("winter-day")),
                 QStringLiteral("variants/winter-day.webp"));
        QCOMPARE(result.package.variants.value(QStringLiteral("winter-night")),
                 QStringLiteral("variants/winter-night.webp"));
    }
};

QTEST_GUILESS_MAIN(BuiltInPetTest)
#include "test_builtin_pet.moc"
