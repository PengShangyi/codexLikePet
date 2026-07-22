#pragma once

#include "pet/PetAtlas.h"

#include <QObject>
#include <QSharedPointer>
#include <QString>

class QTimer;
class AnimationClip;

class AnimationPlayer final : public QObject
{
    Q_OBJECT

public:
    explicit AnimationPlayer(QObject *parent = nullptr);

    void setAtlas(QSharedPointer<PetAtlas> atlas);
    void setState(V2AnimationState state, bool restart = true);
    void setClip(QSharedPointer<AnimationClip> clip,
                 const QString &name,
                 bool restart = true);
    void setSpeedFactor(double factor);
    void setReducedMotion(bool reduced);
    void start();
    void stop();
    bool isRunning() const;

signals:
    void frameReady(const QImage &frame);
    void loopCompleted(V2AnimationState state);
    void clipLoopCompleted(const QString &name);

private:
    void presentCurrentFrame();
    void scheduleNextFrame();
    void advance();

    QSharedPointer<PetAtlas> m_atlas;
    QSharedPointer<AnimationClip> m_clip;
    QString m_clipName;
    QTimer *m_timer;
    V2AnimationState m_state = V2AnimationState::Idle;
    int m_frameIndex = 0;
    double m_speedFactor = 1.0;
    bool m_reducedMotion = false;
    bool m_shouldRun = false;
};
