/*
 *  IODBCadm_DrvConfigController.m
 *
 *  $Id$
 *
 *  The iODBC driver manager.
 *
 *  Copyright (C) 1996-2022 OpenLink Software <iodbc@openlinksw.com>
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
        self.drv_file = @"";
        self.setup_file = @"";
        self.Attrs_list = [NSMutableArray arrayWithCapacity:16];
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
            self.drv_file = conv_wchar_to_NSString(curr + wcslen(L"Driver="));
            continue;
        }
        
        if (!wcsncasecmp (curr, L"Setup=", wcslen(L"Setup=")))
        {
            self.setup_file = conv_wchar_to_NSString(curr + wcslen(L"Setup="));
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
    [self.window makeKeyAndOrderFront:self.window];
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
    [self.window makeKeyAndOrderFront:self.window];
}


@end
