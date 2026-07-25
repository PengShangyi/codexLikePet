#include "settings/AppSettings.h"

#include <QSettings>
#include <QPair>
#include <QVector>

#include <algorithm>
#include <cmath>

namespace {
double boundedSetting(const QVariant &stored, double fallback)
{
    bool ok = false;
    const double value = stored.toDouble(&ok);
    return ok && std::isfinite(value) ? std::clamp(value, 0.5, 2.0) : fallback;
}
}

AppSettings::AppSettings(QObject *parent)
    : QObject(parent)
    , m_settings(std::make_unique<QSettings>())
{
    migrate();
}

AppSettings::AppSettings(const QString &iniFilePath, QObject *parent)
    : QObject(parent)
    , m_settings(std::make_unique<QSettings>(iniFilePath, QSettings::IniFormat))
{
    migrate();
}

AppSettings::~AppSettings() = default;

double AppSettings::scale() const
{
    return boundedSetting(m_settings->value(QStringLiteral("appearance/scale"), 1.0), 1.0);
}

double AppSettings::animationSpeed() const
{
    return boundedSetting(m_settings->value(QStringLiteral("appearance/animationSpeed"), 1.0), 1.0);
}

bool AppSettings::alwaysOnTop() const
{
    return m_settings->value(QStringLiteral("appearance/alwaysOnTop"), true).toBool();
}

bool AppSettings::launchAtLogin() const
{
    return m_settings->value(QStringLiteral("system/launchAtLogin"), false).toBool();
}

bool AppSettings::typingDetectionEnabled() const
{
    return m_settings->value(QStringLiteral("privacy/typingDetection"), false).toBool();
}

MotionPreference AppSettings::motionPreference() const
{
    return static_cast<MotionPreference>(std::clamp(m_settings->value(QStringLiteral("accessibility/motion"), 0).toInt(), 0, 2));
}

Hemisphere AppSettings::hemisphere() const
{
    return static_cast<Hemisphere>(std::clamp(m_settings->value(QStringLiteral("environment/hemisphere"), 0).toInt(), 0, 1));
}

QTime AppSettings::dayStartsAt() const
{
    const QTime value = m_settings->value(QStringLiteral("environment/dayStart"), QTime(7, 0)).toTime();
    return value.isValid() ? value : QTime(7, 0);
}

QTime AppSettings::nightStartsAt() const
{
    const QTime value = m_settings->value(QStringLiteral("environment/nightStart"), QTime(19, 0)).toTime();
    return value.isValid() ? value : QTime(19, 0);
}

AppLanguage AppSettings::language() const
{
    return static_cast<AppLanguage>(std::clamp(m_settings->value(QStringLiteral("interface/language"), 0).toInt(), 0, 2));
}

QString AppSettings::selectedPetId() const
{
    return m_settings->value(QStringLiteral("pets/selectedId"), QStringLiteral("potato")).toString();
}

bool AppSettings::onboardingCompleted() const
{
    return m_settings->value(QStringLiteral("onboarding/welcomeShown"), false).toBool();
}

void AppSettings::setScale(double value)
{
    value = std::isfinite(value) ? std::clamp(value, 0.5, 2.0) : 1.0;
    if (writeIfChanged(QStringLiteral("appearance/scale"), value, scale())) emit scaleChanged(value);
}

void AppSettings::setAnimationSpeed(double value)
{
    value = std::isfinite(value) ? std::clamp(value, 0.5, 2.0) : 1.0;
    if (writeIfChanged(QStringLiteral("appearance/animationSpeed"), value, animationSpeed())) emit animationSpeedChanged(value);
}

void AppSettings::setAlwaysOnTop(bool value)
{
    if (writeIfChanged(QStringLiteral("appearance/alwaysOnTop"), value, alwaysOnTop())) emit alwaysOnTopChanged(value);
}

void AppSettings::setLaunchAtLogin(bool value)
{
    if (writeIfChanged(QStringLiteral("system/launchAtLogin"), value, launchAtLogin())) emit launchAtLoginChanged(value);
}

void AppSettings::setTypingDetectionEnabled(bool value)
{
    if (writeIfChanged(QStringLiteral("privacy/typingDetection"), value, typingDetectionEnabled())) emit typingDetectionEnabledChanged(value);
}

