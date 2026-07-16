#include "input/InputActivitySource.h"
#include "input/TypingActivityDetector.h"

#include <QSignalSpy>
#include <QTest>

class FakeInputActivitySource final : public InputActivitySource
{
    Q_OBJECT
public:
    using InputActivitySource::InputActivitySource;
    InputStartResult start() override { m_active = true; return InputStartResult::Started; }
    void stop() override { m_active = false; }
    bool isActive() const override { return m_active; }
    void simulateActivity() { emit activityDetected(); }
private:
    bool m_active = false;
};

class TypingActivityTest final : public QObject
{
    Q_OBJECT

private slots:
    void exposesNoKeyPayloadAndExpiresAfterInactivity()
    {
        FakeInputActivitySource source;
        TypingActivityDetector detector(&source, 40);
        QSignalSpy spy(&detector, &TypingActivityDetector::typingChanged);
        source.simulateActivity();
        QVERIFY(detector.isTyping());
        QCOMPARE(spy.count(), 1);
        source.simulateActivity();
        QCOMPARE(spy.count(), 1);
        QTRY_VERIFY_WITH_TIMEOUT(!detector.isTyping(), 150);
        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy.last().first().toBool(), false);
    }

    void resetImmediatelyEndsTyping()
    {
        FakeInputActivitySource source;
        TypingActivityDetector detector(&source, 1000);
        source.simulateActivity();
        detector.reset();
        QVERIFY(!detector.isTyping());
    }
};

QTEST_GUILESS_MAIN(TypingActivityTest)

#include "test_typing_activity.moc"
