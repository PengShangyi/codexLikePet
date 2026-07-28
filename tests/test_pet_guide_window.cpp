#include "pet/AtlasComposer.h"
#include "settings/AppSettings.h"
#include "settings/Localization.h"
#include "settings/PetGuideWindow.h"
#include "ui/Theme.h"

#include <QClipboard>
#include <QLabel>
#include <QGuiApplication>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTextCursor>
#include <QTextDocument>

class PetGuideWindowTest final : public QObject
{
    Q_OBJECT

private slots:
    // The three snippet boxes carry the whole point of the window, so they must be
    // the objectName the stylesheet targets. Unnamed, they fall back to the native
    // sunken panel on a default background -- which is how they shipped first, and
    // is the one place in the app that did not look like the rest of it.
    void snippetBoxesAreNamedForTheStylesheetRuleThatStylesThem()
    {
        QTemporaryDir temp;
        AppSettings settings(temp.filePath(QStringLiteral("settings.ini")));
        Localization localization(&settings);
        PetGuideWindow window(&localization);

        const auto boxes = window.findChildren<QPlainTextEdit *>();
        QCOMPARE(boxes.size(), 3);
        for (const QPlainTextEdit *box : boxes) {
            QCOMPARE(box->objectName(), QStringLiteral("petGuideSnippet"));
            // The stylesheet draws the border; a native frame as well would nest one
            // inside the other.
            QCOMPARE(box->frameShape(), QFrame::NoFrame);
        }

        // Both ends of the arrangement: the rule exists, and the window is inside
        // the styled subtree at all. It is a separate top-level window, so it does
        // not inherit the settings window's sheet -- it applies its own on show.
        QVERIFY(Theme::styleSheet(Qt::ColorScheme::Light)
                    .contains(QStringLiteral("QPlainTextEdit#petGuideSnippet")));
        QVERIFY(window.styleSheet().isEmpty());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QVERIFY(window.styleSheet().contains(QStringLiteral("QPlainTextEdit#petGuideSnippet")));
    }

    // The window takes its background from Theme but a QLabel takes its colour from
    // QPalette, and the two authorities only coincidentally agree -- Theme defines
    // both schemes outright and deliberately never consults the palette. Measured
    // with the scheme forced to Dark while the platform stayed light, an unnamed
    // label painted rgb(0, 0, 0) on the rgb(28, 26, 24) page. So every line of prose
    // here has to be named for the stylesheet to colour it.
    void everyLabelIsNamedSoTheSheetOwnsItsColour()
    {
        QTemporaryDir temp;
        AppSettings settings(temp.filePath(QStringLiteral("settings.ini")));
        Localization localization(&settings);
        PetGuideWindow window(&localization);

        const auto labels = window.findChildren<QLabel *>();
        QVERIFY(labels.size() >= 12);
        for (const QLabel *label : labels) {
            QVERIFY2(label->objectName() == QStringLiteral("petGuideText"),
                     qPrintable(QStringLiteral("label \"%1\" is not named for the stylesheet")
                                    .arg(label->text().left(40))));
        }
        QVERIFY(Theme::styleSheet(Qt::ColorScheme::Dark)
                    .contains(QStringLiteral("QLabel#petGuideText")));
    }

