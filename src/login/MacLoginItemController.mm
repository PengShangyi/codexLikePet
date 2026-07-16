#include "login/MacLoginItemController.h"

#import <Foundation/Foundation.h>
#import <ServiceManagement/ServiceManagement.h>

namespace {
QString errorText(NSError *error)
{
    if (!error) return QStringLiteral("Unknown Service Management error");
    return QString::fromNSString(error.localizedDescription);
}
}

bool MacLoginItemController::setEnabled(bool enabled, QString *error)
{
    if (@available(macOS 13.0, *)) {
        SMAppService *service = [SMAppService mainAppService];
        NSError *serviceError = nil;
        const BOOL ok = enabled ? [service registerAndReturnError:&serviceError]
                                : [service unregisterAndReturnError:&serviceError];
        if (!ok) {
            // Disabling an already-unregistered item is an idempotent success.
            if (!enabled && service.status == SMAppServiceStatusNotRegistered) return true;
            *error = errorText(serviceError);
            return false;
        }
        return true;
    }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    const Boolean ok = SMLoginItemSetEnabled(CFSTR("com.peng.potato.login-helper"), enabled);
#pragma clang diagnostic pop
    if (!ok) {
        *error = QStringLiteral("Unable to update the macOS 12 login helper");
        return false;
    }
    return true;
}
