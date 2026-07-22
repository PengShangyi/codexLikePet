#pragma once

#include <QObject>

enum class InputStartResult {
    Started,
    PermissionDenied,
    Failed,
};

class InputActivitySource : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;
    ~InputActivitySource() override = default;

    virtual InputStartResult start() = 0;
    virtual void stop() = 0;
    virtual bool isActive() const = 0;

signals:
    // Intentionally carries no event, character, scan code, modifier, or app.
    void activityDetected();
    void monitoringInvalidated();
};

Q_DECLARE_METATYPE(InputStartResult)
