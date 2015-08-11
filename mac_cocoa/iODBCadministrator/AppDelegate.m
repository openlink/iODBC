//
//  AppDelegate.m
//  Test
//
//  Created by sergei on 09/08/15.
//  Copyright (c) 2015 openlink. All rights reserved.
//

#import "AppDelegate.h"

@interface AppDelegate ()

//@property (weak) IBOutlet NSWindow *window;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
  BOOL manage_return = false;

    // Insert code here to initialize your application
//??    show_DSNmanage();
  manage_return = SQLManageDataSources (-1L);

}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
    // Insert code here to tear down your application
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
    return YES;
}

@end
