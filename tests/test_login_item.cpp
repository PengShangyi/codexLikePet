#include "login/LoginItemController.h"
#include "login/LoginItemCoordinator.h"
#include "settings/AppSettings.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

class FakeLoginItemController final : public LoginItemController
{
public:
    bool setEnabled(bool enabled, QString *error) override
    {
        calls.append(enabled);
        if (failNext) {
            failNext = false;
            *error = QStringLiteral("simulated failure");
            return false;
        }
        return true;
    }
    QVector<bool> calls;
    bool failNext = false;
};

class LoginItemTest final : public QObject
{
    Q_OBJECT

private slots:
    void appliesChangesAndRollsBackFailedEnablement()
    {
        QTemporaryDir temp;
        AppSettings settings(temp.filePath(QStringLiteral("settings.ini")));
        FakeLoginItemController controller;
        LoginItemCoordinator coordinator(&settings, &controller);
        QSignalSpy failure(&coordinator, &LoginItemCoordinator::updateFailed);
        settings.setLaunchAtLogin(true);
        QCOMPARE(controller.calls, QVector<bool>({true}));
        QVERIFY(settings.launchAtLogin());
        settings.setLaunchAtLogin(false);
        QCOMPARE(controller.calls, QVector<bool>({true, false}));
        controller.failNext = true;
        settings.setLaunchAtLogin(true);
        QVERIFY(!settings.launchAtLogin());
        QCOMPARE(failure.count(), 1);
        QCOMPARE(controller.calls, QVector<bool>({true, false, true}));
    }

    void startupReconcilesAStaleDisabledLoginItem()
    {
        QTemporaryDir temp;
        AppSettings settings(temp.filePath(QStringLiteral("settings.ini")));
        FakeLoginItemController controller;
        LoginItemCoordinator coordinator(&settings, &controller);
        coordinator.initialize();
        QCOMPARE(controller.calls, QVector<bool>({false}));
    }
};

QTEST_GUILESS_MAIN(LoginItemTest)

#include "test_login_item.moc"
