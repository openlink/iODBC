/*
 *  IODBCadm_DSNmanageController.m
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


#import "IODBCadm_DSNmanageController.h"
#import "utils.h"
#import "Helpers.h"
#import "IODBCadm_DriverChooseDSNController.h"
#import "IODBCadm_PoolConfigController.h"
#import "IODBCadm_DrvRemoveController.h"
#import "IODBCadm_DrvConfigController.h"



void create_administrator (HWND hwnd)
{
    @autoreleasepool {
        
        NSApplication *app = [NSApplication sharedApplication];
        
        IODBCadm_DSNmanageController *dlg = [[IODBCadm_DSNmanageController alloc] init];
        
        [app runModalForWindow:dlg.window];

        [dlg.window orderOut:dlg.window];
        [dlg release];
    }
}


static LPWSTR create_driversetupw (LPCWSTR driver, LPCWSTR attrs, BOOL add, BOOL user)
{
    wchar_t *connstr = NULL;

    IODBCadm_DrvConfigController *dlg = [[IODBCadm_DrvConfigController alloc] initWithAttrs:attrs];
    dlg.drv_name = conv_wchar_to_NSString(driver);
    dlg.add = add;
    
    NSInteger rc = [NSApp runModalForWindow:dlg.window];
    if (rc == 1) {
        int i = 0, size = 0;
        wchar_t *cour, *prov;
        int STRCONN_NB_TOKENS = 3;
        
        size += (dlg.drv_name!=nil?dlg.drv_name.length:0) + 1;
        size += (dlg.drv_file!=nil?dlg.drv_file.length:0) + wcslen(L"Driver=") +1;
        size += (dlg.setup_file!=nil?dlg.setup_file.length:0) + wcslen(L"Setup=") +1;
        
        size += wcslen(dlg.rb_sysuser.selectedColumn==0?L"USR" : L"SYS") + 1;
        /* Malloc it */
        if ((connstr = (wchar_t *) malloc (++size * sizeof(wchar_t))))
        {
            wcscpy(connstr, (dlg.rb_sysuser.selectedColumn==0? L"USR" : L"SYS"));
            
            for (cour = connstr + 4; i < STRCONN_NB_TOKENS ; i++)
            {
                switch (i)
                {
                    case 0:
                        prov = conv_NSString_to_wchar(dlg.drv_name);
                        if(prov)
                        {
                            wcscpy(cour, prov);
                            free(prov);
                        }
                        break;
                    case 1:
                        prov = conv_NSString_to_wchar(dlg.drv_file);
                        if(prov)
                        {
                            wcscpy(cour, L"Driver=");
                            wcscat(cour, prov);
                            free(prov);
                        }
                        break;
                    case 2:
                        prov = conv_NSString_to_wchar(dlg.setup_file);
                        if(prov)
                        {
                            wcscpy(cour, L"Setup=");
                            wcscat(cour, prov);
                            free(prov);
                        }
                        break;
                };
                
                cour += (wcslen (cour) + 1);
            }
            
            for (i = 0; i < dlg.Attrs_list.count; i++)
            {
                wchar_t *val,*key;
                NSDictionary *row = [dlg.Attrs_list objectAtIndex:i];
                NSString *nskey = (NSString*)[row valueForKey:@"key"];
                if ([nskey isEqualToString:@"..."] || nskey.length==0)
                    continue;
                key = conv_NSString_to_wchar(nskey);
                val = conv_NSString_to_wchar((NSString*)[row valueForKey:@"val"]);
                cour = connstr;
                connstr = (wchar_t*) malloc ((size + wcslen(key) + wcslen(val) + 2) * sizeof(wchar_t));
                if (connstr)
                {
                    memcpy (connstr, cour, size*sizeof(wchar_t));
                    if (key) {
                        wcscpy(connstr + size - 1, key);
                        wcscat(connstr + size - 1, L"=");
                        if (val)
                            wcscat(connstr + size - 1, val);
                    }
                    free (cour);
                    size += wcslen(key) + wcslen(val) + 2;
                }
                else
                    connstr = cour;
                
                if (key!=NULL) free(key);
                if (val!=NULL) free(val);
            }
            
            connstr[size - 1] = '\0';
        }
    }
    
    [dlg.window orderOut:dlg.window];
    [dlg release];
    
    return connstr;
}



@interface IODBCadm_DSNmanageController ()

@end

