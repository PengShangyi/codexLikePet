#pragma once

#include <QColor>
#include <QFont>
#include <QObject>
#include <Qt>

// The single home for every color, metric, and font the settings UI paints with.
// Before this existed the values were restated as literals across the tray icon,
// the pet's fallback drawing, the speech bubble, and one inline error stylesheet,
// and none of them followed the system appearance.
//
// Two rules keep it that way:
//   - Nothing outside Theme::styleSheet() calls setStyleSheet().
//   - Widgets receive a resolved Qt::ColorScheme; they never read the global one
//     themselves, so a painter stays testable without an appearance observer.
namespace Theme {

struct Palette {
    QColor pageBackground;
    QColor cardBackground;
    QColor cardBorder;
    QColor hairline;
    QColor sunken;

    QColor textPrimary;
    QColor textSecondary;
    QColor accent;
    QColor accentText;
    QColor navHover;

    QColor danger;
    QColor dangerBackground;
    QColor dangerBorder;
    QColor infoBackground;
    QColor infoBorder;

    // Preview stage: a rounded card with a soft vertical gradient and a contact
    // shadow under the pet, replacing the old flat grey checkerboard.
    QColor stageTop;
    QColor stageBottom;
    QColor stageBorder;
    QColor stageShadow;

    QColor bubbleFill;
    QColor bubbleBorder;
    QColor bubbleText;
};

namespace Metrics {
inline constexpr int windowMinWidth = 680;
inline constexpr int windowMinHeight = 520;

inline constexpr int pageMargin = 20;
inline constexpr int cardSpacing = 16;
inline constexpr int cardRadius = 10;
inline constexpr int cardPaddingH = 14;
inline constexpr int cardPaddingV = 12;
inline constexpr int cardTitleSpacing = 8;

inline constexpr int rowMinHeight = 32;
inline constexpr int rowSpacing = 12;
inline constexpr int controlMinWidth = 190;
inline constexpr int hairlineThickness = 1;

inline constexpr int navBarPaddingH = 12;
inline constexpr int navBarPaddingV = 8;
inline constexpr int navItemSpacing = 4;
inline constexpr int navItemMinWidth = 74;
inline constexpr int navItemRadius = 7;
inline constexpr int navIconSize = 18;

inline constexpr int stageRadius = 12;
inline constexpr int stageInset = 14;
inline constexpr int stageShadowHeight = 10;

inline constexpr int bannerRadius = 8;
inline constexpr int bannerPadding = 10;

// The sunken read-only text boxes: the resource summary and the pet guide's
// copyable snippets. A snippet box freezes its own height to fit every line, and
// it measures that from QPlainTextEdit::frameWidth() plus
// QTextDocument::documentMargin() -- neither of which reports a QSS `padding`.
// So the guide's boxes take the margin through setDocumentMargin() instead,
// where it is counted. The resource summary keeps the QSS padding: its height is
// a hard-coded constant, so nothing there is measured to be thrown off.
inline constexpr int codeBoxRadius = 6;
inline constexpr int codeBoxMargin = 6;

// Speech bubble, kept here so the bubble's geometry stops being a second set of
// magic numbers next to its colors.
inline constexpr int bubbleRadius = 14;
inline constexpr qreal bubbleBorderWidth = 1.5;
}  // namespace Metrics

// Qt reports Unknown when the platform has no opinion; treat that as Light so no
// caller has to branch three ways.
Qt::ColorScheme effectiveScheme(Qt::ColorScheme raw);

// Every entry is defined per scheme in one place, deliberately independent of
// QGuiApplication::palette(): QStyleHints::colorSchemeChanged fires before Qt swaps
// that palette, so anything rebuilt on the signal would read the outgoing scheme.
// See the comment on the definition.
Palette palette(Qt::ColorScheme scheme);

// The warm browns the tray icon, the pet's fallback drawing, and the bubble
// border all used to spell out independently.
QColor brandPotato();
QColor brandOutline();

QFont cardTitleFont(const QFont &base);
QFont rowLabelFont(const QFont &base);
QFont rowDescriptionFont(const QFont &base);
QFont navItemFont(const QFont &base);

// Styles containers only -- cards, labels, hairlines, the nav pill, the scroll
// area. QComboBox, QSlider, QCheckBox, QTimeEdit, QPushButton, QScrollBar, and
// QMenu deliberately get no rule at all: any box-model property makes
// QStyleSheetStyle draw them with its own primitives instead of delegating to
// QMacStyle, which loses the native popup shape, stepper, and focus ring -- and
// leaving them alone is also what makes them follow NSAppearance for free.
//
// Every selector is qualified by an objectName or one of our own class names,
// never a bare `QPushButton {}`. The sheet is applied to the settings window's
// subtree, and QFileDialog/QMessageBox are parented to that window, so an
// unqualified rule would restyle their buttons too.
QString styleSheet(Qt::ColorScheme scheme);

}  // namespace Theme

// Observes the system light/dark preference.
//
// Unlike MotionController, this does not go through an injectable platform
// boundary. That boundary exists because "reduce motion" has no Qt API at all.
// Dark mode has both a getter and a setter on QStyleHints, so the test double is
// already in Qt, and the native style reads the same property -- a hand-written
// source would just be a second truth competing with it.
class ThemeWatcher final : public QObject
{
    Q_OBJECT

public:
    explicit ThemeWatcher(QObject *parent = nullptr);

    Qt::ColorScheme scheme() const;

    // Test seam. Pass Qt::ColorScheme::Unknown to resume following the system.
    // Kept as a fallback in case QStyleHints::setColorScheme() turns out not to
    // emit under the offscreen platform plugin.
    void setSchemeOverride(Qt::ColorScheme scheme);

signals:
    void schemeChanged(Qt::ColorScheme scheme);

private:
    void recompute();

    Qt::ColorScheme m_override = Qt::ColorScheme::Unknown;
    Qt::ColorScheme m_scheme = Qt::ColorScheme::Light;
};
