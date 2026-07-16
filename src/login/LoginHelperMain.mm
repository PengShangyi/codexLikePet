#import <AppKit/AppKit.h>

int main(int argc, const char *argv[])
{
    @autoreleasepool {
        NSArray<NSRunningApplication *> *running =
            [NSRunningApplication runningApplicationsWithBundleIdentifier:@"com.peng.potato"];
        if (running.count == 0) {
            NSURL *helperURL = [NSBundle mainBundle].bundleURL;
            NSURL *mainAppURL = helperURL;
            for (NSInteger level = 0; level < 4; ++level) {
                mainAppURL = [mainAppURL URLByDeletingLastPathComponent];
            }
            NSWorkspaceOpenConfiguration *configuration =
                [NSWorkspaceOpenConfiguration configuration];
            configuration.activates = NO;
            [[NSWorkspace sharedWorkspace] openApplicationAtURL:mainAppURL
                                                   configuration:configuration
                                               completionHandler:nil];
        }
    }
    return 0;
}