@implementation IODBCadm_DSNmanageController
@synthesize fld_CustomTrace = _fld_CustomTrace;
@synthesize fld_LogFilePath = _fld_LogFilePath;
@synthesize rb_WhenToTrace = _rb_WhenToTrace;
@synthesize rb_TraceWide = _rb_TraceWide;
@synthesize fld_RetryWaitTime = _fld_RetryWaitTime;
@synthesize pool_tableView = _pool_tableView;
@synthesize rb_PerfMon = _rb_PerfMon;
@synthesize popup_dir_btn = _popup_dir_btn;
@synthesize fdsn_tableView = _fdsn_tableView;
@synthesize tab_view = _tab_view;
@synthesize FileDSN_ArrController = _FileDSN_ArrController;
@synthesize SysDSN_ArrController = _SysDSN_ArrController;
@synthesize UserDSN_ArrController = _UserDSN_ArrController;
@synthesize Drv_ArrController = _Drv_ArrController;
@synthesize Pool_ArrController = _Pool_ArrController;
@synthesize About_ArrController = _About_ArrController;
@synthesize FileDSN_list=_FileDSN_list;
@synthesize cur_dir=_cur_dir;

- (id)init
{
    char tmp[1024] = {""};
    
    self = [super initWithWindowNibName:@"IODBCadm_DSNmanageController"];
    if (self) {
        self.FileDSN_list = [NSMutableArray arrayWithCapacity:16];
        
        SQLSetConfigMode (ODBC_BOTH_DSN);
        if (!SQLGetPrivateProfileString("ODBC", "FileDSNPath", "", tmp, sizeof(tmp), "odbcinst.ini"))
          self.cur_dir = get_user_documents_dir();
        else
          self.cur_dir = conv_char_to_NSString(tmp);
    }
    _tracing_changed = FALSE;
    _pool_changed = FALSE;
    _drivers_loaded = FALSE;
    return self;
}

- (void)dealloc
{
    [_FileDSN_list release];
    [_cur_dir release];
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
    void *ptr;
    [super windowDidLoad];
    _dialogCode = 0;
    
    [[self window] center];  // Center the window.
    self.window.title = (sizeof(ptr)==8) ? @"iODBC Data Source Administrator  (64-Bit Edition)" : @"iODBC Data Source Administrator";
    addDSNs_to_list(FALSE, _UserDSN_ArrController);
    addDSNs_to_list(TRUE, _SysDSN_ArrController);
    [_fdsn_tableView setDoubleAction:@selector(call_FDSN_DoubleClick)];
    addPools_to_list(_Pool_ArrController);
    [_pool_tableView setDoubleAction:@selector(call_Pool_DoubleClick)];
    addComponents_to_list(_About_ArrController);
}

- (void)windowWillClose:(NSNotification*)notification
{
    [NSApp stopModalWithCode:_dialogCode];
}

- (void) call_FDSN_DoubleClick
{
    NSInteger row = _fdsn_tableView.clickedRow;
    NSDictionary *dict = [_FileDSN_list objectAtIndex:row];
    NSNumber *isdir = [dict valueForKey:@"isdir"];
    if (isdir.boolValue==TRUE){
        NSString *cliked_dir = [dict valueForKey:@"name"];
        NSString *new_path = [NSString stringWithFormat:@"%@/%@", _cur_dir, cliked_dir];
        self.cur_dir = new_path;
        wchar_t *path = conv_NSString_to_wchar(_cur_dir);
        addFDSNs_to_list(_cur_dir, FALSE, _FileDSN_ArrController);
        fill_dir_menu(path, _popup_dir_btn);
        if (path) free(path);
    } else {
        [self call_FileDSN_Config:self];
    }
}


- (void) call_Pool_DoubleClick
{
    NSArray *sarr = [_Pool_ArrController selectedObjects];
    if (sarr.count>0) {
        NSDictionary *dct = [sarr objectAtIndex:0];
        NSString *drv = [dct valueForKey:@"drv"];
        NSString *tm = [dct valueForKey:@"timeout"];
        NSString *qry = [dct valueForKey:@"query"];
        
        IODBCadm_PoolConfigController *dlg = [[IODBCadm_PoolConfigController alloc] initWithTitle:drv Timeout:tm Query:qry];
        
        NSInteger rc = [NSApp runModalForWindow:dlg.window];
        if (rc == 1) {
            UWORD configMode;
            tm = [NSString stringWithFormat:@"CPTimeout=%@", dlg.ptimeout ? dlg.ptimeout :@""];
            qry = [NSString stringWithFormat:@"CPProbe=%@", dlg.pquery ? dlg.pquery : @""];
            
            wchar_t *wtimeout = conv_NSString_to_wchar(tm);
            wchar_t *wquery   = conv_NSString_to_wchar(qry);
            wchar_t *wdrv     = conv_NSString_to_wchar(drv);
            
            SQLGetConfigMode(&configMode);
            SQLSetConfigMode(ODBC_SYSTEM_DSN);
            if (wdrv) {
                if (wtimeout) {
                    SQLSetConfigMode(ODBC_SYSTEM_DSN);
                    if (!SQLConfigDriverW ((void*)1L, ODBC_CONFIG_DRIVER,
                                           wdrv, wtimeout, NULL, 0, NULL))
                        _iodbcdm_errorboxw ((void*)1L, wdrv,
                                            L"An error occurred when trying to set the connection pooling time-out ");
                    free(wtimeout);
                }
                if (wquery) {
                    SQLSetConfigMode(ODBC_SYSTEM_DSN);
                    if (!SQLConfigDriverW ((void*)1L, ODBC_CONFIG_DRIVER,
                                           wdrv, wquery, NULL, 0, NULL))
                        _iodbcdm_errorboxw ((void*)1L, wdrv,
                                            L"An error occurred when trying to set the connection probe query ");
                    
                }
                free(wdrv);
            }
            
            addPools_to_list(_Pool_ArrController);
        }
        
        [dlg.window orderOut:dlg.window];
        [dlg release];
        [self.window makeKeyAndOrderFront:self.window];
    }
}


