#include "pet/AtlasComposer.h"
#include "pet/PetAtlas.h"
#include "resources/PetPackageWriter.h"
#include "settings/AppSettings.h"
#include "settings/AtlasAssemblerWindow.h"
#include "settings/Localization.h"
#include "ui/InlineBanner.h"
#include "ui/SettingsRow.h"
#include "ui/Theme.h"

#include <QDir>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

namespace {

// A strip that composes cleanly, written to disk because the window's whole input is
// a path -- setStrip() is the seam, since QFileDialog cannot be driven offscreen.
bool writeStrip(const QString &path, int count)
{
    const int cellH = PetAtlas::CellHeight;
    const int cellW = PetAtlas::CellWidth;
    QImage strip(cellW * count, cellH, QImage::Format_ARGB32);
    strip.fill(AtlasComposer::defaultChromaKey());
    QPainter painter(&strip);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    for (int i = 0; i < count; ++i) {
        painter.fillRect(i * cellW + (cellW - 60) / 2, cellH - 100, 60, 90, QColor(200, 40, 40));
    }
    painter.end();
    return strip.save(path, "PNG");
}

// Fills every row of the window from strips written into `dir`.
void fillEveryRow(AtlasAssemblerWindow &window, const QDir &dir, int skipRow = -1)
{
    for (const AtlasComposer::RowSpec &spec : AtlasComposer::rows()) {
        if (spec.row == skipRow) continue;
        const QString path = dir.filePath(QStringLiteral("%1.png").arg(spec.name));
        QVERIFY(writeStrip(path, AtlasComposer::frameCount(spec.row)));
        window.setStrip(spec.row, path);
    }
}

InlineBanner *bannerOf(AtlasAssemblerWindow &window)
{
    return window.findChild<InlineBanner *>(QStringLiteral("assemblerBanner"));
}

QPushButton *buttonOf(AtlasAssemblerWindow &window, const char *name)
{
    return window.findChild<QPushButton *>(QLatin1StringView(name));
}

}  // namespace

class AtlasAssemblerWindowTest final : public QObject
{
    Q_OBJECT

private slots:
    void showsOneRowPerAtlasRowWithItsFrameCount()
    {
        QTemporaryDir temp;
        AppSettings settings(temp.filePath(QStringLiteral("settings.ini")));
        settings.setLanguage(AppLanguage::English);
        Localization localization(&settings);
        AtlasAssemblerWindow window(&localization);

        for (const AtlasComposer::RowSpec &spec : AtlasComposer::rows()) {
            auto *row = window.findChild<SettingsRow *>(
                QStringLiteral("assemblerRow%1").arg(spec.row));
            QVERIFY2(row, qPrintable(QString(spec.name)));
            // The contract name has to stay visible: it is what the user typed into
            // the prompt and named the file.
            QVERIFY2(row->labelText().contains(QString(spec.name)),
                     qPrintable(QStringLiteral("row %1 label \"%2\" lost the contract name")
                                    .arg(spec.row).arg(row->labelText())));
            QVERIFY(buttonOf(window, qPrintable(QStringLiteral("assemblerChoose%1").arg(spec.row))));
        }
    }

    // Mirrors the settings window's own check: anything not owned by a SettingsRow
    // has to be named by hand or it reaches VoiceOver as an unlabelled button.
    void everyControlOutsideARowHasAnAccessibleName()
    {
        QTemporaryDir temp;
        AppSettings settings(temp.filePath(QStringLiteral("settings.ini")));
        settings.setLanguage(AppLanguage::English);
        Localization localization(&settings);
        AtlasAssemblerWindow window(&localization);

        for (const char *name : {"assemblerFillFromFolderButton", "assemblerChromaKeyButton",
                                 "assemblerDetectKeyButton", "assemblerComposeButton",
                                 "assemblerInstallButton", "assemblerCloseButton"}) {
            QPushButton *button = buttonOf(window, name);
            QVERIFY2(button, name);
            QVERIFY2(!button->accessibleName().isEmpty(), name);
        }
        for (const AtlasComposer::RowSpec &spec : AtlasComposer::rows()) {
            QPushButton *choose =
                buttonOf(window, qPrintable(QStringLiteral("assemblerChoose%1").arg(spec.row)));
            QVERIFY(choose);
            QVERIFY2(!choose->accessibleName().isEmpty(), qPrintable(QString(spec.name)));
        }
    }

