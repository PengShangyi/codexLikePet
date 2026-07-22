#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QVector>

class InputActivitySource;
class QTimer;

class TypingActivityDetector final : public QObject
{
    Q_OBJECT

public:
    explicit TypingActivityDetector(InputActivitySource *source,
                                    int inactivityTimeoutMs = 1500,
                                    QObject *parent = nullptr);

    bool isTyping() const;
    int inactivityTimeoutMs() const;
    qint64 lastActivityMonotonicMs() const;
    int recentActivityCount() const;
    void reset();

signals:
    void typingChanged(bool active);

private:
    void recordActivity();

    InputActivitySource *m_source;
    QTimer *m_inactivityTimer;
    QElapsedTimer m_monotonicClock;
    QVector<qint64> m_activityTimes;
    bool m_typing = false;
};
