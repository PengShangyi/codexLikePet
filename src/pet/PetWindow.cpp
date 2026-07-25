#include "pet/PetWindow.h"

#include "pet/PetGesture.h"
#include "pet/WindowPlacement.h"
#include "platform/WindowOverlay.h"
#include "settings/AppSettings.h"

#include <QGuiApplication>
#include <QMoveEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QSettings>
#include <QShowEvent>
#include <QTimer>

#include <algorithm>

PetWindow::PetWindow(AppSettings *settings, QWidget *parent)
    : QWidget(parent,
              Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus
                  | Qt::WindowStaysOnTopHint)
{
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

    const auto watchScreen = [this](QScreen *screen) {
        if (!screen) return;
        connect(screen,
                &QScreen::availableGeometryChanged,
                this,
                &PetWindow::clampToPrimaryScreen,
                Qt::UniqueConnection);
    };
    watchScreen(QGuiApplication::primaryScreen());
    connect(qApp, &QGuiApplication::primaryScreenChanged, this, [this, watchScreen](QScreen *screen) {
        watchScreen(screen);
        clampToPrimaryScreen();  // also re-pushes the overlay level (see below)
    });
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

bool PetWindow::usesSmoothRendering() const { return m_smoothRendering; }

void PetWindow::setFrame(const QImage &frame)
{
    m_frame = frame;
    update();
}

void PetWindow::setSmoothRendering(bool smooth)
{
    if (m_smoothRendering == smooth) return;
    m_smoothRendering = smooth;
    update();
}

void PetWindow::setScaleFactor(double factor)
{
    factor = std::clamp(factor, 0.5, 2.0);
    if (qFuzzyCompare(m_scaleFactor, factor)) {
        return;
    }
    const SnapEdge previousEdge = m_snapEdge;
    m_scaleFactor = factor;
    updateWindowSize();
    if (previousEdge == SnapEdge::None) {
        clampToPrimaryScreen();
    } else {
        move(WindowPlacement::snappedPosition(previousEdge,
                                               pos(),
                                               size(),
                                               primaryAvailableGeometry()));
        updateSnapEdge(previousEdge);
    }
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
        show();  // re-shows the recreated native window; showEvent re-pushes the level
    }
}

void PetWindow::reapplyAlwaysOnTop()
{
    WindowOverlay::apply(winId(), m_alwaysOnTop);
}

void PetWindow::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // Toggling window flags and re-showing recreate the NSWindow, resetting the
    // level and collection behavior to Qt's defaults. Re-push now, and once more
    // after this show settles, in case Qt reconfigures the native window right
    // after showEvent is delivered.
    reapplyAlwaysOnTop();
    QTimer::singleShot(0, this, &PetWindow::reapplyAlwaysOnTop);
}

void PetWindow::resetPosition()
{
    QSettings().remove(QStringLiteral("window/position"));
    updateSnapEdge(SnapEdge::None);
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
    updateSnapEdge(WindowPlacement::resolveSnapEdge(position, size(), available, 0));
}

void PetWindow::clampToPrimaryScreen()
{
    const QPoint constrained = WindowPlacement::clampToAvailableGeometry(pos(),
                                                                          size(),
                                                                          primaryAvailableGeometry());
    if (constrained != pos()) {
        move(constrained);
    }
    updateSnapEdge(WindowPlacement::resolveSnapEdge(pos(), size(), primaryAvailableGeometry(), 0));
    // Display events that reach here (screen swap, resolution/scale change, dock
    // or menu-bar reflow, wake) can recreate the native window and reset its level
    // to Qt's default. Re-push the overlay level so every such event keeps the pet
    // above other apps — including over their full-screen Spaces.
    reapplyAlwaysOnTop();
}

void PetWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, m_smoothRendering);

    if (!m_frame.isNull()) {
        painter.drawImage(rect(), m_frame);
        return;
    }

    // Defensive fallback used only when no validated pet frame is available.
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
    if (!m_restoringPosition && !m_dragging) {
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
    const PetGesture::MoveDecision decision =
        PetGesture::onMove(m_dragging, totalDelta, global.x() - m_lastGlobal.x(), 4);
    if (decision.startDrag) {
        m_dragging = true;
        updateSnapEdge(SnapEdge::None);
        setCursor(Qt::ClosedHandCursor);
        emit dragStarted();
    }
    if (m_dragging) {
        if (decision.direction != HorizontalDragDirection::None) emit dragDirectionChanged(decision.direction);
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
    const PetGesture::ReleaseDecision decision =
        PetGesture::onRelease(m_dragging, pos(), size(), primaryAvailableGeometry(), 24);
    if (decision.wasClick) {
        emit clicked();
    } else {
        updateSnapEdge(decision.snapEdge);
        move(decision.snappedTopLeft);
        m_dragging = false;
        QSettings().setValue(QStringLiteral("window/position"), pos());
        emit dragFinished(m_snapEdge);
    }
    event->accept();
}

void PetWindow::updateSnapEdge(SnapEdge edge)
{
    if (m_snapEdge == edge) return;
    m_snapEdge = edge;
    emit snapEdgeChanged(edge);
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
