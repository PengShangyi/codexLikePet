#include "ui/Theme.h"

#include <QGuiApplication>
#include <QPalette>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QStyleHints>
#include <QTest>

class ThemeTest final : public QObject
{
    Q_OBJECT

private slots:
    void unknownSchemeCollapsesToLight()
    {
        QCOMPARE(Theme::effectiveScheme(Qt::ColorScheme::Unknown), Qt::ColorScheme::Light);
        QCOMPARE(Theme::effectiveScheme(Qt::ColorScheme::Light), Qt::ColorScheme::Light);
        QCOMPARE(Theme::effectiveScheme(Qt::ColorScheme::Dark), Qt::ColorScheme::Dark);
    }

    void bothSchemesProduceOpaqueDistinctSurfaces()
    {
        const Theme::Palette light = Theme::palette(Qt::ColorScheme::Light);
        const Theme::Palette dark = Theme::palette(Qt::ColorScheme::Dark);

        // The preview stage relies on these being fully opaque: it fills its whole
        // rect with pageBackground and then paints the gradient card on top, and a
        // translucent fill would composite against uninitialized backing store.
        for (const QColor &color : {light.pageBackground, light.cardBackground,
                                    light.stageTop, light.stageBottom, light.sunken,
                                    dark.pageBackground, dark.cardBackground,
                                    dark.stageTop, dark.stageBottom, dark.sunken}) {
            QCOMPARE(color.alpha(), 255);
        }

        // Text must actually contrast with the card it sits on, in both schemes. The
        // first version of this derived text from QPalette and produced dark-on-dark.
        QVERIFY(qAbs(light.textPrimary.lightnessF() - light.cardBackground.lightnessF()) > 0.4);
        QVERIFY(qAbs(dark.textPrimary.lightnessF() - dark.cardBackground.lightnessF()) > 0.4);
        QVERIFY(qAbs(light.accentText.lightnessF() - light.accent.lightnessF()) > 0.3);
        QVERIFY(qAbs(dark.accentText.lightnessF() - dark.accent.lightnessF()) > 0.3);
        // Secondary text has to be distinguishable from primary, or row descriptions
        // and value readouts carry no hierarchy.
        QVERIFY(light.textSecondary != light.textPrimary);
        QVERIFY(dark.textSecondary != dark.textPrimary);

        QVERIFY(light.pageBackground != dark.pageBackground);
        QVERIFY(light.cardBackground != dark.cardBackground);
        QVERIFY(light.stageTop != dark.stageTop);
        QVERIFY(light.danger != dark.danger);

        // Dark surfaces must actually be darker, or the scheme names lie.
        QVERIFY(dark.pageBackground.lightnessF() < light.pageBackground.lightnessF());
        QVERIFY(dark.cardBackground.lightnessF() < light.cardBackground.lightnessF());
    }

    void styleSheetIsSchemeDependentAndOnlyTargetsQualifiedSelectors()
    {
        const QString light = Theme::styleSheet(Qt::ColorScheme::Light);
        const QString dark = Theme::styleSheet(Qt::ColorScheme::Dark);
        QVERIFY(!light.isEmpty());
        QVERIFY(light != dark);

        // Qt's stylesheet parser understands #RRGGBB but not #AARRGGBB, so an
        // eight-digit hex would be silently misread and the rule dropped with only
        // a stderr line nobody reads. Every color must be rgba(). Matches a '#'
        // in property-value position; '#' in an ID selector is fine.
        const QRegularExpression hexColor(QStringLiteral(":\\s*#"));
        QVERIFY(!light.contains(hexColor));
        QVERIFY(!dark.contains(hexColor));

        // QFileDialog and QMessageBox are parented to the settings window and so
        // inherit its sheet. An unqualified rule on a native control would restyle
        // their buttons and defeat QMacStyle; assert none of them appears bare.
        for (const QString &control : {QStringLiteral("QPushButton"), QStringLiteral("QComboBox"),
                                       QStringLiteral("QSlider"), QStringLiteral("QCheckBox"),
                                       QStringLiteral("QTimeEdit"), QStringLiteral("QScrollBar"),
                                       QStringLiteral("QMenu")}) {
            QVERIFY2(!light.contains(control),
                     qPrintable(QStringLiteral("stylesheet must not mention %1").arg(control)));
        }

        // No font-* property: QSS font sizes are DPI-blind and stop following
        // QApplication::setFont. Fonts come from Theme's font helpers instead.
        QVERIFY(!light.contains(QStringLiteral("font-")));

        // A rule matching PetPreviewWidget would make QStyleSheetStyle set
        // WA_StyledBackground on it, painting a background before paintEvent and
        // silently undoing the opaque-paint optimisation.
        QVERIFY(!light.contains(QStringLiteral("PetPreviewWidget")));
    }

