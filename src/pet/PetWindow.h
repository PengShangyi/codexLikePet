#pragma once

#include <QImage>
#include <QPixmap>
#include <QWidget>

#include "pet/WindowPlacement.h"

class QScreen;
class QMouseEvent;
class QContextMenuEvent;
class QShowEvent;
class AppSettings;

class PetWindow final : public QWidget
{
    Q_OBJECT

public:
    // The pet's size at scale 1.0, in *logical points* -- not the atlas cell size,
    // which is PetAtlas::CellWidth/CellHeight and is measured in source pixels.
    // These used to be a second copy of 192x208, so the two meanings coincided by
    // accident; they are half of it now, deliberately, because the pet runs on
    // Retina displays where devicePixelRatio is 2. That makes the backing store
    // exactly 192x208 device pixels at scale 1.0, so a frame is presented at its
    // authored size and ensureScaledFrame() has nothing to resample. See its
    // comment for what that is worth and where it stops being true.
    static constexpr int BaseWidth = 96;
    static constexpr int BaseHeight = 104;

    explicit PetWindow(AppSettings *settings, QWidget *parent = nullptr);

    SnapEdge snapEdge() const;
    void setFrame(const QImage &frame);
    void setSmoothRendering(bool smooth);
    // Exposed for tests: the device-pixel size the current frame is presented at.
    // The no-resample path is invisible from the outside otherwise, and the
    // offscreen platform tests run at devicePixelRatio 1 where it never engages.
    QSize scaledFrameSize() const;

public slots:
    void setScaleFactor(double factor);
    void setOpacity(double opacity);
    // Suppresses dragging. Clicks and the context menu are untouched, so this is a
    // position lock and not click-through -- the contract rules the latter out.
    void setPositionLocked(bool locked);
    void setAlwaysOnTop(bool enabled);
    void restorePosition();
    void resetPosition();
    void clampToPrimaryScreen();
    // Re-push the native always-on-top level/collection behavior onto the current
    // NSWindow. Needed after events that can recreate or reset it (show, flag
    // toggle, screen change, wake); harmless (idempotent) otherwise.
    void reapplyAlwaysOnTop();

signals:
    void dragStarted();
    void dragDirectionChanged(HorizontalDragDirection direction);
    void dragFinished(SnapEdge edge);
    void snapEdgeChanged(SnapEdge edge);
    void clicked();
    // Right-click (or ctrl-click) on the pet; carries the global cursor position
    // so the owner can pop the shared tray menu there.
    void contextMenuRequested(const QPoint &globalPos);

protected:
    void paintEvent(QPaintEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    QRect primaryAvailableGeometry() const;
    void updateWindowSize();
    void updateSnapEdge(SnapEdge edge);
    void applyRestingCursor();
    // Position-only half of clampToPrimaryScreen(): clamps and re-resolves the
    // snap edge without touching the native window level. The public slot adds
    // the overlay re-push, which is only needed for display/wake events.
    void clampPosition();
    // Brings m_scaled up to date with m_frame, the window size, and the device
    // pixel ratio. Cheap when already current, so paintEvent can call it and no
    // caller has to enumerate every way the geometry might have changed.
    void ensureScaledFrame();

    AppSettings *m_settings;
    QImage m_frame;
    // m_frame scaled to the window, so paintEvent is a blit. Scaling used to
    // happen inside drawImage on every repaint, which is more often than the
    // frame changes: the pet repaints whenever it is exposed, not only when the
    // animation advances.
    QPixmap m_scaled;
    double m_scaleFactor = 1.0;
    bool m_alwaysOnTop = true;
    bool m_positionLocked = false;
    bool m_smoothRendering = true;
    bool m_restoringPosition = false;
    bool m_pointerDown = false;
    bool m_dragging = false;
    QPoint m_pressGlobal;
    QPoint m_pressWindow;
    QPoint m_lastGlobal;
    SnapEdge m_snapEdge = SnapEdge::None;
};
