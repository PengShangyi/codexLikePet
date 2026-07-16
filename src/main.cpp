#include <QApplication>
#include <QCoreApplication>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("Peng"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("com.peng"));
    QCoreApplication::setApplicationName(QStringLiteral("Potato"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    // The long-running menu bar lifecycle is introduced in the next milestone.
    QTimer::singleShot(0, &application, &QCoreApplication::quit);
    return application.exec();
}