    void fontHelpersDeriveFromTheBaseFont()
    {
        QFont base;
        base.setPointSizeF(13.0);
        base.setWeight(QFont::Normal);

        QCOMPARE(Theme::cardTitleFont(base).weight(), QFont::DemiBold);
        QCOMPARE(Theme::rowLabelFont(base).pointSizeF(), 13.0);
        QVERIFY(Theme::rowDescriptionFont(base).pointSizeF() < 13.0);
        QVERIFY(Theme::navItemFont(base).pointSizeF() < 13.0);

        // Never below the macOS minimum legible size, however small the base is.
        QFont tiny;
        tiny.setPointSizeF(8.0);
        QVERIFY(Theme::rowDescriptionFont(tiny).pointSizeF() >= 10.0);
    }

    // Regression guard for an ordering bug that shipped dark text onto dark cards:
    // QStyleHints::colorSchemeChanged is emitted BEFORE Qt swaps the application
    // palette, so a stylesheet rebuilt on that signal saw the outgoing scheme. Theme
    // must therefore not consult QGuiApplication::palette() at all. Forcing the app
    // palette to disagree with the requested scheme proves it does not.
    void paletteIgnoresTheApplicationPalette()
    {
        const Theme::Palette darkBefore = Theme::palette(Qt::ColorScheme::Dark);
        const Theme::Palette lightBefore = Theme::palette(Qt::ColorScheme::Light);

        QPalette hostile;
        hostile.setColor(QPalette::WindowText, QColor(255, 0, 255));
        hostile.setColor(QPalette::PlaceholderText, QColor(0, 255, 0));
        hostile.setColor(QPalette::Highlight, QColor(0, 0, 255));
        hostile.setColor(QPalette::HighlightedText, QColor(255, 255, 0));
        const QPalette saved = QGuiApplication::palette();
        QGuiApplication::setPalette(hostile);

        QCOMPARE(Theme::palette(Qt::ColorScheme::Dark).textPrimary, darkBefore.textPrimary);
        QCOMPARE(Theme::palette(Qt::ColorScheme::Dark).textSecondary, darkBefore.textSecondary);
        QCOMPARE(Theme::palette(Qt::ColorScheme::Dark).accent, darkBefore.accent);
        QCOMPARE(Theme::palette(Qt::ColorScheme::Light).accentText, lightBefore.accentText);

        QGuiApplication::setPalette(saved);
    }

    // The system-driven path, which is the one that actually runs in the shipped
    // app. Measured behaviour: under the cocoa plugin setColorScheme() updates
    // colorScheme() and emits colorSchemeChanged; under the offscreen plugin --
    // which every widget test here uses -- it is a complete no-op and
    // colorScheme() stays Unknown forever. So this half can only be verified on a
    // real platform, and ThemeWatcher::setSchemeOverride() exists to give the
    // offscreen tests a way in. Run with QT_QPA_PLATFORM=cocoa to exercise it.
    void watcherFollowsSystemStyleHints()
    {
        QStyleHints *hints = QGuiApplication::styleHints();
        QVERIFY(hints);

        hints->setColorScheme(Qt::ColorScheme::Light);
        if (hints->colorScheme() != Qt::ColorScheme::Light) {
            QSKIP("platform plugin ignores QStyleHints::setColorScheme(); "
                  "covered through setSchemeOverride() instead");
        }

        ThemeWatcher watcher;
        QCOMPARE(watcher.scheme(), Qt::ColorScheme::Light);

        QSignalSpy spy(&watcher, &ThemeWatcher::schemeChanged);
        hints->setColorScheme(Qt::ColorScheme::Dark);
        QCOMPARE(watcher.scheme(), Qt::ColorScheme::Dark);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).value<Qt::ColorScheme>(), Qt::ColorScheme::Dark);

        hints->unsetColorScheme();
    }

    // Runs on every platform, and is what actually guards the emit-only-on-change
    // discipline the settings window relies on to avoid needless restyling.
    void watcherOverrideDrivesSchemeAndEmitsOnlyOnRealChanges()
    {
        ThemeWatcher watcher;
        watcher.setSchemeOverride(Qt::ColorScheme::Light);
        QCOMPARE(watcher.scheme(), Qt::ColorScheme::Light);

        QSignalSpy spy(&watcher, &ThemeWatcher::schemeChanged);

        watcher.setSchemeOverride(Qt::ColorScheme::Dark);
        QCOMPARE(watcher.scheme(), Qt::ColorScheme::Dark);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).value<Qt::ColorScheme>(), Qt::ColorScheme::Dark);

        // Same value again: no signal.
        watcher.setSchemeOverride(Qt::ColorScheme::Dark);
        QCOMPARE(spy.count(), 0);

        // Unknown means "follow the system again". Under offscreen the system
        // reports Unknown, which collapses to Light -- a real change from Dark.
        watcher.setSchemeOverride(Qt::ColorScheme::Unknown);
        QCOMPARE(watcher.scheme(), Theme::effectiveScheme(
                                       QGuiApplication::styleHints()->colorScheme()));
        QCOMPARE(spy.count(), 1);
    }
};

QTEST_MAIN(ThemeTest)
#include "test_theme.moc"
