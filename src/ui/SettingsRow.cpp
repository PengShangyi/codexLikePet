#include "ui/SettingsRow.h"

#include "ui/Theme.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

SettingsRow::SettingsRow(TextKey label,
                         QWidget *control,
                         Localization *localization,
                         QWidget *parent)
    : QWidget(parent)
    , m_localization(localization)
    , m_labelKey(label)
    , m_descriptionKey(label)
    , m_control(control)
{
    m_root = new QHBoxLayout(this);
    m_root->setContentsMargins(0, 0, 0, 0);
    m_root->setSpacing(Theme::Metrics::rowSpacing);

    m_textColumn = new QVBoxLayout;
    m_textColumn->setContentsMargins(0, 0, 0, 0);
    m_textColumn->setSpacing(2);
    m_label = new QLabel(this);
    m_label->setObjectName(QStringLiteral("rowLabel"));
    m_label->setFont(Theme::rowLabelFont(font()));
    m_textColumn->addWidget(m_label);
    m_root->addLayout(m_textColumn, 1);
    m_root->addStretch();

    if (m_control) {
        m_control->setParent(this);
        // Controls keep their native look, so they also keep their natural size;
        // a floor stops a combo and a time edit in adjacent rows from ending up
        // visibly different widths.
        if (m_control->minimumWidth() == 0) {
            m_control->setMinimumWidth(Theme::Metrics::controlMinWidth);
        }
        m_root->addWidget(m_control, 0, Qt::AlignRight | Qt::AlignVCenter);
    }

    setMinimumHeight(Theme::Metrics::rowMinHeight);
    retranslate();
}

void SettingsRow::setDescriptionKey(TextKey description)
{
    m_hasDescription = true;
    m_descriptionKey = description;
    if (!m_description) {
        m_description = new QLabel(this);
        m_description->setObjectName(QStringLiteral("rowDescription"));
        m_description->setFont(Theme::rowDescriptionFont(font()));
        m_description->setWordWrap(true);
        m_textColumn->addWidget(m_description);
    }
    retranslate();
}

void SettingsRow::setValueText(const QString &text)
{
    if (!m_value) {
        m_value = new QLabel(this);
        m_value->setObjectName(QStringLiteral("rowValue"));
        m_value->setFont(Theme::rowLabelFont(font()));
        m_root->addWidget(m_value, 0, Qt::AlignRight | Qt::AlignVCenter);
    }
    m_value->setText(text);
}

TextKey SettingsRow::labelKey() const
{
    return m_labelKey;
}

QWidget *SettingsRow::control() const
{
    return m_control;
}

QString SettingsRow::labelText() const
{
    return m_label->text();
}

void SettingsRow::retranslate()
{
    const QString label = m_localization->text(m_labelKey);
    m_label->setText(label);
    if (m_description) m_description->setText(m_localization->text(m_descriptionKey));

    // Set here rather than in a separate pass over the whole window: the row is the
    // only place that knows both the control and the key that names it, so there is
    // no way for the two to drift apart.
    if (m_control) {
        m_control->setAccessibleName(label);
        if (m_hasDescription) {
            m_control->setAccessibleDescription(m_localization->text(m_descriptionKey));
        }
    }
}
