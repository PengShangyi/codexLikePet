#include "pet/PetAtlas.h"
#include "settings/AppSettings.h"
#include "settings/Localization.h"
#include "settings/PetPreviewWidget.h"
#include "settings/SettingsWindow.h"
#include "ui/Disclosure.h"
#include "ui/InlineBanner.h"
#include "ui/SettingsRow.h"

#include <QComboBox>
#include <QGuiApplication>
#include <QScreen>
#include <QLabel>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTest>
#include <QToolButton>

namespace {

// Renders the widget over a chosen fill without clearing it. QWidget::render() is
// used rather than grab() because grab() pre-fills only when the widget is *not*
// opaque (`if (!d->isOpaque) res.fill(Qt::transparent)`), which is the opposite of
// the case under test here.
QImage renderOver(QWidget *widget, QRgb sentinel)
{
    const qreal dpr = widget->devicePixelRatioF();
    QImage canvas(widget->size() * dpr, QImage::Format_ARGB32_Premultiplied);
    canvas.setDevicePixelRatio(dpr);
    canvas.fill(sentinel);
    widget->render(&canvas);
    return canvas;
}

// Any pixel paintEvent leaves alone keeps whichever sentinel was underneath, so the
// two renders differ exactly at the unpainted pixels -- without this test naming a
// single one of the widget's own colors.
void assertCoversEveryPixel(QWidget *widget, const char *context)
{
    const QImage overMagenta = renderOver(widget, qRgb(255, 0, 255));
    const QImage overGreen = renderOver(widget, qRgb(0, 255, 0));
    QCOMPARE(overMagenta.size(), widget->size() * widget->devicePixelRatioF());
    if (overMagenta == overGreen) return;
    for (int y = 0; y < overMagenta.height(); ++y) {
        for (int x = 0; x < overMagenta.width(); ++x) {
            if (overMagenta.pixel(x, y) != overGreen.pixel(x, y)) {
                QFAIL(qPrintable(QStringLiteral("%1: unpainted pixel at %2,%3")
                                     .arg(QLatin1String(context))
                                     .arg(x)
                                     .arg(y)));
            }
        }
    }
    QFAIL(qPrintable(QStringLiteral("%1: images differ but no differing pixel found")
                         .arg(QLatin1String(context))));
}

}  // namespace

class SettingsWindowTest final : public QObject
{
    Q_OBJECT

private slots:
    void navigationExposesEveryPage()
    {
        QTemporaryDir temp;
        AppSettings settings(temp.filePath(QStringLiteral("settings.ini")));
        settings.setLanguage(AppLanguage::English);
        Localization localization(&settings);
        SettingsWindow window(&settings, &localization);
        window.show();
        QTest::qWait(20);
        QVERIFY(window.isVisible());

        const QList<QToolButton *> nav = window.findChildren<QToolButton *>(QStringLiteral("navItem"));
        QCOMPARE(nav.size(), 5);
        for (QToolButton *button : nav) {
            QVERIFY(button->isCheckable());
            QVERIFY2(!button->text().isEmpty(), "every nav item needs a localized label");
            QCOMPARE(button->accessibleName(), button->text());
        }
        // Exactly one page selected at a time.
        int checked = 0;
        for (QToolButton *button : nav) checked += button->isChecked() ? 1 : 0;
        QCOMPARE(checked, 1);
    }

    // Replaces an assertion that looked for the literal strings "Preview atlas" and
    // "Preview animation". Those labels still exist, but they now sit inside a
    // collapsed disclosure -- and findChildren ignores visibility, so the old test
    // would have kept passing while no longer testing what it claimed. Assert the
    // structure instead; the copy is asserted in potato_settings_test, where it
    // belongs.
    void authoringControlsStartCollapsedBehindADisclosure()
    {
        QTemporaryDir temp;
        AppSettings settings(temp.filePath(QStringLiteral("settings.ini")));
        settings.setLanguage(AppLanguage::English);
        Localization localization(&settings);
        SettingsWindow window(&settings, &localization);
        window.show();
        QTest::qWait(20);

        auto *details = window.findChild<Disclosure *>(QStringLiteral("resourceDetails"));
        QVERIFY(details);
        QVERIFY2(!details->isExpanded(), "pet-authoring controls must not greet every user");

        // The three preview pickers and the fallback table live in there.
        QCOMPARE(details->findChildren<QComboBox *>().size(), 3);
        auto *summary = window.findChild<QPlainTextEdit *>(QStringLiteral("resourceSummary"));
        QVERIFY(summary);
        QVERIFY(details->isAncestorOf(summary));
        QVERIFY(!summary->isVisible());

        details->setExpanded(true);
        QVERIFY(details->isExpanded());
        QVERIFY(summary->isVisible());
    }

