#include "resources/PetPackage.h"
#include "resources/PetResourceSummary.h"

#include <QTest>

// The Settings "Resources and fallbacks" table. Extracted from AppController so
// the fallback resolution it reports can be checked directly instead of only
// through a running controller.
class ResourceSummaryTest final : public QObject
{
    Q_OBJECT

    static PetPackage basePackage()
    {
        PetPackage package;
        package.id = QStringLiteral("potato-test");
        package.spriteSheetPath = QStringLiteral("spritesheet.webp");
        return package;
    }

    static QStringList variantKeys()
    {
        return {QStringLiteral("spring-day"), QStringLiteral("spring-night"),
                QStringLiteral("summer-day"), QStringLiteral("summer-night"),
                QStringLiteral("autumn-day"), QStringLiteral("autumn-night"),
                QStringLiteral("winter-day"), QStringLiteral("winter-night")};
    }

private slots:
    void reportsEveryVariantAndFallsBackForMissingClips()
    {
        const QString summary = PetResourceSummary::build(basePackage(),
                                                          QStringLiteral("FALLBACK"));
        const QStringList lines = summary.split(QLatin1Char('\n'));
        QCOMPARE(lines.size(), 16); // eight variants, two lines each

        for (const QString &key : variantKeys()) {
            QVERIFY2(summary.contains(key), qPrintable(key));
        }
        // Bare package: every variant resolves to the base sheet and every clip
        // slot reports the fallback label.
        QCOMPARE(summary.count(QStringLiteral("spritesheet.webp")), 8);
        for (const QString &slot : PetResourceSummary::clipSlots()) {
            QCOMPARE(summary.count(QStringLiteral("%1=FALLBACK").arg(slot)), 8);
        }
    }

    void prefersVariantResourcesOverBaseOnes()
    {
        PetPackage package = basePackage();
        package.variants.insert(QStringLiteral("winter-night"),
                                QStringLiteral("variants/winter-night.webp"));
        // A base clip applies to every variant...
        package.clips.insert(QStringLiteral("click"),
                             ClipDefinition{QStringLiteral("clips/click.webp"), {120, 120}});
        // ...unless that variant declares its own.
        package.variantClips[QStringLiteral("winter-night")].insert(
            QStringLiteral("click"),
            ClipDefinition{QStringLiteral("clips/winter-night-click.webp"), {120, 120}});

        const QString summary = PetResourceSummary::build(package, QStringLiteral("FALLBACK"));
        const QStringList lines = summary.split(QLatin1Char('\n'));

        // Locate the winter-night block: its header line names the variant.
        int headerIndex = -1;
        for (int index = 0; index < lines.size(); ++index) {
            if (lines.at(index).startsWith(QStringLiteral("winter-night "))) headerIndex = index;
        }
        QVERIFY2(headerIndex >= 0, qPrintable(summary));
        QVERIFY(lines.at(headerIndex).contains(QStringLiteral("variants/winter-night.webp")));
        QVERIFY(lines.at(headerIndex + 1).contains(QStringLiteral("click=clips/winter-night-click.webp")));
        // Slots the variant does not override still fall back.
        QVERIFY(lines.at(headerIndex + 1).contains(QStringLiteral("typing=FALLBACK")));

        // Every other variant keeps the base atlas and the base click clip.
        QCOMPARE(summary.count(QStringLiteral("click=clips/click.webp")), 7);
        QCOMPARE(summary.count(QStringLiteral("spritesheet.webp")), 7);
    }

    void isIndependentOfTheCurrentEnvironment()
    {
        // The table enumerates all eight keys itself, so repeated builds of the
        // same package are byte-identical -- nothing here reads a clock.
        const PetPackage package = basePackage();
        QCOMPARE(PetResourceSummary::build(package, QStringLiteral("FALLBACK")),
                 PetResourceSummary::build(package, QStringLiteral("FALLBACK")));
        // Only the label varies with language.
        QVERIFY(PetResourceSummary::build(package, QStringLiteral("回退"))
                    .contains(QStringLiteral("回退")));
    }
};

QTEST_GUILESS_MAIN(ResourceSummaryTest)

#include "test_resource_summary.moc"
