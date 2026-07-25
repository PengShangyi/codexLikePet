#include "settings/AppSettings.h"
#include "settings/Localization.h"

#include <QSignalSpy>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

#include <limits>

class SettingsTest final : public QObject
{
    Q_OBJECT

private slots:
    void persistsAndClampsValues()
    {
        QTemporaryDir temp;
        const QString path = temp.filePath(QStringLiteral("settings.ini"));
        {
            AppSettings settings(path);
            QSignalSpy spy(&settings, &AppSettings::scaleChanged);
            settings.setScale(9.0);
            settings.setAnimationSpeed(0.1);
            settings.setHemisphere(Hemisphere::South);
            QCOMPARE(settings.scale(), 2.0);
            QCOMPARE(settings.animationSpeed(), 0.5);
            QCOMPARE(spy.count(), 1);
        }
        AppSettings restored(path);
        QCOMPARE(restored.scale(), 2.0);
        QCOMPARE(restored.animationSpeed(), 0.5);
        QCOMPARE(restored.hemisphere(), Hemisphere::South);
        QCOMPARE(restored.dayStartsAt(), QTime(7, 0));
        QCOMPARE(restored.nightStartsAt(), QTime(19, 0));
    }

    void switchesLanguageWithoutChangingKeys()
    {
        QTemporaryDir temp;
        AppSettings settings(temp.filePath(QStringLiteral("settings.ini")));
        Localization localization(&settings);
        settings.setLanguage(AppLanguage::English);
        QCOMPARE(localization.text(TextKey::ShowPet), QStringLiteral("Show Pet"));
        settings.setLanguage(AppLanguage::SimplifiedChinese);
        QCOMPARE(localization.text(TextKey::ShowPet), QStringLiteral("显示宠物"));
    }

    void migratesLegacyKeysAndRecoversInvalidTimes()
    {
        QTemporaryDir temp;
        const QString path = temp.filePath(QStringLiteral("settings.ini"));
        {
            QSettings legacy(path, QSettings::IniFormat);
            legacy.setValue(QStringLiteral("scale"), 1.5);
            legacy.setValue(QStringLiteral("typingDetectionEnabled"), true);
            legacy.setValue(QStringLiteral("environment/dayStart"), QStringLiteral("invalid"));
        }
        AppSettings settings(path);
        QCOMPARE(settings.scale(), 1.5);
        QVERIFY(settings.typingDetectionEnabled());
        QCOMPARE(settings.dayStartsAt(), QTime(7, 0));
        // An existing profile is treated as an upgrader: onboarding is suppressed.
        QVERIFY(settings.onboardingCompleted());
        QSettings migrated(path, QSettings::IniFormat);
        QCOMPARE(migrated.value(QStringLiteral("meta/schemaVersion")).toInt(), 2);
        QVERIFY(!migrated.contains(QStringLiteral("scale")));
        QVERIFY(!migrated.contains(QStringLiteral("typingDetectionEnabled")));
    }

    void freshProfileShowsOnboardingOnce()
    {
        QTemporaryDir temp;
        const QString path = temp.filePath(QStringLiteral("settings.ini"));
        AppSettings settings(path);
        QVERIFY(!settings.onboardingCompleted()); // fresh install: welcome pending
        settings.setOnboardingCompleted(true);
        QVERIFY(settings.onboardingCompleted());
        AppSettings restored(path);
        QVERIFY(restored.onboardingCompleted()); // persists across launches
    }

    void recoversCorruptAndNonFiniteNumericValues()
    {
        QTemporaryDir temp;
        const QString path = temp.filePath(QStringLiteral("settings.ini"));
        {
            QSettings corrupt(path, QSettings::IniFormat);
            corrupt.setValue(QStringLiteral("appearance/scale"), QStringLiteral("not-a-number"));
            corrupt.setValue(QStringLiteral("appearance/animationSpeed"),
                             std::numeric_limits<double>::infinity());
        }
        AppSettings settings(path);
        QCOMPARE(settings.scale(), 1.0);
        QCOMPARE(settings.animationSpeed(), 1.0);
        settings.setScale(std::numeric_limits<double>::quiet_NaN());
        settings.setAnimationSpeed(-std::numeric_limits<double>::infinity());
        QCOMPARE(settings.scale(), 1.0);
        QCOMPARE(settings.animationSpeed(), 1.0);
    }
};

QTEST_GUILESS_MAIN(SettingsTest)

#include "test_settings.moc"
