#include "settings/PetPreviewWidget.h"

#include "pet/AnimationClip.h"
#include "ui/Theme.h"

#include <QPainter>
#include <QHideEvent>
#include <QShowEvent>
#include <QTimer>

PetPreviewWidget::PetPreviewWidget(QWidget *parent)
    : QWidget(parent)
    , m_timer(new QTimer(this))
{
    setMinimumHeight(230);
    // paintEvent fills the whole rect (the stage runs edge to edge), so tell Qt not
    // to repaint what is behind us. Without this every preview frame dirtied the
    // ancestors too, and repainting their macOS-style chrome cost ~10x the preview
    // itself while Settings was open. It matters more now, not less: the preview's
    // ancestors include a stylesheet-styled page background, which repaints through
    // QStyleSheetStyle and is dearer than the tab frame this originally fixed.
    setAttribute(Qt::WA_OpaquePaintEvent);
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

void PetPreviewWidget::setColorScheme(Qt::ColorScheme scheme)
{
    if (m_scheme == scheme) return;
    m_scheme = scheme;
    update();  // renderStage() re-runs on the next paint, keyed on m_stageScheme
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

    // The stage (opaque fill, rounded gradient card, border) only changes when the
    // size, device pixel ratio, or colour scheme does -- not per frame. Caching it
    // keeps a running preview down to one drawPixmap plus the shadow and the frame,
    // which is what protects the cost that WA_OpaquePaintEvent was set to control.
    const qreal dpr = devicePixelRatioF();
    const QSize deviceSize = size() * dpr;
    if (m_stageCache.size() != deviceSize || m_stageScheme != m_scheme) {
        m_stageCache = QPixmap(deviceSize);
        m_stageCache.setDevicePixelRatio(dpr);
        m_stageScheme = m_scheme;
        renderStage();
    }
    painter.drawPixmap(0, 0, m_stageCache);

    if (!m_atlas && !m_clip) return;

    // A view rather than a copy: this is paintEvent, so it ran on every repaint
    // and not merely once per animation frame.
    const QImage frame = m_clip ? m_clip->frameView(m_frameIndex)
                                : m_atlas->frameView(m_state, m_frameIndex);
    QSize target = frame.size();
    const int inset = Theme::Metrics::stageInset;
    target.scale(size() - QSize(inset * 2, inset * 2), Qt::KeepAspectRatio);
    const QRect destination(QPoint((width() - target.width()) / 2,
                                   (height() - target.height()) / 2),
                            target);

    // A soft elliptical contact shadow under the pet, so it reads as standing on the
    // stage rather than floating in front of it.
    const Theme::Palette palette = Theme::palette(m_scheme);
    const int shadowHeight = Theme::Metrics::stageShadowHeight;
    const QRectF shadowRect(destination.center().x() - target.width() * 0.32,
                            destination.bottom() - shadowHeight / 2.0,
                            target.width() * 0.64,
                            shadowHeight);
    QRadialGradient shadow(shadowRect.center(), shadowRect.width() / 2.0);
    shadow.setColorAt(0.0, palette.stageShadow);
    shadow.setColorAt(1.0, QColor(palette.stageShadow.red(),
                                  palette.stageShadow.green(),
                                  palette.stageShadow.blue(),
                                  0));
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(shadow);
    painter.drawEllipse(shadowRect);

    painter.setRenderHint(QPainter::SmoothPixmapTransform, m_smooth);
    painter.drawImage(destination, frame);
}

void PetPreviewWidget::renderStage()
{
    const Theme::Palette palette = Theme::palette(m_scheme);
    QPainter painter(&m_stageCache);

    // Step one is a plain opaque fill of the entire rect, with antialiasing off.
    // WA_OpaquePaintEvent means Qt does not erase the background, so every pixel has
    // to be written here -- including the ones the rounded card's corners leave over,
    // which would otherwise blend against uninitialized backing store.
    painter.fillRect(QRect(QPoint(0, 0), size()), palette.pageBackground);

    painter.setRenderHint(QPainter::Antialiasing, true);
    const QRectF card = QRectF(QPoint(0, 0), size()).adjusted(0.5, 0.5, -0.5, -0.5);
    QLinearGradient gradient(card.topLeft(), card.bottomLeft());
    gradient.setColorAt(0.0, palette.stageTop);
    gradient.setColorAt(1.0, palette.stageBottom);
    painter.setBrush(gradient);
    painter.setPen(QPen(palette.stageBorder, 1.0));
    painter.drawRoundedRect(card, Theme::Metrics::stageRadius, Theme::Metrics::stageRadius);
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
        : PetAtlas::animationSpec(m_state).durationAt(m_frameIndex, 140);
    m_timer->start(duration);
}
