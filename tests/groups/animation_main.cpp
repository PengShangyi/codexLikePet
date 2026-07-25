#include "support/TestRunner.h"

#include <QtCore/QCoreApplication>

// Atlas and clip decoding, the caches over them, and the players that drive them.
//
// Each class below is still its own CTest entry, registered with --class in
// tests/CMakeLists.txt, so filtering and parallelism are unchanged.
QObject *createAtlasTest();
QObject *createIdleSchedulerTest();

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    return TestRunner::run(argc, argv, {
        {"AtlasTest", createAtlasTest},
        {"IdleSchedulerTest", createIdleSchedulerTest},
    });
}
