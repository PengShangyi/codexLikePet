#pragma once

#include <QObject>
#include <QSharedPointer>

#include "pet/BehaviorController.h"

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
    bool m_petVisible = true;
};
