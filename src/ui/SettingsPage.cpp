#include "ui/SettingsPage.h"

#include "ui/SettingsCard.h"
#include "ui/Theme.h"

#include <QScrollArea>
#include <QVBoxLayout>

SettingsPage::SettingsPage(QWidget *parent)
    : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    m_scroll = new QScrollArea(this);
    m_scroll->setObjectName(QStringLiteral("pageScroll"));
    // The C++ half is the reliable half: setFrameShape kills the sunken border and
    // an unfilled viewport lets the page background show through. The stylesheet
    // rule for #pageScroll only backs these up.
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setWidgetResizable(true);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->viewport()->setAutoFillBackground(false);
    root->addWidget(m_scroll);

    auto *content = new QWidget(m_scroll);
    content->setObjectName(QStringLiteral("pageContent"));
    // Not a QFrame, so the stylesheet's background rule needs this to take effect.
    // Easiest thing in the whole redesign to forget.
    content->setAttribute(Qt::WA_StyledBackground, true);
    m_column = new QVBoxLayout(content);
    m_column->setContentsMargins(Theme::Metrics::pageMargin,
                                 Theme::Metrics::pageMargin,
                                 Theme::Metrics::pageMargin,
                                 Theme::Metrics::pageMargin);
    m_column->setSpacing(Theme::Metrics::cardSpacing);
    m_column->addStretch();
    m_scroll->setWidget(content);
}

void SettingsPage::addCard(SettingsCard *card)
{
    // Insert before the trailing stretch so cards stay top-aligned.
    m_column->insertWidget(m_column->count() - 1, card);
    m_cards.append(card);
}

void SettingsPage::addContent(QWidget *widget)
{
    m_column->insertWidget(m_column->count() - 1, widget);
}

void SettingsPage::addStretchingContent(QWidget *widget)
{
    m_column->insertWidget(m_column->count() - 1, widget, 1);
}

void SettingsPage::retranslate()
{
    for (SettingsCard *card : m_cards) card->retranslate();
}
