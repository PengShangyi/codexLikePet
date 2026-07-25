#include "ui/Disclosure.h"

#include "ui/Theme.h"

#include <QSize>
#include <QToolButton>
#include <QVBoxLayout>

Disclosure::Disclosure(TextKey title, Localization *localization, QWidget *parent)
    : QWidget(parent)
    , m_localization(localization)
    , m_titleKey(title)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(Theme::Metrics::cardTitleSpacing);

    m_header = new QToolButton(this);
    m_header->setObjectName(QStringLiteral("disclosureHeader"));
    m_header->setCheckable(true);
    m_header->setChecked(false);
    m_header->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_header->setArrowType(Qt::RightArrow);
    // The style draws the arrow at icon size, and the default is large enough to
    // out-weigh the label it belongs to.
    m_header->setIconSize(QSize(10, 10));
    m_header->setFocusPolicy(Qt::StrongFocus);
    m_header->setCursor(Qt::PointingHandCursor);
    m_header->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    root->addWidget(m_header, 0, Qt::AlignLeft);

    m_content = new QWidget(this);
    m_contentLayout = new QVBoxLayout(m_content);
    // Indented to the same gutter as card content, so revealed rows line up with the
    // rest of the page instead of hanging off its left edge.
    m_contentLayout->setContentsMargins(Theme::Metrics::cardPaddingH, 0, 0, 0);
    m_contentLayout->setSpacing(Theme::Metrics::cardTitleSpacing);
    m_content->hide();
    root->addWidget(m_content);

    connect(m_header, &QToolButton::toggled, this, &Disclosure::setExpanded);
    retranslate();
}

QVBoxLayout *Disclosure::contentLayout() const
{
    return m_contentLayout;
}

bool Disclosure::isExpanded() const
{
    return m_header->isChecked();
}

void Disclosure::setExpanded(bool expanded)
{
    if (m_header->isChecked() != expanded) {
        m_header->setChecked(expanded);  // re-enters through toggled
        return;
    }
    m_content->setVisible(expanded);
    m_header->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
}

void Disclosure::retranslate()
{
    const QString title = m_localization->text(m_titleKey);
    m_header->setText(title);
    m_header->setAccessibleName(title);
}
