/*
 *  driverchooser.c
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

#include "dlf.h"
#include "dlproc.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unicode.h>

extern wchar_t* convert_CFString_to_wchar(const CFStringRef str);
extern CFStringRef convert_wchar_to_CFString(wchar_t *str);

UInt32 Drivers_nrows;
CFStringRef Drivers_array[5][100];

TDRIVERCHOOSER *DRIVERCHOOSER = NULL;

static pascal OSStatus
drivers_getset_item (ControlRef browser,
    DataBrowserItemID itemID, DataBrowserPropertyID property,
    DataBrowserItemDataRef itemData, Boolean changeValue)
{
  OSStatus err = noErr;

  if (!changeValue)
    switch (property)
      {
      case DBNAME_ID:
	SetDataBrowserItemDataText (itemData,
	    Drivers_array[0][itemID - DBITEM_ID - 1]);
	break;

      case DBFILE_ID:
	SetDataBrowserItemDataText (itemData,
	    Drivers_array[1][itemID - DBITEM_ID - 1]);
	break;

      case DBVERSION_ID:
	SetDataBrowserItemDataText (itemData,
	    Drivers_array[2][itemID - DBITEM_ID - 1]);
	break;

      case DBSIZE_ID:
	SetDataBrowserItemDataText (itemData,
	    Drivers_array[3][itemID - DBITEM_ID - 1]);
	break;

      case DBDATE_ID:
	SetDataBrowserItemDataText (itemData,
	    Drivers_array[4][itemID - DBITEM_ID - 1]);
	break;

      case kDataBrowserItemIsActiveProperty:
	if (itemID > DBITEM_ID && itemID <= DBITEM_ID + Drivers_nrows)
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
drivers_notification_item (ControlRef browser,
    DataBrowserItemID itemID, DataBrowserItemNotification message)
{
  static Boolean ignore_next = false, selected = false;

  switch (message)
    {
    case kDataBrowserItemSelected:
      if (DRIVERCHOOSER->b_configure)
        {
          ActivateControl (DRIVERCHOOSER->b_remove);
          DrawOneControl (DRIVERCHOOSER->b_remove);
          ActivateControl (DRIVERCHOOSER->b_configure);
          DrawOneControl (DRIVERCHOOSER->b_configure);
        }
      else
        {
          ActivateControl (DRIVERCHOOSER->b_add);
          DrawOneControl (DRIVERCHOOSER->b_add);
        }

      if (selected)
	ignore_next = true;
      else
	selected = true;
      break;

    case kDataBrowserItemDeselected:
      if (!ignore_next)
	{
          if (DRIVERCHOOSER->b_configure)
            {
	      DeactivateControl (DRIVERCHOOSER->b_remove);
	      DrawOneControl (DRIVERCHOOSER->b_remove);
	      DeactivateControl (DRIVERCHOOSER->b_configure);
	      DrawOneControl (DRIVERCHOOSER->b_configure);
            }
      else
        {
          DeactivateControl (DRIVERCHOOSER->b_add);
          DrawOneControl (DRIVERCHOOSER->b_add);
        }

	  selected = false;
	}
      else
	{
	  ignore_next = false;
	  selected = true;
	}
      break;

    case kDataBrowserItemDoubleClicked:
      if (DRIVERCHOOSER->b_configure)
        driver_configure_clicked (NULL, NULL, DRIVERCHOOSER);
      break;
    };
}

/* Changed ... Have to implement the same for translators */
void
adddrivers_to_list (ControlRef widget, WindowRef dlg, BOOL addNotify)
{
  wchar_t drvdesc[1024], drvattrs[1024], driver[1024];
  DataBrowserItemID item = DBITEM_ID + 1;
  DataBrowserCallbacks dbCallbacks;
  ThemeDrawingState outState = NULL;
  UInt16 colSize[5] = { 150, 150, 100, 50 , 50};
  void *handle;
  SInt16 outBaseline;
  Point ioBound;
  struct stat _stat;
  SQLSMALLINT len, len1;
  SQLRETURN ret;
  HENV henv, drv_henv;
  HDBC drv_hdbc;
  pSQLGetInfoFunc funcHdl;
  pSQLAllocHandle allocHdl;
  pSQLAllocEnv allocEnvHdl = NULL;
  pSQLAllocConnect allocConnectHdl = NULL;
  pSQLFreeHandle freeHdl;
  pSQLFreeEnv freeEnvHdl;
  pSQLFreeConnect freeConnectHdl;
  char *_drv_u8 = NULL;
  int i;

  if (!widget)
    return;

  GetThemeDrawingState (&outState);

  /* Install an event handler on the component databrowser */
  dbCallbacks.version = kDataBrowserLatestCallbacks;
  InitDataBrowserCallbacks (&dbCallbacks);

  if (addNotify)
    dbCallbacks.u.v1.itemNotificationCallback =
      NewDataBrowserItemNotificationUPP (drivers_notification_item);

  /* On Mac OS X 10.0.x : clientDataCallback */
  dbCallbacks.u.v1.itemDataCallback =
      NewDataBrowserItemDataUPP (drivers_getset_item);
  SetDataBrowserCallbacks (widget, &dbCallbacks);
  /* Begin the draw of the data browser */
  SetDataBrowserTarget (widget, DBITEM_ID);

  /* Make the clean up */
  for (i = 0; i < Drivers_nrows; i++, item++)
    {
      CFRelease (Drivers_array[0][i]);
      Drivers_array[0][i] = NULL;
      CFRelease (Drivers_array[1][i]);
      Drivers_array[1][i] = NULL;
      CFRelease (Drivers_array[2][i]);
      Drivers_array[2][i] = NULL;
      CFRelease (Drivers_array[3][i]);
      Drivers_array[3][i] = NULL;
      CFRelease (Drivers_array[4][i]);
      Drivers_array[4][i] = NULL;
      RemoveDataBrowserItems (widget, DBITEM_ID, 1, &item, DBNAME_ID);
    }

  /* Global Initialization */
  Drivers_nrows = 0;
  item = DBITEM_ID + 1;

  /* Create a HENV to get the list of drivers then */
  ret = SQLAllocHandle (SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
  if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
    {
      _iodbcdm_nativeerrorbox (dlg, henv, SQL_NULL_HANDLE, SQL_NULL_HANDLE);
      goto end;
    }

  /* Set the version ODBC API to use */
  SQLSetEnvAttr (henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER) SQL_OV_ODBC3,
      SQL_IS_UINTEGER);

  /* Get the list of drivers */
  ret =
      SQLDriversW (henv, SQL_FETCH_FIRST, drvdesc, sizeof (drvdesc)/sizeof(wchar_t),
        &len, drvattrs, sizeof (drvattrs)/sizeof(wchar_t), &len1);
  if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO && ret != SQL_NO_DATA)
    {
      _iodbcdm_nativeerrorbox (dlg, henv, SQL_NULL_HANDLE, SQL_NULL_HANDLE);
      goto error;
    }

  while (ret != SQL_NO_DATA)
    {
      Drivers_array[0][Drivers_nrows] = convert_wchar_to_CFString(drvdesc);

      /* Get the driver library name */
      SQLSetConfigMode (ODBC_BOTH_DSN);
      SQLGetPrivateProfileStringW (drvdesc, L"Driver", L"", driver,
	  sizeof (driver)/sizeof(wchar_t), L"odbcinst.ini");
      if (driver[0] == L'\0')
	SQLGetPrivateProfileStringW (L"Default", L"Driver", L"", driver,
	    sizeof (driver)/sizeof(wchar_t), L"odbcinst.ini");
      if (driver[0] == L'\0')
	{
	  if (Drivers_array[0][Drivers_nrows])
            {
              CFRelease (Drivers_array[0][Drivers_nrows]);
              Drivers_array[0][Drivers_nrows] = NULL;
            }
	  goto skip;
	}

      Drivers_array[1][Drivers_nrows] = convert_wchar_to_CFString(driver);

      /* Alloc a connection handle */
      drv_hdbc = NULL;
      drv_henv = NULL;

      _drv_u8 = (char *) dm_SQL_WtoU8((SQLWCHAR*)driver, SQL_NTS);
      if (_drv_u8 == NULL)
        goto skip;

      if ((handle = DLL_OPEN(_drv_u8)) != NULL)
        {
          if ((allocHdl = (pSQLAllocHandle)DLL_PROC(handle, "SQLAllocHandle")) != NULL)
            {
              ret = allocHdl(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &drv_henv);
	      if (ret == SQL_ERROR) goto nodriverver;
              ret = allocHdl(SQL_HANDLE_DBC, drv_henv, &drv_hdbc);
	      if (ret == SQL_ERROR) goto nodriverver;
            }
          else
            {
              if ((allocEnvHdl = (pSQLAllocEnv)DLL_PROC(handle, "SQLAllocEnv")) != NULL)
                {
                  ret = allocEnvHdl(&drv_henv);
	          if (ret == SQL_ERROR) goto nodriverver;
                }
              else goto nodriverver;

              if ((allocConnectHdl = (pSQLAllocConnect)DLL_PROC(handle, "SQLAllocConnect")) != NULL)
                {
                  ret = allocConnectHdl(drv_henv, &drv_hdbc);
	          if (ret == SQL_ERROR) goto nodriverver;
                }
              else goto nodriverver;
            }

	  /*
	   *  Use SQLGetInfoA for Unicode drivers
	   *  and SQLGetInfo  for ANSI drivers
	   */
          funcHdl = (pSQLGetInfoFunc)DLL_PROC(handle, "SQLGetInfoA");
	  if (!funcHdl)
	      funcHdl = (pSQLGetInfoFunc)DLL_PROC(handle, "SQLGetInfo");

	  if (funcHdl)
            {
              /* Retrieve some informations */
              ret = funcHdl (drv_hdbc, SQL_DRIVER_VER, drvattrs, sizeof(drvattrs), &len);
              if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
                {
		  char *p = drvattrs;

                  /* Find the description if one provided */
		  for (; *p ; p++) 
		    {
		      if (*p == ' ')
		        {
			  *p++ = '\0';
			  break;
			}
		    }

		  /*
		   * Store Version
		   */
                  Drivers_array[2][Drivers_nrows] = CFStringCreateWithCString(NULL, (char *)drvattrs, kCFStringEncodingUTF8);
                }
              else goto nodriverver;
            }
	  else if ((funcHdl = (pSQLGetInfoFunc)DLL_PROC(handle, "SQLGetInfoW")) != NULL) 
	    {
              /* Retrieve some informations */
              ret = funcHdl (drv_hdbc, SQL_DRIVER_VER, drvattrs, sizeof(drvattrs), &len);
              if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
                {
		  wchar_t *p = drvattrs;

                  /* Find the description if one provided */
		  for (; *p ; p++) 
		    {
		      if (*p == L' ')
		        {
			  *p++ = L'\0';
			  break;
			}
		    }

		  /*
		   * Store Version
		   */
                  Drivers_array[2][Drivers_nrows] = convert_wchar_to_CFString(drvattrs);
                }
              else goto nodriverver;
	        
	    }
          else goto nodriverver;
        }
      else
        {
nodriverver:
          Drivers_array[2][Drivers_nrows] = CFStringCreateWithCString(NULL,
            "##.##", kCFStringEncodingUTF8);
        }

      if(drv_hdbc || drv_henv)
        {
          if(allocConnectHdl &&
            (freeConnectHdl = (pSQLFreeConnect)DLL_PROC(handle, "SQLFreeConnect")) != NULL)
            { freeConnectHdl(drv_hdbc); drv_hdbc = NULL; }

          if(allocEnvHdl &&
            (freeEnvHdl = (pSQLFreeEnv)DLL_PROC(handle, "SQLFreeEnv")) != NULL)
            { freeEnvHdl(drv_henv); drv_henv = NULL; }
        }

      if ((drv_hdbc || drv_henv) &&
         (freeHdl = (pSQLFreeHandle)DLL_PROC(handle, "SQLFreeHandle")) != NULL)
        {
          if(drv_hdbc) freeHdl(SQL_HANDLE_DBC, drv_hdbc);
          if(drv_henv) freeHdl(SQL_HANDLE_ENV, drv_henv);
        }

      DLL_CLOSE(handle);

      /* Get the size and date of the driver */
      if (!stat (_drv_u8, &_stat))
	{
          CFStringRef strRef;
	  struct tm drivertime;
	  char buf[100];

          Drivers_array[3][Drivers_nrows] = 
            CFStringCreateWithFormat(NULL, NULL,
              strRef = CFStringCreateWithCString(NULL, "%d Kb", kCFStringEncodingUTF8),
              (int) (_stat.st_size / 1024));
	  CFRelease(strRef);

	  localtime_r (&_stat.st_mtime, &drivertime);
	  strftime (buf, sizeof (buf), "%c", &drivertime);
	  Drivers_array[4][Drivers_nrows] = CFStringCreateWithCString(NULL, 
	  	buf, kCFStringEncodingUTF8);
        }
      else
        {
	    Drivers_array[3][Drivers_nrows] = CFStringCreateWithCString(NULL,
	      "-", kCFStringEncodingUTF8);
	    Drivers_array[4][Drivers_nrows] = CFStringCreateWithCString(NULL,
	      "-", kCFStringEncodingUTF8);
	}

      for(i = 0 ; i < 5 ; i++)
        {
          GetThemeTextDimensions (Drivers_array[i][Drivers_nrows], kThemeSystemFont,
            kThemeStateActive, false, &ioBound, &outBaseline);
          if(colSize[i] < ioBound.h) colSize[i] = ioBound.h;
        }

      AddDataBrowserItems (widget, DBITEM_ID, 1, &item, DBNAME_ID);
      item++;
      Drivers_nrows++;

      /* Process next one */
    skip:
      MEM_FREE (_drv_u8);
      _drv_u8 = NULL;

      ret = SQLDriversW (henv, SQL_FETCH_NEXT, drvdesc,
	  sizeof (drvdesc)/sizeof(wchar_t), &len, drvattrs,
	  sizeof (drvattrs)/sizeof(wchar_t), &len1);
      if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO && ret != SQL_NO_DATA)
	{
	  _iodbcdm_nativeerrorbox (dlg, henv, SQL_NULL_HANDLE,
	      SQL_NULL_HANDLE);
	  goto error;
	}
    }

