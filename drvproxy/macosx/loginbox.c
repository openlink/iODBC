/*
 *  loginbox.c
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
