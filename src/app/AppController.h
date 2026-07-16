#pragma once

#include <QObject>

class QAction;
class QMenu;
class QSystemTrayIcon;

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
    bool m_petVisible = true;
};
