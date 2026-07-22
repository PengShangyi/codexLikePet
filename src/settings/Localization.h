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
    AppSettings *m_settings;
};
