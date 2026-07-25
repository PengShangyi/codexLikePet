#include "pet/IdleActivityScheduler.h"

#include <QRandomGenerator>
#include <QTimer>

#include <utility>

namespace {
// Long, jittered dwell keeps idle CPU negligible (single-shot timer, no polling)
// while still feeling alive; well under the release runtime-check budget.
constexpr int kMinIntervalMs = 18000;
constexpr int kMaxIntervalMs = 45000;
}

QVector<V2AnimationState> IdleActivityScheduler::defaultPool()
{
    return {V2AnimationState::Jumping,
            V2AnimationState::Failed,
            V2AnimationState::Waiting,
            V2AnimationState::Review};
}

IdleActivityScheduler::IdleActivityScheduler(IdleFidgetPolicy policy, QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
    , m_policy(std::move(policy))
{
    m_timer->setSingleShot(true);
    if (!m_policy.nextIntervalMs) {
        m_policy.nextIntervalMs = [] {
            return QRandomGenerator::global()->bounded(kMinIntervalMs, kMaxIntervalMs);
        };
    }
    if (!m_policy.nextAnimation) {
        m_policy.nextAnimation = [] {
            const QVector<V2AnimationState> pool = defaultPool();
            return pool.at(QRandomGenerator::global()->bounded(static_cast<int>(pool.size())));
        };
    }
    connect(m_timer, &QTimer::timeout, this, [this] {
        emit fidgetRequested(m_policy.nextAnimation());
    });
}

bool IdleActivityScheduler::isArmed() const
{
    return m_timer->isActive();
}

void IdleActivityScheduler::setActive(bool active)
{
    if (active == m_active) return;
    m_active = active;
    if (active) arm();
    else m_timer->stop();
}

void IdleActivityScheduler::notifyFidgetFinished()
{
    if (m_active) arm();
}

void IdleActivityScheduler::arm()
{
    int interval = m_policy.nextIntervalMs();
    if (interval < 0) interval = 0;
    m_timer->start(interval);
}
