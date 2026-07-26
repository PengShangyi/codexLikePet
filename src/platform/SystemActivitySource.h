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
    // The display went to sleep or came back while the machine stayed awake. Not the
    // same as willSleep(): on a laptop left plugged in, or one kept awake by a
    // download, this is most of the day, and nothing on screen can be seen. Neither
    // sleep notification fires for it.
    //
    // Edge-triggered, like the pair above. Nothing replays them, so a launch that
    // happens while the display is already asleep starts out believing it is awake.
    void screensDidSleep();
    void screensDidWake();
    void reduceMotionChanged(bool reduced);
};
