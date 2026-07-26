#include "environment/EnvironmentResolver.h"

#include "environment/EnvironmentClock.h"
#include "settings/AppSettings.h"

#include <QTimer>

EnvironmentResolver::EnvironmentResolver(AppSettings *settings,
                                         EnvironmentClock *clock,
                                         QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_clock(clock)
    , m_timer(new QTimer(this))
{
    m_timer->setInterval(60'000);
    connect(m_timer, &QTimer::timeout, this, &EnvironmentResolver::reevaluate);
    connect(settings, &AppSettings::hemisphereChanged, this, &EnvironmentResolver::reevaluate);
    connect(settings, &AppSettings::dayStartsAtChanged, this, &EnvironmentResolver::reevaluate);
    connect(settings, &AppSettings::nightStartsAtChanged, this, &EnvironmentResolver::reevaluate);
    reevaluate();
}

VariantKey EnvironmentResolver::current() const { return m_current; }
void EnvironmentResolver::start() { reevaluate(); m_timer->start(); }
void EnvironmentResolver::stop() { m_timer->stop(); }
bool EnvironmentResolver::isRunning() const { return m_timer->isActive(); }

void EnvironmentResolver::reevaluate()
{
    const QDateTime now = m_clock->now();
    const VariantKey next{seasonForDate(now.date(), m_settings->hemisphere()),
                          phaseForTime(now.time(), m_settings->dayStartsAt(), m_settings->nightStartsAt())};
    if (m_initialized && next == m_current) return;
    m_initialized = true;
    m_current = next;
    emit environmentChanged(next);
}

Season EnvironmentResolver::seasonForDate(const QDate &date, Hemisphere hemisphere)
{
    Season northern;
    if (date.month() >= 3 && date.month() <= 5) northern = Season::Spring;
    else if (date.month() >= 6 && date.month() <= 8) northern = Season::Summer;
    else if (date.month() >= 9 && date.month() <= 11) northern = Season::Autumn;
    else northern = Season::Winter;
    if (hemisphere == Hemisphere::North) return northern;
    switch (northern) {
    case Season::Spring: return Season::Autumn;
    case Season::Summer: return Season::Winter;
    case Season::Autumn: return Season::Spring;
    case Season::Winter: return Season::Summer;
    }
    return Season::Spring;
}

TimePhase EnvironmentResolver::phaseForTime(const QTime &time,
                                            const QTime &dayStart,
                                            const QTime &nightStart)
{
    if (dayStart == nightStart) return TimePhase::Day;
    const bool day = dayStart < nightStart
        ? time >= dayStart && time < nightStart
        : time >= dayStart || time < nightStart;
    return day ? TimePhase::Day : TimePhase::Night;
}

QString EnvironmentResolver::atlasRelativePath(const PetPackage &package, const VariantKey &key)
{
    for (const QString &candidate : key.fallbackNames()) {
        const QString path = package.variants.value(candidate);
        if (!path.isEmpty()) return path;
    }
    return package.spriteSheetPath;
}

std::optional<ClipDefinition> EnvironmentResolver::clipDefinition(const PetPackage &package,
                                                                  const VariantKey &key,
                                                                  const QString &name)
{
    for (const QString &candidate : key.fallbackNames()) {
        const auto variant = package.variantClips.constFind(candidate);
        if (variant == package.variantClips.cend()) continue;
        const auto clip = variant->constFind(name);
        if (clip != variant->cend()) return *clip;
    }
    const auto base = package.clips.constFind(name);
    if (base != package.clips.cend()) return *base;
    return std::nullopt;
}
