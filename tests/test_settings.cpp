#include "settings/AppSettings.h"
#include "settings/Localization.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

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
};

QTEST_GUILESS_MAIN(SettingsTest)

#include "test_settings.moc"
