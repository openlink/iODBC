/*
 *  main.c
 *
 *  $Id$
 *
 *  Main program
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

#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

#include <gui.h>

Boolean manage_return = false;

pascal OSStatus
about_ok_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  WindowPtr about = (WindowPtr) *((void**)inUserData);

  if (about)
  {
    DisposeWindow (about);
    *((void**)inUserData) = NULL;
  }

  return noErr;
}

#define OK_CNTL	140

pascal OSStatus
HandleMenuCommand (EventHandlerCallRef referee, EventRef inEvent, void *data)
{
  EventTypeSpec controlSpec = { kEventClassControl, kEventControlHit };
  OSStatus result = noErr, err;
  RgnHandle cursorRgn = NULL;
  HICommand command;
  IBNibRef nibRef;
  WindowPtr about;
  ControlID controlID;
  ControlRef control;
  EventRecord event;

  switch (GetEventClass (inEvent))
    {
    case kEventClassCommand:
      /* Extract the command from the event */
      GetEventParameter (inEvent, kEventParamDirectObject, typeHICommand,
	  NULL, sizeof (command), NULL, &command);
      check (command.attributes & kHICommandFromMenu);

      /* Treat the command */
      switch (command.commandID)
	{
	case 'abou':
	  /* Search the bundle for a .nib file named 'about'. */
	  err = CreateNibReference (CFSTR ("about"), &nibRef);
	  if (err == noErr)
	    {
	      /* Nib found ... so create the window */
	      CreateWindowFromNib (nibRef, CFSTR ("Window"), &about);
	      DisposeNibReference (nibRef);
              GETCONTROLBYID (controlID, CNTL_SIGNATURE, OK_CNTL, about,
	        control);
              InstallEventHandler (GetControlEventTarget (control),
	        NewEventHandlerUPP (about_ok_clicked), 1, &controlSpec,
	        &about, NULL);
	      /* Show the window */
	      ShowWindow (about);
	      SetPort ((GrafPtr) GetWindowPort (about));
              /* The main loop */
              while (about)
	        WaitNextEvent (everyEvent, &event, 60L, cursorRgn);
	    }

	  break;

	default:
	  result = eventNotHandledErr;
	};

      break;

    default:
      result = eventNotHandledErr;
    };

  return result;
}

void *
main_thread (void *threadParam)
{
  RunApplicationEventLoop ();
  return NULL;
}

pascal OSErr
QuitEventHandler (const AppleEvent * theEvent, AppleEvent * theReply,
    SInt32 refCon)
{
  QuitApplicationEventLoop ();
  kill(getpid(), SIGKILL);
  return noErr;
}

int
macosx_gui (int *argc, char **argv[])
{
  EventTypeSpec menuEvents[] = { {kEventClassCommand, kEventCommandProcess} };
  WindowRef mainwnd = (WindowRef) (-1L);
  pthread_attr_t pattr;
  pthread_t thread_id;
  IBNibRef nibRef;
  OSStatus err;
  MenuRef menu;

  InitCursor ();

  AEInstallEventHandler (kCoreEventClass, kAEQuitApplication,
      NewAEEventHandlerUPP (QuitEventHandler), 0L, false);

  /* Search the bundle for a .nib file named 'odbcadmin'. */
  err = CreateNibReference (CFSTR ("main"), &nibRef);
  if (err == noErr)
    {
      /* Load the menu bar */
      err = SetMenuBarFromNib (nibRef, CFSTR ("MainMenu"));
      if (err != noErr)
	{
	  fprintf (stderr, "Can't load MenuBar. Err: %d\n", (int) err);
	  ExitToShell ();
	}
      /* And no need anymore the nib */
      DisposeNibReference (nibRef);
    }
  else
    goto error;

  /* Install the menu handler */
  menu = GetMenuHandle (128);
  InstallMenuEventHandler (menu, NewEventHandlerUPP (HandleMenuCommand),
      sizeof (menuEvents), menuEvents, NULL, NULL);

  /* Install a thread */
  pthread_attr_init (&pattr);
  pthread_attr_setdetachstate (&pattr, PTHREAD_CREATE_DETACHED);
  pthread_create (&thread_id, &pattr, main_thread, mainwnd);

  manage_return = SQLManageDataSources (mainwnd);

  pthread_attr_destroy (&pattr);

  return manage_return;

error:
  fprintf (stderr, "Can't load Window. Err: %d\n", (int) err);
  ExitToShell ();

  return 0;
}