/** NSTabViewDelegate **/
- (void)tabView:(NSTabView *)tabView didSelectTabViewItem:(NSTabViewItem *)tabViewItem
{
    char tmp[4096];
    NSString *identifier = [tabViewItem identifier];

    if ([identifier isEqualToString:@"userdsn"]){
        addDSNs_to_list(FALSE, _UserDSN_ArrController);
    }
    else if ([identifier isEqualToString:@"sysdsn"]){
        addDSNs_to_list(TRUE, _SysDSN_ArrController);
    }
    else if ([identifier isEqualToString:@"filedsn"]){
        wchar_t *cur_path = conv_NSString_to_wchar(_cur_dir);
        addFDSNs_to_list(_cur_dir, FALSE, _FileDSN_ArrController);
        fill_dir_menu(cur_path, _popup_dir_btn);
        if (cur_path) free(cur_path);
    }
    else if ([identifier isEqualToString:@"drivers"]){
        if (!_drivers_loaded) {
            addDrivers_to_list(_Drv_ArrController);
            _drivers_loaded = TRUE;
        }
    }
    else if ([identifier isEqualToString:@"pool"]){
        if (!_pool_changed)
        {
            BOOL perfmon = FALSE;
            /* Get the connection pooling options */
            SQLGetPrivateProfileString ("ODBC Connection Pooling", "Perfmon",
                                        "", (char *)tmp, sizeof (tmp), "odbcinst.ini");
            if (!strcasecmp (tmp, "1") || !strcasecmp (tmp, "On"))
                perfmon = TRUE;
            SQLGetPrivateProfileString ("ODBC Connection Pooling", "Retry Wait",
                                        "", tmp, sizeof (tmp), "odbcinst.ini");
            if (perfmon)
                [_rb_PerfMon selectCellAtRow:0 column:0];
            else
                [_rb_PerfMon selectCellAtRow:1 column:0];
            
            _fld_RetryWaitTime.stringValue = conv_char_to_NSString(tmp);
            
            _pool_changed = TRUE;
        }
        addPools_to_list(_Pool_ArrController);
    }
    else if ([identifier isEqualToString:@"trace"]){
        wchar_t tokenstr[4096] = { L'\0' }, tokenstr1[4096] = { L'\0' };
        BOOL trace = FALSE;
        BOOL traceauto = FALSE;

        if (!_tracing_changed)
        {
            int mode = ODBC_SYSTEM_DSN;
            
            /* Get the traces options */
            SQLSetConfigMode (mode);
            SQLGetPrivateProfileStringW (L"ODBC", L"TraceFile", L"", tokenstr,
                                         sizeof (tokenstr) / sizeof(wchar_t), NULL);
            if (tokenstr[0] != L'\0')
            {
                /* All users wide */
                [_rb_TraceWide selectCellAtRow:1 column:0];
            }
            else
            {
                /* Only for current user */
                mode = ODBC_USER_DSN;
                SQLSetConfigMode (mode);
                [_rb_TraceWide selectCellAtRow:0 column:0];
            }
            
            SQLSetConfigMode (mode);
            SQLGetPrivateProfileString ("ODBC", "Trace", "", tmp, sizeof (tmp), NULL);
            if (!strcasecmp (tmp, "1") || !strcasecmp (tmp, "On"))
                trace = TRUE;
            
            SQLSetConfigMode (mode);
            SQLGetPrivateProfileString ("ODBC", "TraceAutoStop", "", (char*)tokenstr,
                                        sizeof (tokenstr), NULL);
            if (!strcasecmp ((char*)tokenstr, "1") || !strcasecmp ((char*)tokenstr, "On"))
                traceauto = TRUE;
            
            SQLSetConfigMode (mode);
            SQLGetPrivateProfileStringW (L"ODBC", L"TraceFile", L"", tokenstr,
                                         sizeof (tokenstr) / sizeof(wchar_t), NULL);
            
            SQLSetConfigMode (mode);
            SQLGetPrivateProfileStringW (L"ODBC", L"TraceDLL", L"", tokenstr1,
                                         sizeof (tokenstr1) / sizeof(wchar_t), NULL);
            /* Set the widgets */
            if (trace)
            {
                if (!traceauto)
                    [_rb_WhenToTrace selectCellAtRow:1 column:0];
                else
                    [_rb_WhenToTrace selectCellAtRow:2 column:0];
            }
            else
                [_rb_WhenToTrace selectCellAtRow:0 column:0];
            
            _fld_LogFilePath.stringValue = tokenstr[0] == L'\0'
                                            ? [NSString stringWithFormat:@"%@/sql.log", NSHomeDirectory()]
                                            : conv_wchar_to_NSString(tokenstr);
            
            _fld_CustomTrace.stringValue = conv_wchar_to_NSString(tokenstr1);
            _tracing_changed = TRUE;
        }
        

    }
    else if ([identifier isEqualToString:@"about"]){
        addComponents_to_list(_About_ArrController);
    }
}



