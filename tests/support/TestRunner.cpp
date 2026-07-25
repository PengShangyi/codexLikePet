#include "support/TestRunner.h"

#include <QByteArray>
#include <QDebug>
#include <QObject>
#include <QTest>

#include <memory>

namespace TestRunner {

int run(int argc, char **argv, const QList<Entry> &entries)
{
    // QTest::qExec rejects arguments it does not recognise, so --class is
    // stripped rather than forwarded.
    QByteArray wanted;
    QList<char *> forwarded;
    forwarded.append(argv[0]);
    for (int i = 1; i < argc; ++i) {
        if (qstrcmp(argv[i], "--class") == 0 && i + 1 < argc) {
            wanted = argv[++i];
            continue;
        }
        forwarded.append(argv[i]);
    }

    int failures = 0;
    bool matched = false;
    for (const Entry &entry : entries) {
        if (!wanted.isEmpty() && wanted != entry.name) continue;
        matched = true;
        // A fresh instance per class: qExec must not be handed an object it has
        // already run.
        const std::unique_ptr<QObject> object(entry.create());
        failures += QTest::qExec(object.get(),
                                 static_cast<int>(forwarded.size()),
                                 forwarded.data());
    }

    if (!wanted.isEmpty() && !matched) {
        qCritical() << "No test class named" << wanted << "in this binary.";
        return 1;
    }
    return failures;
}

}  // namespace TestRunner
