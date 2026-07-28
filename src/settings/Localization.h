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
    // Settings navigation pages. Pet and General reuse the keys above.
    Appearance, Behavior, Environment,
    // Card titles.
    SectionCurrentPet, SectionDisplay, SectionWindowInteraction, SectionActivity,
    SectionSeasonPhase, SectionStartup, SectionInterface, SectionAbout,
    // Rows.
    Opacity, LockPosition, LockPositionNote, CurrentEnvironment, ResourceDetails,
    UseStandardAnimation,
    // The "Make a pet" authoring guide, opened from the Pet page. The prompt
    // templates it shows are not keys: they are English in both languages by
    // design (see PetGuideWindow).
    PetGuideButton, PetGuideTitle, PetGuideIntro,
    PetGuideContractHeading, PetGuideContractBody,
    PetGuideStep1Heading, PetGuideStep1Body,
    PetGuideStep2Heading, PetGuideStep2Body,
    PetGuideStep3Heading, PetGuideStep3Body,
    PetGuideStep4Heading, PetGuideStep4Body,
    PetGuidePromptLabel, PetGuideRowPromptLabel, PetGuideManifestLabel,
    PetGuidePlaceholderNote, PetGuideCopy, PetGuideCopied,
    PetGuideOpenDoc, PetGuideRevealSkill,
    // The atlas assembler, opened from the guide's step 3. The eleven row names are
    // the contract's own words rather than translations: they are what the user typed
    // into the prompt ("State: idle") and therefore what they named the file, so the
    // Chinese entries gloss them instead of replacing them.
    AssemblerButton, AssemblerTitle, AssemblerIntro,
    AssemblerSectionRows, AssemblerSectionKey, AssemblerSectionPet,
    AssemblerChromaKey, AssemblerKeyTolerance, AssemblerDespill,
    AssemblerDisplayName, AssemblerPetId,
    AssemblerChoose, AssemblerFillFromFolder, AssemblerDetectKey,
    AssemblerCompose, AssemblerInstall,
    AssemblerChooseStripTitle, AssemblerFillFromFolderTitle,
    AssemblerNoStrip, AssemblerScaleReadout, AssemblerFilledCount, AssemblerReady,
    AssemblerRowIdle, AssemblerRowRunningRight, AssemblerRowRunningLeft,
    AssemblerRowWaving, AssemblerRowJumping, AssemblerRowFailed,
    AssemblerRowWaiting, AssemblerRowRunning, AssemblerRowReview,
    AssemblerRowLookA, AssemblerRowLookB,
    AssemblerProblemMissingRow, AssemblerProblemStripTooSmall,
    AssemblerProblemEmptyFrame, AssemblerProblemOccupancy,
    AssemblerProblemStripAspect, AssemblerProblemOutlierFrame,
    AssemblerProblemDoesNotFit,
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
