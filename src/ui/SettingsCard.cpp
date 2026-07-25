#include "ui/SettingsCard.h"

#include "ui/SettingsRow.h"
#include "ui/Theme.h"

#include <QLabel>
#include <QVBoxLayout>

namespace {

QFrame *makeHairline(QWidget *parent)
{
    auto *line = new QFrame(parent);
    line->setObjectName(QStringLiteral("cardHairline"));
    line->setFrameShape(QFrame::NoFrame);
    line->setFixedHeight(Theme::Metrics::hairlineThickness);
    return line;
}

}  // namespace

SettingsCard::SettingsCard(TextKey title, Localization *localization, QWidget *parent)
    : QFrame(parent)
    , m_localization(localization)
    , m_titleKey(title)
{
    setFrameShape(QFrame::NoFrame);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(Theme::Metrics::cardPaddingH,
                             Theme::Metrics::cardPaddingV,
                             Theme::Metrics::cardPaddingH,
                             Theme::Metrics::cardPaddingV);
    root->setSpacing(Theme::Metrics::cardTitleSpacing);

    m_title = new QLabel(this);
    m_title->setObjectName(QStringLiteral("cardTitle"));
    m_title->setFont(Theme::cardTitleFont(font()));
    root->addWidget(m_title);

    m_body = new QVBoxLayout;
    m_body->setContentsMargins(0, 0, 0, 0);
    m_body->setSpacing(Theme::Metrics::cardTitleSpacing);
    root->addLayout(m_body);

    retranslate();
}

void SettingsCard::addRow(SettingsRow *row)
{
    if (m_hasBodyContent) m_body->addWidget(makeHairline(this));
    m_body->addWidget(row);
    m_rows.append(row);
    m_hasBodyContent = true;
}

void SettingsCard::addContent(QWidget *widget, bool separated)
{
    if (separated && m_hasBodyContent) m_body->addWidget(makeHairline(this));
    m_body->addWidget(widget);
    m_hasBodyContent = true;
}

void SettingsCard::retranslate()
{
    m_title->setText(m_localization->text(m_titleKey));
    for (SettingsRow *row : m_rows) row->retranslate();
}
