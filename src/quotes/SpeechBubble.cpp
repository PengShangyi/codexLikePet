#include "quotes/SpeechBubble.h"

#include "pet/WindowPlacement.h"

#include <QFontMetrics>
#include <QHideEvent>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>

SpeechBubble::SpeechBubble(QWidget *parent)
    : QWidget(parent,
              Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus
                  | Qt::WindowStaysOnTopHint)
    , m_hideTimer(new QTimer(this))
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    m_hideTimer->setSingleShot(true);
    connect(m_hideTimer, &QTimer::timeout, this, &QWidget::hide);
}

void SpeechBubble::setAlwaysOnTop(bool enabled)
{
    if (windowFlags().testFlag(Qt::WindowStaysOnTopHint) == enabled) return;
    const bool wasVisible = isVisible();
    const int remainingTime = m_hideTimer->remainingTime();
    setWindowFlag(Qt::WindowStaysOnTopHint, enabled);
    if (wasVisible) {
        show();
        m_hideTimer->start(remainingTime > 0 ? remainingTime : 3000);
    }
}

void SpeechBubble::showMessage(const QString &text,
                               const QRect &petGeometry,
                               const QRect &availableGeometry,
                               int durationMs)
{
    m_text = text;
    const QFontMetrics metrics(font());
    const QRect textBounds = metrics.boundingRect(QRect(0, 0, 240, 200),
                                                   Qt::TextWordWrap | Qt::AlignCenter,
                                                   text);
    resize(textBounds.size() + QSize(32, 24));
    move(placementFor(size(), petGeometry, availableGeometry));
    show();
    raise();
    m_hideTimer->start(durationMs);
}

QPoint SpeechBubble::placementFor(const QSize &bubbleSize,
                                  const QRect &petGeometry,
                                  const QRect &availableGeometry,
                                  int gap)
{
    QPoint proposed(petGeometry.center().x() - bubbleSize.width() / 2,
                    petGeometry.top() - bubbleSize.height() - gap);
    if (proposed.y() < availableGeometry.top()) {
        proposed = QPoint(petGeometry.right() + gap,
                          petGeometry.center().y() - bubbleSize.height() / 2);
        if (proposed.x() + bubbleSize.width() > availableGeometry.right() + 1) {
            proposed.setX(petGeometry.left() - bubbleSize.width() - gap);
        }
    }
    return WindowPlacement::clampToAvailableGeometry(proposed, bubbleSize, availableGeometry);
}

void SpeechBubble::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addRoundedRect(rect().adjusted(1, 1, -1, -1), 14, 14);
    painter.fillPath(path, QColor(255, 252, 244, 245));
    painter.setPen(QPen(QColor(94, 67, 45), 1.5));
    painter.drawPath(path);
    painter.setPen(QColor(50, 38, 30));
    painter.drawText(rect().adjusted(16, 10, -16, -10),
                     Qt::TextWordWrap | Qt::AlignCenter,
                     m_text);
}

void SpeechBubble::hideEvent(QHideEvent *event)
{
    m_hideTimer->stop();
    QWidget::hideEvent(event);
}
