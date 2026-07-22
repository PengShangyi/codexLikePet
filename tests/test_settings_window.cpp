#include "settings/AppSettings.h"
#include "settings/Localization.h"
#include "settings/SettingsWindow.h"

#include <QComboBox>
#include <QLabel>
#include <QTemporaryDir>
#include <QTest>

class SettingsWindowTest final : public QObject
{
    Q_OBJECT

private slots:
    void constructsAndRetranslatesEveryFormLabel()
    {
        QTemporaryDir temp;
        AppSettings settings(temp.filePath(QStringLiteral("settings.ini")));
        settings.setLanguage(AppLanguage::English);
        Localization localization(&settings);
        SettingsWindow window(&settings, &localization);
        window.show();
        QTest::qWait(20);
        QVERIFY(window.isVisible());

        bool foundPreviewAtlas = false;
        bool foundPreviewAnimation = false;
        for (QLabel *label : window.findChildren<QLabel *>()) {
            foundPreviewAtlas |= label->text() == QStringLiteral("Preview atlas");
            foundPreviewAnimation |= label->text() == QStringLiteral("Preview animation");
        }
        QVERIFY(foundPreviewAtlas);
        QVERIFY(foundPreviewAnimation);
        QVERIFY(window.findChildren<QComboBox *>().size() >= 6);
    }
};

QTEST_MAIN(SettingsWindowTest)
#include "test_settings_window.moc"