void
display_help (void)
{
  printf ("-help\t\t\tDisplay the list of options.\n\r");
  printf ("-odbc filename\t\tSet the location of the user ODBC.INI file.\n\r");
  printf ("-odbcinst filename\tSet the location of the user ODBCINST.INI file.\n\r");
  printf ("-sysodbc filename\tSet the location of the system ODBC.INI file.\n\r");
  printf ("-sysodbcinst filename\tSet the location of the system ODBCINST.INI file.\n\r");
  printf ("-debug\t\t\tThe error messages are displayed on the console.\n\r");
  printf ("-admin odbcinstfile\tUsed to administrate the system odbcinst.ini file.\n\r\n\r");
  printf ("-odbcfiledsn filename\tSet the location of the default File DSN directory.\n\r");
  _exit (1);
}

int
main (int argc, char *argv[])
{
  BOOL debug = FALSE;
  char *gui = NULL;
  int i = 1;

  setlocale (LC_ALL, "");	/* Use native locale */

  printf ("OpenLink iODBC Administrator (Mac OS X)\n");
  printf ("iODBC Driver Manager %s\n", VERSION);
  printf ("Copyright (C) 2000-2015 OpenLink Software\n");
  printf ("Please report all bugs to <iodbc@openlinksw.com>\n");

  /* Check options commands */
  if (argc > 1)
    {
      for (; i < argc; i++)
	{
	  if (!strcasecmp (argv[i], "-help"))
	    display_help ();

	  if (!strcasecmp (argv[i], "-debug"))
	    debug = TRUE;

	  if (!strcasecmp (argv[i], "-odbc"))
	    {
	      if (i + 1 >= argc)
		display_help ();
	      setenv ("ODBCINI", argv[++i], TRUE);
	    }

	  if (!strcasecmp (argv[i], "-admin"))
	    {
	      if (i + 1 >= argc)
		display_help ();
	      setenv ("ODBCINSTINI", argv[++i], TRUE);
	      setenv ("SYSODBCINSTINI", argv[i], TRUE);
	    }

	  if (!strcasecmp (argv[i], "-odbcinst"))
	    {
	      if (i + 1 >= argc)
		display_help ();
	      setenv ("ODBCINSTINI", argv[++i], TRUE);
	    }

	  if (!strcasecmp (argv[i], "-sysodbc"))
	    {
	      if (i + 1 >= argc)
		display_help ();
	      setenv ("SYSODBCINI", argv[++i], TRUE);
	    }

	  if (!strcasecmp (argv[i], "-sysodbcinst"))
	    {
	      if (i + 1 >= argc)
		display_help ();
	      setenv ("SYSODBCINSTINI", argv[++i], TRUE);
	    }

	  if (!strcasecmp (argv[i], "-gui"))
	    {
	      if (i + 2 >= argc)
		display_help ();
	      gui = argv[++i];
	    }

	  if (!strcasecmp (argv[i], "-odbcfiledsn"))
	    {
	      if (i + 1 >= argc)
		display_help ();
	      setenv ("ODBCFILEDSN", argv[++i], TRUE);
	    }
	}
    }

  if (!debug)
    {
#ifndef __APPLE__
      close (STDOUT_FILENO);
      close (STDERR_FILENO);
#endif
    }

  return macosx_gui (&argc, &argv);
}
