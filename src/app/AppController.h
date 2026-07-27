#pragma once

#include <QObject>
#include <QFlags>
#include <QHash>
#include <QSharedPointer>
#include <QString>
#include <QUuid>
#include <functional>
#include <memory>
#include <optional>

#include "app/SettingsViewState.h"
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
class PetGuideWindow;
class PetLibrary;
class PetPackageImporter;
class AtlasCache;
class ClipCache;
class AnimationPlayer;
class AnimationClip;
class PetAtlas;
class BehaviorController;
class TypingAnimationDriver;
class AppNotifier;
class QuoteProvider;
class SpeechBubble;
class ThemeWatcher;
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
    void showPetGuide();
    void presentStartupFailure();
    // Puts the pet away or brings it back, as the tray item does. Public because it
    // is a command like requestSettings(), and because hiding is the one quiet reason
    // whose effect on the window and the tray label a test needs to be able to drive.
    void setPetVisible(bool visible);

    // Exposed for tests: whether the idle-fidget scheduler is currently armed.
    bool isIdleFidgetArmed() const;
    // Exposed for tests: whether the rollover poll is armed. Unlike the fidget
    // scheduler, which recomputes from the quiet reasons every time it is asked, this
    // is plain timer state -- so it is what catches the pet being resumed while it
    // should still be quiet.
    bool isEnvironmentPollRunning() const;
    // Exposed for tests: whether keystroke-driven typing presses are engaged
    // (a dedicated typing clip is loaded and motion is allowed).
    bool isTypingPressActive() const;

signals:
    void petVisibilityRequested(bool visible);
    void settingsRequested();

private:
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
    void loadCurrentVariant();
    void configurePetPreview();
    void updateResourceSummary();
    void updateEnvironmentSummary();
    void loadPreviewAtlas(const QString &relativePath);
    // Builds the settings window on first call, wires it, and replays the state
    // recorded in m_settingsView. Never returns null.
    SettingsWindow *settingsWindow();
    OnboardingWindow *onboardingWindow();
    AboutWindow *aboutWindow();
    PetGuideWindow *petGuideWindow();
    void loadPreviewClip(const QString &key);
    QSharedPointer<AnimationClip> loadClip(const QString &name);
    bool playClip(const QString &name, bool restart = true);
    // Why the pet is not animating. More than one can hold at once -- hiding the pet
    // and then letting the display sleep is two -- and it stays quiet until every one
    // of them clears, which is what a single bool per reason kept getting wrong.
    //
    // Hidden is the only reason that touches window visibility or the tray item's
    // label. When the display sleeps the pet has not gone anywhere, it just stops
    // moving, so reporting that as hidden would offer a "Show Pet" item for a pet
    // that is already shown and would hand QWidget::setVisible a window it must not
    // touch.
    enum class QuietReason {
        Hidden = 0x1,
        SystemAsleep = 0x2,
        DisplayAsleep = 0x4,
    };

    void setQuiet(QuietReason reason, bool quiet);
    bool isQuiet() const { return m_quietReasons != QFlags<QuietReason>(); }
    bool isHidden() const { return m_quietReasons.testFlag(QuietReason::Hidden); }
    // The two halves the quiet reasons share. Every reason stops the same seven
    // things and resumes the same four; they used to be written out once per reason,
    // byte for byte, which is why a third reason was worth factoring first.
    void stopPetActivity();
    void resumePetActivity();
    // Wake recomputes placement and re-reads the environment whether or not the pet
    // is still quiet for another reason: display geometry can change while a machine
    // is asleep, and a rollover during a long sleep would otherwise wait for the next
    // poll -- which never comes while the resolver is stopped.
    void handleWake();

    void handleSystemSleep();
    void handleSystemWake();
    void handleDisplayAsleep();
    void handleDisplayAwake();

    QSystemTrayIcon *m_trayIcon;
    std::unique_ptr<QMenu> m_menu;
    QMenu *m_petMenu;
    QAction *m_visibilityAction;
    QAction *m_settingsAction;
    QAction *m_aboutAction = nullptr;
    QAction *m_quitAction;
    AppSettings *m_settings;
    Localization *m_localization;
    // All four are built on first use, not at startup: between them they are
    // five settings pages, a preview widget and three more windows, and most
    // sessions never open any of them. m_settingsView absorbs the state pushed
    // at the settings window in the meantime.
    std::unique_ptr<SettingsWindow> m_settingsWindow;
    std::unique_ptr<OnboardingWindow> m_onboardingWindow;
    std::unique_ptr<AboutWindow> m_aboutWindow;
    std::unique_ptr<PetGuideWindow> m_petGuideWindow;
    SettingsViewState m_settingsView;
    std::unique_ptr<PetWindow> m_petWindow;
    PetLibrary *m_petLibrary;
    std::unique_ptr<PetPackageImporter> m_importer;
    std::unique_ptr<AtlasCache> m_atlasCache;
    std::unique_ptr<ClipCache> m_clipCache;
    AnimationPlayer *m_animationPlayer;
    QSharedPointer<PetAtlas> m_currentAtlas;
    BehaviorController *m_behavior;
    IdleActivityScheduler *m_idleScheduler;
    std::optional<V2AnimationState> m_activeFidget;
    // Owned only when we created the default; null when the caller injected one.
    // The raw members below stay valid either way.
    std::unique_ptr<AppNotifier> m_ownedNotifier;
    AppNotifier *m_notifier = nullptr;
    std::function<bool()> m_systemTrayAvailable;
    QuoteProvider *m_quoteProvider;
    std::unique_ptr<SpeechBubble> m_speechBubble;
    InputActivitySource *m_inputSource;
    TypingActivityDetector *m_typingDetector;
    EnvironmentClock *m_environmentClock;
    EnvironmentResolver *m_environmentResolver;
    std::optional<PetPackage> m_currentPackage;
    SystemActivitySource *m_systemActivity;
    MotionController *m_motionController;
    std::unique_ptr<LoginItemController> m_ownedLoginItem;
    LoginItemController *m_loginItemController;
    LoginItemCoordinator *m_loginItemCoordinator;
    // Owns the keystroke-driven typing animation state and its relax timer.
    TypingAnimationDriver *m_typingDriver;
    ThemeWatcher *m_bubbleTheme = nullptr;
    QTimer *m_clickCompletionTimer;
    bool m_suppressSystemMutations = false;
    // Held across the Input Monitoring prompt, which runs a nested event loop: a
    // wake delivered inside it comes back through resumePetActivity() and would
    // stack a second copy of the same dialog.
    bool m_resolvingInputPermission = false;
    QFlags<QuietReason> m_quietReasons;
    QUuid m_activeQuoteRequest;
};
