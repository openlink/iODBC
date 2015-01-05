/*
 *  translator.c
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

#include <sys/stat.h>
#include <unistd.h>

extern wchar_t* convert_CFString_to_wchar(const CFStringRef str);
extern CFStringRef convert_wchar_to_CFString(wchar_t *str);

UInt32 Translators_nrows;
CFStringRef Translators_array[5][100];

TTRANSLATORCHOOSER *TRANSLATORCHOOSER = NULL;

pascal OSStatus
translators_getset_item (ControlRef browser,
    DataBrowserItemID itemID, DataBrowserPropertyID property,
    DataBrowserItemDataRef itemData, Boolean changeValue)
{
  OSStatus err = noErr;

  if (!changeValue)
    switch (property)
      {
      case DBNAME_ID:
	SetDataBrowserItemDataText (itemData,
	    Translators_array[0][itemID - DBITEM_ID - 1]);
	break;

      case DBFILE_ID:
	SetDataBrowserItemDataText (itemData,
	    Translators_array[1][itemID - DBITEM_ID - 1]);
	break;

      case DBVERSION_ID:
	SetDataBrowserItemDataText (itemData,
	    Translators_array[2][itemID - DBITEM_ID - 1]);
	break;

      case DBSIZE_ID:
	SetDataBrowserItemDataText (itemData,
	    Translators_array[3][itemID - DBITEM_ID - 1]);
	break;

      case DBDATE_ID:
	SetDataBrowserItemDataText (itemData,
	    Translators_array[4][itemID - DBITEM_ID - 1]);
	break;

      case kDataBrowserItemIsActiveProperty:
	if (itemID > DBITEM_ID && itemID <= DBITEM_ID + Translators_nrows)
	  err = SetDataBrowserItemDataBooleanValue (itemData, true);
	break;

      case kDataBrowserItemIsEditableProperty:
	err = SetDataBrowserItemDataBooleanValue (itemData, true);
	break;

      case kDataBrowserItemIsContainerProperty:
	err =
	    SetDataBrowserItemDataBooleanValue (itemData,
	    itemID == DBITEM_ID);
	break;

      case kDataBrowserContainerAliasIDProperty:
	break;

      case kDataBrowserItemParentContainerProperty:
	break;

      default:
	err = errDataBrowserPropertyNotSupported;
	break;
      }
  else
    err = errDataBrowserPropertyNotSupported;

  return err;
}

void
translators_notification_item (ControlRef browser,
    DataBrowserItemID itemID, DataBrowserItemNotification message)
{
  switch (message)
    {
    case kDataBrowserItemDoubleClicked:
      translatorchooser_ok_clicked (NULL, NULL, TRANSLATORCHOOSER);
      break;
    };
}

void
addtranslators_to_list (ControlRef widget, WindowRef dlg)
{
  wchar_t *curr, *buffer = (wchar_t *) malloc (sizeof (wchar_t) * 65535);
  char _date[1024], _size[1024];
  wchar_t driver[1024];
  DataBrowserItemID item = DBITEM_ID + 1;
  DataBrowserCallbacks dbCallbacks;
  ThemeDrawingState outState = NULL;
  UInt16 colSize[5] = { 150, 150, 100, 50, 50 };
  SInt16 outBaseline;
  Point ioBound;
  UWORD confMode = ODBC_USER_DSN;
  CFStringRef szDriver;
  struct stat _stat;
  BOOL careabout;
  char *_drv_u8 = NULL;
  int len, i;


  if (!widget || !buffer)
    return;

  GetThemeDrawingState (&outState);

  /* Install an event handler on the component databrowser */
  dbCallbacks.version = kDataBrowserLatestCallbacks;
  InitDataBrowserCallbacks (&dbCallbacks);
  dbCallbacks.u.v1.itemNotificationCallback =
      NewDataBrowserItemNotificationUPP (translators_notification_item);
  /* On Mac OS X 10.0.x : clientDataCallback */
  dbCallbacks.u.v1.itemDataCallback =
      NewDataBrowserItemDataUPP (translators_getset_item);
  SetDataBrowserCallbacks (widget, &dbCallbacks);
  /* Begin the draw of the data browser */
  SetDataBrowserTarget (widget, DBITEM_ID);

  /* Make the clean up */
  for (i = 0; i < Translators_nrows; i++, item++)
    {
      CFRelease (Translators_array[0][i]);
      Translators_array[0][i] = NULL;
      CFRelease (Translators_array[1][i]);
      Translators_array[1][i] = NULL;
      CFRelease (Translators_array[2][i]);
      Translators_array[2][i] = NULL;
      CFRelease (Translators_array[3][i]);
      Translators_array[3][i] = NULL;
      CFRelease (Translators_array[4][i]);
      Translators_array[4][i] = NULL;
      RemoveDataBrowserItems (widget, DBITEM_ID, 1, &item, DBNAME_ID);
    }

  /* Global Initialization */
  Translators_nrows = 0;
  item = DBITEM_ID + 1;

  /* Get the current config mode */
  while (confMode != ODBC_SYSTEM_DSN + 1)
    {
      /* Get the list of drivers in the user context */
      SQLSetConfigMode (confMode);
      len = SQLGetPrivateProfileStringW (L"ODBC Translators", NULL, L"",
          buffer, 65535, L"odbcinst.ini");
      if (len)
	goto process;

      goto end;

    process:
      for (curr = buffer; *curr != L'\0'; curr += (WCSLEN (curr) + 1))
	{
	  /* Shadowing system odbcinst.ini */
	  for (i = 0, careabout = TRUE; i < Translators_nrows; i++)
	    {
	      szDriver = convert_wchar_to_CFString(curr);
	      if (CFStringCompare (Translators_array[0][i] ?
		      Translators_array[0][i] : CFSTR (""), szDriver,
		      kCFCompareCaseInsensitive) == kCFCompareEqualTo)
		{
		  careabout = FALSE;
		  break;
		}
	      CFRelease (szDriver);
	    }

	  if (!careabout)
	    continue;

	  SQLSetConfigMode (confMode);
	  SQLGetPrivateProfileStringW (L"ODBC Translators", curr, L"", driver,
	      sizeof (driver)/sizeof(wchar_t), L"odbcinst.ini");

	  /* Check if the driver is installed */
	  if (wcsncasecmp (driver, L"Installed", WCSLEN(L"Installed")))
	    goto end;

	  /* Get the driver library name */
	  SQLSetConfigMode (confMode);
	  if (!SQLGetPrivateProfileStringW (curr, L"Translator", L"", driver,
		  sizeof (driver) / sizeof(wchar_t), L"odbcinst.ini"))
	    {
	      SQLSetConfigMode (confMode);
	      SQLGetPrivateProfileStringW (L"Default", L"Translator", L"", driver,
		  sizeof (driver) / sizeof(wchar_t), L"odbcinst.ini");
	    }

	  if (WCSLEN (curr) && WCSLEN (driver))
	    {
              _drv_u8 = (char *) dm_SQL_WtoU8((SQLWCHAR*)driver, SQL_NTS);
              if (_drv_u8 == NULL)
                continue;

	      Translators_array[0][Translators_nrows] =
                convert_wchar_to_CFString(curr);
	      Translators_array[1][Translators_nrows] =
                convert_wchar_to_CFString(driver);
	      Translators_array[2][Translators_nrows] =
	        CFStringCreateWithCString (NULL, "", kCFStringEncodingUTF8);

	      /* Get some information about the driver */
	      if (!stat (_drv_u8, &_stat))
		{
                  struct tm drivertime;

                  localtime_r (&_stat.st_mtime, &drivertime);
                  strftime (_date, sizeof (_date), "%c", &drivertime);

		  sprintf (_size, "%d Kb", (int) (_stat.st_size / 1024));

		  Translators_array[3][Translators_nrows] =
		      CFStringCreateWithCString (NULL, _size,
		      kCFStringEncodingUTF8);
		  Translators_array[4][Translators_nrows] =
		      CFStringCreateWithCString (NULL, _date,
		      kCFStringEncodingUTF8);
		}

              MEM_FREE (_drv_u8);

              for(i = 0 ; i < 5 ; i++)
                {
                  GetThemeTextDimensions (Translators_array[i][Translators_nrows], kThemeSystemFont,
                    kThemeStateActive, false, &ioBound, &outBaseline);
                  if(colSize[i] < ioBound.h) colSize[i] = ioBound.h;
                }

              AddDataBrowserItems (widget, DBITEM_ID, 1, &item, DBNAME_ID);
	      item++;
	      Translators_nrows++;
	    }
	}

    end:
      if (confMode == ODBC_USER_DSN)
	confMode = ODBC_SYSTEM_DSN;
      else
	confMode = ODBC_SYSTEM_DSN + 1;
    }

  ActivateControl (widget);
  /* Resize the columns to have a good look */
  SetDataBrowserTableViewNamedColumnWidth (widget, DBNAME_ID, colSize[0] + 20);
  SetDataBrowserTableViewNamedColumnWidth (widget, DBFILE_ID, colSize[1] + 20);
  SetDataBrowserTableViewNamedColumnWidth (widget, DBVERSION_ID, colSize[2] + 20);
  SetDataBrowserTableViewNamedColumnWidth (widget, DBSIZE_ID, colSize[3] + 20);
  SetDataBrowserTableViewNamedColumnWidth (widget, DBDATE_ID, colSize[4] + 20);
  DrawOneControl (widget);
  /* Remove the DataBrowser callback */
  SetDataBrowserCallbacks (NULL, &dbCallbacks);
  if(outState) DisposeThemeDrawingState (outState);
  free (buffer);
}

