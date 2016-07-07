/*
 *  IODBCadm_DSNmanageController.h
 *
 *  $Id$
 *
 *  The iODBC driver manager.
 *
 *  Copyright (C) 1996-2016 by OpenLink Software <iodbc@openlinksw.com>
 *  All Rights Reserved.
 *
 *  This software is released under the terms of either of the following
 *  licenses:
 *
 *      - GNU Library General Public License (see LICENSE.LGPL)
 *      - The BSD License (see LICENSE.BSD).
 *
 *  Note that the only valid version of the LGPL license as far as this
 *  project is concerned is the original GNU Library General Public License
 *  Version 2, dated June 1991.
 *
 *  While not mandated by the BSD license, any patches you make to the
 *  iODBC source code may be contributed back into the iODBC project
 *  at your discretion. Contributions will benefit the Open Source and
 *  Data Access community as a whole. Submissions may be made at:
 *
 *      http://www.iodbc.org
 *
 *
 *  GNU Library Generic Public License Version 2
 *  ============================================
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; only
 *  Version 2 of the License dated June 1991.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 *  The BSD License
 *  ===============
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *  3. Neither the name of OpenLink Software Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL OPENLINK OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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
    BOOL _drivers_loaded;
    
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

- (IBAction)call_FileDSN_Dir_Browse:(id)sender;
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
