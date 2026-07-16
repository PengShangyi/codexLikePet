#pragma once

#include <QImage>
#include <QWidget>

class QScreen;

class PetWindow final : public QWidget
{
    Q_OBJECT

public:
    static constexpr int CellWidth = 192;
    static constexpr int CellHeight = 208;

    explicit PetWindow(QWidget *parent = nullptr);

    double scaleFactor() const;
    bool isAlwaysOnTop() const;
    void setFrame(const QImage &frame);

public slots:
    void setScaleFactor(double factor);
    void setAlwaysOnTop(bool enabled);
    void restorePosition();
    void clampToPrimaryScreen();

protected:
    void paintEvent(QPaintEvent *event) override;
    void moveEvent(QMoveEvent *event) override;

private:
    QRect primaryAvailableGeometry() const;
    void updateWindowSize();

    QImage m_frame;
    double m_scaleFactor = 1.0;
    bool m_alwaysOnTop = true;
    bool m_restoringPosition = false;
};
