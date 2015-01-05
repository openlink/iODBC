/*
 *  errorbox.c
 *
 *  $Id$
 *
 *  The iODBC driver manager.
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

#include <gui.h>
#include <unicode.h>

wchar_t* convert_CFString_to_wchar(const CFStringRef str)
{
  wchar_t *prov = malloc(sizeof(wchar_t) * (CFStringGetLength(str)+1));
  CFIndex i;
  
  if(prov)
    {
      for(i = 0 ; i<CFStringGetLength(str) ; i++)
        prov[i] = CFStringGetCharacterAtIndex(str, i);
      prov[i] = L'\0';
    }

  return prov;
}

char* convert_CFString_to_char(const CFStringRef str)
{
  wchar_t *prov = convert_CFString_to_wchar (str);
  char *buffer = NULL;

  if (prov)
    {
      buffer = dm_SQL_W2A (prov, SQL_NTS);
      free(prov);
    }

  return buffer;
}

CFStringRef convert_wchar_to_CFString(wchar_t *str)
{
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
  
  return prov;
}

void
create_error_Internal (HWND hwnd,
    SQLPOINTER dsn,
    SQLPOINTER text,
    SQLPOINTER errmsg,
    SQLCHAR waMode)
{
  CFStringRef msg, msg1;
  DialogRef dlg;
  SInt16 out;

  if (hwnd == NULL)
    return;

  if (waMode == 'A')
    {
      msg = CFStringCreateWithBytes (NULL, (unsigned char*)text, STRLEN(text), 
        kCFStringEncodingUTF8, false);
      msg1 = CFStringCreateWithBytes (NULL, (unsigned char*)errmsg, STRLEN(errmsg), 
        kCFStringEncodingUTF8, false);
    }
  else
    {
      msg = convert_wchar_to_CFString((wchar_t*)text);
      msg1 = convert_wchar_to_CFString((wchar_t*)errmsg);
    }

  CreateStandardAlert (kAlertStopAlert, msg, msg1, NULL, &dlg);
  RunStandardAlert (dlg, NULL, &out);

  CFRelease(msg);
  CFRelease(msg1);

  return;
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
