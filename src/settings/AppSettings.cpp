#include "settings/AppSettings.h"

#include <QSettings>

#include <algorithm>

AppSettings::AppSettings(QObject *parent)
    : QObject(parent)
    , m_settings(std::make_unique<QSettings>())
{
}

AppSettings::AppSettings(const QString &iniFilePath, QObject *parent)
    : QObject(parent)
    , m_settings(std::make_unique<QSettings>(iniFilePath, QSettings::IniFormat))
{
}

AppSettings::~AppSettings() = default;

double AppSettings::scale() const
{
    return std::clamp(m_settings->value(QStringLiteral("appearance/scale"), 1.0).toDouble(), 0.5, 2.0);
}

double AppSettings::animationSpeed() const
{
    return std::clamp(m_settings->value(QStringLiteral("appearance/animationSpeed"), 1.0).toDouble(), 0.5, 2.0);
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
    return m_settings->value(QStringLiteral("environment/dayStart"), QTime(7, 0)).toTime();
}

QTime AppSettings::nightStartsAt() const
{
    return m_settings->value(QStringLiteral("environment/nightStart"), QTime(19, 0)).toTime();
}

AppLanguage AppSettings::language() const
{
    return static_cast<AppLanguage>(std::clamp(m_settings->value(QStringLiteral("interface/language"), 0).toInt(), 0, 2));
}

QString AppSettings::selectedPetId() const
{
    return m_settings->value(QStringLiteral("pets/selectedId"), QStringLiteral("potato")).toString();
}

void AppSettings::setScale(double value)
{
    value = std::clamp(value, 0.5, 2.0);
    if (writeIfChanged(QStringLiteral("appearance/scale"), value, scale())) emit scaleChanged(value);
}

void AppSettings::setAnimationSpeed(double value)
{
    value = std::clamp(value, 0.5, 2.0);
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

template<typename T>
bool AppSettings::writeIfChanged(const QString &key, const T &value, const T &current)
{
    if (value == current) return false;
    m_settings->setValue(key, QVariant::fromValue(value));
    m_settings->sync();
    return true;
}