    // The install button may only be live when all three things the validator checks
    // already hold, so a failure this window could have predicted is unreachable.
    void installStaysDisabledUntilTheAtlasAndTheIdAreBothGood()
    {
        QTemporaryDir temp;
        AppSettings settings(temp.filePath(QStringLiteral("settings.ini")));
        settings.setLanguage(AppLanguage::English);
        Localization localization(&settings);
        AtlasAssemblerWindow window(&localization);

        QPushButton *install = buttonOf(window, "assemblerInstallButton");
        QPushButton *compose = buttonOf(window, "assemblerComposeButton");
        QVERIFY(install && compose);
        QVERIFY(!install->isEnabled());
        QVERIFY(!compose->isEnabled());

        // Ten of eleven rows: still blocked, because the missing one is blocking.
        QDir dir(temp.path());
        fillEveryRow(window, dir, 6);
        compose->click();
        QVERIFY(!install->isEnabled());

        // The eleventh row arrives, but there is still no name or id.
        const QString path = dir.filePath(QStringLiteral("waiting.png"));
        QVERIFY(writeStrip(path, AtlasComposer::frameCount(6)));
        window.setStrip(6, path);
        compose->click();
        QVERIFY(!install->isEnabled());

        auto *displayName = window.findChild<QLineEdit *>(QStringLiteral("assemblerDisplayName"));
        auto *petId = window.findChild<QLineEdit *>(QStringLiteral("assemblerPetId"));
        QVERIFY(displayName && petId);

        // Typing a name suggests an id, and that is enough for both checks at once.
        QTest::keyClicks(displayName, QStringLiteral("My Pet"));
        QCOMPARE(petId->text(), QStringLiteral("my-pet"));
        QVERIFY(install->isEnabled());

        // An id the validator would reject disables it again, rather than being
        // discovered at install time.
        petId->clear();
        QTest::keyClicks(petId, QStringLiteral("Not Valid"));
        QVERIFY(!install->isEnabled());
    }

    void installRequestedCarriesTheComposedAtlasAndTheTypedDetails()
    {
        QTemporaryDir temp;
        AppSettings settings(temp.filePath(QStringLiteral("settings.ini")));
        settings.setLanguage(AppLanguage::English);
        Localization localization(&settings);
        AtlasAssemblerWindow window(&localization);

        fillEveryRow(window, QDir(temp.path()));
        buttonOf(window, "assemblerComposeButton")->click();
        QTest::keyClicks(window.findChild<QLineEdit *>(QStringLiteral("assemblerDisplayName")),
                         QStringLiteral("Spud"));

        QSignalSpy spy(&window, &AtlasAssemblerWindow::installRequested);
        QPushButton *install = buttonOf(window, "assemblerInstallButton");
        QVERIFY(install->isEnabled());
        install->click();

        QCOMPARE(spy.count(), 1);
        const QList<QVariant> arguments = spy.takeFirst();
        const QImage atlas = arguments.at(0).value<QImage>();
        QCOMPARE(atlas.size(), QSize(PetAtlas::Width, PetAtlas::Height));
        QString error;
        QVERIFY2(PetAtlas::validateV2Occupancy(atlas, &error), qPrintable(error));

        const auto info = arguments.at(1).value<PetPackageWriter::PetInfo>();
        QCOMPARE(info.displayName, QStringLiteral("Spud"));
        QCOMPARE(info.id, QStringLiteral("spud"));
    }

    // The composer speaks in codes; the window has to turn them into a sentence that
    // names the row the way the user knows it.
    void theBannerNamesTheRowThatIsMissing()
    {
        QTemporaryDir temp;
        AppSettings settings(temp.filePath(QStringLiteral("settings.ini")));
        settings.setLanguage(AppLanguage::English);
        Localization localization(&settings);
        AtlasAssemblerWindow window(&localization);

        fillEveryRow(window, QDir(temp.path()), 4);
        buttonOf(window, "assemblerComposeButton")->click();

        InlineBanner *banner = bannerOf(window);
        QVERIFY(banner);
        QCOMPARE(banner->severity(), InlineBanner::Severity::Error);
        QVERIFY2(banner->message().contains(QStringLiteral("jumping")),
                 qPrintable(banner->message()));

        // And once it is there, the banner reports readiness with the fitted scale.
        const QString path = QDir(temp.path()).filePath(QStringLiteral("jumping.png"));
        QVERIFY(writeStrip(path, AtlasComposer::frameCount(4)));
        window.setStrip(4, path);
        buttonOf(window, "assemblerComposeButton")->click();
        QCOMPARE(banner->severity(), InlineBanner::Severity::Info);
        QVERIFY2(banner->message().contains(QStringLiteral("ready")), qPrintable(banner->message()));
    }