pascal OSStatus
translatorchooser_ok_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TTRANSLATORCHOOSER *choose_t = (TTRANSLATORCHOOSER *) inUserData;
  DataBrowserItemID first, last;
  OSStatus err;

  if (choose_t)
    {
      /* Get the selection */
      if ((err = GetDataBrowserSelectionAnchor (choose_t->translatorlist,
        &first, &last)) == noErr)
	{
	  if (first > DBITEM_ID && first <= DBITEM_ID + Translators_nrows)
	    {
	      /* Get the driver name */
	      choose_t->translator =
                convert_CFString_to_wchar(Translators_array[0][first - DBITEM_ID - 1]);
	    }
	  else
	    choose_t->translator = NULL;
	}
      else
	choose_t->translator = NULL;

      DisposeWindow (choose_t->mainwnd);
	  choose_t->mainwnd = NULL;
      choose_t->translatorlist = NULL;
      Translators_nrows = 0;
    }

  return noErr;
}

pascal OSStatus
translatorchooser_cancel_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TTRANSLATORCHOOSER *choose_t = (TTRANSLATORCHOOSER *) inUserData;

  if (choose_t)
    {
      DisposeWindow (choose_t->mainwnd);
	  choose_t->mainwnd = NULL;
      Translators_nrows = 0;
      /* No driver choosen ... cancel pressed */
      choose_t->translator = NULL;
      choose_t->translatorlist = NULL;
    }

  return noErr;
}

