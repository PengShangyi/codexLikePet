#include "login/LoginItemCoordinator.h"

#include "login/LoginItemController.h"
#include "settings/AppSettings.h"

LoginItemCoordinator::LoginItemCoordinator(AppSettings *settings,
                                           LoginItemController *controller,
                                           QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_controller(controller)
{
    connect(settings, &AppSettings::launchAtLoginChanged, this, &LoginItemCoordinator::apply);
}

void LoginItemCoordinator::initialize()
{
    apply(m_settings->launchAtLogin());
}

void LoginItemCoordinator::apply(bool enabled)
{
    if (m_applying) return;
    m_applying = true;
    QString error;
    if (!m_controller->setEnabled(enabled, &error)) {
        m_settings->setLaunchAtLogin(!enabled);
        emit updateFailed(error);
    }
    m_applying = false;
}
