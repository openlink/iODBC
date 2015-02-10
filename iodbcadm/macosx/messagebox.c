/*
 *  messagebox.c
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

#include <gui.h>
#include <unicode.h>

wchar_t* convert_CFString_to_wchar(const CFStringRef str);
CFStringRef convert_wchar_to_CFString(wchar_t *str);

void
__create_message (HWND hwnd,
    SQLPOINTER dsn,
    SQLPOINTER text,
    SQLCHAR waMode,
    AlertType id)
{
  CFStringRef msg, msg1;
  DialogRef dlg;
  SInt16 out;
  char buf[1024];

  if (hwnd == NULL)
    return;

  if (waMode == 'A')
    {
      if (dsn)
        {
          STRCPY(buf, "DSN: ");
          STRCAT(buf, dsn);
          msg = CFStringCreateWithBytes (NULL, (unsigned char*)buf, STRLEN(buf),
            kCFStringEncodingUTF8, false);
          msg1 = CFStringCreateWithBytes (NULL, (unsigned char*)text, STRLEN(text), 
            kCFStringEncodingUTF8, false);
        }
      else
        {
          STRCPY(buf, "");
          msg = CFStringCreateWithBytes (NULL, (unsigned char*)text, STRLEN(text),
            kCFStringEncodingUTF8, false);
          msg1 = CFStringCreateWithBytes (NULL, (unsigned char*)buf, STRLEN(buf), 
            kCFStringEncodingUTF8, false);
        }
    }
  else
    {
      if (dsn)
        {
          WCSCPY(buf, L"DSN: ");
          WCSCAT(buf, dsn);
          msg = convert_wchar_to_CFString((wchar_t*)buf);
          msg1 = convert_wchar_to_CFString((wchar_t*)text);
        }
      else
        {
          WCSCPY(buf, L"");
          msg = convert_wchar_to_CFString((wchar_t*)text);
          msg1 = convert_wchar_to_CFString((wchar_t*)buf);
        }
    }

  CreateStandardAlert (id, msg, msg1, NULL, &dlg);
  RunStandardAlert (dlg, NULL, &out);

  CFRelease(msg);
  CFRelease(msg1);

  return;
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