error:
  /* Clean all that */
  SQLFreeHandle (SQL_HANDLE_ENV, henv);

end:
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
}

pascal OSStatus
driverchooser_ok_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TDRIVERCHOOSER *choose_t = (TDRIVERCHOOSER *) inUserData;
  DataBrowserItemID first, last;
  OSStatus err;

  if (choose_t)
    {
      /* Get the selection */
      if ((err =
	      GetDataBrowserSelectionAnchor (choose_t->driverlist, &first,
	       &last)) == noErr)
	{
	  if (first > DBITEM_ID && first <= DBITEM_ID + Drivers_nrows)
	    {
	      /* Get the driver name */
              choose_t->driver =
                convert_CFString_to_wchar(Drivers_array[0][first - DBITEM_ID - 1]);
	    }
	  else
	    choose_t->driver = NULL;
	}
      else
	choose_t->driver = NULL;

      DisposeWindow (choose_t->mainwnd);
	  choose_t->mainwnd = NULL;
      choose_t->driverlist = NULL;
      Drivers_nrows = 0;
    }

  return noErr;
}

pascal OSStatus
driverchooser_cancel_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TDRIVERCHOOSER *choose_t = (TDRIVERCHOOSER *) inUserData;

  if (choose_t)
    {
      DisposeWindow (choose_t->mainwnd);
	  choose_t->mainwnd = NULL;
      /* No driver choosen ... cancel pressed */
      choose_t->driver = NULL;
      choose_t->driverlist = NULL;
      Drivers_nrows = 0;
    }

  return noErr;
}

