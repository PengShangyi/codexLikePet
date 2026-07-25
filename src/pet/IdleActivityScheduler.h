#pragma once

#include "pet/PetAtlas.h"

#include <QObject>
#include <QVector>

#include <functional>

class QTimer;

// Timing + selection policy for idle "fidget" animations. Both hooks are
// injectable so tests are deterministic; production uses QRandomGenerator.
struct IdleFidgetPolicy {
    std::function<int()> nextIntervalMs;
    std::function<V2AnimationState()> nextAnimation;
};

// Emits fidgetRequested() after the pet has been continuously idle for a
// randomized interval, letting otherwise-unused atlas rows (jump/wait/glance…)
// play in place. It owns only the timing: the caller (AppController) arms it
// exclusively while the resolved behavior is Idle and motion is allowed, so
// behavior priority, reduced-motion, visibility, and sleep are handled there.
class IdleActivityScheduler final : public QObject
{
    Q_OBJECT

public:
    explicit IdleActivityScheduler(IdleFidgetPolicy policy = {}, QObject *parent = nullptr);

    bool isArmed() const;
    static QVector<V2AnimationState> defaultPool();

public slots:
    // Enable/disable idle fidgets. Enabling while already active is a no-op so a
    // burst of Idle re-entries can't perpetually reset (starve) the timer.
    void setActive(bool active);
    // Called once a requested fidget has finished playing to schedule the next.
    void notifyFidgetFinished();

signals:
    void fidgetRequested(V2AnimationState state);

private:
    void arm();

    QTimer *m_timer;
    IdleFidgetPolicy m_policy;
    bool m_active = false;
};
