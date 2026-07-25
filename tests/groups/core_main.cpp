#include "support/TestRunner.h"

#include <QtCore/QCoreApplication>

// Everything that needs no more than a QCoreApplication: settings, the
// environment resolver, the activity detectors, the login-item coordinator,
// and the overlay policy. Nine classes that between them ran for a quarter of
// a second and each paid a link and a process launch.
//
// Each class below is still its own CTest entry, registered with --class in
// tests/CMakeLists.txt, so filtering and parallelism are unchanged.
QObject *createSmokeTest();
QObject *createSingleInstanceTest();
QObject *createSettingsTest();
QObject *createTypingActivityTest();
QObject *createMotionTest();
QObject *createLoginItemTest();
QObject *createWindowOverlayTest();
QObject *createEnvironmentTest();
QObject *createResourceSummaryTest();

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    return TestRunner::run(argc, argv, {
        {"SmokeTest", createSmokeTest},
        {"SingleInstanceTest", createSingleInstanceTest},
        {"SettingsTest", createSettingsTest},
        {"TypingActivityTest", createTypingActivityTest},
        {"MotionTest", createMotionTest},
        {"LoginItemTest", createLoginItemTest},
        {"WindowOverlayTest", createWindowOverlayTest},
        {"EnvironmentTest", createEnvironmentTest},
        {"ResourceSummaryTest", createResourceSummaryTest},
    });
}