    // The replacement for the QStringLiteral(" ") placeholder labels and the
    // separate setAccessibleName block: every row carries a TextKey, so this can
    // assert coverage over all of them at once instead of trusting a list.
    void everyRowHasALocalizedLabelAndNamesItsControl()
    {
        QTemporaryDir temp;
        AppSettings settings(temp.filePath(QStringLiteral("settings.ini")));
        settings.setLanguage(AppLanguage::English);
        Localization localization(&settings);
        SettingsWindow window(&settings, &localization);

        const QList<SettingsRow *> rows = window.findChildren<SettingsRow *>();
        QVERIFY(rows.size() >= 14);

        QStringList english;
        for (SettingsRow *row : rows) {
            QVERIFY2(!row->labelText().isEmpty(),
                     qPrintable(QStringLiteral("row %1 has no label").arg(row->objectName())));
            QVERIFY2(row->labelText() != QStringLiteral(" "),
                     qPrintable(QStringLiteral("row %1 kept a placeholder label")
                                    .arg(row->objectName())));
            if (row->control()) {
                QCOMPARE(row->control()->accessibleName(), row->labelText());
            }
            english << row->labelText();
        }

        // No leftover placeholder anywhere in the window, row-owned or not.
        for (QLabel *label : window.findChildren<QLabel *>()) {
            QVERIFY(label->text() != QStringLiteral(" "));
        }

        settings.setLanguage(AppLanguage::SimplifiedChinese);
        for (int index = 0; index < rows.size(); ++index) {
            SettingsRow *row = rows.at(index);
            QVERIFY2(!row->labelText().isEmpty(),
                     qPrintable(QStringLiteral("row %1 lost its label in Chinese")
                                    .arg(row->objectName())));
            QVERIFY2(row->labelText() != english.at(index),
                     qPrintable(QStringLiteral("row %1 has no distinct Chinese string")
                                    .arg(row->objectName())));
            if (row->control()) {
                QCOMPARE(row->control()->accessibleName(), row->labelText());
            }
        }
    }

    // The five controls that are not owned by a row lost their shared
    // setAccessibleName block, so each is checked here by objectName.
    void controlsOutsideRowsKeepTheirAccessibleNames()
    {
        QTemporaryDir temp;
        AppSettings settings(temp.filePath(QStringLiteral("settings.ini")));
        settings.setLanguage(AppLanguage::English);
        Localization localization(&settings);
        SettingsWindow window(&settings, &localization);

        for (const QString &name : {QStringLiteral("importButton"), QStringLiteral("removeButton"),
                                    QStringLiteral("resetPositionButton"),
                                    QStringLiteral("welcomeButton")}) {
            auto *button = window.findChild<QPushButton *>(name);
            QVERIFY2(button, qPrintable(name));
            QVERIFY2(!button->accessibleName().isEmpty(), qPrintable(name));
        }
        auto *summary = window.findChild<QPlainTextEdit *>(QStringLiteral("resourceSummary"));
        QVERIFY(summary);
        QVERIFY(!summary->accessibleName().isEmpty());
    }

    // Regression: the motion, hemisphere, and language combos get their items in
    // retranslate(), so seeding their index anywhere earlier is a no-op on an empty
    // combo and the selection settled on the first entry. Reopening Settings then
    // reported "Follow system", "Northern", and "System default" regardless of what
    // was stored -- while the stored values themselves were untouched, so the panel
    // simply lied about them.
    void combosOpenShowingTheStoredSelectionNotTheFirstItem()
    {
        QTemporaryDir temp;
        const QString path = temp.filePath(QStringLiteral("settings.ini"));
        {
            AppSettings seed(path);
            seed.setMotionPreference(MotionPreference::Full);
            seed.setHemisphere(Hemisphere::South);
            seed.setLanguage(AppLanguage::English);
        }
        AppSettings settings(path);
        Localization localization(&settings);
        SettingsWindow window(&settings, &localization);

        auto comboIn = [&window](const QString &rowName) {
            auto *row = window.findChild<SettingsRow *>(rowName);
            return row ? qobject_cast<QComboBox *>(row->control()) : nullptr;
        };
        QCOMPARE(comboIn(QStringLiteral("motionRow"))->currentIndex(),
                 static_cast<int>(MotionPreference::Full));
        QCOMPARE(comboIn(QStringLiteral("hemisphereRow"))->currentIndex(),
                 static_cast<int>(Hemisphere::South));
        QCOMPARE(comboIn(QStringLiteral("languageRow"))->currentIndex(),
                 static_cast<int>(AppLanguage::English));

        // And nothing was written back while repopulating.
        QCOMPARE(settings.motionPreference(), MotionPreference::Full);
        QCOMPARE(settings.hemisphere(), Hemisphere::South);
        QCOMPARE(settings.language(), AppLanguage::English);
    }

