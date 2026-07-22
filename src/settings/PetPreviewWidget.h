#pragma once

#include "pet/PetAtlas.h"

#include <QSharedPointer>
#include <QWidget>

class QTimer;
class AnimationClip;

class PetPreviewWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit PetPreviewWidget(QWidget *parent = nullptr);

    void setAtlas(QSharedPointer<PetAtlas> atlas, bool smoothRendering);
    void setClip(QSharedPointer<AnimationClip> clip, bool smoothRendering);
    void clear();
    void setReducedMotion(bool reduced);
    void setState(V2AnimationState state);

protected:
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    void advance();
    void scheduleNextFrame();

    QSharedPointer<PetAtlas> m_atlas;
    QSharedPointer<AnimationClip> m_clip;
    QTimer *m_timer;
    int m_frameIndex = 0;
    V2AnimationState m_state = V2AnimationState::Idle;
    bool m_smooth = true;
    bool m_reducedMotion = false;
};
