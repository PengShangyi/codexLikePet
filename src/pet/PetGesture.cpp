#include "pet/PetGesture.h"

namespace PetGesture {

MoveDecision onMove(bool alreadyDragging,
                    const QPoint &totalDeltaFromPress,
                    int deltaXSinceLast,
                    DragLock lock,
                    int dragThreshold)
{
    // A locked pet never starts a drag and never reports a direction, so the caller
    // has nothing to act on and the window stays put. The release still comes back
    // as a click from onRelease(), which is what keeps the lock distinct from
    // click-through.
    if (lock == DragLock::Locked) return {};

    MoveDecision decision;
    decision.startDrag = !alreadyDragging
        && WindowPlacement::exceedsDragThreshold(totalDeltaFromPress, dragThreshold);
    if (alreadyDragging || decision.startDrag) {
        decision.direction = WindowPlacement::horizontalDirectionForDelta(deltaXSinceLast);
    }
    return decision;
}

ReleaseDecision onRelease(bool dragging,
                          const QPoint &windowTopLeft,
                          const QSize &windowSize,
                          const QRect &availableGeometry,
                          int snapThreshold)
{
    ReleaseDecision decision;
    if (!dragging) {
        decision.wasClick = true;
        decision.snappedTopLeft = windowTopLeft;
        return decision;
    }
    decision.snapEdge =
        WindowPlacement::resolveSnapEdge(windowTopLeft, windowSize, availableGeometry, snapThreshold);
    decision.snappedTopLeft =
        WindowPlacement::snappedPosition(decision.snapEdge, windowTopLeft, windowSize, availableGeometry);
    return decision;
}

}
