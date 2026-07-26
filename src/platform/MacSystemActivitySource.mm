#include "platform/MacSystemActivitySource.h"

#import <AppKit/AppKit.h>

MacSystemActivitySource::MacSystemActivitySource(QObject *parent)
    : SystemActivitySource(parent)
{
    NSNotificationCenter *center = [[NSWorkspace sharedWorkspace] notificationCenter];
    id sleepObserver = [center addObserverForName:NSWorkspaceWillSleepNotification
                                           object:nil
                                            queue:[NSOperationQueue mainQueue]
                                       usingBlock:^(NSNotification *) { emit willSleep(); }];
    id wakeObserver = [center addObserverForName:NSWorkspaceDidWakeNotification
                                          object:nil
                                           queue:[NSOperationQueue mainQueue]
                                      usingBlock:^(NSNotification *) { emit didWake(); }];
    // Same centre as the sleep pair above, so no second teardown path: distributed
    // notifications (screen lock, screen saver) live on a different centre and would
    // dangle if removed from this one, which is why they are not observed here.
    id screensSleepObserver = [center addObserverForName:NSWorkspaceScreensDidSleepNotification
                                                  object:nil
                                                   queue:[NSOperationQueue mainQueue]
                                              usingBlock:^(NSNotification *) { emit screensDidSleep(); }];
    id screensWakeObserver = [center addObserverForName:NSWorkspaceScreensDidWakeNotification
                                                 object:nil
                                                  queue:[NSOperationQueue mainQueue]
                                             usingBlock:^(NSNotification *) { emit screensDidWake(); }];
    id accessibilityObserver = [center addObserverForName:NSWorkspaceAccessibilityDisplayOptionsDidChangeNotification
                                                    object:nil
                                                     queue:[NSOperationQueue mainQueue]
                                                usingBlock:^(NSNotification *) {
                                                    emit reduceMotionChanged(systemReduceMotion());
                                                }];
    m_sleepObserver = (__bridge void *)sleepObserver;
    m_wakeObserver = (__bridge void *)wakeObserver;
    m_screensSleepObserver = (__bridge void *)screensSleepObserver;
    m_screensWakeObserver = (__bridge void *)screensWakeObserver;
    m_accessibilityObserver = (__bridge void *)accessibilityObserver;
}

MacSystemActivitySource::~MacSystemActivitySource()
{
    NSNotificationCenter *center = [[NSWorkspace sharedWorkspace] notificationCenter];
    if (m_sleepObserver) [center removeObserver:(__bridge id)m_sleepObserver];
    if (m_wakeObserver) [center removeObserver:(__bridge id)m_wakeObserver];
    if (m_screensSleepObserver) [center removeObserver:(__bridge id)m_screensSleepObserver];
    if (m_screensWakeObserver) [center removeObserver:(__bridge id)m_screensWakeObserver];
    if (m_accessibilityObserver) [center removeObserver:(__bridge id)m_accessibilityObserver];
}

bool MacSystemActivitySource::systemReduceMotion() const
{
    return [[NSWorkspace sharedWorkspace] accessibilityDisplayShouldReduceMotion];
}