void
create_driverchooser (HWND hwnd, TDRIVERCHOOSER * choose_t)
{
  EventTypeSpec controlSpec = { kEventClassControl, kEventControlHit };
  RgnHandle cursorRgn = NULL;
  WindowRef wdrvchooser;
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
      CreateWindowFromNib (nibRef, CFSTR ("Dialog"), &wdrvchooser);
      DisposeNibReference (nibRef);
      /* Set the control into the structure */
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, DCLIST_CNTL, wdrvchooser,
	  choose_t->driverlist);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, DCFINISH_CNTL, wdrvchooser,
	  choose_t->b_add);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, DCCANCEL_CNTL, wdrvchooser,
          choose_t->b_remove);
      choose_t->driver = NULL;
      choose_t->mainwnd = wdrvchooser;
      /* Install handlers for the finish button, the cancel */
      InstallEventHandler (GetControlEventTarget (choose_t->b_add),
	  NewEventHandlerUPP (driverchooser_ok_clicked), 1, &controlSpec,
	  choose_t, NULL);
      InstallEventHandler (GetControlEventTarget (choose_t->b_remove),
	  NewEventHandlerUPP (driverchooser_cancel_clicked), 1, &controlSpec,
	  choose_t, NULL);
      Drivers_nrows = 0;
      adddrivers_to_list (choose_t->driverlist, choose_t->mainwnd, TRUE);

      /* Show the window and run the loop */
      choose_t->b_configure = NULL;
      DRIVERCHOOSER = choose_t;
      DeactivateControl (DRIVERCHOOSER->b_add);
      ShowWindow (wdrvchooser);

      /* The main loop */
      while (choose_t->mainwnd)
	WaitNextEvent (everyEvent, &event, 60L, cursorRgn);
    }
  else
    goto error;

  return;

error:
  choose_t->driver = NULL;
  fprintf (stderr, "Can't load Window. Err: %d\n", (int) err);
  return;
}
