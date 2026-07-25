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

    void centersASettingsWindowOnTheAvailableArea()
    {
        const QRect available(0, 25, 1000, 675);
        const QPoint centered = WindowPlacement::centeredPosition(QSize(680, 520), available);
        QCOMPARE(centered, QPoint(160, 102));

        // A window larger than the screen centers to a negative offset; clamping is
        // fitToAvailableGeometry's job, not this function's.
        const QPoint oversized = WindowPlacement::centeredPosition(QSize(1200, 900), available);
        QVERIFY(oversized.x() < available.left());
    }

    // The settings window is resizable and its saved rect can outlive the display it
    // was saved on, so unlike the pet's saved position this asks for an intersection
    // rather than full containment: a window nudged past the edge is still findable
    // and gets clamped, one on a monitor that is gone must be discarded.
    void acceptsPartlyVisibleSavedGeometryButNotAVanishedDisplay()
    {
        const QRect available(0, 25, 1000, 675);
        QVERIFY(WindowPlacement::isUsableSavedGeometry(QRect(100, 100, 680, 520), available));
        QVERIFY(WindowPlacement::isUsableSavedGeometry(QRect(960, 660, 680, 520), available));

        QVERIFY(!WindowPlacement::isUsableSavedGeometry(QRect(2000, 100, 680, 520), available));
        QVERIFY(!WindowPlacement::isUsableSavedGeometry(QRect(), available));
        QVERIFY(!WindowPlacement::isUsableSavedGeometry(QRect(100, 100, 0, 0), available));
    }

    void fitsSavedGeometryBackInsideTheAvailableArea()
    {
        const QRect available(0, 25, 1000, 675);

        // Already inside: unchanged.
        QCOMPARE(WindowPlacement::fitToAvailableGeometry(QRect(100, 100, 680, 520), available),
                 QRect(100, 100, 680, 520));

        // Hanging off the bottom right: moved back, size kept.
        QCOMPARE(WindowPlacement::fitToAvailableGeometry(QRect(900, 600, 680, 520), available),
                 QRect(320, 180, 680, 520));

        // Bigger than the screen: shrunk to fit, then positioned at the origin.
        QCOMPARE(WindowPlacement::fitToAvailableGeometry(QRect(-50, 0, 1400, 900), available),
                 QRect(0, 25, 1000, 675));
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

    void distinguishesClicksFromFourPointDragsByEuclideanDistance()
    {
        QVERIFY(!WindowPlacement::exceedsDragThreshold(QPoint(2, 2), 4));
        QVERIFY(!WindowPlacement::exceedsDragThreshold(QPoint(3, 0), 4));
        QVERIFY(WindowPlacement::exceedsDragThreshold(QPoint(4, 0), 4));
        QVERIFY(WindowPlacement::exceedsDragThreshold(QPoint(3, 3), 4));
    }
};

// Runs inside a grouped binary; see tests/support/TestRunner.h.
QObject *createWindowPlacementTest() { return new WindowPlacementTest; }

#include "test_window_placement.moc"
