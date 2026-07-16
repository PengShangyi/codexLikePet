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
SnapEdge resolveSnapEdge(const QPoint &position,
                         const QSize &windowSize,
                         const QRect &availableGeometry,
                         int threshold = 24);
QPoint snappedPosition(SnapEdge edge,
                       const QPoint &position,
                       const QSize &windowSize,
                       const QRect &availableGeometry);
HorizontalDragDirection horizontalDirectionForDelta(int deltaX, int minimumDelta = 1);
}

Q_DECLARE_METATYPE(SnapEdge)
Q_DECLARE_METATYPE(HorizontalDragDirection)
