#include "pet/AtlasComposer.h"
#include "pet/PetAtlas.h"
#include "resources/PetPackageValidator.h"

#include <QCryptographicHash>
#include <QDir>
#include <QPainter>
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
            QCOMPARE(clips.size(), edgeClips.size() + 1); // 3 edge clips + 1 typing clip
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
            // Keystroke-driven typing clip: 3 frames (rest, left press, right press).
            QVERIFY2(clips.contains(QStringLiteral("typing")),
                     qPrintable(environment + QStringLiteral("/typing")));
            const auto typing = clips.value(QStringLiteral("typing"));
            QCOMPARE(typing.durationsMs.size(), 3);
            QCOMPARE(typing.path, QStringLiteral("clips/%1-typing.webp").arg(environment));
            const QImage typingStrip(QDir(root).filePath(typing.path));
            QVERIFY2(!typingStrip.isNull(), qPrintable(typing.path));
            QCOMPARE(typingStrip.size(), QSize(192 * 3, 208));
            QVERIFY(typingStrip.hasAlphaChannel());
        }
    }

    // The composer against real art rather than synthetic blocks.
    //
    // The shipped atlas is taken apart into the eleven row strips a model would have
    // returned -- each row's used cells flattened onto the chroma key the guide's
    // prompts ask for -- and put back together. Anything the pipeline gets wrong on
    // soft-shaded, antialiased artwork with a green-leaved subject shows up here and
    // nowhere else in the suite.
    //
    // What is asserted is geometry, because that is what has to be exact: every frame
    // must come back the same size, and land within a couple of pixels of where it
    // started. Colour is not asserted at all. The composer deliberately re-registers
    // frames to a common baseline, and recovering a soft edge that was flattened onto
    // green is lossy by nature, so a pixel comparison here would measure the
    // re-registration and the flattening rather than a defect.
    void theComposerRebuildsTheShippedAtlasFromStrips()
    {
        const QString sheet = QDir(QStringLiteral(POTATO_SOURCE_DIR))
                                  .filePath(QStringLiteral("assets/Pets/potato/spritesheet.webp"));
        const QImage original = QImage(sheet).convertedTo(QImage::Format_ARGB32);
        QVERIFY2(!original.isNull(), qPrintable(sheet));
        QCOMPARE(original.size(), QSize(PetAtlas::Width, PetAtlas::Height));

        QVector<AtlasComposer::RowInput> inputs;
        for (const AtlasComposer::RowSpec &spec : AtlasComposer::rows()) {
            const int count = AtlasComposer::frameCount(spec.row);
            QImage strip(count * PetAtlas::CellWidth, PetAtlas::CellHeight, QImage::Format_ARGB32);
            strip.fill(AtlasComposer::defaultChromaKey());
            QPainter painter(&strip);
            for (int column = 0; column < count; ++column) {
                painter.drawImage(QPoint(column * PetAtlas::CellWidth, 0), original,
                                  QRect(column * PetAtlas::CellWidth,
                                        spec.row * PetAtlas::CellHeight,
                                        PetAtlas::CellWidth, PetAtlas::CellHeight));
            }
            painter.end();
            inputs.append({spec.row, strip});
        }

        const AtlasComposer::Result result = AtlasComposer::compose(inputs, {});
        QVERIFY2(result.isInstallable(),
                 qPrintable(QStringLiteral("%1 issues").arg(result.issues.size())));
        QCOMPARE(result.issues.size(), 0);
        // Real art at the authored size needs no rescaling to fit the cell.
        QCOMPARE(result.scale, 1.0);

        QString error;
        QVERIFY2(PetAtlas::validateV2Occupancy(result.atlas, &error), qPrintable(error));

        for (const AtlasComposer::RowSpec &spec : AtlasComposer::rows()) {
            for (int column = 0; column < AtlasComposer::frameCount(spec.row); ++column) {
                const QRect cell(column * PetAtlas::CellWidth,
                                 spec.row * PetAtlas::CellHeight,
                                 PetAtlas::CellWidth, PetAtlas::CellHeight);
                const QRect before = AtlasComposer::alphaBounds(original, cell);
                const QRect after = AtlasComposer::alphaBounds(result.atlas, cell);
                const QString where = QStringLiteral("row %1 column %2").arg(spec.row).arg(column);

                QVERIFY2(!before.isEmpty() && !after.isEmpty(), qPrintable(where));
                // Within a pixel on each axis, not exact. The outermost fringe row of
                // a soft edge can have an original alpha barely above the threshold
                // alphaBounds measures at; flattened onto the key it becomes very
                // nearly pure key, and re-keying takes it. One row out of ~192 is the
                // cost of the round trip, not of the composer.
                QVERIFY2(qAbs(after.width() - before.width()) <= 1
                             && qAbs(after.height() - before.height()) <= 1,
                         qPrintable(QStringLiteral("%1 resized from %2x%3 to %4x%5")
                                        .arg(where)
                                        .arg(before.width()).arg(before.height())
                                        .arg(after.width()).arg(after.height())));
                QVERIFY2(qAbs(after.left() - before.left()) <= 3
                             && qAbs(after.top() - before.top()) <= 2,
                         qPrintable(QStringLiteral("%1 moved from %2,%3 to %4,%5")
                                        .arg(where)
                                        .arg(before.left()).arg(before.top())
                                        .arg(after.left()).arg(after.top())));
            }
        }
    }
};

QTEST_GUILESS_MAIN(BuiltInPetTest)
#include "test_builtin_pet.moc"
