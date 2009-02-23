/*
 *  errorbox.c
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
