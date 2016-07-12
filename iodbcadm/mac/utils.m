/*
 *  utils.m
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

#import "utils.h"
#import <gui.h>

#define OPL_W2A(XW, XA, SIZE)      wcstombs((char *) XA, (wchar_t *) XW, SIZE)
#define OPL_A2W(XA, XW, SIZE)      mbstowcs((wchar_t *) XW, (char *) XA, SIZE)


static char *str_W2A (const wchar_t *inStr)
{
    char *outStr = NULL;
    size_t len;
    
    if (inStr == NULL)
        return NULL;
    
    len = wcslen (inStr);
    
    if ((outStr = (char *) malloc (len*4 + 1)) != NULL)  /* for multi-byte encodings */
    {
        if (len > 0)
            OPL_W2A (inStr, outStr, len);
        outStr[len] = '\0';
    }
    return outStr;
}


static wchar_t* str_A2W (const char *inStr)
{
    wchar_t *outStr = NULL;
    size_t len;
    
    if (inStr == NULL)
        return NULL;
    
    len = strlen (inStr);
    
    if ((outStr = (wchar_t*) calloc (len + 1, sizeof (wchar_t))) != NULL)
    {
        if (len > 0)
            OPL_A2W (inStr, outStr, len);
        outStr[len] = L'\0';
    }
    return outStr;
}


NSString* conv_wchar_to_NSString(const wchar_t* str)
{
    if (!str)
        return nil;

    int num = 1;
    if(*(char *)&num == 1)
        return [[[NSString alloc] initWithBytes:str length:wcslen(str)*sizeof(wchar_t) encoding:NSUTF32LittleEndianStringEncoding] autorelease];
    else
        return [[[NSString alloc] initWithBytes:str length:wcslen(str)*sizeof(wchar_t) encoding:NSUTF32BigEndianStringEncoding] autorelease];
}

#if OLD_1
NSString* conv_wchar_to_NSString(const wchar_t* str)
{
    if (!str)
        return nil;

    CFMutableStringRef prov = CFStringCreateMutable(NULL, 0);
    CFIndex i;
    UniChar c;
    
    if(prov)
    {
        for(i = 0 ; str[i] != L'\0' ; i++)
        {
            c = (UniChar)str[i];
            CFStringAppendCharacters(prov, &c, 1);
        }
    }
    return (NSString*)prov;
}
#endif

wchar_t* conv_NSString_to_wchar(NSString* str)
{
    if (str == nil)
        return NULL;

    int len = str.length;
    wchar_t *prov = malloc(sizeof(wchar_t) * (len+1));
    CFIndex i;
    
    if(prov)
    {
        for(i = 0 ; i<len ; i++)
            prov[i] = CFStringGetCharacterAtIndex((CFStringRef)str, i);
        prov[i] = L'\0';
    }
    return prov;
}

char* conv_NSString_to_char(NSString* str)
{
    if (str == nil)
        return NULL;
    
    wchar_t *prov = conv_NSString_to_wchar (str);
    char *buffer = NULL;
    
    if (prov)
    {
        buffer = str_W2A(prov);
        free(prov);
    }
    return buffer;
}


NSString* conv_char_to_NSString(const char* str)
{
    if (!str)
        return nil;
    NSString *ret = nil;
    wchar_t *buffer = str_A2W(str);
    if (buffer)
    {
        ret = conv_wchar_to_NSString(buffer);
        free(buffer);
    }
    return ret;
}

NSString* conv_to_NSString(const void * str, char waMode)
{
    if (str) {
        if (waMode == 'A')
            return conv_char_to_NSString((const char*)str);
        else
            return conv_wchar_to_NSString((const wchar_t*)str);
    }
    else
        return nil;
}

static BOOL showConfirm(const void *title, const void *message, char waMode)
{
    @autoreleasepool {
        NSAlert *alert = [[[NSAlert alloc] init] autorelease];
        [alert addButtonWithTitle:@"Yes"]; /* first button */
        [alert addButtonWithTitle:@"No"];
        [alert setMessageText:(title?conv_to_NSString(title, waMode):@"")];
        [alert setInformativeText:(message?conv_to_NSString(message, waMode):@"")];
        [alert setAlertStyle:NSInformationalAlertStyle];
        BOOL rc = ([alert runModal] == NSAlertFirstButtonReturn);
        return rc;
        
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


static void create_error_Internal (void *hwnd, const void *dsn, const void *text, const void *errmsg, char waMode)
{
    if (hwnd == NULL)
        return;
    
    @autoreleasepool {
        NSString *title = conv_to_NSString(text, waMode);
        NSString *message = conv_to_NSString(errmsg, waMode);
        
        NSAlert *alert = [[[NSAlert alloc] init] autorelease];
        [alert setMessageText:title];
        [alert setInformativeText:message];
        [alert runModal];
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
        NSString *title = nil;
        NSString *message = nil;

        if (dsn)
        {
            title = [NSString stringWithFormat:@"DSN: %@", conv_to_NSString(dsn, waMode)];
            message = conv_to_NSString(text, waMode);
        }
        else
        {
            title = conv_to_NSString(text, waMode);
            message = @"";
        }

        NSAlert *alert = [[[NSAlert alloc] init] autorelease];
        [alert setMessageText:title];
        [alert setInformativeText:message];
        [alert runModal];
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




