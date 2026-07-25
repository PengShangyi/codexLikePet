#pragma once

#include <QColor>
#include <QImage>
#include <QList>
#include <QString>

// Shared pet-atlas fixtures.
//
// Five test files each grew their own near-identical generator, and the suite
// ended up encoding a 1536x2288 PNG about thirty times. Measured, that encode is
// the entire cost: building the QImage takes 0.30ms, encoding it takes 69ms,
// writing already-encoded bytes takes nothing measurable.
//
// So every generator here encodes once per process and caches the bytes; each
// later call for the same fixture is a file write. Tests that need a
// deliberately invalid atlas (wrong size, an opaque unused cell, a nine-frame
// clip) still build their own -- this namespace only covers the shapes that are
// shared, and adding a one-off here would make the cache pay for itself once.
namespace TestAtlas {

// A 1536x2288 atlas satisfying PetAtlas' v2 occupancy contract: one opaque pixel
// in every used cell, and every unused cell fully transparent.
const QImage &validImage();
bool writeValid(const QString &path);

// Right geometry, solid fill. Passes PetAtlas::load, which checks size and alpha
// but not occupancy; deliberately does NOT pass the package validator. For tests
// that only care about cache identity or frame geometry.
bool writeFilled(const QString &path, const QColor &color);

// A horizontal strip of `frames` 192x208 cells in one colour.
bool writeClip(const QString &path, int frames, const QColor &color);

// The same, but each frame gets its own colour, so a test reading one pixel can
// tell which frame is currently on screen.
bool writeClipFrames(const QString &path, const QList<QColor> &colors);

}  // namespace TestAtlas
