#include "pet/PetWindow.h"

#include "pet/PetGesture.h"
#include "pet/WindowPlacement.h"
#include "platform/WindowOverlay.h"
#include "settings/AppSettings.h"
#include "ui/Theme.h"

#include <QContextMenuEvent>
#include <QGuiApplication>
#include <QMoveEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QShowEvent>
#include <QTimer>

#include <algorithm>

PetWindow::PetWindow(AppSettings *settings, QWidget *parent)
    : QWidget(parent,
              Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus
                  | Qt::WindowStaysOnTopHint)
    , m_settings(settings)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAutoFillBackground(false);

    m_scaleFactor = settings->scale();
    m_alwaysOnTop = settings->alwaysOnTop();
    m_positionLocked = settings->positionLocked();
    applyRestingCursor();
    setOpacity(settings->opacity());
    if (!m_alwaysOnTop) {
        setWindowFlag(Qt::WindowStaysOnTopHint, false);
    }
    updateWindowSize();
    connect(settings, &AppSettings::scaleChanged, this, &PetWindow::setScaleFactor);
    connect(settings, &AppSettings::opacityChanged, this, &PetWindow::setOpacity);
    connect(settings, &AppSettings::positionLockedChanged, this, &PetWindow::setPositionLocked);
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

SnapEdge PetWindow::snapEdge() const
{
    return m_snapEdge;
}

void PetWindow::setFrame(const QImage &frame)
{
    m_frame = frame;
    m_scaled = QPixmap();  // a new frame at the same size still needs rescaling
    update();
}

void PetWindow::setSmoothRendering(bool smooth)
{
    if (m_smoothRendering == smooth) return;
    m_smoothRendering = smooth;
    m_scaled = QPixmap();  // the filter changed, so the cached scale is wrong
    update();
}

void PetWindow::ensureScaledFrame()
{
    if (m_frame.isNull()) {
        m_scaled = QPixmap();
        return;
    }
    const qreal dpr = devicePixelRatioF();
    const QSize deviceSize = size() * dpr;
    if (m_scaled.size() == deviceSize && qFuzzyCompare(m_scaled.devicePixelRatio(), dpr)) {
        return;
    }
    if (m_frame.size() == deviceSize) {
        // The frame is already the size we need, which is the point of BaseWidth /
        // BaseHeight: at scale 1.0 on a devicePixelRatio-2 display the backing store
        // is exactly the atlas cell. Measured end to end, taking this path instead
        // of resampling is the difference between 1.10% and 0.70% idle CPU -- the
        // single largest saving available here, and the frame stops being blurry.
        //
        // QImage::scaled() happens to return *this when the size already matches, so
        // this branch is not what makes it cheap; it is here so the property is
        // stated rather than inherited from an undocumented shortcut, and so the
        // fast path is visible to a test.
        //
        // Where it stops holding: a devicePixelRatio-1 display asks for 96x104 and
        // this downsamples a 192x208 cell instead, which is more work than the old
        // base did there (it matched 1:1 at ratio 1). The contract is primary
        // display only and the target hardware is Retina, so that is the trade.
        m_scaled = QPixmap::fromImage(m_frame);
    } else {
        m_scaled = QPixmap::fromImage(m_frame.scaled(deviceSize,
                                                     Qt::IgnoreAspectRatio,
                                                     m_smoothRendering ? Qt::SmoothTransformation
                                                                       : Qt::FastTransformation));
    }
    m_scaled.setDevicePixelRatio(dpr);
}

QSize PetWindow::scaledFrameSize() const
{
    return m_scaled.size();
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
        // clampPosition(), not clampToPrimaryScreen(): resizing does not recreate
        // the native window, and the slider drives this once per step.
        clampPosition();
    } else {
        move(WindowPlacement::snappedPosition(previousEdge,
                                               pos(),
                                               size(),
                                               primaryAvailableGeometry()));
        updateSnapEdge(previousEdge);
    }
}

void PetWindow::setOpacity(double opacity)
{
    // Mirrors AppSettings' floor: a fully transparent pet cannot be clicked or found
    // again. The speech bubble deliberately does not follow this -- a translucent
    // bubble would be unreadable, which is the opposite of its purpose.
    setWindowOpacity(qBound(0.3, opacity, 1.0));
}

void PetWindow::setPositionLocked(bool locked)
{
    if (m_positionLocked == locked) return;
    m_positionLocked = locked;
    // Deliberately does not interrupt a drag already in flight: reaching this needs
    // the settings window focused, so it cannot happen mid-gesture in practice, and
    // the drag ends normally on release.
    if (!m_pointerDown) applyRestingCursor();
}

