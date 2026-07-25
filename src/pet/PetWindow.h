#pragma once

#include <QImage>
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
    static constexpr int CellWidth = 192;
    static constexpr int CellHeight = 208;

    explicit PetWindow(AppSettings *settings, QWidget *parent = nullptr);

    double scaleFactor() const;
    bool isAlwaysOnTop() const;
    SnapEdge snapEdge() const;
    bool usesSmoothRendering() const;
    void setFrame(const QImage &frame);
    void setSmoothRendering(bool smooth);

public slots:
    void setScaleFactor(double factor);
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

    QImage m_frame;
    double m_scaleFactor = 1.0;
    bool m_alwaysOnTop = true;
    bool m_smoothRendering = true;
    bool m_restoringPosition = false;
    bool m_pointerDown = false;
    bool m_dragging = false;
    QPoint m_pressGlobal;
    QPoint m_pressWindow;
    QPoint m_lastGlobal;
    SnapEdge m_snapEdge = SnapEdge::None;
};
