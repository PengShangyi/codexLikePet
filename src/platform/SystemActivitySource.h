#pragma once

#include <QObject>

class SystemActivitySource : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;
    ~SystemActivitySource() override = default;
    virtual bool systemReduceMotion() const = 0;

signals:
    void willSleep();
    void didWake();
    void reduceMotionChanged(bool reduced);
};
