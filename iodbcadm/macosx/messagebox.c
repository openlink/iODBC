/*
 *  messagebox.c
 *
 *  $Id$
 *
 *  The iODBC driver manager.
 *  
 *  Copyright (C) 1999-2009 by OpenLink Software <iodbc@openlinksw.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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
