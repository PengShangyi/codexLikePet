#include "pet/PetGesture.h"
#include "pet/WindowPlacement.h"

#include <QTest>

class PetGestureTest final : public QObject
{
    Q_OBJECT

    static constexpr QSize kPetSize{192, 208};
    static const QRect kAvailable;

private slots:
    void subThresholdMoveDoesNotStartDrag()
    {
        const PetGesture::MoveDecision d = PetGesture::onMove(false, QPoint(2, 1), 2, 4);
        QVERIFY(!d.startDrag);
    }

    void crossingThresholdStartsDragWithDirection()
    {
        const PetGesture::MoveDecision right = PetGesture::onMove(false, QPoint(6, 0), 6, 4);
        QVERIFY(right.startDrag);
        QCOMPARE(right.direction, HorizontalDragDirection::Right);
    }

    void alreadyDraggingReportsDirectionWithoutRestarting()
    {
        const PetGesture::MoveDecision left = PetGesture::onMove(true, QPoint(-40, 0), -8, 4);
        QVERIFY(!left.startDrag); // does not re-start an in-progress drag
        QCOMPARE(left.direction, HorizontalDragDirection::Left);
    }

    void releaseWithoutDraggingIsAClick()
    {
        const PetGesture::ReleaseDecision d =
            PetGesture::onRelease(false, QPoint(400, 400), kPetSize, kAvailable, 24);
        QVERIFY(d.wasClick);
        QCOMPARE(d.snapEdge, SnapEdge::None);
        QCOMPARE(d.snappedTopLeft, QPoint(400, 400));
    }

    void releaseNearLeftEdgeSnapsLeft()
    {
        const QPoint origin(5, 400);
        const PetGesture::ReleaseDecision d =
            PetGesture::onRelease(true, origin, kPetSize, kAvailable, 24);
        QVERIFY(!d.wasClick);
        QCOMPARE(d.snapEdge, SnapEdge::Left);
        QCOMPARE(d.snappedTopLeft,
                 WindowPlacement::snappedPosition(SnapEdge::Left, origin, kPetSize, kAvailable));
    }

    void releaseInCenterDoesNotSnap()
    {
        const PetGesture::ReleaseDecision d =
            PetGesture::onRelease(true, QPoint(400, 300), kPetSize, kAvailable, 24);
        QVERIFY(!d.wasClick);
        QCOMPARE(d.snapEdge, SnapEdge::None);
    }
};

const QRect PetGestureTest::kAvailable{0, 0, 1000, 800};

QTEST_GUILESS_MAIN(PetGestureTest)

#include "test_pet_gesture.moc"
