#include "platform/WindowOverlay.h"

namespace WindowOverlay {

Policy policyFor(bool alwaysOnTop)
{
    if (!alwaysOnTop) {
        // Ordinary, fully managed window: normal level, no Space overrides, so it
        // behaves like any other window and can be covered.
        return Policy{Level::Normal, false, false};
    }
    // Overlay level plus the two Space behaviors that let the pet follow the user
    // into every Space and draw over full-screen apps.
    return Policy{Level::Overlay, true, true};
}
}
