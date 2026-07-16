#include "settings/PetPreviewWidget.h"

#include <QPainter>
#include <QHideEvent>
#include <QShowEvent>
#include <QTimer>

PetPreviewWidget::PetPreviewWidget(QWidget *parent)
    : QWidget(parent)
    , m_timer(new QTimer(this))
{
    setMinimumHeight(230);
    m_timer->setInterval(140);
    connect(m_timer, &QTimer::timeout, this, &PetPreviewWidget::advance);
}

void PetPreviewWidget::setAtlas(QSharedPointer<PetAtlas> atlas, bool smoothRendering)
{
    m_atlas = std::move(atlas);
    m_smooth = smoothRendering;
    m_frameIndex = 0;
    update();
    if (isVisible() && m_atlas) m_timer->start();
}

void PetPreviewWidget::clear()
{
    m_timer->stop();
    m_atlas.clear();
    m_frameIndex = 0;
    update();
}

void PetPreviewWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), palette().window());
    if (!m_atlas) return;
    painter.setRenderHint(QPainter::SmoothPixmapTransform, m_smooth);
    const QImage frame = m_atlas->frame(V2AnimationState::Idle, m_frameIndex);
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
    if (m_atlas) m_timer->start();
}

void PetPreviewWidget::hideEvent(QHideEvent *event)
{
    m_timer->stop();
    QWidget::hideEvent(event);
}

void PetPreviewWidget::advance()
{
    m_frameIndex = (m_frameIndex + 1) % PetAtlas::animationSpec(V2AnimationState::Idle).frameCount;
    update();
}
