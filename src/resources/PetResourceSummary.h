#pragma once

#include <QString>
#include <QStringList>

struct PetPackage;

// Builds the human-readable season/day-night fallback table shown in Settings:
// for each of the eight variant keys, which atlas resolves and which clip each
// slot resolves to (or the fallback label when the package ships none).
//
// A pure function of the package plus one label, so it is testable on its own
// and cannot accidentally start depending on the current environment -- the
// whole point being that this table is environment-independent.
namespace PetResourceSummary {

// Names of the clip slots reported per variant, in display order.
QStringList clipSlots();

// builtInFallbackLabel is the already-localized text for "no clip, falls back to
// the built-in v2 row".
QString build(const PetPackage &package, const QString &builtInFallbackLabel);

}  // namespace PetResourceSummary
