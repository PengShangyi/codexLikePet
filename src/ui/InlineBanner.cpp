#include "ui/InlineBanner.h"

#include "ui/Theme.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QStyle>

InlineBanner::InlineBanner(QWidget *parent)
    : QFrame(parent)
{
    setFrameShape(QFrame::NoFrame);
    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(Theme::Metrics::bannerPadding,
                             Theme::Metrics::bannerPadding,
                             Theme::Metrics::bannerPadding,
                             Theme::Metrics::bannerPadding);
    m_text = new QLabel(this);
    m_text->setObjectName(QStringLiteral("bannerText"));
    m_text->setWordWrap(true);
    m_text->setTextInteractionFlags(Qt::TextSelectableByMouse);
    root->addWidget(m_text);
    hide();
}

void InlineBanner::setMessage(const QString &message, Severity severity)
{
    m_text->setText(message);
    if (m_severity != severity) {
        m_severity = severity;
        // A Q_PROPERTY used in a selector is read once at polish time, so the style
        // has to be told to re-evaluate it.
        style()->unpolish(this);
        style()->polish(this);
    }
    setVisible(!message.isEmpty());
}

void InlineBanner::clear()
{
    setMessage(QString(), Severity::Info);
}

QString InlineBanner::message() const
{
    return m_text->text();
}

InlineBanner::Severity InlineBanner::severity() const
{
    return m_severity;
}

QString InlineBanner::severityName() const
{
    return m_severity == Severity::Error ? QStringLiteral("error") : QStringLiteral("info");
}
