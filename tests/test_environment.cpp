#include "environment/EnvironmentClock.h"
#include "environment/EnvironmentResolver.h"
#include "settings/AppSettings.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTimeZone>

class FakeClock final : public EnvironmentClock
{
    Q_OBJECT
public:
    using EnvironmentClock::EnvironmentClock;
    QDateTime now() const override { return value; }
    QDateTime value;
};

class EnvironmentTest final : public QObject
{
    Q_OBJECT

private slots:
    void mapsMeteorologicalSeasonsAcrossHemispheres()
    {
        QCOMPARE(EnvironmentResolver::seasonForDate(QDate(2026, 3, 1), Hemisphere::North), Season::Spring);
        QCOMPARE(EnvironmentResolver::seasonForDate(QDate(2026, 7, 1), Hemisphere::North), Season::Summer);
        QCOMPARE(EnvironmentResolver::seasonForDate(QDate(2026, 10, 1), Hemisphere::South), Season::Spring);
        QCOMPARE(EnvironmentResolver::seasonForDate(QDate(2026, 1, 1), Hemisphere::South), Season::Summer);
    }

    void handlesNormalAndCrossMidnightDayWindows()
    {
        QCOMPARE(EnvironmentResolver::phaseForTime(QTime(7, 0), QTime(7, 0), QTime(19, 0)), TimePhase::Day);
        QCOMPARE(EnvironmentResolver::phaseForTime(QTime(19, 0), QTime(7, 0), QTime(19, 0)), TimePhase::Night);
        QCOMPARE(EnvironmentResolver::phaseForTime(QTime(23, 0), QTime(20, 0), QTime(6, 0)), TimePhase::Day);
        QCOMPARE(EnvironmentResolver::phaseForTime(QTime(12, 0), QTime(20, 0), QTime(6, 0)), TimePhase::Night);
    }

    void appliesTheSpecifiedFallbackOrder()
    {
        PetPackage package;
        package.spriteSheetPath = QStringLiteral("base.webp");
        package.variants.insert(QStringLiteral("winter"), QStringLiteral("winter.webp"));
        package.variants.insert(QStringLiteral("night"), QStringLiteral("night.webp"));
        package.variants.insert(QStringLiteral("winter-night"), QStringLiteral("winter-night.webp"));
        QCOMPARE(EnvironmentResolver::atlasRelativePath(package, {Season::Winter, TimePhase::Night}),
                 QStringLiteral("winter-night.webp"));
        package.variants.remove(QStringLiteral("winter-night"));
        QCOMPARE(EnvironmentResolver::atlasRelativePath(package, {Season::Winter, TimePhase::Night}),
                 QStringLiteral("winter.webp"));
        package.variants.remove(QStringLiteral("winter"));
        QCOMPARE(EnvironmentResolver::atlasRelativePath(package, {Season::Winter, TimePhase::Night}),
                 QStringLiteral("night.webp"));
    }

    void appliesVariantClipFallbackBeforeBaseClips()
    {
        PetPackage package;
        package.clips.insert(QStringLiteral("click"),
                             {QStringLiteral("base.png"), {100}});
        package.variantClips[QStringLiteral("night")].insert(
            QStringLiteral("click"),
            {QStringLiteral("night.png"), {120}});
        package.variantClips[QStringLiteral("winter-night")].insert(
            QStringLiteral("click"),
            {QStringLiteral("winter-night.png"), {140}});

        const VariantKey key{Season::Winter, TimePhase::Night};
        QCOMPARE(EnvironmentResolver::clipDefinition(package, key, QStringLiteral("click"))->path,
                 QStringLiteral("winter-night.png"));
        package.variantClips.remove(QStringLiteral("winter-night"));
        QCOMPARE(EnvironmentResolver::clipDefinition(package, key, QStringLiteral("click"))->path,
                 QStringLiteral("night.png"));
        package.variantClips.clear();
        QCOMPARE(EnvironmentResolver::clipDefinition(package, key, QStringLiteral("click"))->path,
                 QStringLiteral("base.png"));
        QVERIFY(!EnvironmentResolver::clipDefinition(package, key, QStringLiteral("typing")));
    }

    void emitsOnlyWhenTheResolvedEnvironmentChanges()
    {
        QTemporaryDir temp;
        AppSettings settings(temp.filePath(QStringLiteral("settings.ini")));
        FakeClock clock;
        clock.value = QDateTime(QDate(2026, 1, 10), QTime(8, 0));
        EnvironmentResolver resolver(&settings, &clock);
        QSignalSpy spy(&resolver, &EnvironmentResolver::environmentChanged);
        resolver.reevaluate();
        QCOMPARE(spy.count(), 0);
        clock.value = QDateTime(QDate(2026, 1, 10), QTime(20, 0));
        resolver.reevaluate();
        QCOMPARE(spy.count(), 1);
        QCOMPARE(resolver.current().phase, TimePhase::Night);
    }

    void reevaluatesUsingLocalWallClockAcrossDstChanges()
    {
        QTemporaryDir temp;
        AppSettings settings(temp.filePath(QStringLiteral("settings.ini")));
        settings.setDayStartsAt(QTime(3, 0));
        settings.setNightStartsAt(QTime(19, 0));
        FakeClock clock;
        const QTimeZone london("Europe/London");
        QVERIFY(london.isValid());
        clock.value = QDateTime(QDate(2026, 3, 29), QTime(0, 30), QTimeZone::UTC)
                          .toTimeZone(london);
        EnvironmentResolver resolver(&settings, &clock);
        QCOMPARE(resolver.current().phase, TimePhase::Night);

        clock.value = clock.value.addSecs(2 * 60 * 60);
        QCOMPARE(clock.value.time(), QTime(3, 30));
        resolver.reevaluate();
        QCOMPARE(resolver.current().phase, TimePhase::Day);
    }
};

QTEST_GUILESS_MAIN(EnvironmentTest)

#include "test_environment.moc"
