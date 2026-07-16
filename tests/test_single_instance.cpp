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
        const QString name = QStringLiteral("potato-test-%1").arg(QUuid::createUuid().toString());
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

QTEST_GUILESS_MAIN(SingleInstanceTest)

#include "test_single_instance.moc"
