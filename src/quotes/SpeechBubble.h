#pragma once

#include <QWidget>

class QTimer;

class SpeechBubble final : public QWidget
{
    Q_OBJECT

public:
    explicit SpeechBubble(QWidget *parent = nullptr);

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

private:
    QString m_text;
    QTimer *m_hideTimer;
};
