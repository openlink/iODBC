/*
 *  IODBCadm_KeyValController.m
 *
 *  $Id$
 *
 *  The iODBC driver manager.
 *
 *  Copyright (C) 1996-2018 by OpenLink Software <iodbc@openlinksw.com>
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

#import "IODBCadm_KeyValController.h"
#import "utils.h"




@interface IODBCadm_KeyValController ()

@end

@implementation IODBCadm_KeyValController
@synthesize Attrs_ArrController;
@synthesize chk_Verify;
@synthesize Attrs_list=_Attrs_list;

- (id)initWithAttrs:(const char*)attrs
{
    self = [super initWithWindowNibName:@"IODBCadm_KeyValController"];
    if (self) {
        self.Attrs_list = [NSMutableArray arrayWithCapacity:16];
        [self parse_attrs:attrs];
    }
    return self;
}

- (void)dealloc
{
    [_Attrs_list removeAllObjects];
    [_Attrs_list release];
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

- (void)parse_attrs:(const char *)attrs
{
    char *curr, *cour;
    
    [_Attrs_list removeAllObjects];
    
    for (curr = (char*) attrs; *curr; curr += (strlen(curr) + 1))
    {
        if (!strncasecmp (curr, "DSN=", strlen("DSN=")) ||
            !strncasecmp (curr, "Driver=", strlen("Driver=")) ||
            !strncasecmp (curr, "Description=", strlen("Description=")))
            continue;
        
        if ((cour = strchr (curr, '=')))
        {
            NSString *key, *val;
            *cour = '\0';
            key = conv_char_to_NSString(curr);
            *cour = '=';
            val = conv_char_to_NSString(cour+1);
            [_Attrs_list addObject:[NSMutableDictionary dictionaryWithObjectsAndKeys:key!=nil?key:@"", @"key",
                                val!=nil?val:@"", @"val", nil]];
        }
        else{
            [_Attrs_list addObject:[NSMutableDictionary dictionaryWithObjectsAndKeys:@"", @"key", @"", @"val", nil]];
        }
    }
}

- (void)windowDidLoad
{
    [super windowDidLoad];
    
    _dialogCode = 0;
    
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

@end
