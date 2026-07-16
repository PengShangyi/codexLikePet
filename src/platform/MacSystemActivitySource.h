#pragma once

#include "platform/SystemActivitySource.h"

class MacSystemActivitySource final : public SystemActivitySource
{
    Q_OBJECT

public:
    explicit MacSystemActivitySource(QObject *parent = nullptr);
    ~MacSystemActivitySource() override;

    bool systemReduceMotion() const override;

private:
    void *m_sleepObserver = nullptr;
    void *m_wakeObserver = nullptr;
    void *m_accessibilityObserver = nullptr;
};
