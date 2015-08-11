/*
 *  IODBCadm_ConfirmController.m
 *
 *  $Id$
 *
 *  The iODBC driver manager.
 *
 *  Copyright (C) 1996-2015 by OpenLink Software <iodbc@openlinksw.com>
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

#import "IODBCadm_ConfirmController.h"
#import "utils.h"

static void create_error_Internal (void *hwnd, const void *dsn, const void *text, const void *errmsg, char waMode)
{
    if (hwnd == NULL)
        return;

	@autoreleasepool {
//        NSApplication *app = [NSApplication sharedApplication];
    
        NSString *title = [conv_to_NSString(text, waMode) autorelease];
        NSString *message = [conv_to_NSString(errmsg, waMode) autorelease];
        
        NSRunAlertPanel(title, message, @"OK", nil, nil);
    }
}

void
create_error (HWND hwnd,
    LPCSTR dsn,
    LPCSTR text,
    LPCSTR errmsg)
{
  create_error_Internal (hwnd, (SQLPOINTER)dsn, (SQLPOINTER)text, (SQLPOINTER)errmsg, 'A');
}

void
create_errorw (HWND hwnd,
    LPCWSTR dsn,
    LPCWSTR text,
    LPCWSTR errmsg)
{
  create_error_Internal (hwnd, (SQLPOINTER)dsn, (SQLPOINTER)text, (SQLPOINTER)errmsg, 'W');
}


static void __create_message (void* hwnd, const void *dsn, const void *text, char waMode, int alertType)
{
    if (hwnd == NULL)
        return;
    
	@autoreleasepool {
//        NSApplication *app = [NSApplication sharedApplication];
        
        NSString *title = nil;
        NSString *message = nil;

        if (dsn)
        {
            title = [NSString stringWithFormat:@"DSN: %@", conv_to_NSString(dsn, waMode)];
            message = [conv_to_NSString(text, waMode) autorelease];
        }
        else
        {
            title = [conv_to_NSString(text, waMode) autorelease];
            message = @"";
        }
        
        NSRunAlertPanel(title, message, @"OK", nil, nil);
    }
}


void
create_message (HWND hwnd,
    LPCSTR dsn,
    LPCSTR text)
{
  __create_message (hwnd, (SQLPOINTER)dsn, (SQLPOINTER)text, 'A', kAlertStopAlert);
}

void
create_messagew (HWND hwnd,
    LPCWSTR dsn,
    LPCWSTR text)
{
  __create_message (hwnd, (SQLPOINTER)dsn, (SQLPOINTER)text, 'W', kAlertStopAlert);
}


static BOOL showConfirm(const void *title, const void *message, char waMode)
{
    /**
     WindowRef *wref = nil;
     NSWindow *win = [[[NSWindow alloc] initWithWindowRef:wref] retain];
     **/
	@autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        
        IODBCadm_ConfirmController *dlg = [[IODBCadm_ConfirmController alloc] init];
        dlg.d_title = [conv_to_NSString(title, waMode) autorelease];
        dlg.d_message = [conv_to_NSString(message, waMode) autorelease];
        
        NSInteger rc = [app runModalForWindow:dlg.window];
        [dlg.window orderOut:dlg.window];
        [dlg release];
        
        return rc==1?TRUE:FALSE;
    }
}


BOOL create_confirm (HWND hwnd,
                     LPCSTR dsn,
                     LPCSTR text)
{
    return showConfirm(dsn, text, 'A');
}

BOOL create_confirmw (HWND hwnd,
                      LPCWSTR dsn,
                      LPCWSTR text)
{
    return showConfirm(dsn, text, 'W');
}


@interface IODBCadm_ConfirmController ()

@end

@implementation IODBCadm_ConfirmController
@synthesize lb_message;
@synthesize d_title;
@synthesize d_message;


- (id)init
{
    return [super initWithWindowNibName:@"IODBCadm_ConfirmController"];
}

- (void)dealloc
{
    [d_title release];
    [d_message release];
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

- (void)awakeFromNib
{
    [super awakeFromNib];
}

- (void)windowDidLoad
{
    [super windowDidLoad];
    _dialogCode = 0;
    
    [[self window] center];  // Center the window.
    if (d_title!=nil)
        self.window.title = d_title;
    if (d_message!=nil)
        self.lb_message.stringValue = d_message;
}

- (void)windowWillClose:(NSNotification*)notification
{
    [NSApp stopModalWithCode:_dialogCode];
}


- (IBAction)call_Yes:(id)sender {
    _dialogCode = 1;
    [self.window close];
}

- (IBAction)call_No:(id)sender {
    _dialogCode = 0;
    [self.window close];
}
@end
