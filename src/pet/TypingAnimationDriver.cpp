#include "pet/TypingAnimationDriver.h"

#include "pet/AnimationClip.h"
#include "pet/AnimationPlayer.h"

#include <QTimer>

namespace {
// How long a paw stays down before relaxing back to the rest frame.
constexpr int kPressReleaseMs = 110;
}

TypingAnimationDriver::TypingAnimationDriver(AnimationPlayer *player, QObject *parent)
    : QObject(parent)
    , m_player(player)
    , m_returnTimer(new QTimer(this))
{
    m_returnTimer->setSingleShot(true);
    m_returnTimer->setInterval(kPressReleaseMs);
    connect(m_returnTimer, &QTimer::timeout, this, [this] {
        // Relax back to the resting "hands on keyboard" frame after a typing pause.
        // stop() clears m_pressActive and stops this timer together, so no separate
        // check of the caller's behavior state is needed here.
        if (m_pressActive) m_player->showClipFrame(0);
    });
}

void TypingAnimationDriver::begin(const QSharedPointer<AnimationClip> &clip, bool reducedMotion)
{
    if (clip) {
        m_clipFrames = clip->frameCount();
        m_player->setClip(clip, QStringLiteral("typing"), true);
        m_player->showClipFrame(0);  // rest: hands on the keyboard (stops the timer)
        // Reduced motion holds that static rest frame with no per-key motion.
        m_pressActive = !reducedMotion && m_clipFrames >= 2;
        m_returnTimer->stop();
        return;
    }
    m_pressActive = false;
    m_player->setState(V2AnimationState::Running);
    m_player->start();
}

void TypingAnimationDriver::stop()
{
    m_pressActive = false;
    m_returnTimer->stop();
}

void TypingAnimationDriver::onKey()
{
    if (!m_pressActive || m_clipFrames < 2) return;
    // Alternate press frames when the clip provides them (1 = left paw, 2 = right);
    // a 2-frame clip just toggles rest/press.
    m_pressToggle = !m_pressToggle;
    const int press = m_clipFrames >= 3 ? (m_pressToggle ? 1 : 2) : 1;
    m_player->showClipFrame(press);
    m_returnTimer->start();  // relax back to rest after a short pause
}

bool TypingAnimationDriver::isPressActive() const
{
    return m_pressActive;
}
