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

    enum class ScaleKey { Absent, Present };

    // Seeds a schema-2 profile, opens it through AppSettings so migrate() runs, and
    // reports the scale that survived. Both objects are locals, so the store is
    // destroyed before its directory and check-leaks.sh stays at zero.
    static double migratedScale(ScaleKey key, double storedScale = 0.0)
    {
        QTemporaryDir temp;
        const QString path = temp.filePath(QStringLiteral("settings.ini"));
        {
            QSettings before(path, QSettings::IniFormat);
            before.setValue(QStringLiteral("meta/schemaVersion"), 2);
            if (key == ScaleKey::Present) {
                before.setValue(QStringLiteral("appearance/scale"), storedScale);
            }
        }
        AppSettings settings(path);
        return settings.scale();
    }

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

    // The resolved language is cached, so text() does not re-read QSettings for
    // each of the ~70 strings a retranslate asks for. The cache must be refreshed
    // before languageChanged is emitted: every retranslate slot in the app reads
    // text() from inside that signal and would otherwise get the previous
    // language for one cycle.
    void languageChangedSlotsSeeTheNewLanguage()
    {
        QTemporaryDir temp;
        AppSettings settings(temp.filePath(QStringLiteral("settings.ini")));
        settings.setLanguage(AppLanguage::English);
        Localization localization(&settings);
        QVERIFY(!localization.usesChinese());

        QStringList observed;
        connect(&localization, &Localization::languageChanged, &localization, [&] {
            observed.append(localization.text(TextKey::ShowPet));
        });

        settings.setLanguage(AppLanguage::SimplifiedChinese);
        QCOMPARE(observed, QStringList{QStringLiteral("显示宠物")});
        QVERIFY(localization.usesChinese());

        settings.setLanguage(AppLanguage::English);
        QCOMPARE(observed.size(), 2);
        QCOMPARE(observed.last(), QStringLiteral("Show Pet"));
        QVERIFY(!localization.usesChinese());
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
        // 1.5 against the old base, doubled by the v3 step, then clamped to the
        // maximum -- see scaleIsRebasedForExistingProfiles.
        QCOMPARE(settings.scale(), 2.0);
        QVERIFY(settings.typingDetectionEnabled());
        QCOMPARE(settings.dayStartsAt(), QTime(7, 0));
        // An existing profile is treated as an upgrader: onboarding is suppressed.
        QVERIFY(settings.onboardingCompleted());
        QSettings migrated(path, QSettings::IniFormat);
        QCOMPARE(migrated.value(QStringLiteral("meta/schemaVersion")).toInt(), 3);
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

    // migrate() used to ask allKeys().isEmpty(), which counts keys Potato never
    // wrote. Only our own keys may mark a profile as an upgrade; an ungrouped
    // stranger must leave the welcome pending.
    void foreignKeysDoNotMakeAProfileLookLikeAnUpgrade()
    {
        QTemporaryDir temp;
        const QString path = temp.filePath(QStringLiteral("settings.ini"));
        {
            QSettings foreign(path, QSettings::IniFormat);
            foreign.setValue(QStringLiteral("AppleLanguages"), QStringLiteral("en"));
            foreign.setValue(QStringLiteral("AppleLocale"), QStringLiteral("en_GB"));
        }
        AppSettings settings(path);
        QVERIFY2(!settings.onboardingCompleted(),
                 "keys Potato never wrote must not suppress the first-run welcome");

        // ...while any group of ours does mark it, without needing a legacy key.
        QTemporaryDir ours;
        const QString ourPath = ours.filePath(QStringLiteral("settings.ini"));
        {
            QSettings seeded(ourPath, QSettings::IniFormat);
            seeded.setValue(QStringLiteral("appearance/opacity"), 0.8);
        }
        AppSettings upgraded(ourPath);
        QVERIFY(upgraded.onboardingCompleted());
    }

    // Read-only probe, writes nothing. Pins the platform behaviour that made the
    // check above necessary: with fallbacks left on, a domain that does not exist
    // still answers with the user's global preferences, so "is this profile
    // empty?" can never be asked that way on the native backend.
    void nativeSettingsFallbacksReachTheGlobalDomain()
    {
        QSettings leaky(QStringLiteral("com.peng"), QStringLiteral("PotatoFallbackProbe"));
        if (leaky.format() != QSettings::NativeFormat) {
            QSKIP("fallback chain is a native-backend behaviour");
        }
        if (leaky.allKeys().isEmpty()) {
            QSKIP("no global preferences on this host, so there is nothing to leak");
        }
        QSettings sealed(QStringLiteral("com.peng"), QStringLiteral("PotatoFallbackProbe"));
        sealed.setFallbacksEnabled(false);
        QVERIFY2(sealed.allKeys().isEmpty(),
                 "AppSettings disables fallbacks so a nonexistent domain reads as empty");
    }

    // The pet window's position must live in the injected backing store like every
    // other preference. PetWindow used to reach for a bare QSettings(), so running
    // the test suite wrote window/position into real macOS preference domains.
    void windowPositionRoundTripsThroughTheInjectedStore()
    {
        QTemporaryDir temp;
        const QString path = temp.filePath(QStringLiteral("settings.ini"));
        {
            AppSettings settings(path);
            QVERIFY(!settings.hasWindowPosition());

            settings.setWindowPosition(QPoint(0, 0)); // must persist, not read as "absent"
            QVERIFY(settings.hasWindowPosition());
            QCOMPARE(settings.windowPosition(), QPoint(0, 0));

            settings.setWindowPosition(QPoint(412, 96));
            QCOMPARE(settings.windowPosition(), QPoint(412, 96));
        }
        {
            AppSettings restored(path);
            QVERIFY(restored.hasWindowPosition());
            QCOMPARE(restored.windowPosition(), QPoint(412, 96));
            restored.clearWindowPosition();
            QVERIFY(!restored.hasWindowPosition());
        }
        AppSettings cleared(path);
        QVERIFY(!cleared.hasWindowPosition());
    }

    // Moved out of potato_settings_window_test, which used to assert these strings
    // while claiming to test the window's layout. Copy is a localization fact, and
    // asserting it here means editing a label no longer breaks a widget test.
    void everyTextKeyHasBothLanguages()
    {
        QTemporaryDir temp;
        AppSettings settings(temp.filePath(QStringLiteral("settings.ini")));
        settings.setLanguage(AppLanguage::English);
        Localization localization(&settings);

        QCOMPARE(localization.text(TextKey::PreviewVariant), QStringLiteral("Preview atlas"));
        QCOMPARE(localization.text(TextKey::PreviewAnimation), QStringLiteral("Preview animation"));

        // Localization::text() has no default: label, so -Wswitch catches a new
        // TextKey with no case -- but the fallthrough returns an empty string, and
        // nothing else would notice. Sweep the whole enum for both languages.
        const int lastKey = static_cast<int>(TextKey::UseStandardAnimation);
        for (int raw = 0; raw <= lastKey; ++raw) {
            const auto key = static_cast<TextKey>(raw);
            settings.setLanguage(AppLanguage::English);
            const QString english = localization.text(key);
            settings.setLanguage(AppLanguage::SimplifiedChinese);
            const QString chinese = localization.text(key);
            QVERIFY2(!english.isEmpty(),
                     qPrintable(QStringLiteral("TextKey %1 has no English string").arg(raw)));
            QVERIFY2(!chinese.isEmpty(),
                     qPrintable(QStringLiteral("TextKey %1 has no Chinese string").arg(raw)));
        }
    }

    // Opacity clamps to a narrower range than scale and speed: never below 0.3,
    // because a fully transparent pet cannot be clicked or found again.
    void opacityClampsToItsOwnNarrowerRange()
    {
        QTemporaryDir temp;
        AppSettings settings(temp.filePath(QStringLiteral("settings.ini")));
        QCOMPARE(settings.opacity(), 1.0);

        QSignalSpy spy(&settings, &AppSettings::opacityChanged);
        settings.setOpacity(0.75);
        QCOMPARE(settings.opacity(), 0.75);
        QCOMPARE(spy.count(), 1);

        settings.setOpacity(0.0);
        QCOMPARE(settings.opacity(), 0.3);
        settings.setOpacity(4.0);
        QCOMPARE(settings.opacity(), 1.0);
        settings.setOpacity(std::numeric_limits<double>::quiet_NaN());
        QCOMPARE(settings.opacity(), 1.0);

        // Scale still uses the wider factor range, so the shared helper did not get
        // narrowed for everyone.
        settings.setScale(2.0);
        QCOMPARE(settings.scale(), 2.0);
    }

    void positionLockDefaultsOffAndRoundTrips()
    {
        QTemporaryDir temp;
        const QString path = temp.filePath(QStringLiteral("settings.ini"));
        {
            AppSettings settings(path);
            QCOMPARE(settings.positionLocked(), false);

            QSignalSpy spy(&settings, &AppSettings::positionLockedChanged);
            settings.setPositionLocked(true);
            QCOMPARE(settings.positionLocked(), true);
            QCOMPARE(spy.count(), 1);

            settings.setPositionLocked(true);  // no change, no signal
            QCOMPARE(spy.count(), 1);
        }
        AppSettings restored(path);
        QCOMPARE(restored.positionLocked(), true);
    }

    // QRect through the ini backend is asserted rather than assumed: QSettings
    // serialises it as @Rect(x y w h), and if that ever stops round-tripping the
    // settings window would silently reopen at the wrong size every launch.
    void settingsGeometryRoundTripsThroughTheIniBackend()
    {
        QTemporaryDir temp;
        const QString path = temp.filePath(QStringLiteral("settings.ini"));
        {
            AppSettings settings(path);
            QVERIFY(!settings.hasSettingsGeometry());

            settings.setSettingsGeometry(QRect(120, 80, 700, 540));
            QVERIFY(settings.hasSettingsGeometry());
            QCOMPARE(settings.settingsGeometry(), QRect(120, 80, 700, 540));

            // An invalid rect must not be stored -- restoring it would produce a
            // zero-sized window with no way back.
            settings.setSettingsGeometry(QRect());
            QCOMPARE(settings.settingsGeometry(), QRect(120, 80, 700, 540));
        }
        {
            AppSettings restored(path);
            QVERIFY(restored.hasSettingsGeometry());
            QCOMPARE(restored.settingsGeometry(), QRect(120, 80, 700, 540));
            restored.clearSettingsGeometry();
            QVERIFY(!restored.hasSettingsGeometry());
        }
        AppSettings cleared(path);
        QVERIFY(!cleared.hasSettingsGeometry());
    }

    // The v3 step exists so halving PetWindow's base render size does not halve
    // every existing pet with it. The absent-key case is the one that matters: it
    // meant the old default, so it has to be written out rather than left to pick up
    // the new one.
    void scaleIsRebasedForExistingProfiles()
    {
        // Never touched the slider: was the old default, must stay that size.
        QCOMPARE(migratedScale(ScaleKey::Absent), 2.0);
        // Had shrunk the pet: doubling keeps it where it was.
        QCOMPARE(migratedScale(ScaleKey::Present, 0.5), 1.0);
        QCOMPARE(migratedScale(ScaleKey::Present, 0.75), 1.5);
        // Had enlarged it beyond what the new base can express: clamps.
        QCOMPARE(migratedScale(ScaleKey::Present, 1.5), 2.0);

        // A fresh profile is not an upgrade and keeps the new default, which is the
        // whole point -- if this ever reads 2.0, first runs are being misdetected
        // and the rebase has silently undone itself.
        QTemporaryDir fresh;
        AppSettings freshSettings(fresh.filePath(QStringLiteral("settings.ini")));
        QCOMPARE(freshSettings.scale(), 1.0);
    }

    void recoversCorruptAndNonFiniteNumericValues()
    {
        QTemporaryDir temp;
        const QString path = temp.filePath(QStringLiteral("settings.ini"));
        {
            QSettings corrupt(path, QSettings::IniFormat);
            // Already current, so migrate() returns before the v3 rebase and this
            // stays a test of the read path's clamping rather than of migration.
            corrupt.setValue(QStringLiteral("meta/schemaVersion"), 3);
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

// Runs inside a grouped binary; see tests/support/TestRunner.h.
QObject *createSettingsTest() { return new SettingsTest; }

#include "test_settings.moc"
