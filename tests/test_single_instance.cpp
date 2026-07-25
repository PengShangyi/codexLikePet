#include "app/SingleInstance.h"

#include <QSignalSpy>
#include <QTest>
#include <QUuid>

class SingleInstanceTest final : public QObject
{
    Q_OBJECT

private slots:
    void forwardsMessagesToThePrimaryInstance()
    {
        // QLocalServer appends this to macOS' already-long per-user temporary
        // directory. Keep the random suffix short enough for sockaddr_un.
        const QString suffix = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
        const QString name = QStringLiteral("potato-test-%1").arg(suffix);
        QVERIFY(name.size() <= 32);
        SingleInstance primary(name);
        QVERIFY(primary.acquire());
        QVERIFY(primary.isPrimary());

        SingleInstance secondary(name);
        QVERIFY(!secondary.acquire());
        QSignalSpy spy(&primary, &SingleInstance::messageReceived);
        QVERIFY(secondary.notifyPrimary(QStringLiteral("show-settings")));
        QTRY_COMPARE(spy.count(), 1);
        QCOMPARE(spy.first().first().toString(), QStringLiteral("show-settings"));
    }
};

// Runs inside a grouped binary; see tests/support/TestRunner.h.
QObject *createSingleInstanceTest() { return new SingleInstanceTest; }

#include "test_single_instance.moc"
