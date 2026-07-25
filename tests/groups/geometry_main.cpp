#include "support/TestRunner.h"

#include <QtCore/QCoreApplication>

// Window placement and pointer gestures -- the two consumers of potato_geometry.
//
// Each class below is still its own CTest entry, registered with --class in
// tests/CMakeLists.txt, so filtering and parallelism are unchanged.
QObject *createWindowPlacementTest();
QObject *createPetGestureTest();

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    return TestRunner::run(argc, argv, {
        {"WindowPlacementTest", createWindowPlacementTest},
        {"PetGestureTest", createPetGestureTest},
    });
}
