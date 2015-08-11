
#import <Cocoa/Cocoa.h>
#import <gui.h>


@interface IODBCadm_PoolConfigController : NSWindowController {

    int _dialogCode;
    NSBox *_g_box;
    
    NSString *_ptitle;
    NSString *_ptimeout;
    NSString *_pquery;
}

@property (assign) IBOutlet NSBox *g_box;

@property (nonatomic, retain) NSString *ptitle;
@property (nonatomic, retain) NSString *ptimeout;
@property (nonatomic, retain) NSString *pquery;

- (id)initWithTitle:(NSString *) title Timeout:(NSString *) timeout Query:(NSString *) query;

- (IBAction)call_Cancel:(id)sender;
- (IBAction)call_Ok:(id)sender;
@end
