#include "pet/IdleActivityScheduler.h"
#include "pet/PetAtlas.h"

#include <QSignalSpy>
#include <QTest>

class IdleSchedulerTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() { qRegisterMetaType<V2AnimationState>("V2AnimationState"); }

    void armsAndFiresTheChosenAnimationAfterInterval()
    {
        IdleFidgetPolicy policy;
        policy.nextIntervalMs = [] { return 5; };
        policy.nextAnimation = [] { return V2AnimationState::Waiting; };
        IdleActivityScheduler scheduler(policy);
        QSignalSpy spy(&scheduler, &IdleActivityScheduler::fidgetRequested);

        QVERIFY(!scheduler.isArmed());
        scheduler.setActive(true);
        QVERIFY(scheduler.isArmed());
        QVERIFY(spy.wait(500));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.first().first().value<V2AnimationState>(), V2AnimationState::Waiting);
        // Single-shot: stays quiet until the fidget is reported finished.
        QVERIFY(!scheduler.isArmed());
    }

    void reArmsOnlyAfterFidgetFinished()
    {
        IdleFidgetPolicy policy;
        policy.nextIntervalMs = [] { return 5; };
        policy.nextAnimation = [] { return V2AnimationState::Jumping; };
        IdleActivityScheduler scheduler(policy);
        QSignalSpy spy(&scheduler, &IdleActivityScheduler::fidgetRequested);

        scheduler.setActive(true);
        QVERIFY(spy.wait(500));
        QCOMPARE(spy.count(), 1);
        QVERIFY(!scheduler.isArmed());
        scheduler.notifyFidgetFinished();
        QVERIFY(scheduler.isArmed());
        QVERIFY(spy.wait(500));
        QCOMPARE(spy.count(), 2);
    }

    void disarmingSuppressesFurtherFidgets()
    {
        IdleFidgetPolicy policy;
        policy.nextIntervalMs = [] { return 5; };
        policy.nextAnimation = [] { return V2AnimationState::Review; };
        IdleActivityScheduler scheduler(policy);
        QSignalSpy spy(&scheduler, &IdleActivityScheduler::fidgetRequested);

        scheduler.setActive(true);
        scheduler.setActive(false);
        QVERIFY(!scheduler.isArmed());
        QTest::qWait(60);
        QCOMPARE(spy.count(), 0);
    }

    void reActivatingWhileActiveDoesNotRestartTheTimer()
    {
        int intervalRequests = 0;
        IdleFidgetPolicy policy;
        policy.nextIntervalMs = [&intervalRequests] {
            ++intervalRequests;
            return 100000;
        };
        policy.nextAnimation = [] { return V2AnimationState::Jumping; };
        IdleActivityScheduler scheduler(policy);

        scheduler.setActive(true);
        scheduler.setActive(true);
        scheduler.setActive(true);
        QCOMPARE(intervalRequests, 1); // armed exactly once, no starvation
        QVERIFY(scheduler.isArmed());
    }

    void defaultPoolIsTheFourOtherwiseUnusedRows()
    {
        const QVector<V2AnimationState> pool = IdleActivityScheduler::defaultPool();
        QCOMPARE(pool.size(), 4);
        QVERIFY(pool.contains(V2AnimationState::Jumping));
        QVERIFY(pool.contains(V2AnimationState::Failed));
        QVERIFY(pool.contains(V2AnimationState::Waiting));
        QVERIFY(pool.contains(V2AnimationState::Review));
    }
};

// Runs inside a grouped binary; see tests/support/TestRunner.h.
QObject *createIdleSchedulerTest() { return new IdleSchedulerTest; }

#include "test_idle_scheduler.moc"
