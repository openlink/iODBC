/*
 *  IODBCadm_DriverChooseFDSNController.m
 *
 *  $Id$
 *
 *  The iODBC driver manager.
 *
 *  Copyright (C) 1996-2020 OpenLink Software <iodbc@openlinksw.com>
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

#import "IODBCadm_DriverChooseFDSNController.h"
#import "IODBCadm_KeyValController.h"
#import "utils.h"
#import "Helpers.h"


static char* showKeyVal(NSWindow *mainWin, char* attrs, BOOL *verify_conn);


void
create_fdriverchooser (HWND hwnd, TFDRIVERCHOOSER * choose_t)
{
    choose_t->attrs = NULL;
    choose_t->verify_conn = TRUE;
    choose_t->driver = NULL;
    choose_t->dsn = NULL;
    choose_t->ok = FALSE;
    
    if (hwnd == NULL)
        return;
    
	@autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        
        IODBCadm_DriverChooseFDSNController *dlg = [[IODBCadm_DriverChooseFDSNController alloc] init];
        if (choose_t && choose_t->curr_dir)
            dlg.fdsn_dir = [NSString stringWithUTF8String:choose_t->curr_dir];
        
        NSInteger rc = [app runModalForWindow:dlg.window];
        if (choose_t) {
            choose_t->attrs = NULL;
            choose_t->verify_conn = TRUE;
            choose_t->driver = NULL;
            choose_t->dsn = NULL;
            choose_t->ok = FALSE;
            
            if (rc == 1) {
                NSArray *item = [dlg.Drv_ArrController selectedObjects];
                if (item!=nil && item.count>0){
                    NSDictionary *dict = [item objectAtIndex:0];
                    choose_t->driver = conv_NSString_to_wchar([dict valueForKey:@"name"]);
                }
                
                NSString *fdsn = dlg.fld_FDSN.stringValue;
                NSCharacterSet *set = [NSCharacterSet characterSetWithCharactersInString:@"/"];
                if ([fdsn rangeOfCharacterFromSet:set].location == NSNotFound)
                    choose_t->dsn = strdup([[NSString stringWithFormat:@"%@/%@", dlg.fdsn_dir, fdsn] UTF8String]);
                else
                    choose_t->dsn = strdup(fdsn.UTF8String);
                choose_t->attrs = dlg.attrs;
                dlg.attrs = NULL;
                choose_t->ok = TRUE;
            }
        }
        [dlg.window orderOut:dlg.window];
        [dlg release];
        
    }
    
}

static char* showKeyVal(NSWindow *mainWin, char* attrs, BOOL *verify_conn)
{
    char *connstr = NULL;
	@autoreleasepool {

        IODBCadm_KeyValController *dlg = [[IODBCadm_KeyValController alloc] initWithAttrs:attrs];
        
        NSInteger rc = [NSApp runModalForWindow:dlg.window];
        [dlg.window orderOut:dlg.window];
        if (rc == 1){
            char *cour;
            int i = 0, size = 1;
            char *val,*key;
            
            /* Malloc it */
            if ((connstr = (char *) calloc(sizeof(char), 2)))
            {
                for (i = 0; i < dlg.Attrs_list.count; i++)
                {
                    NSDictionary *row = [dlg.Attrs_list objectAtIndex:i];
                    key = (char*)conv_NSString_to_char((NSString*)[row valueForKey:@"key"]);
                    val = (char*)conv_NSString_to_char((NSString*)[row valueForKey:@"val"]);
                    
                    cour = connstr;
                    connstr = (char*) malloc (size + strlen(key) + strlen(val) + 2);
                    if (connstr)
                    {
                        memcpy (connstr, cour, size);
                        sprintf (connstr + size - 1, "%s=%s", key, val);
                        free (cour);
                        size += strlen(key) + strlen(val) + 2;
                    }
                    else
                        connstr = cour;
                    
                    if (key!=NULL) free(key);
                    if (val!=NULL) free(val);
                }
                
                connstr[size - 1] = '\0';
            }
            if (verify_conn)
                *verify_conn = (dlg.chk_Verify.state == NSOnState);
        }
        [dlg release];
        return connstr;
    }
}


@interface IODBCadm_DriverChooseFDSNController ()

@end

@implementation IODBCadm_DriverChooseFDSNController
@synthesize Drv_ArrController = _Drv_ArrController;
@synthesize Results_ArrController = _Results_ArrController;
@synthesize btn_Continue = _btn_Continue;
@synthesize tab_view = _tab_view;
@synthesize btn_Back = _btn_Back;
@synthesize fld_FDSN = _fld_FDSN;
@synthesize Drv_list=_Drv_list;
@synthesize Results_list=_Results_list;
@synthesize fdsn_dir=_fdsn_dir;
@synthesize attrs=_attrs;


- (id)init
{
    self = [super initWithWindowNibName:@"IODBCadm_DriverChooseFDSNController"];
    if (self) {
        self.Drv_list = [NSMutableArray arrayWithCapacity:16];
        self.Results_list = [NSMutableArray arrayWithCapacity:16];
        _verify_conn = TRUE;
        _attrs = calloc(sizeof(char), 2);
    }
    return self;
}

