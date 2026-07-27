#pragma once

#include <QVector>
#include <QWidget>

class QScrollArea;
class QVBoxLayout;
class SettingsCard;

// One page behind the nav bar: a scroll area that does not look like one,
// wrapping a column of cards. Pages are taller than the window on a small
// display, so the scroll area is not optional -- but its frame and its opaque
// viewport are, and both are turned off.
class SettingsPage final : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(QWidget *parent = nullptr);

    void addCard(SettingsCard *card);
    // Content that is not a card (a bare disclosure at the bottom of a page).
    void addContent(QWidget *widget);
    // Lets one widget absorb the leftover height instead of the trailing stretch,
    // which is how the pet preview keeps filling its page.
    void addStretchingContent(QWidget *widget);

    // Brings a widget's top edge to the top of the viewport. Content revealed at
    // the bottom of a full page is otherwise below the fold, which reads as
    // nothing having happened. Deliberately not ensureWidgetVisible(): that
    // centres anything taller than the viewport, cutting off the header of the
    // section just opened. The caller must let the pending LayoutRequest run
    // first, or the position it asks about is the one from before the reveal.
    void scrollToContent(QWidget *widget);

    void retranslate();

private:
    QScrollArea *m_scroll;
    QVBoxLayout *m_column;
    QVector<SettingsCard *> m_cards;
};