/** NSTableViewDelegate **/
- (NSView *)tableView:(NSTableView *)tableView viewForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    // Group our "model" object, which is a dictionary
    if (tableView == _fdsn_tableView) {
        NSDictionary *dict = [_FileDSN_list objectAtIndex:row];
        NSString *identifier = [tableColumn identifier];
        NSTableCellView *cellView = [tableView makeViewWithIdentifier:identifier owner:self];
        // Then setup properties on the cellView based on the column
        cellView.textField.stringValue = [dict objectForKey:@"name"];
        cellView.imageView.objectValue = [dict objectForKey:@"icon"];
        return cellView;
    }
    return nil;
}
 

- (IBAction)call_Ok:(id)sender {
    _dialogCode = 1;
    if (_tracing_changed)
        [self call_Trace_Apply:nil];
    if (_pool_changed) {
        UWORD configMode;
        char *tmp = NULL;
        
        SQLGetConfigMode(&configMode);
        /* Write keywords for tracing in the ini file */
        SQLSetConfigMode(ODBC_SYSTEM_DSN);
        SQLWritePrivateProfileString ("ODBC Connection Pooling", "PerfMon",
                                      _rb_PerfMon.selectedRow == 0 ? "1" : "0", "odbcinst.ini");
        
        tmp = conv_NSString_to_char(_fld_RetryWaitTime.stringValue);
        if (tmp) {
            SQLSetConfigMode(ODBC_SYSTEM_DSN);
            SQLWritePrivateProfileString ("ODBC Connection Pooling",
                                          "Retry Wait", tmp, "odbcinst.ini");
            free(tmp);
        }
        SQLSetConfigMode(configMode);
    }
    [self.window close];
}

- (IBAction)call_Cancel:(id)sender {
    _dialogCode = 0;
    [self.window close];
}



- (IBAction)call_UserDSN_Add:(id)sender {
    wchar_t drv[1024] = { L'\0' };
    int sqlstat;
    
    SQLSetConfigMode (ODBC_USER_DSN);
    /* Try first to get the driver name */
    if (_iodbcdm_drvchoose_dialboxw ((void*)1L, drv, sizeof (drv) / sizeof(wchar_t), &sqlstat) == SQL_SUCCESS)
    {
        SQLSetConfigMode (ODBC_USER_DSN);
        if (!SQLConfigDataSourceW ((void*)1L, ODBC_ADD_DSN, drv + WCSLEN (L"DRIVER="), L"\0\0"))
        {
            _iodbcdm_errorboxw ((void*)1L, NULL, L"An error occurred when trying to add the DSN : ");
            goto done;
        }
        
        addDSNs_to_list(FALSE, _UserDSN_ArrController);
    }
done:
    [self.window makeKeyAndOrderFront:self.window];
    return;
}

- (IBAction)call_UserDSN_Remove:(id)sender {
    NSArray *item = [_UserDSN_ArrController selectedObjects];
    if (item!=nil && item.count>0){
        NSDictionary *dict = [item objectAtIndex:0];
        BOOL rc = remove_dsn(FALSE, [dict valueForKey:@"name"], [dict valueForKey:@"drv"]);
        if (rc)
            addDSNs_to_list(FALSE, _UserDSN_ArrController);
        [self.window makeKeyAndOrderFront:self.window];
    }
}

- (IBAction)call_UserDSN_Config:(id)sender {
    NSArray *item = [_UserDSN_ArrController selectedObjects];
    if (item!=nil && item.count>0){
        NSDictionary *dict = [item objectAtIndex:0];
        BOOL rc = configure_dsn(FALSE, [dict valueForKey:@"name"], [dict valueForKey:@"drv"]);
        if (rc)
            addDSNs_to_list(FALSE, _UserDSN_ArrController);
        [self.window makeKeyAndOrderFront:self.window];
    }
}


