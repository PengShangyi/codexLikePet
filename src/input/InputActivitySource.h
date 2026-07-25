#pragma once

#include <QObject>

enum class InputStartResult {
    Started,
    // The OS access prompt was just shown (permission was undetermined). The tap
    // is not running yet; the caller should let the system prompt stand alone
    // rather than stacking its own dialog, then re-enable once access is granted.
    PermissionRequested,
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
