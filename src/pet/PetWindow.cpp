#include "pet/PetWindow.h"

#include "pet/WindowPlacement.h"
#include "settings/AppSettings.h"

#include <QGuiApplication>
#include <QMoveEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QSettings>

#include <algorithm>

PetWindow::PetWindow(AppSettings *settings, QWidget *parent)
    : QWidget(parent,
              Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus
                  | Qt::WindowStaysOnTopHint)
{
    m_settings = settings;
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAutoFillBackground(false);
    setCursor(Qt::OpenHandCursor);

    m_scaleFactor = settings->scale();
    m_alwaysOnTop = settings->alwaysOnTop();
    if (!m_alwaysOnTop) {
        setWindowFlag(Qt::WindowStaysOnTopHint, false);
    }
    updateWindowSize();
    connect(settings, &AppSettings::scaleChanged, this, &PetWindow::setScaleFactor);
    connect(settings, &AppSettings::alwaysOnTopChanged, this, &PetWindow::setAlwaysOnTop);

    connect(qApp, &QGuiApplication::primaryScreenChanged, this, [this](QScreen *) {
        clampToPrimaryScreen();
    });
    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        connect(screen, &QScreen::availableGeometryChanged, this, [this] {
            clampToPrimaryScreen();
        });
    }
}

double PetWindow::scaleFactor() const
{
    return m_scaleFactor;
}

bool PetWindow::isAlwaysOnTop() const
{
    return m_alwaysOnTop;
}

SnapEdge PetWindow::snapEdge() const
{
    return m_snapEdge;
}

void PetWindow::setFrame(const QImage &frame)
{
    m_frame = frame;
    update();
}

void PetWindow::setScaleFactor(double factor)
{
    factor = std::clamp(factor, 0.5, 2.0);
    if (qFuzzyCompare(m_scaleFactor, factor)) {
        return;
    }
    m_scaleFactor = factor;
    updateWindowSize();
    clampToPrimaryScreen();
}

void PetWindow::setAlwaysOnTop(bool enabled)
{
    if (m_alwaysOnTop == enabled) {
        return;
    }
    const bool wasVisible = isVisible();
    m_alwaysOnTop = enabled;
    setWindowFlag(Qt::WindowStaysOnTopHint, enabled);
    if (wasVisible) {
        show();
    }
}

void PetWindow::resetPosition()
{
    QSettings().remove(QStringLiteral("window/position"));
    m_snapEdge = SnapEdge::None;
    restorePosition();
}

void PetWindow::restorePosition()
{
    const QRect available = primaryAvailableGeometry();
    const QPoint saved = QSettings().value(QStringLiteral("window/position")).toPoint();
    const bool hasSaved = QSettings().contains(QStringLiteral("window/position"));
    const QPoint position = hasSaved
        && WindowPlacement::isUsableSavedPosition(saved, size(), available)
        ? saved
        : WindowPlacement::defaultPosition(available, size());

    m_restoringPosition = true;
    move(position);
    m_restoringPosition = false;
    m_snapEdge = WindowPlacement::resolveSnapEdge(position, size(), available, 0);
}

void PetWindow::clampToPrimaryScreen()
{
    const QPoint constrained = WindowPlacement::clampToAvailableGeometry(pos(),
                                                                          size(),
                                                                          primaryAvailableGeometry());
    if (constrained != pos()) {
        move(constrained);
    }
}

void PetWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    if (!m_frame.isNull()) {
        painter.drawImage(rect(), m_frame);
        return;
    }

    // Development-only fallback until the validated built-in pet is packaged.
    const QRectF body(width() * 0.17, height() * 0.18, width() * 0.66, height() * 0.68);
    painter.setPen(QPen(QColor(86, 57, 38), std::max(2.0, width() / 80.0)));
    painter.setBrush(QColor(205, 154, 92));
    painter.drawEllipse(body);
    painter.setBrush(QColor(68, 47, 35));
    painter.drawEllipse(QRectF(width() * 0.37, height() * 0.45, width() * 0.035, width() * 0.035));
    painter.drawEllipse(QRectF(width() * 0.60, height() * 0.45, width() * 0.035, width() * 0.035));
    painter.drawArc(QRectF(width() * 0.43, height() * 0.48, width() * 0.16, height() * 0.10),
                    200 * 16,
                    140 * 16);
}

void PetWindow::moveEvent(QMoveEvent *event)
{
    QWidget::moveEvent(event);
    if (!m_restoringPosition) {
        QSettings().setValue(QStringLiteral("window/position"), event->pos());
    }
}

void PetWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    m_pointerDown = true;
    m_dragging = false;
    m_pressGlobal = event->globalPosition().toPoint();
    m_lastGlobal = m_pressGlobal;
    m_pressWindow = pos();
    event->accept();
}

void PetWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_pointerDown || !(event->buttons() & Qt::LeftButton)) {
        QWidget::mouseMoveEvent(event);
        return;
    }
    const QPoint global = event->globalPosition().toPoint();
    const QPoint totalDelta = global - m_pressGlobal;
    if (!m_dragging && totalDelta.manhattanLength() >= 4) {
        m_dragging = true;
        m_snapEdge = SnapEdge::None;
        setCursor(Qt::ClosedHandCursor);
        emit dragStarted();
    }
    if (m_dragging) {
        const HorizontalDragDirection direction = WindowPlacement::horizontalDirectionForDelta(global.x() - m_lastGlobal.x());
        if (direction != HorizontalDragDirection::None) emit dragDirectionChanged(direction);
        move(WindowPlacement::clampToAvailableGeometry(m_pressWindow + totalDelta,
                                                        size(),
                                                        primaryAvailableGeometry()));
    }
    m_lastGlobal = global;
    event->accept();
}

void PetWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !m_pointerDown) {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    m_pointerDown = false;
    setCursor(Qt::OpenHandCursor);
    if (m_dragging) {
        const QRect available = primaryAvailableGeometry();
        m_snapEdge = WindowPlacement::resolveSnapEdge(pos(), size(), available, 24);
        move(WindowPlacement::snappedPosition(m_snapEdge, pos(), size(), available));
        m_dragging = false;
        emit dragFinished(m_snapEdge);
    } else {
        emit clicked();
    }
    event->accept();
}

QRect PetWindow::primaryAvailableGeometry() const
{
    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        return screen->availableGeometry();
    }
    return QRect(0, 0, width(), height());
}

void PetWindow::updateWindowSize()
{
    resize(qRound(CellWidth * m_scaleFactor), qRound(CellHeight * m_scaleFactor));
}