    // retranslate() rebuilds these three combos rather than relabelling them, so it
    // has to save and restore the index under a signal blocker. Without that,
    // switching language silently resets the user's choices and writes the resets
    // back to settings -- a data-loss bug with no error message.
    void switchingLanguageDoesNotResetRebuiltCombos()
    {
        QTemporaryDir temp;
        AppSettings settings(temp.filePath(QStringLiteral("settings.ini")));
        settings.setLanguage(AppLanguage::English);
        Localization localization(&settings);
        SettingsWindow window(&settings, &localization);

        settings.setMotionPreference(MotionPreference::Reduce);
        settings.setHemisphere(Hemisphere::South);
        auto comboIn = [&window](const QString &rowName) {
            auto *row = window.findChild<SettingsRow *>(rowName);
            return row ? qobject_cast<QComboBox *>(row->control()) : nullptr;
        };
        QComboBox *motion = comboIn(QStringLiteral("motionRow"));
        QComboBox *hemisphere = comboIn(QStringLiteral("hemisphereRow"));
        QComboBox *language = comboIn(QStringLiteral("languageRow"));
        QVERIFY(motion && hemisphere && language);
        motion->setCurrentIndex(static_cast<int>(MotionPreference::Reduce));
        hemisphere->setCurrentIndex(static_cast<int>(Hemisphere::South));

        settings.setLanguage(AppLanguage::SimplifiedChinese);

        QCOMPARE(motion->currentIndex(), static_cast<int>(MotionPreference::Reduce));
        QCOMPARE(hemisphere->currentIndex(), static_cast<int>(Hemisphere::South));
        QCOMPARE(language->currentIndex(), static_cast<int>(AppLanguage::SimplifiedChinese));
        QCOMPARE(settings.motionPreference(), MotionPreference::Reduce);
        QCOMPARE(settings.hemisphere(), Hemisphere::South);
        // The combos were genuinely rebuilt, not merely left alone.
        QCOMPARE(motion->itemText(0), localization.text(TextKey::FollowSystem));
    }

    void validationReportsGoToTheBannerNotAnInlineStylesheet()
    {
        QTemporaryDir temp;
        AppSettings settings(temp.filePath(QStringLiteral("settings.ini")));
        settings.setLanguage(AppLanguage::English);
        Localization localization(&settings);
        SettingsWindow window(&settings, &localization);
        window.show();
        QTest::qWait(20);

        auto *banner = window.findChild<InlineBanner *>(QStringLiteral("validationBanner"));
        QVERIFY(banner);
        QVERIFY(!banner->isVisible());

        window.setValidationReport(QStringLiteral("clip too tall"), true);
        QVERIFY(banner->isVisible());
        QCOMPARE(banner->message(), QStringLiteral("clip too tall"));
        QCOMPARE(banner->severityName(), QStringLiteral("error"));

        window.setValidationReport(QStringLiteral("imported"), false);
        QCOMPARE(banner->severityName(), QStringLiteral("info"));

        window.setValidationReport(QString(), false);
        QVERIFY(!banner->isVisible());

        // The old red-text QPlainTextEdit was the repository's only setStyleSheet
        // call site outside Theme, and it hardcoded a light-mode colour.
        QVERIFY(banner->styleSheet().isEmpty());
    }

    void resourceSummaryStillRoundTripsForTheAppController()
    {
        QTemporaryDir temp;
        AppSettings settings(temp.filePath(QStringLiteral("settings.ini")));
        Localization localization(&settings);
        SettingsWindow window(&settings, &localization);

        const QString summary = QStringLiteral("spring-day -> spritesheet.webp\n  click=v2 fallback");
        window.setResourceSummary(summary);
        QCOMPARE(window.resourceSummary(), summary);
    }

    void environmentSummaryShowsOnTheEnvironmentPage()
    {
        QTemporaryDir temp;
        AppSettings settings(temp.filePath(QStringLiteral("settings.ini")));
        Localization localization(&settings);
        SettingsWindow window(&settings, &localization);

        window.setEnvironmentSummary(QStringLiteral("summer-day"));
        auto *row = window.findChild<SettingsRow *>(QStringLiteral("currentEnvironmentRow"));
        QVERIFY(row);
        bool found = false;
        for (QLabel *label : row->findChildren<QLabel *>()) {
            found |= label->text() == QStringLiteral("summer-day");
        }
        QVERIFY(found);
    }


