#include "support/TestRunner.h"

#include <QtCore/QCoreApplication>

// The resource pipeline: the filesystem safety rules, the validator that
// enforces them, the library that lists installed pets, the importer, and the
// composer that builds an atlas for them to accept.
//
// Each class below is still its own CTest entry, registered with --class in
// tests/CMakeLists.txt, so filtering and parallelism are unchanged.
QObject *createPackagePolicyTest();
QObject *createPackageValidatorTest();
QObject *createLibraryTest();
QObject *createPackageImporterTest();
QObject *createAtlasComposerTest();
QObject *createPetPackageWriterTest();

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    return TestRunner::run(argc, argv, {
        {"PackagePolicyTest", createPackagePolicyTest},
        {"PackageValidatorTest", createPackageValidatorTest},
        {"LibraryTest", createLibraryTest},
        {"PackageImporterTest", createPackageImporterTest},
        {"AtlasComposerTest", createAtlasComposerTest},
        {"PetPackageWriterTest", createPetPackageWriterTest},
    });
}
