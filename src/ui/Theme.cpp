#include "ui/Theme.h"

#include <QGuiApplication>
#include <QStringList>
#include <QStyleHints>

namespace {

// Qt's stylesheet color parser understands #RGB and #RRGGBB but not #AARRGGBB,
// so an eight-digit hex would be silently misread. rgba() with an integer alpha
// is the one spelling that covers opaque and translucent alike.
QString css(const QColor &color)
{
    return QStringLiteral("rgba(%1, %2, %3, %4)")
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue())
        .arg(color.alpha());
}

QColor withAlpha(QColor color, int alpha)
{
    color.setAlpha(alpha);
    return color;
}

}  // namespace

namespace Theme {

Qt::ColorScheme effectiveScheme(Qt::ColorScheme raw)
{
    return raw == Qt::ColorScheme::Dark ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light;
}

QColor brandPotato()
{
    return QColor(205, 154, 92);
}

QColor brandOutline()
{
    return QColor(63, 45, 33);
}

Palette palette(Qt::ColorScheme scheme)
{
    const bool dark = effectiveScheme(scheme) == Qt::ColorScheme::Dark;

    // Every colour is defined here rather than read from QGuiApplication::palette(),
    // for two measured reasons:
    //
    //  - Ordering. QStyleHints::colorSchemeChanged is emitted *before* Qt swaps the
    //    application palette, so a handler that rebuilds the stylesheet on that
    //    signal reads the outgoing scheme's colours. That produced dark text on dark
    //    cards whenever macOS appearance changed while Settings was open.
    //  - Hierarchy. On this platform QPalette::PlaceholderText equals
    //    QPalette::WindowText (both #000000 in light, #ffffff in dark), so deriving
    //    "secondary" text from it gave row descriptions and value readouts no
    //    distinction from their labels at all.
    //
    // The trade-off is that the panel no longer follows the user's macOS accent
    // colour. The nav pill uses the brand warm instead, which suits a desktop-pet
    // app better than a borrowed system blue -- and the native controls inside the
    // cards are unstyled, so they still follow the accent and NSAppearance for free.
    Palette p;
    if (dark) {
        p.textPrimary = QColor(239, 234, 226);
        p.textSecondary = QColor(167, 156, 142);
        p.accent = QColor(206, 151, 90);
        p.accentText = QColor(36, 30, 24);

        p.pageBackground = QColor(28, 26, 24);
        p.cardBackground = QColor(42, 39, 36);
        p.cardBorder = QColor(255, 255, 255, 24);
        p.hairline = QColor(255, 255, 255, 18);
        p.sunken = QColor(33, 31, 28);
        p.navHover = QColor(255, 255, 255, 20);

        p.danger = QColor(255, 138, 128);
        p.dangerBackground = QColor(255, 138, 128, 28);
        p.dangerBorder = QColor(255, 138, 128, 70);

        p.stageTop = QColor(53, 48, 42);
        p.stageBottom = QColor(37, 34, 30);
        p.stageBorder = QColor(255, 255, 255, 20);
        p.stageShadow = QColor(0, 0, 0, 120);

        p.bubbleFill = QColor(48, 42, 36, 245);
        p.bubbleBorder = QColor(196, 168, 132);
        p.bubbleText = QColor(245, 238, 228);
    } else {
        p.textPrimary = QColor(30, 27, 24);
        p.textSecondary = QColor(120, 110, 98);
        p.accent = QColor(178, 120, 62);
        p.accentText = QColor(255, 252, 246);

        p.pageBackground = QColor(245, 241, 234);
        p.cardBackground = QColor(255, 255, 255);
        p.cardBorder = QColor(0, 0, 0, 22);
        p.hairline = QColor(0, 0, 0, 16);
        p.sunken = QColor(248, 245, 239);
        p.navHover = QColor(0, 0, 0, 14);

        p.danger = QColor(160, 32, 32);
        p.dangerBackground = QColor(160, 32, 32, 20);
        p.dangerBorder = QColor(160, 32, 32, 60);

        p.stageTop = QColor(250, 246, 238);
        p.stageBottom = QColor(238, 229, 215);
        p.stageBorder = QColor(0, 0, 0, 20);
        p.stageShadow = QColor(94, 67, 45, 46);

        p.bubbleFill = QColor(255, 252, 244, 245);
        p.bubbleBorder = QColor(94, 67, 45);
        p.bubbleText = QColor(50, 38, 30);
    }

    p.infoBackground = withAlpha(p.accent, dark ? 34 : 24);
    p.infoBorder = withAlpha(p.accent, dark ? 80 : 62);
    return p;
}

QFont cardTitleFont(const QFont &base)
{
    QFont font = base;
    font.setWeight(QFont::DemiBold);
    return font;
}

QFont rowLabelFont(const QFont &base)
{
    return base;
}

QFont rowDescriptionFont(const QFont &base)
{
    QFont font = base;
    // Fonts are set in C++ rather than through QSS on purpose: a QSS font-size in
    // px ignores the device DPI, and a QSS-set font stops following
    // QApplication::setFont.
    font.setPointSizeF(qMax(10.0, base.pointSizeF() - 2.0));
    return font;
}

QFont navItemFont(const QFont &base)
{
    QFont font = base;
    font.setPointSizeF(qMax(10.0, base.pointSizeF() - 2.0));
    return font;
}

QString styleSheet(Qt::ColorScheme scheme)
{
    const Palette p = palette(scheme);
    QStringList rules;

    rules << QStringLiteral("QWidget#settingsRoot, QWidget#pageContent,"
                            " QWidget#petGuideRoot { background: %1; }")
                 .arg(css(p.pageBackground));

    rules << QStringLiteral("QWidget#navBar { background: %1; border-bottom: 1px solid %2; }")
                 .arg(css(p.cardBackground), css(p.hairline));
    // The pet guide's action strip is the same separated chrome bar with the one
    // edge flipped: it sits below a scrolling body rather than above one.
    rules << QStringLiteral("QWidget#petGuideActions { background: %1;"
                            " border-top: 1px solid %2; }")
                 .arg(css(p.cardBackground), css(p.hairline));

    rules << QStringLiteral("QToolButton#navItem { background: transparent; border: none;"
                            " border-radius: %1px; color: %2; padding: 6px 8px;"
                            " min-width: %3px; }")
                 .arg(Metrics::navItemRadius)
                 .arg(css(p.textSecondary))
                 .arg(Metrics::navItemMinWidth);
    rules << QStringLiteral("QToolButton#navItem:hover { background: %1; color: %2; }")
                 .arg(css(p.navHover), css(p.textPrimary));
    rules << QStringLiteral("QToolButton#navItem:checked { background: %1; color: %2; }")
                 .arg(css(p.accent), css(p.accentText));

    rules << QStringLiteral("SettingsCard { background: %1; border: 1px solid %2;"
                            " border-radius: %3px; }")
                 .arg(css(p.cardBackground), css(p.cardBorder))
                 .arg(Metrics::cardRadius);

    // petGuideText covers every line of prose in the pet guide -- its title, intro,
    // step headings and bodies alike. All of it has to be named, because that window
    // takes its background from this sheet: leaving a label unnamed would colour it
    // from QPalette instead, and the two authorities only happen to agree. Forcing a
    // dark scheme while the system is light paints pure black on a near-black page.
    // Hierarchy there comes from weight and size, not colour.
    rules << QStringLiteral("QLabel#cardTitle, QLabel#rowLabel,"
                            " QLabel#petGuideText { color: %1; background: transparent; }")
                 .arg(css(p.textPrimary));
    rules << QStringLiteral("QLabel#rowDescription, QLabel#valueReadout,"
                            " QLabel#rowValue { color: %1; background: transparent; }")
                 .arg(css(p.textSecondary));

    rules << QStringLiteral("QFrame#cardHairline { background: %1; border: none;"
                            " max-height: %2px; min-height: %2px; }")
                 .arg(css(p.hairline))
                 .arg(Metrics::hairlineThickness);

    rules << QStringLiteral("InlineBanner { background: %1; border: 1px solid %2;"
                            " border-radius: %3px; }")
                 .arg(css(p.infoBackground), css(p.infoBorder))
                 .arg(Metrics::bannerRadius);
    rules << QStringLiteral("InlineBanner[severity=\"error\"] { background: %1;"
                            " border: 1px solid %2; }")
                 .arg(css(p.dangerBackground), css(p.dangerBorder));
    rules << QStringLiteral("InlineBanner QLabel#bannerText { color: %1; background: transparent; }")
                 .arg(css(p.textPrimary));
    rules << QStringLiteral("InlineBanner[severity=\"error\"] QLabel#bannerText { color: %1; }")
                 .arg(css(p.danger));

    rules << QStringLiteral("QToolButton#disclosureHeader { background: transparent;"
                            " border: none; color: %1; padding: 4px 2px;"
                            " text-align: left; }")
                 .arg(css(p.textSecondary));
    rules << QStringLiteral("QToolButton#disclosureHeader:hover,"
                            " QToolButton#disclosureHeader:checked { color: %1; }")
                 .arg(css(p.textPrimary));

    rules << QStringLiteral("QScrollArea#pageScroll { border: none; background: transparent; }");

    rules << QStringLiteral("QPlainTextEdit#resourceSummary { background: %1;"
                            " border: 1px solid %2; border-radius: %3px; color: %4;"
                            " padding: %5px; selection-background-color: %6;"
                            " selection-color: %7; }")
                 .arg(css(p.sunken), css(p.hairline))
                 .arg(Metrics::codeBoxRadius)
                 .arg(css(p.textSecondary))
                 .arg(Metrics::codeBoxMargin)
                 .arg(css(p.accent), css(p.accentText));

    // Like the resource summary, but on cardBackground rather than sunken, and with
    // three differences that all follow from where it sits.
    //
    // It sits directly on the page, not inside a card. sunken is only three or four
    // steps from pageBackground -- it reads as recessed against the white of a
    // SettingsCard, which is the only place the resource summary ever appears, and
    // as nothing at all against the page. cardBackground is what every other
    // on-page surface in the app uses, so the snippet reads as a card like the rest
    // of them.
    //
    // It takes textPrimary, because a prompt template is content to read rather
    // than a subtitle. And it gets no padding: the inner margin is on the text
    // document instead, so the box's frozen height still accounts for it. See
    // Metrics::codeBoxMargin.
    rules << QStringLiteral("QPlainTextEdit#petGuideSnippet { background: %1;"
                            " border: 1px solid %2; border-radius: %3px; color: %4;"
                            " selection-background-color: %5; selection-color: %6; }")
                 .arg(css(p.cardBackground), css(p.cardBorder))
                 .arg(Metrics::codeBoxRadius)
                 .arg(css(p.textPrimary))
                 .arg(css(p.accent), css(p.accentText));

    return rules.join(QLatin1Char('\n'));
}

}  // namespace Theme

ThemeWatcher::ThemeWatcher(QObject *parent)
    : QObject(parent)
{
    if (QStyleHints *hints = QGuiApplication::styleHints()) {
        m_scheme = Theme::effectiveScheme(hints->colorScheme());
        connect(hints, &QStyleHints::colorSchemeChanged, this, &ThemeWatcher::recompute);
    }
}

Qt::ColorScheme ThemeWatcher::scheme() const
{
    return m_scheme;
}

void ThemeWatcher::setSchemeOverride(Qt::ColorScheme scheme)
{
    m_override = scheme;
    recompute();
}

void ThemeWatcher::recompute()
{
    Qt::ColorScheme raw = m_override;
    if (raw == Qt::ColorScheme::Unknown) {
        QStyleHints *hints = QGuiApplication::styleHints();
        raw = hints ? hints->colorScheme() : Qt::ColorScheme::Light;
    }
    const Qt::ColorScheme resolved = Theme::effectiveScheme(raw);
    // Emit only on a real change, the same discipline MotionController uses: the
    // platform can report Unknown and Light in succession without anything
    // visible having happened.
    if (resolved == m_scheme) return;
    m_scheme = resolved;
    emit schemeChanged(m_scheme);
}