    void geometrySurvivesCloseAndReopen()
    {
        QTemporaryDir temp;
        const QString path = temp.filePath(QStringLiteral("settings.ini"));
        // Derived from the real screen rather than hardcoded: a rect that does not fit
        // is deliberately clamped on restore, which would make a fixed expectation
        // fail for the right reason and hide whether persistence works at all.
        const QRect available = QGuiApplication::primaryScreen()->availableGeometry();
        const QRect target(available.left() + 20,
                           available.top() + 20,
                           qMin(700, available.width() - 40),
                           qMin(540, available.height() - 40));
        QRect placed;
        {
            AppSettings settings(path);
            Localization localization(&settings);
            SettingsWindow window(&settings, &localization);
            window.show();
            QTest::qWait(20);

            window.setGeometry(target);
            QTest::qWait(20);
            placed = window.geometry();
            QCOMPARE(placed, target);  // the chosen rect really does fit
            window.hide();  // flushes the coalesced write
            QVERIFY(settings.hasSettingsGeometry());
        }
        {
            AppSettings settings(path);
            Localization localization(&settings);
            SettingsWindow window(&settings, &localization);
            QCOMPARE(window.geometry(), placed);
        }
    }

    // A rect saved on a display that no longer exists must not strand the window
    // off-screen with no way to reach it.
    void geometrySavedOnAVanishedDisplayFallsBackToCentered()
    {
        QTemporaryDir temp;
        const QString path = temp.filePath(QStringLiteral("settings.ini"));
        {
            AppSettings settings(path);
            settings.setSettingsGeometry(QRect(9000, 7000, 700, 540));
        }
        AppSettings settings(path);
        Localization localization(&settings);
        SettingsWindow window(&settings, &localization);

        const QRect available = QGuiApplication::primaryScreen()->availableGeometry();
        QVERIFY2(available.intersects(window.geometry()),
                 "window must land somewhere reachable");
    }

    // PetPreviewWidget is marked WA_OpaquePaintEvent so an animating preview does not
    // drag its ancestors' chrome into every repaint (see commit 24b0aee). Qt then
    // skips erasing the widget background -- QWidgetPrivate::drawWidget paints it only
    // when the attribute is clear -- so paintEvent has to cover every pixel itself.
    // A gap would composite the pet against whatever was left in the backing store.
    void previewPaintsEveryPixelOfItsRect()
    {
        PetPreviewWidget preview;
        QVERIFY(preview.testAttribute(Qt::WA_OpaquePaintEvent));
        preview.resize(320, 240);
        preview.show();
        QVERIFY(QTest::qWaitForWindowExposed(&preview));

        // The no-frame path is the one that has to hold: a frame and its contact
        // shadow are drawn *over* an already-opaque stage, so they can only add
        // coverage, never leave a gap. Both schemes and two aspect ratios are checked
        // because the stage is cached per (size, DPR, scheme) and each combination
        // re-runs renderStage().
        assertCoversEveryPixel(&preview, "light, no atlas");

        preview.setColorScheme(Qt::ColorScheme::Dark);
        assertCoversEveryPixel(&preview, "dark, no atlas");

        // A non-square size exercises the rounded corners and the gradient at a
        // different aspect ratio.
        preview.resize(180, 300);
        QTest::qWait(20);
        assertCoversEveryPixel(&preview, "dark, tall");
    }

    // The stylesheet must not match PetPreviewWidget: a matching rule makes
    // QStyleSheetStyle set WA_StyledBackground on it, painting a background before
    // paintEvent runs and silently undoing the optimisation above.
    void themedWindowLeavesThePreviewsOpaquePaintIntact()
    {
        QTemporaryDir temp;
        AppSettings settings(temp.filePath(QStringLiteral("settings.ini")));
        Localization localization(&settings);
        SettingsWindow window(&settings, &localization);
        window.show();
        QTest::qWait(20);
        QVERIFY(!window.styleSheet().isEmpty());

        auto *preview = window.findChild<PetPreviewWidget *>();
        QVERIFY(preview);
        QVERIFY(preview->testAttribute(Qt::WA_OpaquePaintEvent));
        QVERIFY2(!preview->testAttribute(Qt::WA_StyledBackground),
                 "a stylesheet rule matched PetPreviewWidget");
    }
};

QTEST_MAIN(SettingsWindowTest)
#include "test_settings_window.moc"