void PetWindow::applyRestingCursor()
{
    // An open hand invites a drag, so a locked pet must not show one.
    setCursor(m_positionLocked ? Qt::ArrowCursor : Qt::OpenHandCursor);
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
    m_settings->clearWindowPosition();
    updateSnapEdge(SnapEdge::None);
    restorePosition();
}

void PetWindow::restorePosition()
{
    const QRect available = primaryAvailableGeometry();
    const QPoint saved = m_settings->windowPosition();
    const QPoint position = m_settings->hasWindowPosition()
        && WindowPlacement::isUsableSavedPosition(saved, size(), available)
        ? saved
        : WindowPlacement::defaultPosition(available, size());

    m_restoringPosition = true;
    move(position);
    m_restoringPosition = false;
    updateSnapEdge(WindowPlacement::resolveSnapEdge(position, size(), available, 0));
}

void PetWindow::clampPosition()
{
    const QPoint constrained = WindowPlacement::clampToAvailableGeometry(pos(),
                                                                          size(),
                                                                          primaryAvailableGeometry());
    if (constrained != pos()) {
        move(constrained);
    }
    updateSnapEdge(WindowPlacement::resolveSnapEdge(pos(), size(), primaryAvailableGeometry(), 0));
}

void PetWindow::clampToPrimaryScreen()
{
    clampPosition();
    // Display events that reach here (screen swap, resolution/scale change, dock
    // or menu-bar reflow, wake) can recreate the native window and reset its level
    // to Qt's default. Re-push the overlay level so every such event keeps the pet
    // above other apps — including over their full-screen Spaces.
    reapplyAlwaysOnTop();
}

void PetWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    if (!m_frame.isNull()) {
        // No render hints and no scaling here: the frame is already at device
        // resolution, so this is a straight blit. drawImage(rect(), ...) used to
        // rescale the cell on every repaint, and SmoothPixmapTransform now lives
        // in ensureScaledFrame's transformation mode instead.
        ensureScaledFrame();
        // SourceOver, the default, is deliberate. m_scaled covers the whole backing
        // store and nothing else paints, so CompositionMode_Source is legal here and
        // is 3-5x cheaper per blit in isolation -- but measured end to end it made no
        // difference at all: three runs each way at maximum scale and speed gave
        // 2.70/2.70/2.70 against 2.60/2.60/2.80 percent idle CPU. At 5.5fps the blit
        // is a fraction of a millisecond per second either way, an order of magnitude
        // under what the acceptance check can resolve. Not worth having a mode that
        // writes destination alpha verbatim on this path for an unmeasurable saving.
        painter.drawPixmap(0, 0, m_scaled);
        return;
    }

    // Defensive fallback used only when no validated pet frame is available.
    painter.setRenderHint(QPainter::Antialiasing);
    const QRectF body(width() * 0.17, height() * 0.18, width() * 0.66, height() * 0.68);
    painter.setPen(QPen(Theme::brandOutline(), std::max(2.0, width() / 80.0)));
    painter.setBrush(Theme::brandPotato());
    painter.drawEllipse(body);
    painter.setBrush(Theme::brandOutline());
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
        m_settings->setWindowPosition(event->pos());
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
        PetGesture::onMove(m_dragging, totalDelta, global.x() - m_lastGlobal.x(),
                           m_positionLocked ? PetGesture::DragLock::Locked
                                            : PetGesture::DragLock::Unlocked,
                           4);
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
    applyRestingCursor();
    const PetGesture::ReleaseDecision decision =
        PetGesture::onRelease(m_dragging, pos(), size(), primaryAvailableGeometry(), 24);
    if (decision.wasClick) {
        emit clicked();
    } else {
        updateSnapEdge(decision.snapEdge);
        move(decision.snappedTopLeft);
        m_dragging = false;
        m_settings->setWindowPosition(pos());
        emit dragFinished(m_snapEdge);
    }
    event->accept();
}

void PetWindow::contextMenuEvent(QContextMenuEvent *event)
{
    // Surface the shared tray menu from the pet itself so it is reachable without
    // hunting the menu-bar icon. Drag state is left-button only, so a right-click
    // never begins a drag.
    emit contextMenuRequested(event->globalPos());
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
    resize(qRound(BaseWidth * m_scaleFactor), qRound(BaseHeight * m_scaleFactor));
}
