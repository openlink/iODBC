/*
 *  IODBCProxy_LoginController.m
 *
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

#import "IODBCProxy_LoginController.h"
#import "utils.h"

BOOL showLogin(const char* title, const char *username, const char *password, TLOGIN *log_t)
{
        if (log_t) {
            log_t->user = NULL;
            log_t->pwd = NULL;
            log_t->ok = 0;
        }

	@autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        
        IODBCProxy_LoginController *dlg = [[IODBCProxy_LoginController alloc] init];
        if (title)
            dlg.d_title = conv_char_to_NSString(title);
        if (username)
            dlg.d_uid = conv_char_to_NSString(username);
        if (password)
            dlg.d_pwd = conv_char_to_NSString(password);
        [dlg.window makeKeyAndOrderFront:nil];
        
        NSInteger rc = [app runModalForWindow:dlg.window];
        [dlg.window orderOut:dlg.window];
        if (log_t && rc == 1){
            log_t->user = (char*)conv_NSString_to_char(dlg.d_uid);
            log_t->pwd = (char*)conv_NSString_to_char(dlg.d_pwd);
            log_t->ok = 1;
        }
        [dlg release];
        return rc == 1 ? TRUE : FALSE;
    }
    
}



@interface IODBCProxy_LoginController ()

@end

@implementation IODBCProxy_LoginController
@synthesize fld_UID;
@synthesize fld_PWD;
@synthesize d_title, d_uid, d_pwd;

- (id)init
{
    return [super initWithWindowNibName:@"IODBCProxy_LoginController"];
}

- (void)dealloc
{
    [d_title release];
    [d_uid release];
    [d_pwd release];
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
    if (d_title!=nil)
        self.window.title = d_title;
    if (d_uid!=nil)
        self.fld_UID.stringValue = d_uid;
    if (d_pwd!=nil)
        self.fld_PWD.stringValue = d_pwd;
}

- (void)windowWillClose:(NSNotification*)notification
{
    [NSApp stopModalWithCode:_dialogCode];
}


- (IBAction)call_OK:(id)sender {
    _dialogCode = 1;
    self.d_uid = fld_UID.stringValue;
    self.d_pwd = fld_PWD.stringValue;
    [self.window close];
}

- (IBAction)call_Cancel:(id)sender {
    _dialogCode = 0;
    [self.window close];
}

@end
