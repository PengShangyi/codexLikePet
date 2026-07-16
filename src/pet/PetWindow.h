#pragma once

#include <QImage>
#include <QWidget>

#include "pet/WindowPlacement.h"

class QScreen;
class QMouseEvent;
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
    void setFrame(const QImage &frame);

public slots:
    void setScaleFactor(double factor);
    void setAlwaysOnTop(bool enabled);
    void restorePosition();
    void resetPosition();
    void clampToPrimaryScreen();

signals:
    void dragStarted();
    void dragDirectionChanged(HorizontalDragDirection direction);
    void dragFinished(SnapEdge edge);
    void clicked();

protected:
    void paintEvent(QPaintEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QRect primaryAvailableGeometry() const;
    void updateWindowSize();

    QImage m_frame;
    double m_scaleFactor = 1.0;
    bool m_alwaysOnTop = true;
    bool m_restoringPosition = false;
    AppSettings *m_settings;
    bool m_pointerDown = false;
    bool m_dragging = false;
    QPoint m_pressGlobal;
    QPoint m_pressWindow;
    QPoint m_lastGlobal;
    SnapEdge m_snapEdge = SnapEdge::None;
};
