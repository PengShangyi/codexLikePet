#pragma once

#include <QWidget>

#include <functional>

class QLabel;
class QSlider;

// A native QSlider with its current value shown beside it.
//
// Replaces the previous arrangement, where the readout was appended to the row's
// *label* text ("Size — 120%") because there was nowhere else to put it. That
// forced the label to be rewritten on every drag and made the label a shared
// mutable surface between retranslate() and the value handler.
class ValueSlider final : public QWidget
{
    Q_OBJECT

public:
    // formatter turns the raw slider value into the readout text, so the caller
    // owns "120%" versus "1.20x" without this class knowing about either.
    ValueSlider(int minimum,
                int maximum,
                std::function<QString(int)> formatter,
                QWidget *parent = nullptr);

    int value() const;
    void setValue(int value);

    QSlider *slider() const;

signals:
    void valueChanged(int value);

private:
    void updateReadout();

    std::function<QString(int)> m_formatter;
    QSlider *m_slider;
    QLabel *m_readout;
};
