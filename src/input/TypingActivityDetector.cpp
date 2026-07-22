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
    m_monotonicClock.start();
    m_inactivityTimer->setSingleShot(true);
    m_inactivityTimer->setInterval(inactivityTimeoutMs);
    connect(source, &InputActivitySource::activityDetected, this, &TypingActivityDetector::recordActivity);
    connect(source, &InputActivitySource::monitoringInvalidated, this, &TypingActivityDetector::reset);
    connect(m_inactivityTimer, &QTimer::timeout, this, [this] {
        if (!m_typing) return;
        m_typing = false;
        emit typingChanged(false);
    });
}

bool TypingActivityDetector::isTyping() const { return m_typing; }
int TypingActivityDetector::inactivityTimeoutMs() const { return m_inactivityTimer->interval(); }
qint64 TypingActivityDetector::lastActivityMonotonicMs() const
{
    return m_activityTimes.isEmpty() ? -1 : m_activityTimes.constLast();
}

int TypingActivityDetector::recentActivityCount() const
{
    const qint64 cutoff = m_monotonicClock.elapsed() - m_inactivityTimer->interval();
    int count = 0;
    for (auto iterator = m_activityTimes.crbegin();
         iterator != m_activityTimes.crend() && *iterator >= cutoff;
         ++iterator) {
        ++count;
    }
    return count;
}

void TypingActivityDetector::reset()
{
    m_inactivityTimer->stop();
    m_activityTimes.clear();
    if (m_typing) {
        m_typing = false;
        emit typingChanged(false);
    }
}

void TypingActivityDetector::recordActivity()
{
    const qint64 now = m_monotonicClock.elapsed();
    const qint64 cutoff = now - m_inactivityTimer->interval();
    while (!m_activityTimes.isEmpty() && m_activityTimes.constFirst() < cutoff) {
        m_activityTimes.removeFirst();
    }
    m_activityTimes.append(now);
    m_inactivityTimer->start();
    if (!m_typing) {
        m_typing = true;
        emit typingChanged(true);
    }
}
