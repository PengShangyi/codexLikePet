#include "ui/ValueSlider.h"

#include "ui/Theme.h"

#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>

ValueSlider::ValueSlider(int minimum,
                         int maximum,
                         std::function<QString(int)> formatter,
                         QWidget *parent)
    : QWidget(parent)
    , m_formatter(std::move(formatter))
{
    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(Theme::Metrics::rowSpacing);

    m_slider = new QSlider(Qt::Horizontal, this);
    m_slider->setRange(minimum, maximum);
    root->addWidget(m_slider, 1);

    m_readout = new QLabel(this);
    m_readout->setObjectName(QStringLiteral("valueReadout"));
    m_readout->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    // Reserve the width of the widest value up front. Otherwise the readout grows
    // as the number gets longer and shoves the slider sideways mid-drag.
    const QFontMetrics metrics(m_readout->font());
    int widest = 0;
    for (int candidate : {minimum, maximum}) {
        widest = qMax(widest, metrics.horizontalAdvance(m_formatter(candidate)));
    }
    m_readout->setMinimumWidth(widest);
    root->addWidget(m_readout);

    connect(m_slider, &QSlider::valueChanged, this, [this](int value) {
        updateReadout();
        emit valueChanged(value);
    });
    updateReadout();
}

int ValueSlider::value() const
{
    return m_slider->value();
}

void ValueSlider::setValue(int value)
{
    m_slider->setValue(value);
    // QSlider only emits when the value actually changes, so refresh explicitly
    // for the case where the stored setting already equals the slider default.
    updateReadout();
}

QSlider *ValueSlider::slider() const
{
    return m_slider;
}

void ValueSlider::updateReadout()
{
    m_readout->setText(m_formatter(m_slider->value()));
}
