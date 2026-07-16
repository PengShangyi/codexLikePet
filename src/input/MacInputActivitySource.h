#pragma once

#include "input/InputActivitySource.h"

struct __CFMachPort;
struct __CFRunLoopSource;

class MacInputActivitySource final : public InputActivitySource
{
    Q_OBJECT

public:
    explicit MacInputActivitySource(QObject *parent = nullptr);
    ~MacInputActivitySource() override;

    InputStartResult start() override;
    void stop() override;
    bool isActive() const override;

private:
    static void *eventCallback(void *proxy, unsigned int type, void *event, void *context);

    __CFMachPort *m_eventTap = nullptr;
    __CFRunLoopSource *m_runLoopSource = nullptr;
};
