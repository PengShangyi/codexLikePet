#pragma once

#include <QtGlobal>  // quintptr (== Qt's WId), without pulling in QtGui

// Boundary for making a window a true always-on-top overlay on macOS: not merely
// above ordinary windows, but visible over *other apps'* full-screen Spaces.
//
// Qt's Qt::WindowStaysOnTopHint only raises the Cocoa window level. That does not
// survive another application entering full screen — the OS switches to that
// app's Space and an ordinary floating window is left behind on the previous
// Space. Appearing over full screen additionally requires the window's
// collection behavior to opt into joining every Space and overlaying full-screen
// apps. policyFor() is a pure, unit-tested decision; apply() is the thin,
// platform-specific poke that realizes it on the native window.
namespace WindowOverlay {

// Native stacking, mapped to a concrete Cocoa NSWindow.level by the platform
// layer so this header stays free of AppKit.
enum class Level {
    Normal,   // ordinary window level; may be covered by other windows
    Overlay,  // above other apps, including over their full-screen Spaces
};

struct Policy {
    Level level = Level::Normal;
    bool joinAllSpaces = false;      // present in every Space, incl. full-screen ones
    bool overlayFullScreen = false;  // may draw over a full-screen app as auxiliary
};

// How a window should behave for a given always-on-top preference.
Policy policyFor(bool alwaysOnTop);

// Applies policyFor(alwaysOnTop) to the native window backing `windowId` (a Qt
// WId, i.e. QWidget::winId()). No-op when the handle has no realized native
// window yet, so it is safe to call early; it MUST be reapplied after the native
// window is recreated (e.g. toggling window flags, or re-showing after a hide),
// because that resets the level and collection behavior back to Qt's defaults.
void apply(quintptr windowId, bool alwaysOnTop);
}
