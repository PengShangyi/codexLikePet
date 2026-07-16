#pragma once

#include <QObject>
#include <QSharedPointer>
#include <optional>

#include "pet/BehaviorController.h"
#include "resources/PetPackage.h"

class QAction;
class QMenu;
class QSystemTrayIcon;
class PetWindow;
class AppSettings;
class Localization;
class SettingsWindow;
class PetLibrary;
class PetPackageImporter;
class AtlasCache;
class AnimationPlayer;
class PetAtlas;
class BehaviorController;
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

class AppController final : public QObject
{
    Q_OBJECT

public:
    explicit AppController(QObject *parent = nullptr);
    ~AppController() override;

    bool start();
    void requestSettings();

signals:
    void petVisibilityRequested(bool visible);
    void settingsRequested();

private:
    void setPetVisible(bool visible);
    void updateVisibilityAction();
    void refreshPetLibrary();
    void selectPet(const QString &id);
    void importPet(bool directory);
    void removeSelectedPet();
    void applyBehaviorState(BehaviorState state);
    void handlePetClick();
    void setTypingMonitoringEnabled(bool enabled);
    void loadCurrentVariant();
    void handleSystemSleep();
    void handleSystemWake();

    QSystemTrayIcon *m_trayIcon;
    QMenu *m_menu;
    QAction *m_visibilityAction;
    QAction *m_settingsAction;
    QAction *m_quitAction;
    AppSettings *m_settings;
    Localization *m_localization;
    SettingsWindow *m_settingsWindow;
    PetWindow *m_petWindow;
    PetLibrary *m_petLibrary;
    PetPackageImporter *m_importer;
    AtlasCache *m_atlasCache;
    AnimationPlayer *m_animationPlayer;
    QSharedPointer<PetAtlas> m_currentAtlas;
    BehaviorController *m_behavior;
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
    bool m_sleeping = false;
    bool m_petVisible = true;
};
