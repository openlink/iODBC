/*
 *  confirm.c
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

#define CONFYES_CNTL	3000
#define CONFNO_CNTL	3001
#define CONFTEXT_CNTL	3002

wchar_t* convert_CFString_to_wchar(const CFStringRef str);
CFStringRef convert_wchar_to_CFString(wchar_t *str);

pascal OSStatus
confirmadm_yes_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TCONFIRM *confirm_t = (TCONFIRM *) inUserData;

  if (confirm_t)
    {
      DisposeWindow (confirm_t->mainwnd);
      confirm_t->mainwnd = NULL;
      confirm_t->yes_no = true;
    }

  return noErr;
}

pascal OSStatus
confirmadm_no_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TCONFIRM *confirm_t = (TCONFIRM *) inUserData;

  if (confirm_t)
    {
      DisposeWindow (confirm_t->mainwnd);
      confirm_t->mainwnd = NULL;
      confirm_t->yes_no = false;
    }

  return noErr;
}

BOOL create_confirm_Internal (HWND hwnd,
    SQLPOINTER dsn,
    SQLPOINTER text,
    SQLCHAR waMode)
{
  EventTypeSpec controlSpec = { kEventClassControl, kEventControlHit };
  RgnHandle cursorRgn = NULL;
  ControlID controlID;
  ControlRef control;
  WindowRef wconfirm;
  TCONFIRM confirm_t;
  EventRecord event;
  IBNibRef nibRef;
  CFStringRef msg;
  OSStatus err;

  /* Search the bundle for a .nib file named 'odbcadmin'. */
  err =
      CreateNibReferenceWithCFBundle (CFBundleGetBundleWithIdentifier (CFSTR
	("org.iodbc.adm")), CFSTR ("confirmation"), &nibRef);
  if (err == noErr)
    {
      /* Nib found ... so create the window */
      CreateWindowFromNib (nibRef, CFSTR ("Dialog"), &wconfirm);
      DisposeNibReference (nibRef);
      /* Set the title with the DSN name */
      if (dsn)
	{
          if (waMode == 'A')
            msg = CFStringCreateWithBytes (NULL, (unsigned char*)dsn, STRLEN(dsn), 
              kCFStringEncodingUTF8, false);
          else
            msg = convert_wchar_to_CFString((wchar_t*)dsn);
          SetWindowTitleWithCFString (wconfirm, msg);
          CFRelease(msg);
	}
      /* Install handlers for the finish button, the cancel */
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, CONFYES_CNTL, wconfirm,
	  control);
      InstallEventHandler (GetControlEventTarget (control),
	  NewEventHandlerUPP (confirmadm_yes_clicked), 1, &controlSpec,
	  &confirm_t, NULL);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, CONFNO_CNTL, wconfirm,
	  control);
      InstallEventHandler (GetControlEventTarget (control),
	  NewEventHandlerUPP (confirmadm_no_clicked), 1, &controlSpec,
	  &confirm_t, NULL);
      /* Change the static field with the message */
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, CONFTEXT_CNTL, wconfirm,
	  control);
      if (waMode == 'A')
        SetControlData (control, 0, kControlEditTextTextTag, STRLEN (text), text);
      else
        {
          msg = convert_wchar_to_CFString((wchar_t*)text);
          SetControlData (control, 0, kControlEditTextCFStringTag, sizeof(CFStringRef), &msg);
          CFRelease(msg);
        }
      DrawOneControl (control);
      confirm_t.yes_no = FALSE;
      confirm_t.mainwnd = wconfirm;
      /* Show the window and run the loop */
      ShowWindow (wconfirm);
      /* The main loop */
      while (confirm_t.mainwnd)
	WaitNextEvent (everyEvent, &event, 60L, cursorRgn);
    }
  else
    goto error;

  return confirm_t.yes_no;

error:
  fprintf (stderr, "Can't load Window. Err: %d\n", (int) err);
  return confirm_t.yes_no;
}

BOOL create_confirm (HWND hwnd,
    LPCSTR dsn,
    LPCSTR text)
{
  return create_confirm_Internal (hwnd, (SQLPOINTER)dsn, (SQLPOINTER)text, 'A');
}

BOOL create_confirmw (HWND hwnd,
    LPCWSTR dsn,
    LPCWSTR text)
{
  return create_confirm_Internal (hwnd, (SQLPOINTER)dsn, (SQLPOINTER)text, 'W');
}
