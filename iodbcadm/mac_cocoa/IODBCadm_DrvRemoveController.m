
#import "IODBCadm_DrvRemoveController.h"
#import "utils.h"
#import "Helpers.h"




@interface IODBCadm_DrvRemoveController ()

@end

@implementation IODBCadm_DrvRemoveController
@synthesize chk_User = _chk_User;
@synthesize chk_System = _chk_System;
@synthesize rb_remove = _rb_remove;
@synthesize deletedsn = _deletedsn;
@synthesize dsns = _dsns;

- (id)initWithDSNS:(int) dsns
{
    self = [super initWithWindowNibName:@"IODBCadm_DrvRemoveController"];
    if (self) {
        _deletedsn = FALSE;
        _dsns = dsns;
    }
    return self;
}

- (void)dealloc
{
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
    if (_deletedsn)
        [_rb_remove selectCellAtRow:0 column:1];
    else
        [_rb_remove selectCellAtRow:0 column:0];
    
    [_chk_User setState: ((_dsns & 1)!=0) ? NSOnState : NSOffState];
    [_chk_System setState: ((_dsns & 2)!=0) ? NSOnState : NSOffState];
    
    [[self window] center];  // Center the window.
}


- (void)windowWillClose:(NSNotification*)notification
{
    [NSApp stopModalWithCode:_dialogCode];
}


- (IBAction)call_Ok:(id)sender {
    _dialogCode = 1;
    _deletedsn = _rb_remove.selectedColumn==1;
    _dsns = 0;
    _dsns |= _chk_User.state==NSOnState?1:0;
    _dsns |= _chk_System.state==NSOnState?2:0;
    [self.window close];
}

- (IBAction)call_Cancel:(id)sender {
    _dialogCode = 0;
    [self.window close];
}

@end