- (IBAction)call_UserDSN_Test:(id)sender {
    NSArray *item = [_UserDSN_ArrController selectedObjects];
    if (item!=nil && item.count>0){
        NSDictionary *dict = [item objectAtIndex:0];
        test_dsn(FALSE, [dict valueForKey:@"name"], [dict valueForKey:@"drv"]);
        [self.window makeKeyAndOrderFront:self.window];
    }
}


- (IBAction)call_SysDSN_Add:(id)sender {
    wchar_t drv[1024] = { L'\0' };
    int sqlstat;
    
    SQLSetConfigMode (ODBC_USER_DSN);
    /* Try first to get the driver name */
    if (_iodbcdm_drvchoose_dialboxw ((void*)1L, drv, sizeof (drv) / sizeof(wchar_t), &sqlstat) == SQL_SUCCESS)
    {
        SQLSetConfigMode (ODBC_USER_DSN);
        if (!SQLConfigDataSourceW ((void*)1L, ODBC_ADD_SYS_DSN, drv + WCSLEN (L"DRIVER="), L"\0\0"))
        {
            _iodbcdm_errorboxw ((void*)1L, NULL, L"An error occurred when trying to add the DSN : ");
            goto done;
        }
        
        addDSNs_to_list(TRUE, _SysDSN_ArrController);
    }
done:
    [self.window makeKeyAndOrderFront:self.window];
    return;
}

- (IBAction)call_SysDSN_Remove:(id)sender {
    NSArray *item = [_SysDSN_ArrController selectedObjects];
    if (item!=nil && item.count>0){
        NSDictionary *dict = [item objectAtIndex:0];
        BOOL rc = remove_dsn(TRUE, [dict valueForKey:@"name"], [dict valueForKey:@"drv"]);
        if (rc)
            addDSNs_to_list(TRUE, _SysDSN_ArrController);
        [self.window makeKeyAndOrderFront:self.window];
    }
}

- (IBAction)call_SysDSN_Config:(id)sender {
    NSArray *item = [_SysDSN_ArrController selectedObjects];
    if (item!=nil && item.count>0){
        NSDictionary *dict = [item objectAtIndex:0];
        BOOL rc = configure_dsn(TRUE, [dict valueForKey:@"name"], [dict valueForKey:@"drv"]);
        if (rc)
            addDSNs_to_list(TRUE, _SysDSN_ArrController);
        [self.window makeKeyAndOrderFront:self.window];
    }
}

- (IBAction)call_SysDSN_Test:(id)sender {
    NSArray *item = [_SysDSN_ArrController selectedObjects];
    if (item!=nil && item.count>0){
        NSDictionary *dict = [item objectAtIndex:0];
        test_dsn(TRUE, [dict valueForKey:@"name"], [dict valueForKey:@"drv"]);
        [self.window makeKeyAndOrderFront:self.window];
    }
}


- (IBAction)call_FileDSN_Dir_Browse:(id)sender {
    NSOpenPanel *panel = [NSOpenPanel openPanel];
    
    [panel setTitle:@"Choose your File DSN directory ..."];
    if (_cur_dir.length>0)
        [panel setDirectoryURL:[NSURL fileURLWithPath:_cur_dir isDirectory:TRUE]];
    panel.allowsMultipleSelection = FALSE;
    panel.canChooseDirectories = TRUE;
    panel.canChooseFiles = FALSE;
    panel.canCreateDirectories = TRUE;
    
    NSInteger rc = [panel runModal];
    if (rc==NSFileHandlingPanelOKButton) {
        self.cur_dir = ((NSURL*)[panel.URLs objectAtIndex:0]).path;
        wchar_t *path = conv_NSString_to_wchar(_cur_dir);
        addFDSNs_to_list(_cur_dir, FALSE, _FileDSN_ArrController);
        fill_dir_menu(path, _popup_dir_btn);
        if (path) free(path);
    }
    [self.window makeKeyAndOrderFront:self.window];
}


- (IBAction)call_Dir_PopupBtn:(id)sender {
    self.cur_dir = _popup_dir_btn.titleOfSelectedItem;
    wchar_t *path = conv_NSString_to_wchar(_cur_dir);
    addFDSNs_to_list(_cur_dir, FALSE, _FileDSN_ArrController);
    fill_dir_menu(path, _popup_dir_btn);
    if (path) free(path);
}

