#include "pet/WindowPlacement.h"

#include <algorithm>

namespace WindowPlacement {
QPoint defaultPosition(const QRect &availableGeometry, const QSize &windowSize, int margin)
{
    return QPoint(availableGeometry.right() - windowSize.width() - margin + 1,
                  availableGeometry.bottom() - windowSize.height() - margin + 1);
}

QPoint clampToAvailableGeometry(const QPoint &position,
                                const QSize &windowSize,
                                const QRect &availableGeometry)
{
    const int maximumX = std::max(availableGeometry.left(),
                                  availableGeometry.right() - windowSize.width() + 1);
    const int maximumY = std::max(availableGeometry.top(),
                                  availableGeometry.bottom() - windowSize.height() + 1);
    return QPoint(std::clamp(position.x(), availableGeometry.left(), maximumX),
                  std::clamp(position.y(), availableGeometry.top(), maximumY));
}

bool isUsableSavedPosition(const QPoint &position,
                           const QSize &windowSize,
                           const QRect &availableGeometry)
{
    const QRect proposed(position, windowSize);
    return availableGeometry.contains(proposed.topLeft())
        && availableGeometry.contains(proposed.bottomRight());
}

SnapEdge resolveSnapEdge(const QPoint &position,
                         const QSize &windowSize,
                         const QRect &availableGeometry,
                         int threshold)
{
    const QPoint clamped = clampToAvailableGeometry(position, windowSize, availableGeometry);
    const int bottomY = availableGeometry.bottom() - windowSize.height() + 1;
    const int rightX = availableGeometry.right() - windowSize.width() + 1;
    if (bottomY - clamped.y() <= threshold) return SnapEdge::Bottom;
    if (clamped.x() - availableGeometry.left() <= threshold) return SnapEdge::Left;
    if (rightX - clamped.x() <= threshold) return SnapEdge::Right;
    return SnapEdge::None;
}

QPoint snappedPosition(SnapEdge edge,
                       const QPoint &position,
                       const QSize &windowSize,
                       const QRect &availableGeometry)
{
    QPoint result = clampToAvailableGeometry(position, windowSize, availableGeometry);
    switch (edge) {
    case SnapEdge::Left: result.setX(availableGeometry.left()); break;
    case SnapEdge::Right: result.setX(availableGeometry.right() - windowSize.width() + 1); break;
    case SnapEdge::Bottom: result.setY(availableGeometry.bottom() - windowSize.height() + 1); break;
    case SnapEdge::None: break;
    }
    return result;
}

HorizontalDragDirection horizontalDirectionForDelta(int deltaX, int minimumDelta)
{
    if (deltaX >= minimumDelta) return HorizontalDragDirection::Right;
    if (deltaX <= -minimumDelta) return HorizontalDragDirection::Left;
    return HorizontalDragDirection::None;
}

bool exceedsDragThreshold(const QPoint &delta, int threshold)
{
    const qint64 x = delta.x();
    const qint64 y = delta.y();
    const qint64 limit = std::max(0, threshold);
    return x * x + y * y >= limit * limit;
}
}
