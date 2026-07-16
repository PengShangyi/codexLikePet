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
}