- (IBAction)call_FileDSN_Add:(id)sender {
    BOOL rc = add_file_dsn(_cur_dir);
    if (rc) {
        wchar_t *cur_path = conv_NSString_to_wchar(_cur_dir);
        addFDSNs_to_list(_cur_dir, FALSE, _FileDSN_ArrController);
        fill_dir_menu(cur_path, _popup_dir_btn);
        if (cur_path) free(cur_path);
    }
    [self.window makeKeyAndOrderFront:self.window];
}

- (IBAction)call_FileDSN_Remove:(id)sender {
    NSArray *item = [_FileDSN_ArrController selectedObjects];
    if (item!=nil && item.count>0){
        NSDictionary *dict = [item objectAtIndex:0];
        NSNumber *isdir = [dict valueForKey:@"isdir"];
        if (isdir.boolValue==FALSE){
            BOOL rc = remove_file_dsn(_cur_dir, [dict valueForKey:@"name"]);
            if (rc){
                wchar_t *cur_path = conv_NSString_to_wchar(_cur_dir);
                addFDSNs_to_list(_cur_dir, FALSE, _FileDSN_ArrController);
                fill_dir_menu(cur_path, _popup_dir_btn);
                if (cur_path) free(cur_path);
            }
        }
        [self.window makeKeyAndOrderFront:self.window];
    }
}

- (IBAction)call_FileDSN_Config:(id)sender {
    NSArray *item = [_FileDSN_ArrController selectedObjects];
    if (item!=nil && item.count>0){
        NSDictionary *dict = [item objectAtIndex:0];
        NSNumber *isdir = [dict valueForKey:@"isdir"];
        if (isdir.boolValue==FALSE){
            configure_file_dsn(_cur_dir, [dict valueForKey:@"name"]);
            char *cur_path = conv_NSString_to_char(_cur_dir);
            addFDSNs_to_list(_cur_dir, FALSE, _FileDSN_ArrController);
            if (cur_path) free(cur_path);
            
        } else {
            
            self.cur_dir = [NSString stringWithFormat:@"%@/%@", _cur_dir, [dict valueForKey:@"name"]];
            wchar_t *path = conv_NSString_to_wchar(_cur_dir);
            addFDSNs_to_list(_cur_dir, FALSE, _FileDSN_ArrController);
            fill_dir_menu(path, _popup_dir_btn);
            if (path) free(path);
        }
        [self.window makeKeyAndOrderFront:self.window];
    }
}

- (IBAction)call_FileDSN_Test:(id)sender {
    NSArray *item = [_FileDSN_ArrController selectedObjects];
    if (item!=nil && item.count>0){
        NSDictionary *dict = [item objectAtIndex:0];
        NSNumber *isdir = [dict valueForKey:@"isdir"];
        if (isdir.boolValue==FALSE){
            test_file_dsn(_cur_dir, [dict valueForKey:@"name"]);
        }
        [self.window makeKeyAndOrderFront:self.window];
    }
}

- (IBAction)call_FileDSN_SetDir:(id)sender {
    setdir_file_dsn(_cur_dir);
    [self.window makeKeyAndOrderFront:self.window];
}


- (IBAction)call_Drv_Add:(id)sender {
    wchar_t connstr[4096] = { L'\0' }, tokenstr[4096] = { L'\0' };
    wchar_t *cstr;
    
    cstr = (LPWSTR)create_driversetupw (NULL, (LPCWSTR)connstr, TRUE, TRUE);
    if (cstr && cstr != connstr && cstr != (LPWSTR)- 1L)
    {
        SQLSetConfigMode (!wcscmp(cstr, L"USR") ? ODBC_USER_DSN : ODBC_SYSTEM_DSN);
        if (!SQLInstallDriverExW (cstr + 4, NULL, tokenstr,
                                  sizeof (tokenstr) / sizeof(wchar_t), NULL, ODBC_INSTALL_COMPLETE, NULL))
        {
            _iodbcdm_errorboxw ((void*)1L, NULL, L"An error occurred when trying to add the driver");
            goto done;
        }
        
        free (cstr);
    }
    
    addDrivers_to_list(_Drv_ArrController);
done:
    [self.window makeKeyAndOrderFront:self.window];
    return;
}


