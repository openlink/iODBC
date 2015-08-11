
#import <Cocoa/Cocoa.h>
#import <gui.h>


@interface IODBCadm_DrvRemoveController : NSWindowController {

    int _dialogCode;
    NSButton *_chk_User;
    NSButton *_chk_System;
    NSMatrix *_rb_remove;
    
    BOOL _deletedsn;
    int  _dsns;
}

@property (assign) IBOutlet NSButton *chk_User;
@property (assign) IBOutlet NSButton *chk_System;
@property (assign) IBOutlet NSMatrix *rb_remove;

@property (assign) BOOL deletedsn;
@property (assign) int dsns;

- (id)initWithDSNS:(int) dsns;

- (IBAction)call_Cancel:(id)sender;
- (IBAction)call_Ok:(id)sender;
@end
