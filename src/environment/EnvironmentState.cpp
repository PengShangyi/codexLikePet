#include "environment/EnvironmentState.h"

QString VariantKey::seasonName() const
{
    switch (season) {
    case Season::Spring: return QStringLiteral("spring");
    case Season::Summer: return QStringLiteral("summer");
    case Season::Autumn: return QStringLiteral("autumn");
    case Season::Winter: return QStringLiteral("winter");
    }
    return QStringLiteral("spring");
}

QString VariantKey::phaseName() const
{
    return phase == TimePhase::Day ? QStringLiteral("day") : QStringLiteral("night");
}

QString VariantKey::combinedName() const
{
    return seasonName() + QLatin1Char('-') + phaseName();
}

QStringList VariantKey::fallbackNames() const
{
    return {combinedName(), seasonName(), phaseName()};
}
