/*
 *  IODBCProxy_SetupController.m
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

#import "IODBCProxy_SetupController.h"
#import "utils.h"

static char *STRCONN = "DSN=%s\0Description=%s\0\0";
static int STRCONN_NB_TOKENS = 2;


char* showSetup(char* dsn, char* attrs, BOOL addEnable)
{
    char *connstr = (char*) - 1L;
	@autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        
        IODBCProxy_SetupController *dlg = [[IODBCProxy_SetupController alloc] initWithAttrs:attrs];
        if (dsn)
            dlg.d_dsn = conv_char_to_NSString(dsn);
        dlg._addEnabled = addEnable;
        
        NSInteger rc = [app runModalForWindow:dlg.window];
        [dlg.window orderOut:dlg.window];
        if (rc == 1){
            char *cour, *curr;
            int i = 0, size = 0;
            char *val,*key;
            char *dsn,*comment;
            Size len;
            
            dsn = (char*)conv_NSString_to_char(dlg.d_dsn);
            comment = (char*)conv_NSString_to_char(dlg.d_comment);
            len = (dsn!=NULL)?strlen(dsn):0;
            size += len + strlen("DSN=") + 1;
            len = (comment!=NULL)?strlen(comment):0;
            size += len + strlen("Description=") + 1;
            
            // Malloc it
            if ((connstr = (char *) malloc (++size)))
            {
                for (curr = STRCONN, cour = connstr;
                     i < STRCONN_NB_TOKENS; i++, curr += (strlen(curr) + 1))
                {
                    switch (i)
                    {
                        case 0:
                            sprintf (cour, curr, dsn?dsn:"");
                            cour += (strlen(cour) + 1);
                            break;
                        case 1:
                            sprintf (cour, curr, comment?comment:"");
                            cour += (strlen(cour) + 1);
                            break;
                    }
                }
                
                for (i = 0; i < dlg.Attrs_list.count; i++)
                {
                    NSDictionary *row = [dlg.Attrs_list objectAtIndex:i];
                    NSString *nskey = (NSString*)[row valueForKey:@"key"];
                    if ([nskey isEqualToString:@"..."] || nskey.length==0)
                        continue;
                    key = (char*)conv_NSString_to_char(nskey);
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
                
                if (dsn!=NULL) free(dsn);
                if (comment!=NULL) free(comment);
            }
        }
        [dlg release];
        return connstr;
    }
}




#define MODE_ADD 1
#define MODE_VIEW 0
@interface IODBCProxy_SetupController ()

@end

@implementation IODBCProxy_SetupController
@synthesize Attrs_ArrController;
@synthesize fld_DSN;
@synthesize fld_Comment;
@synthesize btn_Add;
@synthesize btn_Remove;
@synthesize d_dsn, d_comment, _addEnabled;
@synthesize Attrs_list=_Attrs_list;

- (id)initWithAttrs:(const char*)attrs
{
    self = [super initWithWindowNibName:@"IODBCProxy_SetupController"];
    if (self) {
        self.Attrs_list = [NSMutableArray arrayWithCapacity:16];
        [self parse_attrs:attrs];
    }
    return self;
}

- (void)dealloc
{
    [d_dsn release];
    [d_comment release];
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
    
    for (curr = (char*) attrs; *curr; curr += (strlen(curr) + 1))
    {
        if (!strncasecmp (curr, "Description=", strlen("Description=")))
            self.d_comment = conv_char_to_NSString(curr + strlen("Description="));
        
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
    _curMode = MODE_VIEW;
    
    [[self window] center];  // Center the window.
    if (d_dsn!=nil){
        self.window.title = [NSString stringWithFormat:@"Setup of %@", d_dsn];
        self.fld_DSN.stringValue = d_dsn;
    }
    if (d_comment!=nil)
        self.fld_Comment.stringValue = d_comment;
//??    if (!_addEnabled) {
//??        [btn_Add setEnabled:NO];
//??        [btn_Remove setEnabled:NO];
//??    }
}


- (void)windowWillClose:(NSNotification*)notification
{
    [NSApp stopModalWithCode:_dialogCode];
}


- (IBAction)call_Ok:(id)sender {
    _dialogCode = 1;
    self.d_dsn = fld_DSN.stringValue;
    self.d_comment = fld_Comment.stringValue;
    [self.window close];
}

- (IBAction)call_Cancel:(id)sender {
    _dialogCode = 0;
    [self.window close];
}

@end
