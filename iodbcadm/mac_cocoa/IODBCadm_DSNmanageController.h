//
//  IODBCadm_DSNmanageController.h
//  TestLog
//
//  Created by sergei on 11.07.14.
//  Copyright (c) 2014 sergei. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <gui.h>


@interface IODBCadm_DSNmanageController : NSWindowController<NSTabViewDelegate, NSTableViewDelegate> {
    NSArrayController *_UserDSN_ArrController;
    NSArrayController *_Drv_ArrController;
    NSArrayController *_Pool_ArrController;
    NSArrayController *_About_ArrController;
    NSArrayController *_SysDSN_ArrController;
    NSArrayController *_FileDSN_ArrController;
    NSTabView *_tab_view;
    NSTableView *_fdsn_tableView;
    NSPopUpButton *_popup_dir_btn;

    int _dialogCode;
    BOOL _tracing_changed;
    BOOL _pool_changed;
    
    NSMutableArray *_FileDSN_list;
    NSString *_cur_dir;
    NSTextField *_fld_RetryWaitTime;
    NSTableView *_pool_tableView;
    NSMatrix *_rb_PerfMon;
    NSTextField *_fld_LogFilePath;
    NSTextField *_fld_CustomTrace;
    NSMatrix *_rb_WhenToTrace;
    NSMatrix *_rb_TraceWide;
}

@property (assign) IBOutlet NSPopUpButton *popup_dir_btn;
@property (assign) IBOutlet NSTableView *fdsn_tableView;
@property (assign) IBOutlet NSTabView *tab_view;
@property (assign) IBOutlet NSArrayController *FileDSN_ArrController;
@property (assign) IBOutlet NSArrayController *SysDSN_ArrController;
@property (assign) IBOutlet NSArrayController *UserDSN_ArrController;
@property (assign) IBOutlet NSArrayController *Drv_ArrController;
@property (assign) IBOutlet NSArrayController *Pool_ArrController;
@property (assign) IBOutlet NSArrayController *About_ArrController;

@property (nonatomic, retain) NSMutableArray *FileDSN_list;
@property (nonatomic, retain) NSString *cur_dir;


- (IBAction)call_UserDSN_Add:(id)sender;
- (IBAction)call_UserDSN_Remove:(id)sender;
- (IBAction)call_UserDSN_Config:(id)sender;
- (IBAction)call_UserDSN_Test:(id)sender;

- (IBAction)call_SysDSN_Add:(id)sender;
- (IBAction)call_SysDSN_Remove:(id)sender;
- (IBAction)call_SysDSN_Config:(id)sender;
- (IBAction)call_SysDSN_Test:(id)sender;

- (IBAction)call_Dir_PopupBtn:(id)sender;
- (IBAction)call_FileDSN_Add:(id)sender;
- (IBAction)call_FileDSN_Remove:(id)sender;
- (IBAction)call_FileDSN_Config:(id)sender;
- (IBAction)call_FileDSN_Test:(id)sender;
- (IBAction)call_FileDSN_SetDir:(id)sender;


- (IBAction)call_Drv_Add:(id)sender;
- (IBAction)call_Drv_Remove:(id)sender;
- (IBAction)call_Drv_Config:(id)sender;


@property (assign) IBOutlet NSTextField *fld_RetryWaitTime;
@property (assign) IBOutlet NSTableView *pool_tableView;
@property (assign) IBOutlet NSMatrix *rb_PerfMon;


@property (assign) IBOutlet NSMatrix *rb_WhenToTrace;
@property (assign) IBOutlet NSMatrix *rb_TraceWide;
- (IBAction)call_Trace_Apply:(id)sender;
@property (assign) IBOutlet NSTextField *fld_LogFilePath;
- (IBAction)call_LogFilePath_Browse:(id)sender;
@property (assign) IBOutlet NSTextField *fld_CustomTrace;
- (IBAction)call_CustomTrace_Browse:(id)sender;



- (IBAction)call_Cancel:(id)sender;
- (IBAction)call_Ok:(id)sender;

@end
