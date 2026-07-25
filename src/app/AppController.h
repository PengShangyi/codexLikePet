#pragma once

#include <QObject>
#include <QHash>
#include <QSharedPointer>
#include <QString>
#include <QUuid>
#include <functional>
#include <optional>

#include "pet/BehaviorController.h"
#include "pet/IdleActivityScheduler.h"
#include "resources/PetPackage.h"

class QAction;
class QMenu;
class QSystemTrayIcon;
class QTimer;
class PetWindow;
class AppSettings;
class Localization;
class SettingsWindow;
class OnboardingWindow;
class AboutWindow;
class PetLibrary;
class PetPackageImporter;
class AtlasCache;
class AnimationPlayer;
class AnimationClip;
class PetAtlas;
class BehaviorController;
class AppNotifier;
class QuoteProvider;
class SpeechBubble;
class InputActivitySource;
class TypingActivityDetector;
class EnvironmentClock;
class EnvironmentResolver;
class SystemActivitySource;
class MotionController;
class LoginItemController;
class LoginItemCoordinator;

enum class AppRunMode {
    Normal,
    RuntimeCheck,
};

class AppController final : public QObject
{
    Q_OBJECT

public:
    // Injectable platform boundaries + storage roots. Every field is optional:
    // an unset pointer/callback falls back to the real macOS implementation, so
    // production code constructs AppController(mode) unchanged while tests pass
    // fakes to exercise the wiring headlessly. Injected objects are owned by the
    // caller (never parented to this); defaults are owned by AppController.
    struct Dependencies {
        AppSettings *settings = nullptr;
        InputActivitySource *input = nullptr;
        SystemActivitySource *systemActivity = nullptr;
        EnvironmentClock *clock = nullptr;
        LoginItemController *loginItem = nullptr;
        QuoteProvider *quoteProvider = nullptr;
        AppNotifier *notifier = nullptr;
        QString builtInPetRoot;
        QString userPetRoot;
        std::function<bool()> systemTrayAvailable;
        IdleFidgetPolicy idlePolicy;
    };

    explicit AppController(AppRunMode mode = AppRunMode::Normal, QObject *parent = nullptr);
    AppController(Dependencies deps, AppRunMode mode = AppRunMode::Normal, QObject *parent = nullptr);
    ~AppController() override;

    bool start();
    void requestSettings();
    void showAbout();
    void showWelcome();
    void presentStartupFailure();

    // Exposed for tests: whether the idle-fidget scheduler is currently armed.
    bool isIdleFidgetArmed() const;
    // Exposed for tests: whether keystroke-driven typing presses are engaged
    // (a dedicated typing clip is loaded and motion is allowed).
    bool isTypingPressActive() const;

signals:
    void petVisibilityRequested(bool visible);
    void settingsRequested();

private:
    void setPetVisible(bool visible);
    void updateVisibilityAction();
    void updatePetMenu(const QString &selectedId);
    void refreshPetLibrary();
    void selectPet(const QString &id);
    void importPet(bool directory);
    void removeSelectedPet();
    void applyBehaviorState(BehaviorState state);
    void updateIdleScheduler();
    void playIdleFidget(V2AnimationState state);
    void handlePetClick();
    void setTypingMonitoringEnabled(bool enabled);
    void beginTypingAnimation();
    void onTypingKey();
    void pulseTypingPress();
    void loadCurrentVariant();
    void configurePetPreview();
    void loadPreviewAtlas(const QString &relativePath);
    void loadPreviewClip(const QString &key);
    QSharedPointer<AnimationClip> loadClip(const QString &name);
    bool playClip(const QString &name, bool restart = true);
    void handleSystemSleep();
    void handleSystemWake();

    QSystemTrayIcon *m_trayIcon;
    QMenu *m_menu;
    QMenu *m_petMenu;
    QAction *m_visibilityAction;
    QAction *m_settingsAction;
    QAction *m_welcomeAction = nullptr;
    QAction *m_aboutAction = nullptr;
    QAction *m_quitAction;
    AppSettings *m_settings;
    Localization *m_localization;
    SettingsWindow *m_settingsWindow;
    OnboardingWindow *m_onboardingWindow = nullptr;
    AboutWindow *m_aboutWindow = nullptr;
    PetWindow *m_petWindow;
    PetLibrary *m_petLibrary;
    PetPackageImporter *m_importer;
    AtlasCache *m_atlasCache;
    AnimationPlayer *m_animationPlayer;
    QSharedPointer<PetAtlas> m_currentAtlas;
    QHash<QString, QSharedPointer<AnimationClip>> m_clipCache;
    BehaviorController *m_behavior;
    IdleActivityScheduler *m_idleScheduler;
    std::optional<V2AnimationState> m_activeFidget;
    AppNotifier *m_notifier = nullptr;
    bool m_ownsNotifier = true;
    bool m_ownsLoginItem = true;
    std::function<bool()> m_systemTrayAvailable;
    QuoteProvider *m_quoteProvider;
    SpeechBubble *m_speechBubble;
    InputActivitySource *m_inputSource;
    TypingActivityDetector *m_typingDetector;
    EnvironmentClock *m_environmentClock;
    EnvironmentResolver *m_environmentResolver;
    std::optional<PetPackage> m_currentPackage;
    SystemActivitySource *m_systemActivity;
    MotionController *m_motionController;
    LoginItemController *m_loginItemController;
    LoginItemCoordinator *m_loginItemCoordinator;
    QTimer *m_clickCompletionTimer;
    // Keystroke-driven typing animation: each key advances the held "typing" clip
    // to a press frame; m_typingReturnTimer relaxes it back to rest after a pause.
    QTimer *m_typingReturnTimer;
    bool m_typingPressActive = false;  // true only when a keystroke-driven clip is in use
    bool m_typingPressToggle = false;  // alternates left/right paw press
    int m_typingClipFrames = 0;
    bool m_suppressSystemMutations = false;
    bool m_sleeping = false;
    bool m_petVisible = true;
    QUuid m_activeQuoteRequest;
};