void
create_translatorchooser (HWND hwnd, TTRANSLATORCHOOSER * choose_t)
{
  EventTypeSpec controlSpec = { kEventClassControl, kEventControlHit };
  RgnHandle cursorRgn = NULL;
  WindowRef wtranschooser;
  ControlID controlID;
  EventRecord event;
  IBNibRef nibRef;
  OSStatus err;

  if (hwnd == NULL)
    return;

  /* Search the bundle for a .nib file named 'odbcadmin'. */
  err =
      CreateNibReferenceWithCFBundle (CFBundleGetBundleWithIdentifier (CFSTR
	("org.iodbc.adm")), CFSTR ("odbcdriver"), &nibRef);
  if (err == noErr)
    {
      /* Nib found ... so create the window */
      CreateWindowFromNib (nibRef, CFSTR ("Dialog"), &wtranschooser);
      DisposeNibReference (nibRef);
      /* Set the control into the structure */
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, DCLIST_CNTL, wtranschooser,
	  choose_t->translatorlist);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, DCFINISH_CNTL, wtranschooser,
	  choose_t->b_finish);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, DCCANCEL_CNTL, wtranschooser,
	  choose_t->b_cancel);
      choose_t->translator = NULL;
      choose_t->mainwnd = wtranschooser;
      /* Install handlers for the finish button, the cancel */
      InstallEventHandler (GetControlEventTarget (choose_t->b_finish),
	  NewEventHandlerUPP (translatorchooser_ok_clicked), 1, &controlSpec,
	  choose_t, NULL);
      InstallEventHandler (GetControlEventTarget (choose_t->b_cancel),
	  NewEventHandlerUPP (translatorchooser_cancel_clicked), 1,
	  &controlSpec, choose_t, NULL);
      addtranslators_to_list (choose_t->translatorlist, wtranschooser);
      /* Show the window and run the loop */
      TRANSLATORCHOOSER = choose_t;
      ShowWindow (wtranschooser);
      /* The main loop */
      while (choose_t->mainwnd)
	WaitNextEvent (everyEvent, &event, 60L, cursorRgn);
    }
  else
    goto error;

  return;

error:
  choose_t->translator = NULL;
  fprintf (stderr, "Can't load Window. Err: %d\n", (int) err);
  return;
}
