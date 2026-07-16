#include "input/TypingActivityDetector.h"

#include "input/InputActivitySource.h"

#include <QTimer>

TypingActivityDetector::TypingActivityDetector(InputActivitySource *source,
                                               int inactivityTimeoutMs,
                                               QObject *parent)
    : QObject(parent)
    , m_source(source)
    , m_inactivityTimer(new QTimer(this))
{
    m_inactivityTimer->setSingleShot(true);
    m_inactivityTimer->setInterval(inactivityTimeoutMs);
    connect(source, &InputActivitySource::activityDetected, this, &TypingActivityDetector::recordActivity);
    connect(m_inactivityTimer, &QTimer::timeout, this, [this] {
        if (!m_typing) return;
        m_typing = false;
        emit typingChanged(false);
    });
}

bool TypingActivityDetector::isTyping() const { return m_typing; }
int TypingActivityDetector::inactivityTimeoutMs() const { return m_inactivityTimer->interval(); }

void TypingActivityDetector::reset()
{
    m_inactivityTimer->stop();
    if (m_typing) {
        m_typing = false;
        emit typingChanged(false);
    }
}

void TypingActivityDetector::recordActivity()
{
    m_inactivityTimer->start();
    if (!m_typing) {
        m_typing = true;
        emit typingChanged(true);
    }
}
