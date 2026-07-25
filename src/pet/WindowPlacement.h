#pragma once

#include <QPoint>
#include <QRect>
#include <QSize>
#include <QMetaType>

enum class SnapEdge { None, Left, Right, Bottom };
enum class HorizontalDragDirection { None, Left, Right };

namespace WindowPlacement {
QPoint defaultPosition(const QRect &availableGeometry, const QSize &windowSize, int margin = 24);
QPoint clampToAvailableGeometry(const QPoint &position,
                                const QSize &windowSize,
                                const QRect &availableGeometry);
bool isUsableSavedPosition(const QPoint &position,
                           const QSize &windowSize,
                           const QRect &availableGeometry);
// Settings-window placement. Same discard-if-it-no-longer-fits discipline as the
// pet's saved position, but for a resizable window, so the size is restored too and
// has to be clamped rather than merely accepted or rejected.
QPoint centeredPosition(const QSize &windowSize, const QRect &availableGeometry);
bool isUsableSavedGeometry(const QRect &geometry, const QRect &availableGeometry);
QRect fitToAvailableGeometry(const QRect &geometry, const QRect &availableGeometry);
SnapEdge resolveSnapEdge(const QPoint &position,
                         const QSize &windowSize,
                         const QRect &availableGeometry,
                         int threshold = 24);
QPoint snappedPosition(SnapEdge edge,
                       const QPoint &position,
                       const QSize &windowSize,
                       const QRect &availableGeometry);
HorizontalDragDirection horizontalDirectionForDelta(int deltaX, int minimumDelta = 2);
bool exceedsDragThreshold(const QPoint &delta, int threshold = 4);
}

Q_DECLARE_METATYPE(SnapEdge)
Q_DECLARE_METATYPE(HorizontalDragDirection)
