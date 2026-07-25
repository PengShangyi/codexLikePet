#pragma once

#include "pet/PetAtlas.h"

#include <QPixmap>
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
    // Receives an already-resolved scheme rather than observing the system itself,
    // so the painter stays testable without an appearance observer -- the same
    // arrangement as reduced motion above.
    void setColorScheme(Qt::ColorScheme scheme);

protected:
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    void advance();
    void scheduleNextFrame();
    void renderStage();

    QSharedPointer<PetAtlas> m_atlas;
    QSharedPointer<AnimationClip> m_clip;
    QTimer *m_timer;
    int m_frameIndex = 0;
    V2AnimationState m_state = V2AnimationState::Idle;
    bool m_smooth = true;
    bool m_reducedMotion = false;
    Qt::ColorScheme m_scheme = Qt::ColorScheme::Light;
    // The stage backdrop, rebuilt only on a size, DPR, or scheme change so an
    // animating preview repaints a pixmap rather than a gradient and a border.
    QPixmap m_stageCache;
    Qt::ColorScheme m_stageScheme = Qt::ColorScheme::Light;
};
