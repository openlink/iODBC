
#import "IODBCadm_PoolConfigController.h"
#import "utils.h"
#import "Helpers.h"




@interface IODBCadm_PoolConfigController ()

@end

@implementation IODBCadm_PoolConfigController
@synthesize g_box = _g_box;
@synthesize ptitle = _ptitle;
@synthesize ptimeout = _ptimeout;
@synthesize pquery = _pquery;

- (id)initWithTitle:(NSString *) title Timeout:(NSString *) timeout Query:(NSString *) query
{
    self = [super initWithWindowNibName:@"IODBCadm_PoolConfigController"];
    if (self) {
        self.ptitle = title;
        self.ptimeout = (timeout==nil || timeout.length==0) ? nil : timeout;
        self.pquery = query;
    }
    return self;
}

- (void)dealloc
{
    [_ptitle release];
    [_ptimeout release];
    [_pquery release];
	[super dealloc];
}

- (id)initWithWindow:(NSWindow *)window
{
    self = [super initWithWindow:window];
    if (self) {
        // Initialization code here.
    }
    return self;
}


- (void)windowDidLoad
{
    [super windowDidLoad];
    
    _dialogCode = 0;
    
    [[self window] center];  // Center the window.
}


- (void)windowWillClose:(NSNotification*)notification
{
    [NSApp stopModalWithCode:_dialogCode];
}


- (IBAction)call_Ok:(id)sender {
    _dialogCode = 1;
    [self.window close];
}

- (IBAction)call_Cancel:(id)sender {
    _dialogCode = 0;
    [self.window close];
}

@end
