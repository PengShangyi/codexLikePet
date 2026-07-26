#include "pet/PetGesture.h"
#include "pet/WindowPlacement.h"

#include <QTest>

class PetGestureTest final : public QObject
{
    Q_OBJECT

    // The pet's size at scale 1.0; see PetWindow::BaseWidth/BaseHeight.
    static constexpr QSize kPetSize{96, 104};
    static const QRect kAvailable;

private slots:
    void subThresholdMoveDoesNotStartDrag()
    {
        const PetGesture::MoveDecision d = PetGesture::onMove(false, QPoint(2, 1), 2, PetGesture::DragLock::Unlocked, 4);
        QVERIFY(!d.startDrag);
    }

    void crossingThresholdStartsDragWithDirection()
    {
        const PetGesture::MoveDecision right = PetGesture::onMove(false, QPoint(6, 0), 6, PetGesture::DragLock::Unlocked, 4);
        QVERIFY(right.startDrag);
        QCOMPARE(right.direction, HorizontalDragDirection::Right);
    }

    void alreadyDraggingReportsDirectionWithoutRestarting()
    {
        const PetGesture::MoveDecision left = PetGesture::onMove(true, QPoint(-40, 0), -8, PetGesture::DragLock::Unlocked, 4);
        QVERIFY(!left.startDrag); // does not re-start an in-progress drag
        QCOMPARE(left.direction, HorizontalDragDirection::Left);
    }

    // The lock suppresses dragging and nothing else. That distinction is the whole
    // reason it does not amount to click-through, which docs/PROJECT.md rules out:
    // the pet stops moving but stays interactive.
    void lockedPetNeverStartsADragHoweverFarThePointerTravels()
    {
        const PetGesture::MoveDecision far =
            PetGesture::onMove(false, QPoint(400, 300), 40, PetGesture::DragLock::Locked, 4);
        QVERIFY(!far.startDrag);
        QCOMPARE(far.direction, HorizontalDragDirection::None);

        // Even mid-drag, a lock reports no further motion, so the window stops
        // following the pointer instead of continuing to the release.
        const PetGesture::MoveDecision during =
            PetGesture::onMove(true, QPoint(-40, 0), -8, PetGesture::DragLock::Locked, 4);
        QVERIFY(!during.startDrag);
        QCOMPARE(during.direction, HorizontalDragDirection::None);
    }

    void lockedPetStillReportsAClickOnRelease()
    {
        // onRelease sees dragging=false because the lock stopped the drag from ever
        // beginning, so the click path -- click reaction, speech bubble -- is intact.
        const PetGesture::ReleaseDecision d =
            PetGesture::onRelease(false, QPoint(400, 400), kPetSize, kAvailable, 24);
        QVERIFY(d.wasClick);
        QCOMPARE(d.snapEdge, SnapEdge::None);
        QCOMPARE(d.snappedTopLeft, QPoint(400, 400));
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

// Runs inside a grouped binary; see tests/support/TestRunner.h.
QObject *createPetGestureTest() { return new PetGestureTest; }

#include "test_pet_gesture.moc"
