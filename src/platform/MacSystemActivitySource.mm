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
    id accessibilityObserver = [center addObserverForName:NSWorkspaceAccessibilityDisplayOptionsDidChangeNotification
                                                    object:nil
                                                     queue:[NSOperationQueue mainQueue]
                                                usingBlock:^(NSNotification *) {
                                                    emit reduceMotionChanged(systemReduceMotion());
                                                }];
    m_sleepObserver = (__bridge void *)sleepObserver;
    m_wakeObserver = (__bridge void *)wakeObserver;
    m_accessibilityObserver = (__bridge void *)accessibilityObserver;
}

MacSystemActivitySource::~MacSystemActivitySource()
{
    NSNotificationCenter *center = [[NSWorkspace sharedWorkspace] notificationCenter];
    if (m_sleepObserver) [center removeObserver:(__bridge id)m_sleepObserver];
    if (m_wakeObserver) [center removeObserver:(__bridge id)m_wakeObserver];
    if (m_accessibilityObserver) [center removeObserver:(__bridge id)m_accessibilityObserver];
}

bool MacSystemActivitySource::systemReduceMotion() const
{
    return [[NSWorkspace sharedWorkspace] accessibilityDisplayShouldReduceMotion];
}