    // Each box freezes its height to fit every line, because a box one line short of
    // its content reads as broken rather than as scrollable. That calculation has
    // been wrong once already -- QFontMetrics::lineSpacing() disagrees with the
    // document layout by under a point for the fixed-width system font, which cost
    // the longest prompt its last line -- and it stays fragile: the snippets are
    // literals in the source, so adding a line to one is a plain text edit that
    // nothing else would flag.
    //
    // Measured through the caret rather than by restating the arithmetic. Note
    // QPlainTextDocumentLayout reports documentSize().height() in *lines*, not
    // pixels, so comparing that against a pixel height proves nothing.
    void noSnippetBoxClipsItsLastLine()
    {
        QTemporaryDir temp;
        AppSettings settings(temp.filePath(QStringLiteral("settings.ini")));
        Localization localization(&settings);
        PetGuideWindow window(&localization);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));

        const auto boxes = window.findChildren<QPlainTextEdit *>();
        QCOMPARE(boxes.size(), 3);
        for (QPlainTextEdit *box : boxes) {
            QVERIFY(!box->toPlainText().isEmpty());
            QCOMPARE(box->verticalScrollBar()->maximum(), 0);

            QTextCursor end(box->document());
            end.movePosition(QTextCursor::End);
            QVERIFY2(box->cursorRect(end).bottom() <= box->viewport()->rect().bottom(),
                     qPrintable(QStringLiteral("last line of a %1-line snippet falls outside"
                                               " a %2px viewport")
                                    .arg(box->document()->blockCount())
                                    .arg(box->viewport()->height())));

            // The height reserves a horizontal scrollbar unconditionally, because
            // whether one appears depends on the window width and it would otherwise
            // eat the last line only once the window was narrow. At the shipped
            // widths it never appears, so the reserve is invisible slack -- and
            // forcing the scrollbar on is the only way to find out it still fits.
            //
            // Which is worth doing: before kSnippetHeightSlack existed, the reserve
            // was consumed to the pixel, and the two platforms broke the tie
            // differently -- cocoa showed every line, the offscreen plugin asked for
            // one line of scroll. This assertion is what caught that.
            box->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
            QCOMPARE(box->verticalScrollBar()->maximum(), 0);
            QVERIFY2(box->cursorRect(end).bottom() <= box->viewport()->rect().bottom(),
                     "the reserved horizontal scrollbar height no longer covers a shown one");
        }
    }

    // The snippets are model input and file keys, so what lands on the clipboard has
    // to be the box's text verbatim -- not a reflowed or re-styled version of it.
    void copyPutsTheSnippetOnTheClipboardVerbatim()
    {
        QClipboard *clipboard = QGuiApplication::clipboard();
        if (!clipboard) QSKIP("platform plugin provides no clipboard");

        QTemporaryDir temp;
        AppSettings settings(temp.filePath(QStringLiteral("settings.ini")));
        settings.setLanguage(AppLanguage::English);
        Localization localization(&settings);
        PetGuideWindow window(&localization);

        const auto boxes = window.findChildren<QPlainTextEdit *>();
        QCOMPARE(boxes.size(), 3);
        // Each Copy button shares a caption row with its label and sits beside the
        // box in the column, so pair them by order.
        const auto buttons = window.findChildren<QPushButton *>(QStringLiteral("petGuideCopyButton"));
        QCOMPARE(buttons.size(), 3);

        for (int i = 0; i < 3; ++i) {
            clipboard->clear();
            buttons.at(i)->click();
            QCOMPARE(clipboard->text(), boxes.at(i)->toPlainText());
            // The button reports back, and says so in the current language.
            QCOMPARE(buttons.at(i)->text(), localization.text(TextKey::PetGuideCopied));
        }
    }

    // The assembler button belongs in step 3, which is the step it does the work of.
    // Placed anywhere else it reads as an alternative to the guide rather than as the
    // next thing to do after the two prompts above it.
    void theAssemblerButtonSitsInStepThreeAndAsksForTheAssembler()
    {
        QTemporaryDir temp;
        AppSettings settings(temp.filePath(QStringLiteral("settings.ini")));
        settings.setLanguage(AppLanguage::English);
        Localization localization(&settings);
        PetGuideWindow window(&localization);

        auto *assembler = window.findChild<QPushButton *>(
            QStringLiteral("petGuideAssemblerButton"));
        QVERIFY(assembler);
        QCOMPARE(assembler->text(), localization.text(TextKey::AssemblerButton));
        QVERIFY(!assembler->accessibleName().isEmpty());

        // Between the step 3 body and the step 4 heading, in the scrolling column
        // rather than the action strip at the bottom.
        const auto labels = window.findChildren<QLabel *>();
        int step3 = -1;
        int step4 = -1;
        for (int i = 0; i < labels.size(); ++i) {
            if (labels.at(i)->text() == localization.text(TextKey::PetGuideStep3Body)) step3 = i;
            if (labels.at(i)->text() == localization.text(TextKey::PetGuideStep4Heading)) step4 = i;
        }
        QVERIFY(step3 >= 0 && step4 > step3);
        QVERIFY(assembler->parentWidget() != window.findChild<QPushButton *>(
                                                 QStringLiteral("petGuideCloseButton"))
                                                 ->parentWidget());

        QSignalSpy spy(&window, &PetGuideWindow::atlasAssemblerRequested);
        assembler->click();
        QCOMPARE(spy.count(), 1);
    }

    // Cheap cross-check that the prompt the user pastes and the key the assembler
    // defaults to cannot drift apart. The prompt is a literal with fixed line lengths,
    // so it deliberately is not templated -- which is exactly why this needs asserting.
    void thePromptTemplatesNameTheAssemblersDefaultChromaKey()
    {
        QTemporaryDir temp;
        AppSettings settings(temp.filePath(QStringLiteral("settings.ini")));
        Localization localization(&settings);
        PetGuideWindow window(&localization);

        const QString key = AtlasComposer::defaultChromaKey().name(QColor::HexRgb).toUpper();
        QCOMPARE(key, QStringLiteral("#00B140"));
        int mentions = 0;
        for (const QPlainTextEdit *box : window.findChildren<QPlainTextEdit *>()) {
            if (box->toPlainText().contains(key)) ++mentions;
        }
        QVERIFY2(mentions >= 2,
                 qPrintable(QStringLiteral("only %1 prompt(s) name %2").arg(mentions).arg(key)));
    }

    // The window owns its own ThemeWatcher because it is not in the settings
    // window's subtree, so a scheme change has to reach it directly.
    void schemeChangeRestylesTheOpenWindow()
    {
        QTemporaryDir temp;
        AppSettings settings(temp.filePath(QStringLiteral("settings.ini")));
        Localization localization(&settings);
        PetGuideWindow window(&localization);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));

        auto *watcher = window.findChild<ThemeWatcher *>();
        QVERIFY(watcher);
        watcher->setSchemeOverride(Qt::ColorScheme::Light);
        const QString light = window.styleSheet();
        watcher->setSchemeOverride(Qt::ColorScheme::Dark);
        const QString dark = window.styleSheet();

        QVERIFY(!light.isEmpty());
        QVERIFY(light != dark);
        QCOMPARE(dark, Theme::styleSheet(Qt::ColorScheme::Dark));
    }
};

// Runs inside a grouped binary; see tests/support/TestRunner.h.
QObject *createPetGuideWindowTest() { return new PetGuideWindowTest; }
#include "test_pet_guide_window.moc"
