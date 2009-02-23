/*
 *  loginbox.c
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

#define LBUSER_CNTL	5002
#define LBPASS_CNTL	5003
#define LBOK_CNTL	5000
#define LBCANCEL_CNTL	5001

extern char* convert_CFString_to_char(const CFStringRef str);
extern CFStringRef convert_char_to_CFString(char *str);

pascal OSStatus
login_ok_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TLOGIN *log_t = (TLOGIN *) inUserData;
  Size len;
  CFStringRef strRef;

  if (log_t)
    {
      /* Retrieve the user name */
      GetControlData (log_t->username, kControlEditTextPart,
        kControlEditTextCFStringTag, sizeof(CFStringRef), &strRef, &len);
      log_t->user = convert_CFString_to_char (strRef);

      /* Retrieve the password */
      GetControlData (log_t->password, 0,
        kControlEditTextPasswordCFStringTag, sizeof(CFStringRef), &strRef, &len);
      log_t->pwd = convert_CFString_to_char (strRef);

      log_t->username = log_t->password = NULL;
      log_t->ok = TRUE;
      DisposeWindow (log_t->mainwnd);
	  log_t->mainwnd = NULL;
    }

  return noErr;
}

pascal OSStatus
login_cancel_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TLOGIN *log_t = (TLOGIN *) inUserData;

  if (log_t)
    {
      log_t->user = log_t->pwd = NULL;
      log_t->username = log_t->password = NULL;
      log_t->ok = FALSE;
      DisposeWindow (log_t->mainwnd);
	  log_t->mainwnd = NULL;
    }

  return noErr;
}

void
create_login (HWND hwnd,
    LPCSTR username,
    LPCSTR password,
    LPCSTR dsn,
    TLOGIN * log_t)
{
  EventTypeSpec controlSpec = { kEventClassControl, kEventControlHit };
  RgnHandle cursorRgn = NULL;
  WindowRef wlogin;
  ControlRef control;
  ControlID controlID;
  EventRecord event;
  IBNibRef nibRef;
  OSStatus err;
  char msg[1024];

  if (hwnd == NULL)
    return;

  /* Search the bundle for a .nib file named 'odbcadmin'. */
  err =
      CreateNibReferenceWithCFBundle (CFBundleGetBundleWithIdentifier (CFSTR
	("org.iodbc.drvproxy")), CFSTR ("login"), &nibRef);
  if (err == noErr)
    {
      /* Nib found ... so create the window */
      CreateWindowFromNib (nibRef, CFSTR ("Dialog"), &wlogin);
      DisposeNibReference (nibRef);
      /* Set the title with the DSN */
      if (dsn)
        {
          msg[0] = STRLEN ("Login for ") + STRLEN(dsn);
          sprintf (msg+1, "Login for %s", (char*)dsn);
          SetWTitle (wlogin, msg);
	}
      /* Set the control into the structure */
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, LBUSER_CNTL, wlogin,
	  log_t->username);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, LBPASS_CNTL, wlogin,
          log_t->password);
      log_t->user = log_t->pwd = NULL;
      log_t->mainwnd = wlogin;
      /* Install handlers for the finish button, the cancel */
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, LBOK_CNTL, wlogin, control);
      InstallEventHandler (GetControlEventTarget (control),
	  NewEventHandlerUPP (login_ok_clicked), 1, &controlSpec, log_t,
	  NULL);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, LBCANCEL_CNTL, wlogin,
	  control);
      InstallEventHandler (GetControlEventTarget (control),
	  NewEventHandlerUPP (login_cancel_clicked), 1, &controlSpec, log_t,
	  NULL);

      SetControlData (log_t->username, 0, kControlEditTextTextTag,
        username ? STRLEN (username) : STRLEN(""),
        (UInt8 *) username ? username : "");
      SetControlData (log_t->password, 0, kControlEditTextPasswordTag,
        password ? STRLEN (password) : STRLEN(""),
        (UInt8 *) password ? password : "");

      /* Show the window and run the loop */
      AdvanceKeyboardFocus (wlogin);
      ShowWindow (wlogin);
      /* The main loop */
      while (log_t->mainwnd)
	{
	  switch (WaitNextEvent (everyEvent, &event, 60L, cursorRgn))
	    {
	    };
	}
    }
  else
    goto error;

  return;

error:
  fprintf (stderr, "Can't load Window. Err: %d\n", (int) err);
  return;
}
