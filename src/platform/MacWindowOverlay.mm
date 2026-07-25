#include "platform/WindowOverlay.h"

#include <QGuiApplication>
#include <QString>

#import <AppKit/AppKit.h>

namespace WindowOverlay {

void apply(quintptr windowId, bool alwaysOnTop)
{
    // Only the cocoa QPA plugin backs windows with an NSView. Under headless
    // plugins (offscreen/minimal, used by tests and CI) winId() is an unrelated
    // handle, so casting it to NSView* and messaging it would crash — no-op there.
    if (QGuiApplication::platformName() != QLatin1String("cocoa")) {
        return;
    }

    // On macOS (cocoa) a Qt WId is the backing NSView*. The NSWindow only exists
    // once the widget has been shown; sending -window to a nil view yields nil, so
    // a not-yet-realized handle is a safe no-op and the caller reapplies on show.
    NSView *view = reinterpret_cast<NSView *>(windowId);
    NSWindow *window = view.window;
    if (!window) {
        return;
    }

    const Policy policy = policyFor(alwaysOnTop);

    window.level = policy.level == Level::Overlay ? NSStatusWindowLevel : NSNormalWindowLevel;

    NSWindowCollectionBehavior behavior = NSWindowCollectionBehaviorDefault;
    if (policy.joinAllSpaces) {
        behavior |= NSWindowCollectionBehaviorCanJoinAllSpaces;
    }
    if (policy.overlayFullScreen) {
        behavior |= NSWindowCollectionBehaviorFullScreenAuxiliary;
    }
    window.collectionBehavior = behavior;

    // An accessory (menu-bar) app is almost never the active application; NSPanels
    // (Qt::Tool) can hide on deactivation, which would make the pet vanish the
    // moment any other app is focused. Keep it pinned regardless of active app.
    window.hidesOnDeactivate = NO;
}
}
