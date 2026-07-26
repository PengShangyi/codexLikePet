#include "settings/AppSettings.h"

#include <QSettings>
#include <QPair>
#include <QVector>

#include <algorithm>
#include <cmath>

namespace {
// Bounds are parameters rather than constants because opacity clamps to a
// different range than scale and speed; the read and the write below must always
// be given the same pair.
double boundedSetting(const QVariant &stored, double fallback, double minimum, double maximum)
{
    bool ok = false;
    const double value = stored.toDouble(&ok);
    return ok && std::isfinite(value) ? std::clamp(value, minimum, maximum) : fallback;
}

constexpr double factorMinimum = 0.5;
constexpr double factorMaximum = 2.0;
// Not zero: a fully transparent pet cannot be clicked or found again, which is
// indistinguishable from having lost the window.
constexpr double opacityMinimum = 0.3;
constexpr double opacityMaximum = 1.0;
}

AppSettings::AppSettings(QObject *parent)
    : QObject(parent)
    , m_settings(std::make_unique<QSettings>())
{
    // Reads must see Potato's own domain and nothing else. With fallbacks on --
    // the default -- the native macOS backend also searches the organisation
    // domain and kCFPreferencesAnyApplication (NSGlobalDomain), so allKeys() and
    // contains() answer for AppleLanguages, AppleLocale and the rest of the
    // user's global preferences. Nothing here wants that, and migrate() was
    // actively broken by it.
    m_settings->setFallbacksEnabled(false);
    migrate();
}

AppSettings::AppSettings(const QString &iniFilePath, QObject *parent)
    : QObject(parent)
    , m_settings(std::make_unique<QSettings>(iniFilePath, QSettings::IniFormat))
{
    // No effect for an explicit ini path, which has exactly one file; set anyway
    // so both constructors leave the store in the same state.
    m_settings->setFallbacksEnabled(false);
    migrate();
}

AppSettings::~AppSettings() = default;

double AppSettings::scale() const
{
    return boundedSetting(m_settings->value(QStringLiteral("appearance/scale"), 1.0), 1.0,
                          factorMinimum, factorMaximum);
}

double AppSettings::animationSpeed() const
{
    return boundedSetting(m_settings->value(QStringLiteral("appearance/animationSpeed"), 1.0), 1.0,
                          factorMinimum, factorMaximum);
}

double AppSettings::opacity() const
{
    return boundedSetting(m_settings->value(QStringLiteral("appearance/opacity"), 1.0), 1.0,
                          opacityMinimum, opacityMaximum);
}

