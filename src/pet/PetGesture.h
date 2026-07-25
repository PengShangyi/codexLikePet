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

// An enum rather than a bool, and it carries no default. Adding this as a
// defaulted bool parameter next to the int threshold compiled silently and
// reinterpreted every existing `onMove(dragging, delta, dx, 4)` call as "locked",
// because int converts to bool without a warning. A distinct type makes that
// mistake a compile error.
enum class DragLock { Unlocked, Locked };

struct MoveDecision {
    bool startDrag = false; // this move crosses the threshold and begins a drag
    HorizontalDragDirection direction = HorizontalDragDirection::None;
};

struct ReleaseDecision {
    bool wasClick = false; // released without ever dragging
    SnapEdge snapEdge = SnapEdge::None;
    QPoint snappedTopLeft; // where the window should settle (when not a click)
};

// DragLock::Locked suppresses dragging only. A locked pet still reports the release
// as a click, so click reactions, speech bubbles, and the context menu keep working
// -- the lock is not click-through, which the project contract rules out.
MoveDecision onMove(bool alreadyDragging,
                    const QPoint &totalDeltaFromPress,
                    int deltaXSinceLast,
                    DragLock lock,
                    int dragThreshold = 4);

ReleaseDecision onRelease(bool dragging,
                          const QPoint &windowTopLeft,
                          const QSize &windowSize,
                          const QRect &availableGeometry,
                          int snapThreshold = 24);

}
