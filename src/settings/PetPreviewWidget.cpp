#include "settings/PetPreviewWidget.h"

#include "pet/AnimationClip.h"

#include <QPainter>
#include <QHideEvent>
#include <QShowEvent>
#include <QTimer>

PetPreviewWidget::PetPreviewWidget(QWidget *parent)
    : QWidget(parent)
    , m_timer(new QTimer(this))
{
    setMinimumHeight(230);
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &PetPreviewWidget::advance);
}

void PetPreviewWidget::setAtlas(QSharedPointer<PetAtlas> atlas, bool smoothRendering)
{
    m_atlas = std::move(atlas);
    m_clip.clear();
    m_smooth = smoothRendering;
    m_frameIndex = 0;
    update();
    scheduleNextFrame();
}

void PetPreviewWidget::setClip(QSharedPointer<AnimationClip> clip, bool smoothRendering)
{
    m_clip = std::move(clip);
    m_smooth = smoothRendering;
    m_frameIndex = 0;
    update();
    scheduleNextFrame();
}

void PetPreviewWidget::setReducedMotion(bool reduced)
{
    m_reducedMotion = reduced;
    m_frameIndex = 0;
    if (reduced) m_timer->stop();
    else scheduleNextFrame();
    update();
}

void PetPreviewWidget::setState(V2AnimationState state)
{
    if (!m_clip && m_state == state) return;
    m_clip.clear();
    m_state = state;
    m_frameIndex = 0;
    update();
    scheduleNextFrame();
}

void PetPreviewWidget::clear()
{
    m_timer->stop();
    m_atlas.clear();
    m_clip.clear();
    m_frameIndex = 0;
    update();
}

void PetPreviewWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    constexpr int checkerSize = 12;
    for (int y = 0; y < height(); y += checkerSize) {
        for (int x = 0; x < width(); x += checkerSize) {
            const QColor color = ((x / checkerSize) + (y / checkerSize)) % 2
                ? QColor(220, 220, 220)
                : QColor(245, 245, 245);
            painter.fillRect(QRect(x, y, checkerSize, checkerSize), color);
        }
    }
    if (!m_atlas && !m_clip) return;
    painter.setRenderHint(QPainter::SmoothPixmapTransform, m_smooth);
    const QImage frame = m_clip ? m_clip->frame(m_frameIndex)
                                : m_atlas->frame(m_state, m_frameIndex);
    QSize target = frame.size();
    target.scale(size() - QSize(24, 24), Qt::KeepAspectRatio);
    const QRect destination(QPoint((width() - target.width()) / 2,
                                   (height() - target.height()) / 2),
                            target);
    painter.drawImage(destination, frame);
}

void PetPreviewWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    scheduleNextFrame();
}

void PetPreviewWidget::hideEvent(QHideEvent *event)
{
    m_timer->stop();
    QWidget::hideEvent(event);
}

void PetPreviewWidget::advance()
{
    const int frameCount = m_clip ? m_clip->frameCount()
                                  : PetAtlas::animationSpec(m_state).frameCount;
    if (frameCount <= 0) return;
    m_frameIndex = (m_frameIndex + 1) % frameCount;
    update();
    scheduleNextFrame();
}

void PetPreviewWidget::scheduleNextFrame()
{
    if (!isVisible() || m_reducedMotion || (!m_atlas && !m_clip)) {
        m_timer->stop();
        return;
    }
    const int duration = m_clip
        ? m_clip->durationMs(m_frameIndex)
        : PetAtlas::animationSpec(m_state).durationsMs.value(m_frameIndex, 140);
    m_timer->start(duration);
}