void AppSettings::setMotionPreference(MotionPreference value)
{
    const int raw = std::clamp(static_cast<int>(value), 0, 2);
    value = static_cast<MotionPreference>(raw);
    if (writeIfChanged(QStringLiteral("accessibility/motion"), raw, static_cast<int>(motionPreference()))) emit motionPreferenceChanged(value);
}

void AppSettings::setHemisphere(Hemisphere value)
{
    const int raw = std::clamp(static_cast<int>(value), 0, 1);
    value = static_cast<Hemisphere>(raw);
    if (writeIfChanged(QStringLiteral("environment/hemisphere"), raw, static_cast<int>(hemisphere()))) emit hemisphereChanged(value);
}

void AppSettings::setDayStartsAt(const QTime &value)
{
    if (value.isValid() && writeIfChanged(QStringLiteral("environment/dayStart"), value, dayStartsAt())) emit dayStartsAtChanged(value);
}

void AppSettings::setNightStartsAt(const QTime &value)
{
    if (value.isValid() && writeIfChanged(QStringLiteral("environment/nightStart"), value, nightStartsAt())) emit nightStartsAtChanged(value);
}

void AppSettings::setLanguage(AppLanguage value)
{
    const int raw = std::clamp(static_cast<int>(value), 0, 2);
    value = static_cast<AppLanguage>(raw);
    if (writeIfChanged(QStringLiteral("interface/language"), raw, static_cast<int>(language()))) emit languageChanged(value);
}

void AppSettings::setSelectedPetId(const QString &value)
{
    const QString cleaned = value.trimmed();
    if (!cleaned.isEmpty() && writeIfChanged(QStringLiteral("pets/selectedId"), cleaned, selectedPetId())) emit selectedPetIdChanged(cleaned);
}

void AppSettings::setOnboardingCompleted(bool value)
{
    m_settings->setValue(QStringLiteral("onboarding/welcomeShown"), value);
    m_settings->sync();
}

void AppSettings::migrate()
{
    constexpr int currentSchemaVersion = 2;
    const int storedVersion = m_settings->value(QStringLiteral("meta/schemaVersion"), 0).toInt();
    if (storedVersion >= currentSchemaVersion) return;

    // Any existing key means this profile predates the current schema, i.e. the
    // user has run Potato before and should not be shown the first-run welcome.
    const bool existingProfile = !m_settings->allKeys().isEmpty();

    if (storedVersion < 1) {
        const QVector<QPair<QString, QString>> legacyKeys = {
            {QStringLiteral("scale"), QStringLiteral("appearance/scale")},
            {QStringLiteral("animationSpeed"), QStringLiteral("appearance/animationSpeed")},
            {QStringLiteral("alwaysOnTop"), QStringLiteral("appearance/alwaysOnTop")},
            {QStringLiteral("launchAtLogin"), QStringLiteral("system/launchAtLogin")},
            {QStringLiteral("typingDetectionEnabled"), QStringLiteral("privacy/typingDetection")},
        };
        for (const auto &[legacy, current] : legacyKeys) {
            if (m_settings->contains(legacy) && !m_settings->contains(current)) {
                m_settings->setValue(current, m_settings->value(legacy));
            }
            m_settings->remove(legacy);
        }
    }
    // Suppress the first-run welcome for upgraders; only a genuinely fresh
    // profile leaves onboarding/welcomeShown at its false default.
    if (existingProfile && !m_settings->contains(QStringLiteral("onboarding/welcomeShown"))) {
        m_settings->setValue(QStringLiteral("onboarding/welcomeShown"), true);
    }
    m_settings->setValue(QStringLiteral("meta/schemaVersion"), currentSchemaVersion);
    m_settings->sync();
}

template<typename T>
bool AppSettings::writeIfChanged(const QString &key, const T &value, const T &current)
{
    if (value == current) return false;
    // No sync() here on purpose. Sliders emit one write per step, so flushing on
    // every write turned a single drag into ~150 disk flushes. QSettings persists
    // on destruction, and the native macOS backend hands writes to cfprefsd
    // immediately. The two places that genuinely need an immediate flush --
    // migrate() and setOnboardingCompleted() -- still call sync() explicitly.
    m_settings->setValue(key, QVariant::fromValue(value));
    return true;
}
