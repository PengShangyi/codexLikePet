#pragma once

#include <QObject>

class QAction;
class QMenu;
class QSystemTrayIcon;
class PetWindow;
class AppSettings;
class Localization;
class SettingsWindow;

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

    QSystemTrayIcon *m_trayIcon;
    QMenu *m_menu;
    QAction *m_visibilityAction;
    QAction *m_settingsAction;
    QAction *m_quitAction;
    AppSettings *m_settings;
    Localization *m_localization;
    SettingsWindow *m_settingsWindow;
    PetWindow *m_petWindow;
    bool m_petVisible = true;
};
