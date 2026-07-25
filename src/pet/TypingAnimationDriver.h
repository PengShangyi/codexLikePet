#pragma once

#include "pet/PetAtlas.h"

#include <QObject>
#include <QSharedPointer>

class AnimationClip;
class AnimationPlayer;
class QTimer;

// Drives the keystroke-driven typing animation: the "typing" clip is held on its
// resting frame and each key advances it to a press frame, which relaxes back to
// rest after a short pause. Only a frame index ever moves -- no key content is
// involved at any point, per the privacy contract.
//
// Extracted from AppController, which carried three booleans, a frame count and a
// timer for this alone. The driver owns the press machinery; the caller still
// owns the policy of *when* typing applies (visible, awake, behavior state).
class TypingAnimationDriver final : public QObject
{
    Q_OBJECT

public:
    explicit TypingAnimationDriver(AnimationPlayer *player, QObject *parent = nullptr);

    // Enter typing. With a usable clip (>= 2 frames) and motion allowed, holds it
    // on frame 0 and engages per-keystroke presses. With reduced motion it holds
    // that same rest frame statically. With no clip at all it falls back to the v2
    // "running" row, which the contract defines as active work rather than literal
    // running (see hatch-pet/references/animation-rows.md).
    void begin(const QSharedPointer<AnimationClip> &clip, bool reducedMotion);

    // Leave typing: disengage presses and cancel any pending relax.
    void stop();

    // One keystroke. A no-op unless begin() engaged press mode.
    void onKey();

    bool isPressActive() const;

private:
    AnimationPlayer *m_player;
    QTimer *m_returnTimer;
    bool m_pressActive = false;  // true only when a keystroke-driven clip is in use
    bool m_pressToggle = false;  // alternates left/right paw press
    int m_clipFrames = 0;
};
