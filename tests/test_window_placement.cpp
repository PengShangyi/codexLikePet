#include "pet/WindowPlacement.h"

#include <QTest>

class WindowPlacementTest final : public QObject
{
    Q_OBJECT

private slots:
    void defaultsToBottomRightWithMargin()
    {
        QCOMPARE(WindowPlacement::defaultPosition(QRect(0, 25, 1440, 875), QSize(192, 208), 24),
                 QPoint(1224, 668));
    }

    void clampsEveryEdge()
    {
        const QRect available(10, 20, 1000, 700);
        QCOMPARE(WindowPlacement::clampToAvailableGeometry(QPoint(-100, 900), QSize(200, 100), available),
                 QPoint(10, 620));
        QCOMPARE(WindowPlacement::clampToAvailableGeometry(QPoint(999, -50), QSize(200, 100), available),
                 QPoint(810, 20));
    }

    void rejectsPartiallyOffscreenSavedPosition()
    {
        const QRect available(0, 0, 1000, 700);
        QVERIFY(WindowPlacement::isUsableSavedPosition(QPoint(800, 500), QSize(200, 200), available));
        QVERIFY(!WindowPlacement::isUsableSavedPosition(QPoint(801, 500), QSize(200, 200), available));
    }
};

QTEST_GUILESS_MAIN(WindowPlacementTest)

#include "test_window_placement.moc"
