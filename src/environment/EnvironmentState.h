#pragma once

#include <QMetaType>
#include <QString>
#include <QStringList>

enum class Season {
    Spring,
    Summer,
    Autumn,
    Winter,
};

enum class TimePhase {
    Day,
    Night,
};

struct VariantKey {
    Season season = Season::Spring;
    TimePhase phase = TimePhase::Day;

    QString seasonName() const;
    QString phaseName() const;
    QString combinedName() const;
    QStringList fallbackNames() const;

    bool operator==(const VariantKey &other) const = default;
};

Q_DECLARE_METATYPE(Season)
Q_DECLARE_METATYPE(TimePhase)
Q_DECLARE_METATYPE(VariantKey)
