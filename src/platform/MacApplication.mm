#include "platform/MacApplication.h"

#import <AppKit/AppKit.h>

namespace MacApplication {
void setAccessoryActivationPolicy()
{
    [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];
}

void activateIgnoringOtherApps()
{
    [NSApp activateIgnoringOtherApps:YES];
}
}