- (IBAction)call_Drv_Remove:(id)sender {
    wchar_t tokenstr[4096] = { L'\0' };
    
    NSArray *item = [_Drv_ArrController selectedObjects];
    if (item!=nil && item.count>0){
        int dsns = 0;
        NSDictionary *dict = [item objectAtIndex:0];
        wchar_t *wdrv = conv_NSString_to_wchar([dict valueForKey:@"name"]);
        
        if (wdrv) {
            /* Initialize some values */
            SQLSetConfigMode (ODBC_USER_DSN);
            if (SQLGetPrivateProfileStringW (wdrv, NULL, L"", tokenstr, sizeof (tokenstr)/sizeof(wchar_t), L"odbcinst.ini"))
                dsns |= 1;
            SQLSetConfigMode (ODBC_SYSTEM_DSN);
            if (SQLGetPrivateProfileStringW (wdrv, NULL, L"", tokenstr, sizeof (tokenstr)/sizeof(wchar_t), L"odbcinst.ini"))
                dsns |= 2;

            IODBCadm_DrvRemoveController *dlg = [[IODBCadm_DrvRemoveController alloc] initWithDSNS:dsns];
            NSInteger rc = [NSApp runModalForWindow:dlg.window];
            if (rc == 1) {
                
                if (create_confirmw ((void*)1L, wdrv, L"Are you sure you want to perform the removal of this driver ?"))
                {
                    if ((dlg.dsns & 1) > 0)
                    {
                        SQLSetConfigMode (ODBC_USER_DSN);
                        if (!SQLRemoveDriverW (wdrv, dlg.deletedsn, NULL))
                        {
                            _iodbcdm_errorboxw ((void*)1L, wdrv,
                                                L"An error occurred when trying to remove the driver ");
                            goto done;
                        }
                    }
                    
                    if ((dlg.dsns & 2) > 0)
                    {
                        SQLSetConfigMode (ODBC_SYSTEM_DSN);
                        if (!SQLRemoveDriverW (wdrv, dlg.deletedsn, NULL))
                        {
                            _iodbcdm_errorboxw ((void*)1L, wdrv,
                                                L"An error occurred when trying to remove the driver ");
                            goto done;
                        }
                    }
                done:
                    addDrivers_to_list(_Drv_ArrController);
                }
            }
            
            free(wdrv);
            [dlg.window orderOut:dlg.window];
            [dlg release];
        }
        [self.window makeKeyAndOrderFront:self.window];
    }
}


- (IBAction)call_Drv_Config:(id)sender {
    wchar_t tokenstr[4096] = { L'\0' };
    UWORD conf = ODBC_USER_DSN;
    
    NSArray *item = [_Drv_ArrController selectedObjects];
    if (item!=nil && item.count>0){
        NSDictionary *dict = [item objectAtIndex:0];
        wchar_t *wdrv = conv_NSString_to_wchar([dict valueForKey:@"name"]);
        
        if (wdrv) {
            wchar_t *curr, *cour, *cstr;
            wchar_t connstr[4096] = { L'\0' };
            int size = sizeof (connstr) / sizeof(wchar_t);

            SQLSetConfigMode (ODBC_USER_DSN);
            if (!SQLGetPrivateProfileStringW (wdrv, NULL, L"", tokenstr,
                                              sizeof (tokenstr) / sizeof(wchar_t), L"odbcinst.ini"))
            {
                SQLSetConfigMode (conf = ODBC_SYSTEM_DSN);
                if (!SQLGetPrivateProfileStringW (wdrv, NULL, L"", tokenstr,
                                                  sizeof (tokenstr) / sizeof(wchar_t), L"odbcinst.ini"))
                {
                    _iodbcdm_errorboxw ((void*)1L, wdrv,
                                        L"An error occurred when trying to configure the driver ");
                    goto done;
                }
            }
            
            for (curr = tokenstr, cour = connstr; *curr != L'\0' ;
                 curr += (wcslen (curr) + 1), cour += (wcslen(cour) + 1))
            {
                wcscpy (cour, curr);
                cour[wcslen (curr)] = L'=';
                SQLSetConfigMode (conf);
                SQLGetPrivateProfileStringW (wdrv, curr, L"",
                                             cour + wcslen(curr) + 1, (int)(size - wcslen(curr) - 1),
                                             L"odbcinst.ini");
                size -= (wcslen(cour) + 1);
            }

            *cour = L'\0';

            cstr = (LPWSTR)create_driversetupw (wdrv, (LPCWSTR)connstr, FALSE, (conf == ODBC_SYSTEM_DSN) ? FALSE : TRUE);
            if (cstr && cstr != connstr && cstr != (LPWSTR) - 1L)
            {
                SQLSetConfigMode (conf);
                if (!SQLInstallDriverExW (cstr + 4, NULL, tokenstr,
                                          sizeof (tokenstr) / sizeof(wchar_t), NULL,
                                          ODBC_INSTALL_COMPLETE, NULL))
                {
                    _iodbcdm_errorboxw ((void*)1L, NULL,
                                        L"An error occurred when trying to configure the driver ");
                    goto done;
                }
                free (cstr);
            }

        done:
            addDrivers_to_list(_Drv_ArrController);
            free(wdrv);
        }
        [self.window makeKeyAndOrderFront:self.window];
    }
}


