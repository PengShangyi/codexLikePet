#include <QApplication>
#include <QCoreApplication>

#include "app/AppController.h"
#include "app/SingleInstance.h"

#include <unistd.h>

#ifndef POTATO_VERSION
#define POTATO_VERSION "0.0.0"
#endif

int main(int argc, char *argv[])
{
    // Windows inherit the display's colour space, which on these panels is Display P3,
    // and both Qt and CoreAnimation re-parse its ICC data as frames are flushed. Asking
    // for sRGB here via QSurfaceFormat::setDefaultFormat() does reduce that -- measured
    // 9 samples down to 3 in CGColorSpaceCreateWithICCData, so roughly 0.11% of a core
    // down to 0.04%. Deliberately not done: the bundled sprites are untagged WebP,
    // currently interpreted as P3, so tagging them sRGB visibly desaturates the pet.
    // A permanent change to how the pet looks is not worth 0.07% of a core.
    QApplication application(argc, argv);
    const bool runtimeCheck = application.arguments().contains(
        QStringLiteral("--potato-runtime-check"));
    QCoreApplication::setOrganizationName(QStringLiteral("Peng"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("com.peng"));
    QCoreApplication::setApplicationName(runtimeCheck
                                             ? QStringLiteral("PotatoRuntimeCheck")
                                             : QStringLiteral("Potato"));
    QCoreApplication::setApplicationVersion(QStringLiteral(POTATO_VERSION));

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
        controller.presentStartupFailure();
        return 1;
    }
    return application.exec();
}
