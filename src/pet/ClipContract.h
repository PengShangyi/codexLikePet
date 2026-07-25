#pragma once

#include "pet/PetAtlas.h"

#include <QSize>
#include <QVector>

// The extension-clip half of the pet resource contract, in one place because two
// unrelated layers enforce it: PetPackageValidator rejects a non-conforming clip
// at import, and AnimationClip::load refuses to play one at runtime. Those rules
// used to be written out twice with the frame and duration bounds hard-coded in
// both, so tightening one copy silently left the other permissive.
namespace ClipContract {

inline constexpr int MinFrames = 1;
inline constexpr int MaxFrames = 8;
inline constexpr int MinDurationMs = 50;
inline constexpr int MaxDurationMs = 2000;

// A clip is a horizontal strip of 1..8 cells, each exactly one atlas cell.
// Writes the frame count to *frameCount when non-null (0 on failure).
inline bool isValidGeometry(const QSize &size, int *frameCount = nullptr)
{
    if (frameCount) *frameCount = 0;
    if (!size.isValid() || size.height() != PetAtlas::CellHeight
        || size.width() % PetAtlas::CellWidth != 0) {
        return false;
    }
    const int frames = size.width() / PetAtlas::CellWidth;
    if (frames < MinFrames || frames > MaxFrames) return false;
    if (frameCount) *frameCount = frames;
    return true;
}

inline bool isValidDuration(int durationMs)
{
    return durationMs >= MinDurationMs && durationMs <= MaxDurationMs;
}

inline bool areValidDurations(const QVector<int> &durationsMs)
{
    for (const int duration : durationsMs) {
        if (!isValidDuration(duration)) return false;
    }
    return true;
}

}  // namespace ClipContract
