#include <QApplication>
#include <QCoreApplication>

#include "app/AppController.h"
#include "app/SingleInstance.h"

#include <unistd.h>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    const bool runtimeCheck = application.arguments().contains(
        QStringLiteral("--potato-runtime-check"));
    QCoreApplication::setOrganizationName(QStringLiteral("Peng"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("com.peng"));
    QCoreApplication::setApplicationName(runtimeCheck
                                             ? QStringLiteral("PotatoRuntimeCheck")
                                             : QStringLiteral("Potato"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    SingleInstance instance(QStringLiteral("com.peng.potato.%1").arg(getuid()));
    if (!instance.acquire()) {
        instance.notifyPrimary(QStringLiteral("show-settings"));
        return 0;
    }

    AppController controller(runtimeCheck ? AppRunMode::RuntimeCheck : AppRunMode::Normal);
    QObject::connect(&instance, &SingleInstance::messageReceived, &controller,
                     [&controller](const QString &message) {
                         if (message == QStringLiteral("show-settings")) {
                             controller.requestSettings();
                         }
                     });
    if (!controller.start()) {
        return 1;
    }
    return application.exec();
}
