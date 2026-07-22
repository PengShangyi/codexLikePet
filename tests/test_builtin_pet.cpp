#include "resources/PetPackageValidator.h"

#include <QCryptographicHash>
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

        const QStringList environments{
            QStringLiteral("spring-day"), QStringLiteral("spring-night"),
            QStringLiteral("summer-day"), QStringLiteral("summer-night"),
            QStringLiteral("autumn-day"), QStringLiteral("autumn-night"),
            QStringLiteral("winter-day"), QStringLiteral("winter-night")};
        const QStringList edgeClips{QStringLiteral("edge-left"),
                                    QStringLiteral("edge-right"),
                                    QStringLiteral("edge-bottom")};
        QCOMPARE(result.package.variantClips.size(), environments.size());
        for (const QString &environment : environments) {
            const auto clips = result.package.variantClips.value(environment);
            QCOMPARE(clips.size(), edgeClips.size());
            for (const QString &name : edgeClips) {
                QVERIFY2(clips.contains(name), qPrintable(environment + QLatin1Char('/') + name));
                QCOMPARE(clips.value(name).durationsMs.size(), 6);
                QCOMPARE(clips.value(name).path,
                         QStringLiteral("clips/%1-%2.webp").arg(environment, name));
                const QImage strip(QDir(root).filePath(clips.value(name).path));
                QVERIFY2(!strip.isNull(), qPrintable(clips.value(name).path));
                QCOMPARE(strip.size(), QSize(192 * 6, 208));
                QVERIFY(strip.hasAlphaChannel());
                const auto frame = [&strip](int index) {
                    return strip.copy(index * 192, 0, 192, 208);
                };
                const auto pixelsHash = [](const QImage &image) {
                    QImage normalized = image.convertToFormat(QImage::Format_RGBA8888);
                    for (int y = 0; y < normalized.height(); ++y) {
                        uchar *row = normalized.scanLine(y);
                        for (int x = 0; x < normalized.width(); ++x) {
                            uchar *pixel = row + x * 4;
                            if (pixel[3] == 0) pixel[0] = pixel[1] = pixel[2] = 0;
                        }
                    }
                    return QCryptographicHash::hash(
                        QByteArray::fromRawData(
                            reinterpret_cast<const char *>(normalized.constBits()),
                            static_cast<qsizetype>(normalized.sizeInBytes())),
                        QCryptographicHash::Sha256);
                };
                QCOMPARE(pixelsHash(frame(1)), pixelsHash(frame(5)));
                QCOMPARE(pixelsHash(frame(2)), pixelsHash(frame(4)));
            }
        }
    }
};

QTEST_GUILESS_MAIN(BuiltInPetTest)
#include "test_builtin_pet.moc"