    // The import verdict comes from AppController, and has to arrive labelled --
    // otherwise it is an unattributed line of validator English.
    void theImportVerdictIsLabelledAsAValidationReport()
    {
        QTemporaryDir temp;
        AppSettings settings(temp.filePath(QStringLiteral("settings.ini")));
        settings.setLanguage(AppLanguage::English);
        Localization localization(&settings);
        AtlasAssemblerWindow window(&localization);

        window.setInstallReport(QStringLiteral("spritesheet.webp: atlas must be 1536x2288"), true);
        InlineBanner *banner = bannerOf(window);
        QCOMPARE(banner->severity(), InlineBanner::Severity::Error);
        QVERIFY(banner->message().contains(localization.text(TextKey::ValidationReport)));
        QVERIFY(banner->message().contains(QStringLiteral("1536x2288")));

        window.setInstallReport(QString(), false);
        QVERIFY(banner->message().isEmpty());
    }

    // Eleven strips plus a 14 MB atlas would otherwise stay resident for the session:
    // the window is built on first use and then never destroyed.
    void hidingDropsTheDecodedStripsButKeepsThePaths()
    {
        QTemporaryDir temp;
        AppSettings settings(temp.filePath(QStringLiteral("settings.ini")));
        settings.setLanguage(AppLanguage::English);
        Localization localization(&settings);
        AtlasAssemblerWindow window(&localization);

        fillEveryRow(window, QDir(temp.path()));
        buttonOf(window, "assemblerComposeButton")->click();
        for (const AtlasComposer::RowSpec &spec : AtlasComposer::rows()) {
            QVERIFY2(window.hasDecodedStrip(spec.row), qPrintable(QString(spec.name)));
        }

        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        window.hide();

        for (const AtlasComposer::RowSpec &spec : AtlasComposer::rows()) {
            QVERIFY2(!window.hasDecodedStrip(spec.row), qPrintable(QString(spec.name)));
        }
        // Install is off until it recomposes, so the dropped atlas cannot be sent.
        QVERIFY(!buttonOf(window, "assemblerInstallButton")->isEnabled());
        // But the paths survived, so one click puts it all back.
        buttonOf(window, "assemblerComposeButton")->click();
        for (const AtlasComposer::RowSpec &spec : AtlasComposer::rows()) {
            QVERIFY2(window.hasDecodedStrip(spec.row), qPrintable(QString(spec.name)));
        }
    }

    void retranslatesIntoBothLanguages()
    {
        QTemporaryDir temp;
        AppSettings settings(temp.filePath(QStringLiteral("settings.ini")));
        settings.setLanguage(AppLanguage::English);
        Localization localization(&settings);
        AtlasAssemblerWindow window(&localization);

        const QString english = window.windowTitle();
        QCOMPARE(english, localization.text(TextKey::AssemblerTitle));

        settings.setLanguage(AppLanguage::SimplifiedChinese);
        QVERIFY(window.windowTitle() != english);
        QCOMPARE(window.windowTitle(), localization.text(TextKey::AssemblerTitle));

        // The row labels keep the contract name in both languages, because it is the
        // file name and the prompt's own word.
        for (const AtlasComposer::RowSpec &spec : AtlasComposer::rows()) {
            auto *row = window.findChild<SettingsRow *>(
                QStringLiteral("assemblerRow%1").arg(spec.row));
            QVERIFY2(row->labelText().contains(QString(spec.name)),
                     qPrintable(row->labelText()));
        }
    }

    void themeAppliesOnFirstShowAndFollowsTheScheme()
    {
        QTemporaryDir temp;
        AppSettings settings(temp.filePath(QStringLiteral("settings.ini")));
        Localization localization(&settings);
        AtlasAssemblerWindow window(&localization);

        QVERIFY(window.styleSheet().isEmpty());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QVERIFY(!window.styleSheet().isEmpty());

        auto *watcher = window.findChild<ThemeWatcher *>();
        QVERIFY(watcher);
        watcher->setSchemeOverride(Qt::ColorScheme::Dark);
        QCOMPARE(window.styleSheet(), Theme::styleSheet(Qt::ColorScheme::Dark));
    }
};

// Runs inside a grouped binary; see tests/support/TestRunner.h.
QObject *createAtlasAssemblerWindowTest() { return new AtlasAssemblerWindowTest; }
#include "test_atlas_assembler_window.moc"
