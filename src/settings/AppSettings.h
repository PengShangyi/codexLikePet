#pragma once

#include <QObject>
#include <QTime>

#include <memory>

class QSettings;

enum class MotionPreference {
    FollowSystem,
    Reduce,
    Full,
};

enum class Hemisphere {
    North,
    South,
};

enum class AppLanguage {
    System,
    English,
    SimplifiedChinese,
};

class AppSettings final : public QObject
{
    Q_OBJECT

public:
    explicit AppSettings(QObject *parent = nullptr);
    explicit AppSettings(const QString &iniFilePath, QObject *parent = nullptr);
    ~AppSettings() override;

    double scale() const;
    double animationSpeed() const;
    bool alwaysOnTop() const;
    bool launchAtLogin() const;
    bool typingDetectionEnabled() const;
    MotionPreference motionPreference() const;
    Hemisphere hemisphere() const;
    QTime dayStartsAt() const;
    QTime nightStartsAt() const;
    AppLanguage language() const;
    QString selectedPetId() const;

public slots:
    void setScale(double value);
    void setAnimationSpeed(double value);
    void setAlwaysOnTop(bool value);
    void setLaunchAtLogin(bool value);
    void setTypingDetectionEnabled(bool value);
    void setMotionPreference(MotionPreference value);
    void setHemisphere(Hemisphere value);
    void setDayStartsAt(const QTime &value);
    void setNightStartsAt(const QTime &value);
    void setLanguage(AppLanguage value);
    void setSelectedPetId(const QString &value);

signals:
    void scaleChanged(double value);
    void animationSpeedChanged(double value);
    void alwaysOnTopChanged(bool value);
    void launchAtLoginChanged(bool value);
    void typingDetectionEnabledChanged(bool value);
    void motionPreferenceChanged(MotionPreference value);
    void hemisphereChanged(Hemisphere value);
    void dayStartsAtChanged(const QTime &value);
    void nightStartsAtChanged(const QTime &value);
    void languageChanged(AppLanguage value);
    void selectedPetIdChanged(const QString &value);

private:
    template<typename T>
    bool writeIfChanged(const QString &key, const T &value, const T &current);

    std::unique_ptr<QSettings> m_settings;
};

Q_DECLARE_METATYPE(MotionPreference)
Q_DECLARE_METATYPE(Hemisphere)
Q_DECLARE_METATYPE(AppLanguage)
