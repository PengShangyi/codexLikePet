#pragma once

#include <QDateTime>
#include <QObject>

class EnvironmentClock : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;
    ~EnvironmentClock() override = default;
    virtual QDateTime now() const = 0;
};

class SystemEnvironmentClock final : public EnvironmentClock
{
    Q_OBJECT

public:
    using EnvironmentClock::EnvironmentClock;
    QDateTime now() const override;
};
