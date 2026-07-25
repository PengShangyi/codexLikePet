#pragma once

#include <QList>

class QObject;

// Lets several small test classes share one executable.
//
// The suite was two dozen separate binaries, most of them linking the same Qt
// frameworks to run a handful of assertions. Grouping them by link set cuts the
// link and AUTOMOC work without giving up anything, because the CTest entries do
// NOT collapse with the binaries: each class is still registered under its own
// name via `--class`, so `ctest -R potato_settings_test` works exactly as before
// and `ctest -j` still has one unit of parallelism per class.
//
// A file that is the only one in its binary keeps its plain QTEST_MAIN macro --
// there is nothing to dispatch between.
namespace TestRunner {

struct Entry {
    const char *name;          // class name; also the CTest entry name's subject
    QObject *(*create)();      // fresh instance per run, as QTest::qExec expects
};

// Runs the entry named by `--class <Name>`, or every entry when the flag is
// absent. `--class` and its value are consumed here; everything else is passed
// through to QTest::qExec, so the usual QTest arguments still work.
//
// Returns the total number of failed test functions, or 1 if `--class` named
// something this binary does not contain.
int run(int argc, char **argv, const QList<Entry> &entries);

}  // namespace TestRunner
