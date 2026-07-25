#pragma once

#include <QWidget>

class QTimer;
class QShowEvent;

class SpeechBubble final : public QWidget
{
    Q_OBJECT

public:
    explicit SpeechBubble(QWidget *parent = nullptr);

    void setAlwaysOnTop(bool enabled);
    void setColorScheme(Qt::ColorScheme scheme);
    void showMessage(const QString &text,
                     const QRect &petGeometry,
                     const QRect &availableGeometry,
                     int durationMs = 3000);
    static QPoint placementFor(const QSize &bubbleSize,
                               const QRect &petGeometry,
                               const QRect &availableGeometry,
                               int gap = 8);

protected:
    void paintEvent(QPaintEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    Qt::ColorScheme m_scheme = Qt::ColorScheme::Light;
    QString m_text;
    QTimer *m_hideTimer;
};
