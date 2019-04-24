/*
 *  $Id$
 *
 *  The iODBC driver manager.
 *
 *  Copyright (C) 1996-2019 by OpenLink Software <iodbc@openlinksw.com>
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


#import "DSNchooserController.h"
#import "Helpers.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <pwd.h>


@interface DSNchooserController ()

@end

@implementation DSNchooserController
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
@synthesize selected_dsn = _selected_dsn;
@synthesize uid = _uid;
@synthesize pwd = _pwd;

- (id)init
{
    SQLTCHAR tmp[1024] = {""};
    
    self = [super initWithWindowNibName:@"DSNchooserController"];
    if (self) {
        _UserDSN_list = [[NSMutableArray alloc] initWithCapacity:16];
        _SysDSN_list = [[NSMutableArray alloc] initWithCapacity:16];
        _FileDSN_list = [[NSMutableArray alloc] initWithCapacity:16];
        
        SQLSetConfigMode (ODBC_BOTH_DSN);
        if (!SQLGetPrivateProfileString(TEXT("ODBC"), TEXT("FileDSNPath"), TEXT(""), tmp, sizeof(tmp)/sizeof(SQLTCHAR), TEXT("odbcinst.ini")))
            _cur_dir = [NSString stringWithUTF8String:DEFAULT_FILEDSNPATH];
        else
            _cur_dir = (NSString*)TEXTtoNS(tmp);
    }
    return self;
}

- (void)dealloc
{
    [_UserDSN_list release];
    [_SysDSN_list release];
    [_FileDSN_list release];
    [_type_dsn release];
    [_selected_dsn release];
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
        const char *path = [_cur_dir UTF8String];

        addFDSNs_to_list(path, FALSE, _FileDSN_ArrController);
        fill_dir_menu(path, _popup_dir_btn);
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
        const char *cur_path = [_cur_dir UTF8String];
        addFDSNs_to_list(cur_path, FALSE, _FileDSN_ArrController);
        fill_dir_menu(cur_path, _popup_dir_btn);
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
    if ([_type_dsn isEqualToString:@"userdsn"]) {
        NSArray *item = [_UserDSN_ArrController selectedObjects];
        if (item!=nil && item.count>0){
            NSDictionary *dict = [item objectAtIndex:0];
            self.selected_dsn = [dict valueForKey:@"name"];
        }
    }
    else if ([_type_dsn isEqualToString:@"sysdsn"]) {
        NSArray *item = [_SysDSN_ArrController selectedObjects];
        if (item!=nil && item.count>0){
            NSDictionary *dict = [item objectAtIndex:0];
            self.selected_dsn = [dict valueForKey:@"name"];
        }
    }
    else if ([_type_dsn isEqualToString:@"filedsn"]) {
        NSArray *item = [_FileDSN_ArrController selectedObjects];
        if (item!=nil && item.count>0){
            NSDictionary *dict = [item objectAtIndex:0];
            self.selected_dsn = [NSString stringWithFormat:@"%@/%@", _cur_dir, [dict valueForKey:@"name"]];
        }
    }
    [self.window close];
}

- (IBAction)call_Cancel:(id)sender {
    _dialogCode = 0;
    [self.window close];
}


- (IBAction)call_Dir_PopupBtn:(id)sender {
    self.cur_dir = _popup_dir_btn.titleOfSelectedItem;
    const char *path = [_cur_dir UTF8String];
    addFDSNs_to_list(path, FALSE, _FileDSN_ArrController);
    fill_dir_menu(path, _popup_dir_btn);
}




void addDSNs_to_list(BOOL systemDSN, NSArrayController* list)
{
    SQLTCHAR dsnname[1024], dsndesc[1024];
    SQLSMALLINT len;
    SQLRETURN ret;
    HENV henv;
    
    [list removeObjects:[list arrangedObjects]];
    
    /* Create a HENV to get the list of data sources then */
    ret = SQLAllocHandle (SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
    {
        _nativeerrorbox (henv, SQL_NULL_HANDLE, SQL_NULL_HANDLE);
        goto end;
    }
    
    /* Set the version ODBC API to use */
    SQLSetEnvAttr (henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER) SQL_OV_ODBC3,
                   SQL_IS_UINTEGER);

# ifdef UNICODE
    SQLSetEnvAttr (henv, SQL_ATTR_APP_UNICODE_TYPE,
                   (SQLPOINTER) SQL_DM_CP_DEF, SQL_IS_UINTEGER);
