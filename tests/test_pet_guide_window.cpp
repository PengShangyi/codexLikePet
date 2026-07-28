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
        auto buttons = window.findChildren<QPushButton *>();
        buttons.removeIf([](const QPushButton *button) { return !button->objectName().isEmpty(); });
        QCOMPARE(buttons.size(), 3);

        for (int i = 0; i < 3; ++i) {
            clipboard->clear();
            buttons.at(i)->click();
            QCOMPARE(clipboard->text(), boxes.at(i)->toPlainText());
            // The button reports back, and says so in the current language.
            QCOMPARE(buttons.at(i)->text(), localization.text(TextKey::PetGuideCopied));
        }
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