- (IBAction)call_Trace_Apply:(id)sender {
    int mode;
    wchar_t *tmp;
    
    /* Clear previous setting */
    SQLSetConfigMode (ODBC_USER_DSN);
    SQLWritePrivateProfileString ("ODBC", "Trace", NULL, NULL);
    SQLSetConfigMode (ODBC_USER_DSN);
    SQLWritePrivateProfileString ("ODBC", "TraceAutoStop", NULL, NULL);
    SQLSetConfigMode (ODBC_USER_DSN);
    SQLWritePrivateProfileString ("ODBC", "TraceFile", NULL, NULL);
    SQLSetConfigMode (ODBC_USER_DSN);
    SQLWritePrivateProfileString ("ODBC", "TraceDLL", NULL, NULL);
        
    SQLSetConfigMode (ODBC_SYSTEM_DSN);
    SQLWritePrivateProfileString ("ODBC", "Trace", NULL, NULL);
    SQLSetConfigMode (ODBC_SYSTEM_DSN);
    SQLWritePrivateProfileString ("ODBC", "TraceAutoStop", NULL, NULL);
    SQLSetConfigMode (ODBC_SYSTEM_DSN);
    SQLWritePrivateProfileString ("ODBC", "TraceFile", NULL, NULL);
    SQLSetConfigMode (ODBC_SYSTEM_DSN);
    SQLWritePrivateProfileString ("ODBC", "TraceDLL", NULL, NULL);
        
    mode = _rb_TraceWide.selectedRow == 0 ? ODBC_USER_DSN : ODBC_SYSTEM_DSN;
    
    /* Write keywords for tracing in the ini file */
    SQLSetConfigMode(mode);
    if (_rb_WhenToTrace.selectedRow == 1 || _rb_WhenToTrace.selectedRow == 2)
        SQLWritePrivateProfileString ("ODBC", "Trace", "1", NULL);
    else
        SQLWritePrivateProfileString ("ODBC", "Trace", "0", NULL);
    
    SQLSetConfigMode(mode);
    if (_rb_WhenToTrace.selectedRow == 2)
        SQLWritePrivateProfileString ("ODBC", "TraceAutoStop", "1", NULL);
    else
        SQLWritePrivateProfileString ("ODBC", "TraceAutoStop", "0", NULL);

    tmp = conv_NSString_to_wchar(_fld_LogFilePath.stringValue);
    if (tmp)
    {
        SQLSetConfigMode(mode);
        SQLWritePrivateProfileStringW (L"ODBC", L"TraceFile", tmp, NULL);
        free(tmp);
    }

    tmp = conv_NSString_to_wchar(_fld_CustomTrace.stringValue);
    if (tmp)
    {
        SQLSetConfigMode(mode);
        SQLWritePrivateProfileStringW (L"ODBC", L"TraceDLL", tmp, NULL);
        free(tmp);
    }
}

- (IBAction)call_LogFilePath_Browse:(id)sender {
    NSSavePanel *panel = [NSSavePanel savePanel];
    
    NSURL *file_url = [NSURL fileURLWithPath:_fld_LogFilePath.stringValue];
    NSString *fpath = file_url.path;
    NSString *fname = file_url.lastPathComponent;
    NSString *dir = [fpath substringToIndex:(fpath.length - fname.length)];
    
    [panel setTitle:@"Choose your trace file ..."];
    [panel setNameFieldStringValue:fname];
    [panel setDirectoryURL:[NSURL fileURLWithPath:dir isDirectory:TRUE]];
    NSInteger rc = [panel runModal];
    if (rc==NSFileHandlingPanelOKButton)
        [_fld_LogFilePath setStringValue:[NSString stringWithFormat:@"%@/%@", panel.directoryURL.path, panel.nameFieldStringValue]];
    [self.window makeKeyAndOrderFront:self.window];
}

- (IBAction)call_CustomTrace_Browse:(id)sender {
    NSOpenPanel *panel = [NSOpenPanel openPanel];

    NSURL *file_url = [NSURL fileURLWithPath:_fld_CustomTrace.stringValue];
    NSString *fpath = file_url.path;
    NSString *fname = file_url.lastPathComponent;
    NSString *dir = [fpath substringToIndex:(fpath.length - fname.length)];
    
    
    [panel setTitle:@"Choose your trace library ..."];
    [panel setNameFieldStringValue:(fname.length>0)? fname: @"ODBC Trace Library"];
    if (dir.length>0)
        [panel setDirectoryURL:[NSURL fileURLWithPath:dir isDirectory:TRUE]];
    panel.allowsMultipleSelection = FALSE;
    panel.canChooseDirectories = FALSE;
    
    NSInteger rc = [panel runModal];
    if (rc==NSFileHandlingPanelOKButton)
        [_fld_CustomTrace setStringValue: ((NSURL*)[panel.URLs objectAtIndex:0]).path];
    [self.window makeKeyAndOrderFront:self.window];
}



@end
