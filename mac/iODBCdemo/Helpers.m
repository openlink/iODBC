/*
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

#import "Helpers.h"
#include <iODBC/isql.h>
#include <iODBCinst/iodbcinst.h>



#ifdef UNICODE
wchar_t *
NStoTEXT (NSString * str)
{
    wchar_t *prov;
    unsigned int len, i;
    
    if (str == nil)
        return NULL;
    
    len = [str length];
    prov = malloc (sizeof (wchar_t) * (len + 1));
    
    if (prov)
    {
        for (i = 0; i < len; i++)
            prov[i] = [str characterAtIndex:i];
        prov[i] = L'\0';
    }
    
    return prov;
}


NSString*
TEXTtoNS(wchar_t* str)
{
    if (!str)
        return nil;
    
    if (NSHostByteOrder()==NS_LittleEndian)
        return [[[NSString alloc] initWithBytes:str length:wcslen(str)*sizeof(wchar_t) encoding:NSUTF32LittleEndianStringEncoding] autorelease];
    else
        return [[[NSString alloc] initWithBytes:str length:wcslen(str)*sizeof(wchar_t) encoding:NSUTF32BigEndianStringEncoding] autorelease];
}

#endif

void
_nativeerrorbox (SQLHENV _henv, SQLHDBC _hdbc, SQLHSTMT _hstmt)
{
    SQLTCHAR buf[4096];
    SQLTCHAR sqlstate[15];
    
    /*
     * Get statement errors
     */
    if (SQLError (_henv, _hdbc, _hstmt, sqlstate, NULL,
                  buf, sizeof (buf)/sizeof(SQLTCHAR), NULL) == SQL_SUCCESS)
        NSRunAlertPanel(@"Native ODBC Error",
                        [NSString stringWithFormat:@"%@ [%@]", TEXTtoNS(buf), TEXTtoNS(sqlstate)], NULL, NULL, NULL);
    
    /*
     * Get connection errors
     */
    if (SQLError (_henv, _hdbc, SQL_NULL_HSTMT, sqlstate,
                  NULL, buf, sizeof (buf)/sizeof(SQLTCHAR), NULL) == SQL_SUCCESS)
        NSRunAlertPanel(@"Native ODBC Error",
                        [NSString stringWithFormat:@"%@ [%@]", TEXTtoNS(buf), TEXTtoNS(sqlstate)], NULL, NULL, NULL);
    
    /*
     * Get environmental errors
     */
    if (SQLError (_henv, SQL_NULL_HDBC, SQL_NULL_HSTMT,
                  sqlstate, NULL, buf, sizeof (buf)/sizeof(SQLTCHAR), NULL) == SQL_SUCCESS)
        NSRunAlertPanel(@"Native ODBC Error",
                        [NSString stringWithFormat:@"%@ [%@]", TEXTtoNS(buf), TEXTtoNS(sqlstate)], NULL, NULL, NULL);
}




void create_error(const char *text, const char *errmsg)
{
    NSString *title = [NSString stringWithUTF8String:text];
    NSString *message = [NSString stringWithUTF8String:errmsg];
        
    NSRunAlertPanel(title, message, @"OK", nil, nil);
}







