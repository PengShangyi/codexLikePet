#pragma once

#include <QObject>

class AppSettings;
class LoginItemController;

class LoginItemCoordinator final : public QObject
{
    Q_OBJECT

public:
    LoginItemCoordinator(AppSettings *settings,
                         LoginItemController *controller,
                         QObject *parent = nullptr);

    void initialize();

signals:
    void updateFailed(const QString &message);

private:
    void apply(bool enabled);

    AppSettings *m_settings;
    LoginItemController *m_controller;
    bool m_applying = false;
};
