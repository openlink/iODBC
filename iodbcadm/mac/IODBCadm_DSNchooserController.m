/*
 *  IODBCadm_DSNchooserController.m
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

#import "IODBCadm_DSNchooserController.h"
#import "utils.h"
#import "Helpers.h"


void
create_dsnchooser (HWND hwnd, TDSNCHOOSER * dsnchoose_t)
{
    if (hwnd == NULL)
        return;

	@autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        
        IODBCadm_DSNchooserController *dlg = [[IODBCadm_DSNchooserController alloc] init];
        
        NSInteger rc = [app runModalForWindow:dlg.window];
        if (dsnchoose_t) {
            dsnchoose_t->dsn = NULL;
            dsnchoose_t->fdsn = NULL;
            dsnchoose_t->type_dsn = -1;

            if (rc == 1) {
                if ([dlg.type_dsn isEqualToString:@"userdsn"]) {
                    NSArray *item = [dlg.UserDSN_ArrController selectedObjects];
                    if (item!=nil && item.count>0){
                        NSDictionary *dict = [item objectAtIndex:0];
                        dsnchoose_t->type_dsn = USER_DSN;
                        dsnchoose_t->dsn = conv_NSString_to_wchar([dict valueForKey:@"name"]);
                    }
                }
                else if ([dlg.type_dsn isEqualToString:@"sysdsn"]) {
                    NSArray *item = [dlg.SysDSN_ArrController selectedObjects];
                    if (item!=nil && item.count>0){
                        NSDictionary *dict = [item objectAtIndex:0];
                        dsnchoose_t->type_dsn = SYSTEM_DSN;
                        dsnchoose_t->dsn = conv_NSString_to_wchar([dict valueForKey:@"name"]);
                    }
                }
                else if ([dlg.type_dsn isEqualToString:@"filedsn"]) {
                    NSArray *item = [dlg.FileDSN_ArrController selectedObjects];
                    if (item!=nil && item.count>0){
                        NSDictionary *dict = [item objectAtIndex:0];
                        dsnchoose_t->type_dsn = FILE_DSN;
                        NSString *fdsn = [NSString stringWithFormat:@"%@/%@", dlg.cur_dir, [dict valueForKey:@"name"]];
                        dsnchoose_t->fdsn = conv_NSString_to_wchar(fdsn);
                    }
                }
            }
        }
        [dlg.window orderOut:dlg.window];
        [dlg release];
    }
}




@interface IODBCadm_DSNchooserController ()

@end

@implementation IODBCadm_DSNchooserController
@synthesize popup_dir_btn = _popup_dir_btn;
@synthesize fdsn_tableView = _fdsn_tableView;
@synthesize tab_view = _tab_view;
@synthesize FileDSN_ArrController = _FileDSN_ArrController;
@synthesize SysDSN_ArrController = _SysDSN_ArrController;
@synthesize UserDSN_ArrController = _UserDSN_ArrController;
@synthesize UserDSN_list=_UserDSN_list;
@synthesize SysDSN_list=_SysDSN_list;
@synthesize FileDSN_list=_FileDSN_list;
@synthesize cur_dir=_cur_dir;
@synthesize type_dsn = _type_dsn;

- (id)init
{
    char tmp[1024] = {""};
    
    self = [super initWithWindowNibName:@"IODBCadm_DSNchooserController"];
    if (self) {
        self.UserDSN_list = [NSMutableArray arrayWithCapacity:16];
        self.SysDSN_list = [NSMutableArray arrayWithCapacity:16];
        self.FileDSN_list = [NSMutableArray arrayWithCapacity:16];
        
        SQLSetConfigMode (ODBC_BOTH_DSN);
        if (!SQLGetPrivateProfileString("ODBC", "FileDSNPath", "", tmp, sizeof(tmp), "odbcinst.ini"))
          self.cur_dir = get_user_documents_dir();
        else
          self.cur_dir = conv_char_to_NSString(tmp);
    }
    return self;
}

- (void)dealloc
{
    [_UserDSN_list release];
    [_SysDSN_list release];
    [_FileDSN_list release];
    [_type_dsn release];
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
    [super windowDidLoad];
    _dialogCode = 0;
    
    [[self window] center];  // Center the window.
    addDSNs_to_list(FALSE, _UserDSN_ArrController);
    addDSNs_to_list(TRUE, _SysDSN_ArrController);
    [_fdsn_tableView setDoubleAction:@selector(call_FDSN_DoubleClick)];
    self.type_dsn = @"userdsn";
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
        [self call_Ok:self];
    }
}

/** NSTabViewDelegate **/
- (void)tabView:(NSTabView *)tabView didSelectTabViewItem:(NSTabViewItem *)tabViewItem
{
    NSString *identifier = [tabViewItem identifier];
    self.type_dsn = identifier;

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
        [self.window makeKeyAndOrderFront:self.window];
    }
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

@end
