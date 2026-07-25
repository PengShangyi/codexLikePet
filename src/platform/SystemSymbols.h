#pragma once

#include <QIcon>
#include <QString>

class QColor;

// SF Symbols, for the settings nav bar.
//
// A free-function namespace rather than an injectable interface, matching
// MacApplication: there is nothing here to fake, and a null QIcon is already the
// honest answer when the platform cannot supply one. Callers must degrade
// gracefully -- the nav bar falls back to text-only buttons, which is why this can
// stay out of the widget libraries and out of the headless tests.
namespace SystemSymbols {

// Renders the named SF Symbol tinted to `tint` at roughly `pointSize` points, or
// returns a null QIcon when the symbol does not exist or the build is not macOS.
// The result is a fixed bitmap, so the caller re-requests it when the color scheme
// changes rather than relying on NSImage template behavior.
QIcon icon(const QString &symbolName, int pointSize, const QColor &tint);

}  // namespace SystemSymbols
