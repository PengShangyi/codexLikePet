#include "pet/AnimationPlayer.h"

#include <QTimer>

#include <algorithm>

AnimationPlayer::AnimationPlayer(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
{
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &AnimationPlayer::advance);
}

void AnimationPlayer::setAtlas(QSharedPointer<PetAtlas> atlas)
{
    m_atlas = std::move(atlas);
    m_frameIndex = 0;
    presentCurrentFrame();
    if (m_shouldRun && !m_reducedMotion) {
        scheduleNextFrame();
    }
}

void AnimationPlayer::setState(V2AnimationState state, bool restart)
{
    if (m_state == state && !restart) {
        return;
    }
    m_state = state;
    m_frameIndex = 0;
    presentCurrentFrame();
    if (m_shouldRun && !m_reducedMotion) {
        scheduleNextFrame();
    }
}

void AnimationPlayer::setSpeedFactor(double factor)
{
    m_speedFactor = std::clamp(factor, 0.5, 2.0);
    if (m_shouldRun && !m_reducedMotion) {
        scheduleNextFrame();
    }
}

void AnimationPlayer::setReducedMotion(bool reduced)
{
    m_reducedMotion = reduced;
    m_frameIndex = 0;
    presentCurrentFrame();
    if (reduced) {
        m_timer->stop();
    } else if (m_atlas && m_shouldRun) {
        scheduleNextFrame();
    }
}

void AnimationPlayer::start()
{
    m_shouldRun = true;
    presentCurrentFrame();
    if (!m_reducedMotion && m_atlas) {
        scheduleNextFrame();
    }
}

void AnimationPlayer::stop()
{
    m_shouldRun = false;
    m_timer->stop();
}

bool AnimationPlayer::isRunning() const
{
    return m_timer->isActive();
}

void AnimationPlayer::presentCurrentFrame()
{
    if (m_atlas && m_atlas->isValid()) {
        emit frameReady(m_atlas->frame(m_state, m_frameIndex));
    }
}

void AnimationPlayer::scheduleNextFrame()
{
    if (!m_atlas || m_reducedMotion) {
        m_timer->stop();
        return;
    }
    const AnimationSpec spec = PetAtlas::animationSpec(m_state);
    const int duration = spec.durationsMs.value(m_frameIndex, 150);
    m_timer->start(std::max(1, qRound(duration / m_speedFactor)));
}

void AnimationPlayer::advance()
{
    const AnimationSpec spec = PetAtlas::animationSpec(m_state);
    ++m_frameIndex;
    if (m_frameIndex >= spec.frameCount) {
        m_frameIndex = 0;
        emit loopCompleted(m_state);
    }
    presentCurrentFrame();
    scheduleNextFrame();
}