#endif
                   
    /* Get the list of datasources */
    ret = SQLDataSourcesW (henv,
                           systemDSN ? SQL_FETCH_FIRST_SYSTEM : SQL_FETCH_FIRST_USER,
                           dsnname, sizeof (dsnname)/sizeof(SQLTCHAR), &len,
                           dsndesc, sizeof (dsndesc)/sizeof(SQLTCHAR), NULL);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO && ret != SQL_NO_DATA)
    {
        _nativeerrorbox (henv, SQL_NULL_HANDLE, SQL_NULL_HANDLE);
        goto error;
    }
    
    while (ret != SQL_NO_DATA)
    {
        NSString *name = TEXTtoNS(dsnname);
        if (dsndesc[0] == 0)
        {
            SQLSetConfigMode (ODBC_BOTH_DSN);
            SQLGetPrivateProfileString(TEXT("Default"), TEXT("Driver"), TEXT(""), dsndesc,
                                         sizeof (dsndesc)/sizeof(wchar_t), TEXT("odbc.ini"));
        }
        NSString *drv = dsndesc[0]!=0? TEXTtoNS(dsndesc): @"-";
        
        /* Get the description */
        SQLSetConfigMode (systemDSN ? ODBC_SYSTEM_DSN : ODBC_USER_DSN);
        SQLGetPrivateProfileString (dsnname, TEXT("Description"), TEXT(""), dsndesc,
                                     sizeof (dsndesc)/sizeof(SQLTCHAR), TEXT("odbc.ini"));
        
        NSString *desc = dsndesc[0]!=0? TEXTtoNS(dsndesc): @"-";
        
        [list addObject:[NSMutableDictionary dictionaryWithObjectsAndKeys:name, @"name",
                         desc, @"desc", drv, @"drv", nil]];
        
        /* Process next one */
        ret = SQLDataSources (henv, SQL_FETCH_NEXT, dsnname,
                               sizeof (dsnname)/sizeof(SQLTCHAR), &len, dsndesc,
                               sizeof (dsndesc)/sizeof(SQLTCHAR), NULL);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO
            && ret != SQL_NO_DATA)
        {
            _nativeerrorbox (henv, SQL_NULL_HANDLE, SQL_NULL_HANDLE);
            goto error;
        }
    }
    
error:
    /* Clean all that */
    SQLFreeHandle (SQL_HANDLE_ENV, henv);
end:
    return;
}


#define MAX_ROWS 1024

void addFDSNs_to_list(const char* path, BOOL b_reset, NSArrayController* list)
{
    int nrows;
    DIR *dir;
    char *path_buf;
    struct dirent *dir_entry;
    struct stat fstat;
    
    [list removeObjects:[list arrangedObjects]];
    nrows = 0;
    
    if ((dir = opendir (path)))
    {
        while ((dir_entry = readdir (dir)) && nrows < MAX_ROWS)
        {
            asprintf (&path_buf, "%s/%s", path, dir_entry->d_name);
            
            if (stat ((const char*) path_buf, &fstat) >= 0 && S_ISDIR (fstat.st_mode))
            {
                if (dir_entry->d_name && dir_entry->d_name[0] != '.')
                {
                    NSString *name = [NSString stringWithUTF8String:dir_entry->d_name];
                    NSImage *icon = [NSImage imageNamed:NSImageNameFolder];
                    [list addObject:[NSMutableDictionary dictionaryWithObjectsAndKeys:name, @"name",
                                     icon, @"icon", [NSNumber numberWithBool:TRUE], @"isdir", nil]];
                    nrows++;
                }
            }
            else if (stat ((const char*) path_buf, &fstat) >= 0 && !S_ISDIR (fstat.st_mode)
                     && strstr (dir_entry->d_name, ".dsn"))
            {
                NSString *name = [NSString stringWithUTF8String:dir_entry->d_name];
                NSImage *icon = [[NSWorkspace sharedWorkspace]
                                 iconForFileType:NSFileTypeForHFSTypeCode(kGenericDocumentIcon)];
                [list addObject:[NSMutableDictionary dictionaryWithObjectsAndKeys:name, @"name",
                                 icon, @"icon", [NSNumber numberWithBool:FALSE], @"isdir", nil]];
                nrows++;
            }
            
            if (path_buf)
                free (path_buf);
        }
        
        /* Close the directory entry */
        closedir (dir);
    }
    else
        create_error ("Error during accessing directory information", strerror (errno));
    
    //??    if (b_reset)
    //??        SetDataBrowserScrollPosition(widget, 0, 0);
}

void fill_dir_menu(const char* path, NSPopUpButton* list)
{
    char *curr_dir, *prov, *dir;
    
    if (!path || !(prov = strdup (path)))
        return;
    
    if (prov[strlen(prov) - 1] == '/' && strlen(prov) > 1)
        prov[strlen(prov) - 1] = 0;
    
    [list removeAllItems];
    
    /* Add the root directory */
    [list addItemWithTitle:@"/"];
    
    if (strlen(prov) > 1)
        for (curr_dir = prov, dir = NULL; curr_dir;
             curr_dir = strchr (curr_dir + 1, '/'))
        {
            if (strchr (curr_dir + 1, '/'))
            {
                dir = strchr (curr_dir + 1, '/');
                *dir = 0;
            }
            
            [list addItemWithTitle:[NSString stringWithUTF8String:prov]];
            
            if (dir)
                *dir = '/';
        }
    free(prov);
    [list selectItemAtIndex:list.numberOfItems-1];
}

@end
