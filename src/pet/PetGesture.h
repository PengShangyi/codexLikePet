#pragma once

#include "pet/WindowPlacement.h"

#include <QPoint>
#include <QRect>
#include <QSize>

// Pure click-vs-drag / edge-snap recognizer extracted from PetWindow so the
// gesture rules are unit-testable without a live widget or display. The widget
// keeps the imperative concerns (cursor, move(), QSettings, signal emission);
// this decides only what a move/release *means*.
namespace PetGesture {

struct MoveDecision {
    bool startDrag = false; // this move crosses the threshold and begins a drag
    HorizontalDragDirection direction = HorizontalDragDirection::None;
};

struct ReleaseDecision {
    bool wasClick = false; // released without ever dragging
    SnapEdge snapEdge = SnapEdge::None;
    QPoint snappedTopLeft; // where the window should settle (when not a click)
};

MoveDecision onMove(bool alreadyDragging,
                    const QPoint &totalDeltaFromPress,
                    int deltaXSinceLast,
                    int dragThreshold = 4);

ReleaseDecision onRelease(bool dragging,
                          const QPoint &windowTopLeft,
                          const QSize &windowSize,
                          const QRect &availableGeometry,
                          int snapThreshold = 24);

}
