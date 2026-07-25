#include "pet/AnimationPlayer.h"

#include "pet/AnimationClip.h"

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
    m_clip.clear();
    m_clipName.clear();
    m_frameIndex = 0;
    presentCurrentFrame();
    if (m_shouldRun && !m_reducedMotion) {
        scheduleNextFrame();
    }
}

void AnimationPlayer::setState(V2AnimationState state, bool restart)
{
    if (!m_clip && m_state == state && !restart) {
        return;
    }
    m_clip.clear();
    m_clipName.clear();
    m_state = state;
    m_frameIndex = 0;
    presentCurrentFrame();
    if (m_shouldRun && !m_reducedMotion) {
        scheduleNextFrame();
    }
}

void AnimationPlayer::setClip(QSharedPointer<AnimationClip> clip,
                              const QString &name,
                              bool restart)
{
    if (m_clip == clip && m_clipName == name && !restart) return;
    m_clip = std::move(clip);
    m_clipName = name;
    m_frameIndex = 0;
    presentCurrentFrame();
    if (m_shouldRun && !m_reducedMotion) scheduleNextFrame();
}

void AnimationPlayer::setSpeedFactor(double factor)
{
    // Deliberately does NOT reschedule: the frame timer is single-shot, so
    // restarting it here meant a continuously-dragged speed slider reset the
    // pending frame before it could ever fire and the pet froze mid-drag. The
    // new factor takes effect at the next frame boundary instead (<= 320ms).
    m_speedFactor = std::clamp(factor, 0.5, 2.0);
}

void AnimationPlayer::setReducedMotion(bool reduced)
{
    m_reducedMotion = reduced;
    m_frameIndex = 0;
    presentCurrentFrame();
    if (reduced) {
        m_timer->stop();
    } else if ((m_atlas || m_clip) && m_shouldRun) {
        scheduleNextFrame();
    }
}

void AnimationPlayer::showClipFrame(int index)
{
    if (!m_clip || !m_clip->isValid()) return;
    m_timer->stop();
    m_shouldRun = false;  // held: the caller advances frames explicitly per keystroke
    m_frameIndex = std::clamp(index, 0, m_clip->frameCount() - 1);
    presentCurrentFrame();
}

void AnimationPlayer::start()
{
    m_shouldRun = true;
    presentCurrentFrame();
    if (!m_reducedMotion && (m_atlas || m_clip)) {
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
    if (m_clip && m_clip->isValid()) {
        emit frameReady(m_clip->frame(m_frameIndex));
    } else if (m_atlas && m_atlas->isValid()) {
        emit frameReady(m_atlas->frame(m_state, m_frameIndex));
    }
}

void AnimationPlayer::scheduleNextFrame()
{
    if ((!m_atlas && !m_clip) || m_reducedMotion) {
        m_timer->stop();
        return;
    }
    const int duration = m_clip
        ? m_clip->durationMs(m_frameIndex)
        : PetAtlas::animationSpec(m_state).durationsMs.value(m_frameIndex, 150);
    m_timer->start(std::max(1, qRound(duration / m_speedFactor)));
}

void AnimationPlayer::advance()
{
    const int frameCount = m_clip ? m_clip->frameCount()
                                  : PetAtlas::animationSpec(m_state).frameCount;
    ++m_frameIndex;
    if (m_frameIndex >= frameCount) {
        m_frameIndex = 0;
        if (m_clip) emit clipLoopCompleted(m_clipName);
        else emit loopCompleted(m_state);
    }
    presentCurrentFrame();
    scheduleNextFrame();
}
