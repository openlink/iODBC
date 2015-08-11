
#import <Cocoa/Cocoa.h>
#import <gui.h>


@interface IODBCadm_DrvConfigController : NSWindowController {
    NSArrayController *Attrs_ArrController;

    int _dialogCode;
    NSMutableArray *_Attrs_list;
    NSMatrix *_rb_sysuser;
    NSString *_drv_name;
    NSString *_drv_file;
    NSString *_setup_file;
    BOOL _add;
    BOOL _user;
}

@property (assign) IBOutlet NSArrayController *Attrs_ArrController;

@property (nonatomic, retain) NSMutableArray *Attrs_list;
@property (assign) IBOutlet NSMatrix *rb_sysuser;

@property (nonatomic, retain) NSString *drv_name;
@property (nonatomic, retain) NSString *drv_file;
@property (nonatomic, retain) NSString *setup_file;
@property (assign) BOOL add;
@property (assign) BOOL user;


- (id)initWithAttrs:(const wchar_t*)attrs;

- (IBAction)call_DrvFile_Browse:(id)sender;
- (IBAction)call_SetupFile_Browse:(id)sender;

- (IBAction)call_Cancel:(id)sender;
- (IBAction)call_Ok:(id)sender;
@end
