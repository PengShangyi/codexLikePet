#include "pet/PetGesture.h"

namespace PetGesture {

MoveDecision onMove(bool alreadyDragging,
                    const QPoint &totalDeltaFromPress,
                    int deltaXSinceLast,
                    int dragThreshold)
{
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
