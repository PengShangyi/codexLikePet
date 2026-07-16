#pragma once

#include <QObject>

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
    void reset();

signals:
    void typingChanged(bool active);

private:
    void recordActivity();

    InputActivitySource *m_source;
    QTimer *m_inactivityTimer;
    bool m_typing = false;
};
