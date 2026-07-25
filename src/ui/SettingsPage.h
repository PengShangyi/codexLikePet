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

    void retranslate();

private:
    QScrollArea *m_scroll;
    QVBoxLayout *m_column;
    QVector<SettingsCard *> m_cards;
};
