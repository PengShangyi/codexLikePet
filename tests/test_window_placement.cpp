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

    void resolvesSupportedSnapEdgesWithBottomPriority()
    {
        const QRect available(0, 0, 1000, 700);
        const QSize size(200, 100);
        QCOMPARE(WindowPlacement::resolveSnapEdge(QPoint(10, 300), size, available, 24), SnapEdge::Left);
        QCOMPARE(WindowPlacement::resolveSnapEdge(QPoint(790, 300), size, available, 24), SnapEdge::Right);
        QCOMPARE(WindowPlacement::resolveSnapEdge(QPoint(10, 595), size, available, 24), SnapEdge::Bottom);
        QCOMPARE(WindowPlacement::snappedPosition(SnapEdge::Right, QPoint(780, 200), size, available),
                 QPoint(800, 200));
    }

    void classifiesHorizontalMotionOnlyAboveThreshold()
    {
        QCOMPARE(WindowPlacement::horizontalDirectionForDelta(-3, 2), HorizontalDragDirection::Left);
        QCOMPARE(WindowPlacement::horizontalDirectionForDelta(3, 2), HorizontalDragDirection::Right);
        QCOMPARE(WindowPlacement::horizontalDirectionForDelta(1, 2), HorizontalDragDirection::None);
    }
};

QTEST_GUILESS_MAIN(WindowPlacementTest)

#include "test_window_placement.moc"