bool AppSettings::positionLocked() const
{
    return m_settings->value(QStringLiteral("behavior/positionLocked"), false).toBool();
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

bool AppSettings::hasWindowPosition() const
{
    return m_settings->contains(QStringLiteral("window/position"));
}

QPoint AppSettings::windowPosition() const
{
    return m_settings->value(QStringLiteral("window/position")).toPoint();
}

void AppSettings::setWindowPosition(const QPoint &value)
{
    // Checks hasWindowPosition() as well as the value so that a first move to
    // exactly (0, 0) still persists instead of matching the absent-key default.
    if (hasWindowPosition() && windowPosition() == value) return;
    m_settings->setValue(QStringLiteral("window/position"), value);
}

void AppSettings::clearWindowPosition()
{
    m_settings->remove(QStringLiteral("window/position"));
}

bool AppSettings::hasSettingsGeometry() const
{
    return m_settings->contains(QStringLiteral("window/settingsGeometry"));
}

QRect AppSettings::settingsGeometry() const
{
    return m_settings->value(QStringLiteral("window/settingsGeometry")).toRect();
}

void AppSettings::setSettingsGeometry(const QRect &value)
{
    if (!value.isValid()) return;
    if (hasSettingsGeometry() && settingsGeometry() == value) return;
    m_settings->setValue(QStringLiteral("window/settingsGeometry"), value);
}

void AppSettings::clearSettingsGeometry()
{
    m_settings->remove(QStringLiteral("window/settingsGeometry"));
}

void AppSettings::setScale(double value)
{
    value = std::isfinite(value) ? std::clamp(value, factorMinimum, factorMaximum) : 1.0;
    if (writeIfChanged(QStringLiteral("appearance/scale"), value, scale())) emit scaleChanged(value);
}

void AppSettings::setOpacity(double value)
{
    value = std::isfinite(value) ? std::clamp(value, opacityMinimum, opacityMaximum) : 1.0;
    if (writeIfChanged(QStringLiteral("appearance/opacity"), value, opacity())) emit opacityChanged(value);
}

void AppSettings::setPositionLocked(bool value)
{
    if (writeIfChanged(QStringLiteral("behavior/positionLocked"), value, positionLocked())) emit positionLockedChanged(value);
}

void AppSettings::setAnimationSpeed(double value)
{
    value = std::isfinite(value) ? std::clamp(value, factorMinimum, factorMaximum) : 1.0;
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
    constexpr int currentSchemaVersion = 3;
    const int storedVersion = m_settings->value(QStringLiteral("meta/schemaVersion"), 0).toInt();
    if (storedVersion >= currentSchemaVersion) return;

    const QVector<QPair<QString, QString>> legacyKeys = {
        {QStringLiteral("scale"), QStringLiteral("appearance/scale")},
        {QStringLiteral("animationSpeed"), QStringLiteral("appearance/animationSpeed")},
        {QStringLiteral("alwaysOnTop"), QStringLiteral("appearance/alwaysOnTop")},
        {QStringLiteral("launchAtLogin"), QStringLiteral("system/launchAtLogin")},
        {QStringLiteral("typingDetectionEnabled"), QStringLiteral("privacy/typingDetection")},
    };

    // Has Potato itself written this profile before? Deliberately not
    // allKeys().isEmpty(): that also counts foreign keys, and on the native macOS
    // backend it is never empty, so every first-run user was misread as an
    // upgrader and never saw the welcome window. The constructors disable
    // fallbacks, but this must not depend on that -- it asks for Potato's keys
    // specifically. Every key we store is grouped, so any group at all is ours;
    // the pre-schema profiles that have no group are caught by the legacy list.
    const bool existingProfile = storedVersion > 0
        || !m_settings->childGroups().isEmpty()
        || std::any_of(legacyKeys.cbegin(), legacyKeys.cend(),
                       [this](const QPair<QString, QString> &keys) {
                           return m_settings->contains(keys.first);
                       });

    if (storedVersion < 1) {
        for (const auto &[legacy, current] : legacyKeys) {
            if (m_settings->contains(legacy) && !m_settings->contains(current)) {
                m_settings->setValue(current, m_settings->value(legacy));
            }
            m_settings->remove(legacy);
        }
    }
    // PetWindow's base render size was halved, so that at scale 1.0 on a Retina
    // display a frame lands on the backing store at its authored size. Scale is a
    // multiplier of that base, so doubling a stored value leaves an existing pet
    // exactly the size it already was.
    //
    // Runs after the block above, which may have just moved a pre-schema top-level
    // `scale` into appearance/scale -- the doubling has to see the moved value.
    //
    // Written even when the key is absent, which is the case that matters most:
    // absent meant the old default of 1.0 against the old base, so leaving it
    // absent would halve the pet of every user who never touched the slider. A
    // fresh profile is not an existingProfile and never reaches this, which is how
    // it gets the new default -- so this depends on that check being right.
    if (storedVersion < 3 && existingProfile) {
        const QString scaleKey = QStringLiteral("appearance/scale");
        const double previous =
            boundedSetting(m_settings->value(scaleKey, 1.0), 1.0, factorMinimum, factorMaximum);
        // A previously oversized pet cannot be represented against the new base and
        // clamps to the maximum, which is the old default size. Deliberate: the old
        // maximum was 384x416 logical points, a quarter of the width of a laptop
        // display, and measured over the 2% idle CPU limit.
        m_settings->setValue(scaleKey, std::clamp(previous * 2.0, factorMinimum, factorMaximum));
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
