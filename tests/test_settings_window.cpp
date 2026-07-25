#include "pet/PetAtlas.h"
#include "settings/AppSettings.h"
#include "settings/Localization.h"
#include "settings/PetPreviewWidget.h"
#include "settings/SettingsWindow.h"

#include <QComboBox>
#include <QLabel>
#include <QPixmap>
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

    // PetPreviewWidget is marked WA_OpaquePaintEvent so an animating preview stops
    // dragging the enclosing QTabWidget's chrome into every repaint. Qt then skips
    // erasing the background, so paintEvent must genuinely cover every pixel --
    // any gap would show uninitialized backing-store content.
    void previewPaintsEveryPixelOfItsRect()
    {
        PetPreviewWidget preview;
        QVERIFY(preview.testAttribute(Qt::WA_OpaquePaintEvent));
        preview.resize(320, 240);
        preview.show();
        QTest::qWait(20);

        // With no atlas set, paintEvent draws only the checkerboard: every pixel
        // must be one of its two greys, edges and corners included.
        // grab() honors the device pixel ratio, so on a Retina screen the image is
        // larger than the widget; check whatever it actually produced.
        const QImage rendered = preview.grab().toImage().convertToFormat(QImage::Format_RGB32);
        QVERIFY(!rendered.isNull());
        QCOMPARE(rendered.size(), QSize(320, 240) * preview.devicePixelRatioF());
        const QRgb light = qRgb(245, 245, 245);
        const QRgb dark = qRgb(220, 220, 220);
        for (int y = 0; y < rendered.height(); ++y) {
            for (int x = 0; x < rendered.width(); ++x) {
                const QRgb pixel = rendered.pixel(x, y);
                if (pixel != light && pixel != dark) {
                    QFAIL(qPrintable(QStringLiteral("unpainted pixel at %1,%2: %3")
                                         .arg(x)
                                         .arg(y)
                                         .arg(pixel, 8, 16, QLatin1Char('0'))));
                }
            }
        }
    }
};

QTEST_MAIN(SettingsWindowTest)
#include "test_settings_window.moc"