- (void)dealloc
{
    [_Drv_list release];
    [_Results_list release];
    if (_attrs)
        free(_attrs);
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
    addDrivers_to_list(_Drv_ArrController);
}

- (void)windowWillClose:(NSNotification*)notification
{
    [NSApp stopModalWithCode:_dialogCode];
}


- (void) updateResArray
{
    NSString *fdsn = _fld_FDSN.stringValue;
    
    [_Results_ArrController removeObjects:[_Results_ArrController arrangedObjects]];
    [_Results_ArrController addObject:@"File Data Source:"];
    
    NSCharacterSet *set = [NSCharacterSet characterSetWithCharactersInString:@"/"];
    if ([fdsn rangeOfCharacterFromSet:set].location == NSNotFound)
        [_Results_ArrController addObject:[NSString stringWithFormat:@"Filename: %@/%@", _fdsn_dir, fdsn]];
    else
        [_Results_ArrController addObject:[NSString stringWithFormat:@"Filename: %@", fdsn]];

    NSArray *item = [_Drv_ArrController selectedObjects];
    if (item!=nil && item.count>0){
        NSDictionary *dict = [item objectAtIndex:0];
        [_Results_ArrController addObject:[NSString stringWithFormat:@"Driver: %@", [dict valueForKey:@"name"]]];
    }
    
    [_Results_ArrController addObject:@"Driver-specific Keywords:"];
    if (_attrs){
        char *curr;
        for (curr = _attrs; *curr; curr += (strlen (curr) + 1))
        {
            if (!strncasecmp (curr, "PWD=", strlen ("PWD=")))
            {
                continue;
            }

            [_Results_ArrController addObject:conv_char_to_NSString(curr)];
        }
    }
}

/** NSTabViewDelegate **/
- (void)tabView:(NSTabView *)tabView didSelectTabViewItem:(NSTabViewItem *)tabViewItem
{
    NSString *identifier = [tabViewItem identifier];
    
    if ([identifier isEqualToString:@"drv"]){
        [_btn_Back setEnabled:FALSE];
        [_btn_Continue setTitle:@"Continue"];
    }
    else if ([identifier isEqualToString:@"name"]){
        [_btn_Back setEnabled:TRUE];
        [_btn_Continue setTitle:@"Continue"];
    }
    else if ([identifier isEqualToString:@"res"]){
        [_btn_Back setEnabled:TRUE];
        [_btn_Continue setTitle:@"Finish"];
        [self updateResArray];
    }
}

- (BOOL)tabView:(NSTabView *)tabView shouldSelectTabViewItem:(NSTabViewItem *)tabViewItem
{
    NSString *identifier = [tabViewItem identifier];
    if ([identifier isEqualToString:@"res"]){
        if (_fld_FDSN.stringValue.length==0){
            NSRunAlertPanel(@"Enter File DSN Name...", @"", @"OK", nil, nil);
            return FALSE;
        }
    } else if ([identifier isEqualToString:@"name"]){
        NSArray *item = [_Drv_ArrController selectedObjects];
        if (item==nil && item.count==0){
            NSRunAlertPanel(@"Driver wasn't selected!", @"", @"OK", nil, nil);
            return FALSE;
        }
    }
    return TRUE;
}



- (IBAction)call_Cancel:(id)sender {
    _dialogCode = 0;
    [self.window close];
}

- (IBAction)call_Continue:(id)sender {
    NSString *ident = _tab_view.selectedTabViewItem.identifier;
    if ([ident isEqualToString:@"res"]) {
        _dialogCode = 1;
        [self.window close];
    } else if ([ident isEqualToString:@"name"]) {
        if (_fld_FDSN.stringValue.length==0)
            NSRunAlertPanel(@"Enter File DSN Name...", @"", @"OK", nil, nil);
        else
            [_tab_view selectNextTabViewItem:self];
    } else {
        [_tab_view selectNextTabViewItem:self];
    }
}


- (IBAction)call_Back:(id)sender {
    NSString *ident = _tab_view.selectedTabViewItem.identifier;
    if (![ident isEqualToString:@"drv"]) {
        [_tab_view selectPreviousTabViewItem:self];
    }
}

- (IBAction)call_Advanced:(id)sender {
    char *connstr = showKeyVal(self.window, _attrs, &_verify_conn);
    if (connstr) {
        if (_attrs)
            free(_attrs);
        _attrs = connstr;
    }
    [self.window makeKeyAndOrderFront:self.window];
}

- (IBAction)call_Browse:(id)sender {
    NSSavePanel *panel = [NSSavePanel savePanel];
  
    [panel setTitle:@"Save as ..."];
    [panel setNameFieldStringValue:@"xxx.dsn"];
    [panel setDirectoryURL:[NSURL fileURLWithPath:_fdsn_dir isDirectory:TRUE]];
    NSInteger rc = [panel runModal];
    if (rc==NSFileHandlingPanelOKButton){
        NSURL *dirURL = [panel directoryURL];
        [_fld_FDSN setStringValue:[NSString stringWithFormat:@"%@/%@", dirURL.path, panel.nameFieldStringValue]];
    }
    [self.window makeKeyAndOrderFront:self.window];
}
@end
