#pragma once

#include "settings/AppSettings.h"

#include <QObject>

enum class TextKey {
    ShowPet, HidePet, Settings, Quit, General, Pet, ImportPet, RemovePet,
    Size, AnimationSpeed, AlwaysOnTop, LaunchAtLogin, TypingDetection,
    TypingPrivacyNote, ReducedMotion, FollowSystem, ReduceMotion, FullMotion,
    Hemisphere, North, South, DayStarts, NightStarts, Language, SystemLanguage,
    English, SimplifiedChinese, ResetPosition, Close, Preview, NoPetSelected,
    ImportPackage, ImportDirectory, ImportSucceeded, ImportFailed, RemoveConfirmation,
    ValidationReport,
    InputPermissionTitle, InputPermissionBody, OpenSystemSettings,
    LoginItemErrorTitle,
    PreviewVariant, PreviewAnimation, PreviewClip, ResourceFallbacks,
    PetLoadErrorTitle, StartupFailedTitle, StartupFailedBody,
    OnboardingTitle, OnboardingIntro,
    OnboardingMenuBarHeading, OnboardingMenuBarBody,
    OnboardingSettingsHeading, OnboardingSettingsBody,
    OnboardingTypingHeading, OnboardingTypingBody,
    OnboardingImportHeading, OnboardingImportBody,
    OnboardingGetStarted,
    AboutMenuItem, WelcomeMenuItem, AboutTagline, AboutCopyright,
    AboutVersionLabel, AboutViewLicenses,
    PetAccessibleName, PetAccessibleDescription,
};

class Localization final : public QObject
{
    Q_OBJECT

public:
    explicit Localization(AppSettings *settings, QObject *parent = nullptr);

    QString text(TextKey key) const;
    bool usesChinese() const;

signals:
    void languageChanged();

private:
    void refreshLanguage();

    AppSettings *m_settings;
    // Cached because text() is called once per string and a full retranslate()
    // asks for around seventy of them; resolving it each time meant that many
    // QSettings reads plus system-locale lookups. Refreshed whenever the language
    // preference changes, before languageChanged is re-emitted.
    bool m_usesChinese = false;
};
