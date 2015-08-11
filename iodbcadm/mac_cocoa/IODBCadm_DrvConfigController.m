
//??#include <iODBC/sql.h>
//??#include <iODBC/sqlext.h>
#import "IODBCadm_DrvConfigController.h"
#import "utils.h"
#import "Helpers.h"




@interface IODBCadm_DrvConfigController ()

@end

@implementation IODBCadm_DrvConfigController
@synthesize Attrs_ArrController;
@synthesize Attrs_list=_Attrs_list;
@synthesize rb_sysuser = _rb_sysuser;
@synthesize drv_name = _drv_name;
@synthesize drv_file = _drv_file;
@synthesize setup_file = _setup_file;
@synthesize add = _add;
@synthesize user = _user;

- (id)initWithAttrs:(const wchar_t*)attrs
{
    self = [super initWithWindowNibName:@"IODBCadm_DrvConfigController"];
    if (self) {
        _Attrs_list = [[NSMutableArray alloc] initWithCapacity:16];
        [self parse_attrs:attrs];
    }
    return self;
}

- (void)dealloc
{
    [_Attrs_list removeAllObjects];
    [_Attrs_list release];
    [_drv_name release];
    [_drv_file release];
    [_setup_file release];
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

- (void)parse_attrs:(const wchar_t *)attrs
{
    wchar_t *curr, *cour;
    
    [_Attrs_list removeAllObjects];
    
    for (curr = (LPWSTR) attrs; *curr != L'\0' ; curr += (wcslen (curr) + 1))
    {
        if (!wcsncasecmp (curr, L"Driver=", wcslen (L"Driver=")))
        {
            _drv_file = conv_wchar_to_NSString(curr + wcslen(L"Driver="));
            continue;
        }
        
        if (!wcsncasecmp (curr, L"Setup=", wcslen(L"Setup=")))
        {
            _setup_file = conv_wchar_to_NSString(curr + wcslen(L"Setup="));
            continue;
        }
        
        if ((cour = wcschr (curr, L'=')))
        {
            NSString *key, *val;
            *cour = '\0';
            key = conv_wchar_to_NSString(curr);
            *cour = '=';
            val = conv_wchar_to_NSString(cour+1);
            [_Attrs_list addObject:[NSMutableDictionary dictionaryWithObjectsAndKeys:key!=nil?key:@"", @"key",
                                    val!=nil?val:@"", @"val", nil]];
        }
        else
            [_Attrs_list addObject:[NSMutableDictionary dictionaryWithObjectsAndKeys:@"", @"key", @"", @"val", nil]];
    }
}

- (void)windowDidLoad
{
    [super windowDidLoad];
    
    _dialogCode = 0;
    [_rb_sysuser setEnabled:_add];
    [_rb_sysuser selectCellAtRow:0 column:(_user?0:1)];
    
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

- (IBAction)call_DrvFile_Browse:(id)sender {
    NSOpenPanel *panel = [NSOpenPanel openPanel];
    
    NSURL *file_url = [NSURL fileURLWithPath:_drv_file];
    NSString *fpath = file_url.path;
    NSString *fname = file_url.lastPathComponent;
    NSString *dir = [fpath substringToIndex:(fpath.length - fname.length)];
    
    
    [panel setTitle:@"Choose a file ..."];
    [panel setNameFieldStringValue:(fname.length>0)? fname: @""];
    if (dir.length>0)
        [panel setDirectoryURL:[NSURL fileURLWithPath:dir isDirectory:TRUE]];
    panel.allowsMultipleSelection = FALSE;
    panel.canChooseDirectories = FALSE;
    
    NSInteger rc = [panel runModal];
    if (rc==NSFileHandlingPanelOKButton)
        self.drv_file = ((NSURL*)[panel.URLs objectAtIndex:0]).path;
}

- (IBAction)call_SetupFile_Browse:(id)sender {
    NSOpenPanel *panel = [NSOpenPanel openPanel];
    
    NSURL *file_url = [NSURL fileURLWithPath:_setup_file];
    NSString *fpath = file_url.path;
    NSString *fname = file_url.lastPathComponent;
    NSString *dir = [fpath substringToIndex:(fpath.length - fname.length)];
    
    
    [panel setTitle:@"Choose a file ..."];
    [panel setNameFieldStringValue:(fname.length>0)? fname: @""];
    if (dir.length>0)
        [panel setDirectoryURL:[NSURL fileURLWithPath:dir isDirectory:TRUE]];
    panel.allowsMultipleSelection = FALSE;
    panel.canChooseDirectories = FALSE;
    
    NSInteger rc = [panel runModal];
    if (rc==NSFileHandlingPanelOKButton)
        self.setup_file = ((NSURL*)[panel.URLs objectAtIndex:0]).path;
}


@end
