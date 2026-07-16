#include "accessibility/MotionController.h"
#include "platform/SystemActivitySource.h"
#include "settings/AppSettings.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

class FakeSystemActivity final : public SystemActivitySource
{
    Q_OBJECT
public:
    using SystemActivitySource::SystemActivitySource;
    bool systemReduceMotion() const override { return reduced; }
    void setReduced(bool value) { reduced = value; emit reduceMotionChanged(value); }
    bool reduced = false;
};

class MotionTest final : public QObject
{
    Q_OBJECT

private slots:
    void combinesSystemAndExplicitPreferences()
    {
        QTemporaryDir temp;
        AppSettings settings(temp.filePath(QStringLiteral("settings.ini")));
        FakeSystemActivity system;
        MotionController controller(&settings, &system);
        QSignalSpy spy(&controller, &MotionController::reducedMotionChanged);
        QVERIFY(!controller.reducedMotion());
        system.setReduced(true);
        QVERIFY(controller.reducedMotion());
        settings.setMotionPreference(MotionPreference::Full);
        QVERIFY(!controller.reducedMotion());
        system.setReduced(false);
        QVERIFY(!controller.reducedMotion());
        settings.setMotionPreference(MotionPreference::Reduce);
        QVERIFY(controller.reducedMotion());
        QCOMPARE(spy.count(), 3);
    }
};

QTEST_GUILESS_MAIN(MotionTest)

#include "test_motion.moc"
