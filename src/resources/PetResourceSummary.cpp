#include "resources/PetResourceSummary.h"

#include "environment/EnvironmentResolver.h"
#include "environment/EnvironmentState.h"
#include "resources/PetPackage.h"

#include <QVector>

namespace PetResourceSummary {

QStringList clipSlots()
{
    return {QStringLiteral("click"),
            QStringLiteral("typing"),
            QStringLiteral("edge-left"),
            QStringLiteral("edge-right"),
            QStringLiteral("edge-bottom")};
}

QString build(const PetPackage &package, const QString &builtInFallbackLabel)
{
    static const QVector<Season> seasons{Season::Spring,
                                         Season::Summer,
                                         Season::Autumn,
                                         Season::Winter};
    static const QVector<TimePhase> phases{TimePhase::Day, TimePhase::Night};
    const QStringList clipNames = clipSlots();

    QStringList summary;
    summary.reserve(seasons.size() * phases.size());
    for (const Season season : seasons) {
        for (const TimePhase phase : phases) {
            const VariantKey key{season, phase};
            QStringList clipSummary;
            clipSummary.reserve(clipNames.size());
            for (const QString &clipName : clipNames) {
                const auto clip = EnvironmentResolver::clipDefinition(package, key, clipName);
                clipSummary.append(QStringLiteral("%1=%2")
                                       .arg(clipName, clip ? clip->path : builtInFallbackLabel));
            }
            summary.append(QStringLiteral("%1 → %2\n  %3")
                               .arg(key.combinedName(),
                                    EnvironmentResolver::atlasRelativePath(package, key),
                                    clipSummary.join(QStringLiteral("; "))));
        }
    }
    return summary.join(QLatin1Char('\n'));
}

}  // namespace PetResourceSummary
