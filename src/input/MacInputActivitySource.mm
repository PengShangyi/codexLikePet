#include "input/MacInputActivitySource.h"

#include <QMetaObject>

#import <ApplicationServices/ApplicationServices.h>

MacInputActivitySource::MacInputActivitySource(QObject *parent)
    : InputActivitySource(parent)
{
}

MacInputActivitySource::~MacInputActivitySource()
{
    stop();
}

InputStartResult MacInputActivitySource::start()
{
    if (isActive()) return InputStartResult::Started;
    if (!CGPreflightListenEventAccess() && !CGRequestListenEventAccess()) {
        return InputStartResult::PermissionDenied;
    }

    const CGEventMask mask = CGEventMaskBit(kCGEventKeyDown);
    m_eventTap = CGEventTapCreate(kCGHIDEventTap,
                                  kCGHeadInsertEventTap,
                                  kCGEventTapOptionListenOnly,
                                  mask,
                                  reinterpret_cast<CGEventTapCallBack>(&MacInputActivitySource::eventCallback),
                                  this);
    if (!m_eventTap) return InputStartResult::Failed;

    m_runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault,
                                                    reinterpret_cast<CFMachPortRef>(m_eventTap),
                                                    0);
    if (!m_runLoopSource) {
        stop();
        return InputStartResult::Failed;
    }
    CFRunLoopAddSource(CFRunLoopGetMain(),
                       reinterpret_cast<CFRunLoopSourceRef>(m_runLoopSource),
                       kCFRunLoopCommonModes);
    CGEventTapEnable(reinterpret_cast<CFMachPortRef>(m_eventTap), true);
    return InputStartResult::Started;
}

void MacInputActivitySource::stop()
{
    if (m_runLoopSource) {
        CFRunLoopRemoveSource(CFRunLoopGetMain(),
                              reinterpret_cast<CFRunLoopSourceRef>(m_runLoopSource),
                              kCFRunLoopCommonModes);
        CFRelease(reinterpret_cast<CFRunLoopSourceRef>(m_runLoopSource));
        m_runLoopSource = nullptr;
    }
    if (m_eventTap) {
        CGEventTapEnable(reinterpret_cast<CFMachPortRef>(m_eventTap), false);
        CFRelease(reinterpret_cast<CFMachPortRef>(m_eventTap));
        m_eventTap = nullptr;
    }
}

bool MacInputActivitySource::isActive() const
{
    return m_eventTap && CGEventTapIsEnabled(reinterpret_cast<CFMachPortRef>(m_eventTap));
}

void *MacInputActivitySource::eventCallback(void *, unsigned int typeValue, void *event, void *context)
{
    auto *source = static_cast<MacInputActivitySource *>(context);
    const CGEventType type = static_cast<CGEventType>(typeValue);
    if (type == kCGEventTapDisabledByTimeout || type == kCGEventTapDisabledByUserInput) {
        if (source->m_eventTap) {
            CGEventTapEnable(reinterpret_cast<CFMachPortRef>(source->m_eventTap), true);
        }
        return event;
    }
    if (type == kCGEventKeyDown) {
        // Do not call CGEventGetIntegerValueField or otherwise inspect the event.
        QMetaObject::invokeMethod(source, [source] { emit source->activityDetected(); }, Qt::QueuedConnection);
    }
    return event;
}
