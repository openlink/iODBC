/*
 *  administrator.c
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>

#include <sqlucode.h>
#include <sqltypes.h>
#include <unicode.h>

#include <gui.h>
#include "getfpn.h"

extern wchar_t* convert_CFString_to_wchar(const CFStringRef str);
extern CFStringRef convert_wchar_to_CFString(wchar_t *str);
extern void create_driverremove (HWND hwnd, TDRIVERREMOVE * driverremove_t);

extern UInt32 DSN_nrows;
extern UInt32 FDSN_nrows;

static char *szComponentNames[] = {
  "org.iodbc.core",
  "org.iodbc.inst",
  "org.iodbc.adm",
  "org.iodbc.drvproxy",
  "org.iodbc.trans"
};

CFStringRef Components_array[5][sizeof (szComponentNames) / sizeof (char *)];
UInt32 Components_nrows;

CFStringRef ConnectionPool_array[3][256];
UInt32 ConnectionPool_nrows;

extern TDSNCHOOSER *DSNCHOOSER;
extern TDRIVERCHOOSER *DRIVERCHOOSER;
extern UInt32 Drivers_nrows;
extern CFStringRef Drivers_array[4][256];

/*static pascal Boolean
admin_filter_tracefiles (AEDesc * item, void *info,
    NavCallBackUserData callBackUD, NavFilterModes filterMode)
{
  Boolean display = true;
  NavFileOrFolderInfo *inf = (NavFileOrFolderInfo *) info;

  if (item->descriptorType == typeFSS)
    if (!inf->isFolder
	&& inf->fileAndFolder.fileInfo.finderInfo.fdType != 'TEXT')
      display = false;

  return display;
}*/

char *get_home(char *buf, size_t size)
{
  char *ptr;

  if ((ptr = getenv ("HOME")) == NULL)
    {
      ptr = (char *) getpwuid (getuid ());

      if (ptr != NULL)
        ptr = ((struct passwd *) ptr)->pw_dir;
    }
  STRNCPY(buf, ptr, size);
  buf[size-1]=0;
  return buf;
}

static pascal void
admin_nav_events (NavEventCallbackMessage callBackSelector,
    NavCBRecPtr callBackParms, NavCallBackUserData callBackUD)
{
  switch (callBackSelector)
    {
    case kNavCBEvent:
      switch (callBackParms->eventData.eventDataParms.event->what)
	{
	case nullEvent:
	  break;
	case updateEvt:
	  break;
	case activateEvt:
	  break;
	default:
	  break;
	};
      break;
    };
}

static pascal OSStatus
component_getset_item (ControlRef browser,
    DataBrowserItemID itemID, DataBrowserPropertyID property,
    DataBrowserItemDataRef itemData, Boolean setValue)
{
  OSStatus err = noErr;

  if (!setValue)
    switch (property)
      {
      case DBNAME_ID:
	err =
	    SetDataBrowserItemDataText (itemData,
	    Components_array[0][itemID - DBITEM_ID - 1]);
	break;

      case DBVERSION_ID:
	err =
	    SetDataBrowserItemDataText (itemData,
	    Components_array[1][itemID - DBITEM_ID - 1]);
	break;

      case DBFILE_ID:
	err =
	    SetDataBrowserItemDataText (itemData,
	    Components_array[2][itemID - DBITEM_ID - 1]);
	break;

      case DBDATE_ID:
	err =
	    SetDataBrowserItemDataText (itemData,
	    Components_array[3][itemID - DBITEM_ID - 1]);
	break;

      case DBSIZE_ID:
	err =
	    SetDataBrowserItemDataText (itemData,
	    Components_array[4][itemID - DBITEM_ID - 1]);
	break;

      case kDataBrowserItemIsActiveProperty:
	if (itemID > DBITEM_ID && itemID <= DBITEM_ID + Components_nrows)
	  err = SetDataBrowserItemDataBooleanValue (itemData, true);
	break;

      case kDataBrowserItemIsEditableProperty:
	err = SetDataBrowserItemDataBooleanValue (itemData, itemID == DBITEM_ID);
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

static void
addcomponents_to_list (ControlRef widget)
{
  DataBrowserItemID item = DBITEM_ID + 1;
  DataBrowserCallbacks dbCallbacks;
  ThemeDrawingState outState = NULL;
  UInt16 colSize[5] = { 100, 75, 100, 75, 50 };
  SInt16 outBaseline;
  Point ioBound;
  CFStringRef itemref, libname = NULL;
  CFDictionaryRef bundledict;
  CFBundleRef bundle;
  CFBundleRef bundle0 = NULL;
  char *cbuff;
  struct stat _stat;
  CFURLRef liburl;
  int i, j;

  if (!widget)
    return;

  GetThemeDrawingState (&outState);

  /* Install an event handler on the component databrowser */
  dbCallbacks.version = kDataBrowserLatestCallbacks;
  InitDataBrowserCallbacks (&dbCallbacks);
  /* On Mac OS X 10.0.x : clientDataCallback */
  dbCallbacks.u.v1.itemDataCallback =
      NewDataBrowserItemDataUPP (component_getset_item);
  SetDataBrowserCallbacks (widget, &dbCallbacks);
  /* Begin the draw of the data browser */
  SetDataBrowserTarget (widget, DBITEM_ID);

  /* Make the clean up */
  for (i = 0; i < Components_nrows ; i++, item++)
    {
      CFRelease (Components_array[0][i]);
      Components_array[0][i] = NULL;
      CFRelease (Components_array[1][i]);
      Components_array[1][i] = NULL;
      CFRelease (Components_array[2][i]);
      Components_array[2][i] = NULL;
      CFRelease (Components_array[3][i]);
      Components_array[3][i] = NULL;
      CFRelease (Components_array[4][i]);
      Components_array[4][i] = NULL;
      RemoveDataBrowserItems (widget, DBITEM_ID, 1, &item, DBNAME_ID);
    }

  /* Global Initialization */
  Components_nrows = 0;
  item = DBITEM_ID + 1;

  /* Add lines by lines */
  for (i = 0; i < sizeof (szComponentNames) / sizeof (char *); i++)
    {
      bundle = CFBundleGetBundleWithIdentifier (itemref =
        CFStringCreateWithCString (NULL, szComponentNames[i],
        kCFStringEncodingUTF8));
      if (i == 0)
        bundle0 = bundle;

      if (bundle)
	{
          CFStringRef typeOfPackage = NULL;
          CFStringRef execName = NULL;
          BOOL isBundle = FALSE;

	  bundledict = CFBundleGetInfoDictionary (bundle);

	  Components_array[0][i] = CFStringCreateCopy (NULL,
	      CFDictionaryGetValue (bundledict, CFSTR ("CFBundleName")));
	  Components_array[1][i] = CFStringCreateCopy (NULL,
	      CFDictionaryGetValue (bundledict, CFSTR ("CFBundleVersion")));
	  Components_array[2][i] = CFStringCreateCopy (NULL,
	      CFDictionaryGetValue (bundledict, CFSTR ("CFBundleIdentifier")));

	  typeOfPackage = CFDictionaryGetValue (bundledict, CFSTR ("CFBundlePackageType"));
	  if (typeOfPackage && 
	      CFStringCompare(typeOfPackage, CFSTR("BNDL"), 0) == kCFCompareEqualTo)
	    {
	      isBundle = TRUE;
	    }

	  execName = CFDictionaryGetValue (bundledict, CFSTR ("CFBundleExecutable"));

          if (isBundle && bundle0)
            {
              CFMutableStringRef mstr;

              mstr = CFStringCreateMutableCopy(NULL, 0, execName);
              CFStringAppend(mstr, CFSTR(".bundle/Contents/MacOS") );
	      liburl = CFBundleCopyResourceURL (bundle0, execName, NULL, mstr);
              CFRelease(mstr);
	    }
          else
	    liburl = CFBundleCopyResourceURL (bundle, execName, NULL, NULL);

	  if (liburl && (libname = CFURLCopyFileSystemPath (liburl, kCFURLPOSIXPathStyle)))
	    {
              if ( cbuff = convert_CFString_to_char(libname) )
		{
		  /* Get some information about the component */
		  if (!stat (cbuff, &_stat))
		    {
                      CFStringRef strRef;
		      struct tm drivertime;
		      char buf[100];

		      localtime_r (&_stat.st_mtime, &drivertime);
		      strftime (buf, sizeof(buf), "%c", &drivertime);
		      Components_array[3][i] =
			  CFStringCreateWithCString(NULL, 
			  	buf, kCFStringEncodingUTF8);

		      Components_array[4][i] =
                        CFStringCreateWithFormat(NULL, NULL,
                          strRef = CFStringCreateWithCString(NULL, "%d Kb",
                          kCFStringEncodingUTF8), (int) (_stat.st_size / 1024));
                      CFRelease(strRef);
		    }
		  else
		    {
		      Components_array[3][i] = CFStringCreateWithCString(NULL,
                        "-", kCFStringEncodingUTF8);;
		      Components_array[4][i] = CFStringCreateWithCString(NULL,
                        "-", kCFStringEncodingUTF8);;
		    }
                  free(cbuff);
		}
	      CFRelease (libname);
	    }

          for(j = 0 ; j < 5 ; j++)
            {
              GetThemeTextDimensions (Components_array[j][i], kThemeSystemFont,
                kThemeStateActive, false, &ioBound, &outBaseline);
              if(colSize[j] < ioBound.h) colSize[j] = ioBound.h;
            }

          AddDataBrowserItems (widget, DBITEM_ID, 1, &item, DBNAME_ID);
          item++;
          Components_nrows++;
	}
      CFRelease (itemref);
    }

  ActivateControl (widget);
  /* Resize the columns to have a good look */
  SetDataBrowserTableViewNamedColumnWidth (widget, DBNAME_ID, colSize[0] + 20);
  SetDataBrowserTableViewNamedColumnWidth (widget, DBVERSION_ID, colSize[1] + 20);
  SetDataBrowserTableViewNamedColumnWidth (widget, DBFILE_ID, colSize[2] + 20);
  SetDataBrowserTableViewNamedColumnWidth (widget, DBDATE_ID, colSize[3] + 20);
  SetDataBrowserTableViewNamedColumnWidth (widget, DBSIZE_ID, colSize[4] + 20);
  DrawOneControl (widget);
  /* Remove the DataBrowser callback */
  SetDataBrowserCallbacks (NULL, &dbCallbacks);
  if(outState) DisposeThemeDrawingState (outState);
}

static pascal OSStatus
connectionpool_getset_item (ControlRef browser,
    DataBrowserItemID itemID, DataBrowserPropertyID property,
    DataBrowserItemDataRef itemData, Boolean setValue)
{
  OSStatus err = noErr;

  if (!setValue)
    {
      switch (property)
	{
	case DBNAME_ID:
	  SetDataBrowserItemDataText (itemData,
	      ConnectionPool_array[0][itemID - DBITEM_ID - 1]);
	  break;

	case CPTIMEOUT_ID:
	  SetDataBrowserItemDataText (itemData,
	      ConnectionPool_array[1][itemID - DBITEM_ID - 1]);
	  break;

	case CPPROBE_ID:
	  SetDataBrowserItemDataText (itemData,
	      ConnectionPool_array[2][itemID - DBITEM_ID - 1]);
	  break;

	case kDataBrowserItemIsActiveProperty:
	  if (itemID > DBITEM_ID
	      && itemID <= DBITEM_ID + ConnectionPool_nrows)
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
    }
  else
    err = errDataBrowserPropertyNotSupported;

  return err;
}

pascal OSStatus
cp_finish_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TCONNECTIONPOOLING *connectionpool_t = (TCONNECTIONPOOLING *) inUserData;
  wchar_t ptime[1024] = { L'\0' }, *prov;
  wchar_t pprobe[1024] = { L'\0' };
  DataBrowserItemID first, last;
  OSStatus err = noErr;
  CFStringRef strRef;
  Size len;
  UWORD configMode;

  /* Retrieve the selected line */
  if ((err = GetDataBrowserSelectionAnchor (connectionpool_t->driverlist,
     &first, &last)) == noErr)
    {
      if (first > DBITEM_ID && first <= DBITEM_ID + ConnectionPool_nrows)
        {

          /* Get the driver name and the timeout */
          GetControlData (connectionpool_t->timeout_entry, 0,
              kControlEditTextCFStringTag, sizeof (CFStringRef), &strRef, &len);

          /* Prepare to save it */
          WCSCPY(ptime, L"CPTimeout=");
          prov = convert_CFString_to_wchar(strRef);
          CFRelease(strRef);
          if(prov)
            {
              WCSCAT(ptime, prov);
              free(prov);
            }

          GetControlData (connectionpool_t->probe_entry, 0,
              kControlEditTextCFStringTag, sizeof (CFStringRef), &strRef, &len);

          /* Prepare to save it */
          WCSCPY(pprobe, L"CPProbe=");
          prov = convert_CFString_to_wchar(strRef);
          CFRelease(strRef);
          if(prov)
            {
              WCSCAT(pprobe, prov);
              free(prov);
            }

          SQLGetConfigMode(&configMode);
          SQLSetConfigMode(ODBC_SYSTEM_DSN);
          prov = convert_CFString_to_wchar(ConnectionPool_array[0][first - DBITEM_ID - 1]);
          if(prov)
            {
              SQLSetConfigMode(ODBC_SYSTEM_DSN);
              if (!SQLConfigDriverW (connectionpool_t->mainwnd, ODBC_CONFIG_DRIVER,
                  prov, ptime, NULL, 0, NULL))
                _iodbcdm_errorboxw (connectionpool_t->mainwnd, prov,
                  L"An error occurred when trying to set the connection pooling time-out ");

              SQLSetConfigMode(ODBC_SYSTEM_DSN);
              if (!SQLConfigDriverW (connectionpool_t->mainwnd, ODBC_CONFIG_DRIVER,
                  prov, pprobe, NULL, 0, NULL))
                _iodbcdm_errorboxw (connectionpool_t->mainwnd, prov,
                  L"An error occurred when trying to set the connection probe query ");

              free(prov);
            }

          SQLSetConfigMode(configMode);
          addconnectionpool_to_list (connectionpool_t->driverlist,
              connectionpool_t->mainwnd);
        }
    }

  /* Destroy the dialog box */
  DisposeWindow (connectionpool_t->mainwnd);
  free (connectionpool_t);

  return err;
}

pascal OSStatus
cp_cancel_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TCONNECTIONPOOLING *connectionpool_t = (TCONNECTIONPOOLING *) inUserData;
  DisposeWindow (connectionpool_t->mainwnd);
  free (connectionpool_t);
  return noErr;
}

static void
connectionpool_notification_item (ControlRef browser,
    DataBrowserItemID itemID, DataBrowserItemNotification message)
{
  TCONNECTIONPOOLING *connectionpool_t = malloc (sizeof (TCONNECTIONPOOLING));
  EventTypeSpec controlSpec = { kEventClassControl, kEventControlHit };
  ControlRef controlRef;
  WindowPtr cptimeout;
  ControlID controlID;
  IBNibRef nibRef;
  OSStatus err;

  switch (message)
    {
    case kDataBrowserItemDoubleClicked:
      /* Search the bundle for a .nib file named 'odbcadmin'. */
      err =
	  CreateNibReferenceWithCFBundle (CFBundleGetBundleWithIdentifier
	  (CFSTR ("org.iodbc.adm")), CFSTR ("connectionpool"), &nibRef);
      if (err == noErr)
	{
	  /* Nib found ... so create the window */
	  CreateWindowFromNib (nibRef, CFSTR ("Dialog"), &cptimeout);
	  DisposeNibReference (nibRef);
	  /* Get the different controls and attach callbacks */
	  GETCONTROLBYID (controlID, CNTL_SIGNATURE, CPFINISH_CNTL, cptimeout,
	      controlRef);
          InstallEventHandler (GetControlEventTarget (controlRef),
	      NewEventHandlerUPP (cp_finish_clicked), 1, &controlSpec,
	      connectionpool_t, NULL);
	  GETCONTROLBYID (controlID, CNTL_SIGNATURE, CPCANCEL_CNTL, cptimeout,
	      controlRef);
          InstallEventHandler (GetControlEventTarget (controlRef),
	      NewEventHandlerUPP (cp_cancel_clicked), 1, &controlSpec,
	      connectionpool_t, NULL);
	  /* Set the right value */
	  GETCONTROLBYID (controlID, CNTL_SIGNATURE, CPTITLE_CNTL, cptimeout, 
	      controlRef);
	  SetControlData (controlRef, 0, kControlStaticTextCFStringTag,
	      sizeof(CFStringRef), &ConnectionPool_array[0][itemID - DBITEM_ID - 1]);
	  DrawOneControl (controlRef);

	  GETCONTROLBYID (controlID, CNTL_SIGNATURE, CPTIMEOUT_CNTL,
	      cptimeout, controlRef);
	  SetControlData (controlRef, 0, kControlEditTextCFStringTag,
	      sizeof(CFStringRef), &ConnectionPool_array[1][itemID - DBITEM_ID - 1]);
	  DrawOneControl (controlRef);

	  connectionpool_t->timeout_entry = controlRef;
	  GETCONTROLBYID (controlID, CNTL_SIGNATURE, CPPROBE_CNTL,
	      cptimeout, controlRef);
	  SetControlData (controlRef, 0, kControlEditTextCFStringTag,
	      sizeof(CFStringRef), &ConnectionPool_array[2][itemID - DBITEM_ID - 1]);
	  DrawOneControl (controlRef);

	  connectionpool_t->probe_entry = controlRef;
	  /* Prepare the connection struct */
	  connectionpool_t->driverlist = browser;
	  connectionpool_t->mainwnd = cptimeout;
	  /* Show the window */
	  ShowWindow (cptimeout);
	  SetPort ((GrafPtr) GetWindowPort (cptimeout));
	}
      else
	goto error;

      break;
    };

  return;

error:
  fprintf (stderr, "Can't load Window. Err: %d\n", (int) err);
  return;
}

void
addconnectionpool_to_list (ControlRef widget, WindowRef dlg)
{
  wchar_t drvdesc[1024], drvattrs[1024];
  DataBrowserItemID item = DBITEM_ID + 1;
  DataBrowserCallbacks dbCallbacks;
  ThemeDrawingState outState = NULL;
  UInt16 colSize[3] = { 150, 150, 150 };
  SInt16 outBaseline;
  Point ioBound;
  SQLSMALLINT len, len1;
  SQLRETURN ret;
  HENV henv;
  int i;

  if (!widget)
    return;

  GetThemeDrawingState (&outState);

  /* Install an event handler on the component databrowser */
  dbCallbacks.version = kDataBrowserLatestCallbacks;
  InitDataBrowserCallbacks (&dbCallbacks);
  /* On Mac OS X 10.0.x : clientDataCallback */
  dbCallbacks.u.v1.itemDataCallback =
      NewDataBrowserItemDataUPP (connectionpool_getset_item);
  dbCallbacks.u.v1.itemNotificationCallback =
      NewDataBrowserItemNotificationUPP (connectionpool_notification_item);
  SetDataBrowserCallbacks (widget, &dbCallbacks);
  /* Begin the draw of the data browser */
  SetDataBrowserTarget (widget, DBITEM_ID);

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
      SQLDriversW (henv, SQL_FETCH_FIRST, drvdesc, sizeof (drvdesc) / sizeof(wchar_t),
        &len, drvattrs, sizeof (drvattrs) / sizeof(wchar_t), &len1);
  if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
    {
      _iodbcdm_nativeerrorbox (dlg, henv, SQL_NULL_HANDLE, SQL_NULL_HANDLE);
      goto error;
    }

  /* Make the clean up */
  for (i = 0; i < ConnectionPool_nrows; i++, item++)
    {
      CFRelease (ConnectionPool_array[0][i]);
      ConnectionPool_array[0][i] = NULL;
      CFRelease (ConnectionPool_array[1][i]);
      ConnectionPool_array[1][i] = NULL;
      CFRelease (ConnectionPool_array[2][i]);
      ConnectionPool_array[2][i] = NULL;
      RemoveDataBrowserItems (widget, DBITEM_ID, 1, &item, DBNAME_ID);
    }

  /* Global Initialization */
  ConnectionPool_nrows = 0;
  item = DBITEM_ID + 1;

  while (ret != SQL_NO_DATA)
    {
      ConnectionPool_array[0][ConnectionPool_nrows] = convert_wchar_to_CFString(drvdesc);

      /* Get the cp timeout */
      SQLSetConfigMode (ODBC_BOTH_DSN);
      SQLGetPrivateProfileStringW (drvdesc, L"CPTimeout", L"", drvattrs,
	  sizeof (drvattrs) / sizeof(wchar_t), L"odbcinst.ini");

      if (drvattrs[0] == L'\0')
	SQLGetPrivateProfileStringW (L"Default", L"CPTimeout", L"", drvattrs,
	    sizeof (drvattrs) / sizeof(wchar_t), L"odbcinst.ini");

      ConnectionPool_array[1][ConnectionPool_nrows] =
        convert_wchar_to_CFString(WCSLEN(drvattrs) ? drvattrs : L"<Not pooled>");

      SQLGetPrivateProfileStringW (drvdesc, L"CPProbe", L"", drvattrs,
	  sizeof (drvattrs) / sizeof(wchar_t), L"odbcinst.ini");
      if (drvattrs[0] == L'\0')
	SQLGetPrivateProfileStringW (L"Default", L"CPProbe", L"", drvattrs,
	    sizeof (drvattrs) / sizeof(wchar_t), L"odbcinst.ini");

      ConnectionPool_array[2][ConnectionPool_nrows] =
        convert_wchar_to_CFString(WCSLEN(drvattrs) ? drvattrs : L"");

      for(i = 0 ; i < 3 ; i++)
        {
          GetThemeTextDimensions (ConnectionPool_array[i][ConnectionPool_nrows], kThemeSystemFont,
            kThemeStateActive, false, &ioBound, &outBaseline);
          if(colSize[i] < ioBound.h) colSize[i] = ioBound.h;
        }

      AddDataBrowserItems (widget, DBITEM_ID, 1, &item, DBNAME_ID);
      item++;
      ConnectionPool_nrows++;

      /* Process next one */
      ret =
	  SQLDriversW (henv, SQL_FETCH_NEXT, drvdesc, sizeof (drvdesc) / sizeof(wchar_t),
          &len, drvattrs, sizeof (drvattrs) / sizeof(wchar_t), &len1);
      if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO
	  && ret != SQL_NO_DATA)
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
  SetDataBrowserTableViewNamedColumnWidth (widget, CPTIMEOUT_ID, colSize[1] + 20);
  SetDataBrowserTableViewNamedColumnWidth (widget, CPPROBE_ID, colSize[2] + 20);
  DrawOneControl (widget);
  /* Remove the DataBrowser callback */
  SetDataBrowserCallbacks (NULL, &dbCallbacks);
  if(outState) DisposeThemeDrawingState (outState);
}

pascal OSStatus
driver_add_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TDRIVERCHOOSER *choose_t = (TDRIVERCHOOSER *) inUserData;
  wchar_t connstr[4096] = { L'\0' }, tokenstr[4096] = { L'\0' };
  DataBrowserItemID first, last;
  wchar_t *cstr;
  OSStatus err;

  if (choose_t)
    {
      cstr = (LPWSTR)create_driversetup (choose_t->mainwnd, NULL, (LPCSTR)connstr, FALSE, TRUE);

      if (cstr && cstr != connstr && cstr != (LPWSTR)- 1L)
	{
	  SQLSetConfigMode (!wcscmp(cstr, L"USR") ? ODBC_USER_DSN : ODBC_SYSTEM_DSN);
	  if (!SQLInstallDriverExW (cstr + 4, NULL, tokenstr,
            sizeof (tokenstr) / sizeof(wchar_t), NULL, ODBC_INSTALL_COMPLETE, NULL))
	    {
	      _iodbcdm_errorboxw (choose_t->mainwnd, NULL,
		  L"An error occurred when trying to add the driver");
	      goto done;
	    }

	  free (cstr);
	}

      adddrivers_to_list (choose_t->driverlist, choose_t->mainwnd, TRUE);

    done:
      if ((err = GetDataBrowserSelectionAnchor (choose_t->driverlist,
        &first, &last)) == noErr)
	{
	  if (!first && !last)
	    {
	      DeactivateControl (choose_t->b_remove);
	      DeactivateControl (choose_t->b_configure);
	    }
	}
    }

  return noErr;
}

pascal OSStatus
driver_remove_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TDRIVERCHOOSER *choose_t = (TDRIVERCHOOSER *) inUserData;
  TDRIVERREMOVE driverremove_t;
  DataBrowserItemID first, last;
  wchar_t tokenstr[4096] = { L'\0' };
  wchar_t *szDriver;
  OSStatus err;

  if (choose_t)
    {
      /* Retrieve the DSN name */
      if ((err = GetDataBrowserSelectionAnchor (choose_t->driverlist,
        &first, &last)) == noErr)
	{
	  if (first > DBITEM_ID && first <= DBITEM_ID + Drivers_nrows)
	    {
	      /* Get the driver */
              szDriver =
                convert_CFString_to_wchar(Drivers_array[0][first - DBITEM_ID - 1]);

              if(szDriver)
                {
                  /* Initialize some values */
                  driverremove_t.dsns = 0;
                  SQLSetConfigMode (ODBC_USER_DSN);
                  if (SQLGetPrivateProfileStringW (szDriver, NULL, L"", tokenstr,
                    sizeof (tokenstr) / sizeof(wchar_t), L"odbcinst.ini"))
                    driverremove_t.dsns = 1;
                  SQLSetConfigMode (ODBC_SYSTEM_DSN);
                  if (SQLGetPrivateProfileStringW (szDriver, NULL, L"", tokenstr,
                    sizeof (tokenstr) / sizeof(wchar_t), L"odbcinst.ini"))
                    driverremove_t.dsns = driverremove_t.dsns || 2;

	          /* Call the right function */
                  create_driverremove (choose_t->mainwnd, &driverremove_t);
	          if (driverremove_t.dsns!=-1 && create_confirmw (choose_t->mainwnd,
                    szDriver, L"Are you sure you want to perform the removal of this driver ?"))
		    {
		      if (driverremove_t.dsns & 1)
                        {
                          SQLSetConfigMode (ODBC_USER_DSN);
                          if (!SQLRemoveDriverW (szDriver,driverremove_t.deletedsn,NULL))
		            {
		              _iodbcdm_errorboxw (choose_t->mainwnd, szDriver,
			        L"An error occurred when trying to remove the driver ");
		              goto done;
		            }
                        }

                      if (driverremove_t.dsns & 2)
                        {
                          SQLSetConfigMode (ODBC_SYSTEM_DSN);
                          if (!SQLRemoveDriverW (szDriver,driverremove_t.deletedsn,NULL))
		            {
		              _iodbcdm_errorboxw (choose_t->mainwnd, szDriver,
			        L"An error occurred when trying to remove the driver ");
		              goto done;
		            }
                        }

		      adddrivers_to_list (choose_t->driverlist, choose_t->mainwnd, TRUE);
		    }

                  free(szDriver);
	        }
            }
	}

    done:
      if ((err =
	      GetDataBrowserSelectionAnchor (choose_t->driverlist, &first,
	       &last)) == noErr)
	{
	  if (!first && !last)
	    {
	      DeactivateControl (choose_t->b_remove);
	      DeactivateControl (choose_t->b_configure);
	    }
	}
    }

  return noErr;
}

pascal OSStatus
driver_configure_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TDRIVERCHOOSER *choose_t = (TDRIVERCHOOSER *) inUserData;
  DataBrowserItemID first, last;
  wchar_t connstr[4096] = { L'\0' }, tokenstr[4096] = { L'\0' };
  wchar_t *curr, *cour, *cstr, *szDriver;
  int size = sizeof (connstr) / sizeof(wchar_t);
  OSStatus err;
  UWORD conf = ODBC_USER_DSN;

  if (choose_t)
    {
      /* Retrieve the Driver name */
      if ((err = GetDataBrowserSelectionAnchor (choose_t->driverlist,
        &first, &last)) == noErr)
	{
	  if (first > DBITEM_ID && first <= DBITEM_ID + Drivers_nrows)
	    {
	      /* Get the driver */
              szDriver =
                convert_CFString_to_wchar(Drivers_array[0][first - DBITEM_ID - 1]);

	      /* Call the right function */
	      if (szDriver)
		{
		  SQLSetConfigMode (ODBC_USER_DSN);
		  if (!SQLGetPrivateProfileStringW (szDriver, NULL, L"", tokenstr,
                    sizeof (tokenstr) / sizeof(wchar_t), L"odbcinst.ini"))
		    {
                      SQLSetConfigMode (conf = ODBC_SYSTEM_DSN);
                      if (!SQLGetPrivateProfileStringW (szDriver, NULL, L"", tokenstr,
                        sizeof (tokenstr) / sizeof(wchar_t), L"odbcinst.ini"))
                        {
                          _iodbcdm_errorboxw (choose_t->mainwnd, szDriver,
                              L"An error occurred when trying to configure the driver ");
                          goto done;
                        }
                    }

		  for (curr = tokenstr, cour = connstr; *curr != L'\0' ;
                    curr += (WCSLEN (curr) + 1), cour += (WCSLEN (cour) + 1))
		    {
		      WCSCPY (cour, curr);
		      cour[WCSLEN (curr)] = L'=';
		      SQLSetConfigMode (conf);
		      SQLGetPrivateProfileStringW (szDriver, curr, L"",
                        cour + WCSLEN (curr) + 1, size - WCSLEN (curr) - 1,
                        L"odbcinst.ini");
		      size -= (WCSLEN (cour) + 1);
		    }

		  *cour = L'\0';

		  cstr =
		      (LPWSTR)create_driversetup (choose_t->mainwnd, (LPCSTR)szDriver,
		      (LPCSTR)connstr, FALSE, (conf == ODBC_SYSTEM_DSN) ? FALSE : TRUE);

		  if (cstr && cstr != connstr && cstr != (LPWSTR) - 1L)
		    {
		      SQLSetConfigMode (conf);
		      if (!SQLInstallDriverExW (cstr + 4, NULL, tokenstr,
                        sizeof (tokenstr) / sizeof(wchar_t), NULL,
                        ODBC_INSTALL_COMPLETE, NULL))
			{
			  _iodbcdm_errorboxw (choose_t->mainwnd, NULL,
			      L"An error occurred when trying to configure the driver ");
			  goto done;
			}

		      free (cstr);
		    }

		  adddrivers_to_list (choose_t->driverlist, choose_t->mainwnd, TRUE);
		}
	    }
	}

    done:
      if ((err = GetDataBrowserSelectionAnchor (choose_t->driverlist,
        &first, &last)) == noErr)
	{
	  if (!first && !last)
	    {
	      DeactivateControl (choose_t->b_remove);
	      DeactivateControl (choose_t->b_configure);
	    }
	}
    }

  return noErr;
}

pascal OSStatus
tracing_start_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TTRACING *tracing_t = (TTRACING *) inUserData;
  char *prov;
  CFStringRef strRef;
  Size strl;
  int mode;

  if (tracing_t)
    {
      /* Clear previous setting */
      SQLSetConfigMode (ODBC_USER_DSN);
      SQLWritePrivateProfileString ("ODBC", "Trace", NULL, NULL);
      SQLSetConfigMode (ODBC_USER_DSN);
      SQLWritePrivateProfileString ("ODBC", "TraceAutoStop", NULL, NULL);
      SQLSetConfigMode (ODBC_USER_DSN);
      SQLWritePrivateProfileString ("ODBC", "TraceFile", NULL, NULL);
      SQLSetConfigMode (ODBC_USER_DSN);
      SQLWritePrivateProfileString ("ODBC", "TraceDLL", NULL, NULL);

      SQLSetConfigMode (ODBC_SYSTEM_DSN);
      SQLWritePrivateProfileString ("ODBC", "Trace", NULL, NULL);
      SQLSetConfigMode (ODBC_SYSTEM_DSN);
      SQLWritePrivateProfileString ("ODBC", "TraceAutoStop", NULL, NULL);
      SQLSetConfigMode (ODBC_SYSTEM_DSN);
      SQLWritePrivateProfileString ("ODBC", "TraceFile", NULL, NULL);
      SQLSetConfigMode (ODBC_SYSTEM_DSN);
      SQLWritePrivateProfileString ("ODBC", "TraceDLL", NULL, NULL);

      if (GetControlValue(tracing_t->trace_wide) == 1)
        mode = ODBC_USER_DSN;
      else
        mode = ODBC_SYSTEM_DSN;

      /* Write keywords for tracing in the ini file */
      SQLSetConfigMode(mode);
      if(GetControlValue (tracing_t->trace_rb) == 2
        || GetControlValue (tracing_t->trace_rb) == 3)
        SQLWritePrivateProfileString ("ODBC", "Trace", "1", NULL);
      else
        SQLWritePrivateProfileString ("ODBC", "Trace", "0", NULL);

      SQLSetConfigMode(mode);
      if(GetControlValue (tracing_t->trace_rb) == 3)
        SQLWritePrivateProfileString ("ODBC", "TraceAutoStop", "1", NULL);
      else
        SQLWritePrivateProfileString ("ODBC", "TraceAutoStop", "0", NULL);

      GetControlData (tracing_t->logfile_entry, kControlEditTextPart,
        kControlEditTextCFStringTag, sizeof (CFStringRef), &strRef, &strl);
      prov = (char*)convert_CFString_to_wchar(strRef);
      SQLSetConfigMode(mode);
      SQLWritePrivateProfileStringW (L"ODBC", L"TraceFile", (wchar_t*)prov, NULL);
      CFRelease(strRef);
      free(prov);

      GetControlData (tracing_t->tracelib_entry, kControlEditTextPart,
        kControlEditTextCFStringTag, sizeof (CFStringRef), &strRef, &strl);
      prov = (char*)convert_CFString_to_wchar(strRef);
      SQLSetConfigMode(mode);
      SQLWritePrivateProfileStringW (L"ODBC", L"TraceDLL", (wchar_t*)prov, NULL);
      CFRelease(strRef);
      free(prov);
    }

  return noErr;
}

pascal OSStatus
tracing_logfile_choosen (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  NavDialogOptions dialogOptions;
  TTRACING *tracing_t = (TTRACING *) inUserData;
  NavReplyRecord reply;
  char tokenstr[4096] = { 0 };
  OSStatus err;
  FSSpec file;
  char *dir;

  NavGetDefaultDialogOptions (&dialogOptions);
  STRCPY (dialogOptions.windowTitle + 1, "Choose your trace file ...");
  dialogOptions.windowTitle[0] = STRLEN ("Choose your trace file ...");
  STRCPY (dialogOptions.savedFileName + 1, "sql.log");
  dialogOptions.savedFileName[0] = STRLEN ("sql.log");

  err = NavPutFile (NULL, &reply, &dialogOptions,
    NewNavEventUPP (admin_nav_events),
    /*NewNavObjectFilterUPP (admin_filter_tracefiles)*/
    'TEXT', kNavGenericSignature, NULL);
/*      NavChooseObject (NULL, &reply, &dialogOptions,
      NewNavEventUPP (admin_nav_events),
      NewNavObjectFilterUPP (admin_filter_tracefiles), NULL);*/

  if (!err && reply.validRecord)
    {
      /* Get and transform the file descriptor from the reply */
      err =
	  AEGetNthPtr (&reply.selection, 1, typeFSS, NULL, NULL, &file,
	  sizeof (file), NULL);
      if (!err)
	{
	  /* Get back some information about the directory */
	  dir = get_full_pathname (file.parID, file.vRefNum);
	  sprintf (tokenstr, "%s/", dir ? dir : "");
	  strncat (tokenstr, (char *)&file.name[1], file.name[0]);
	  /* Display the constructed string re. the file choosen */
	  SetControlData (tracing_t->logfile_entry, 0,
	      kControlEditTextTextTag,
	      STRLEN (tokenstr) ? STRLEN (tokenstr) : STRLEN ("sql.log"),
	      STRLEN (tokenstr) ? tokenstr : "sql.log");
	  DrawOneControl (tracing_t->logfile_entry);
	  /* Clean up */
	  if (dir)
	    free (dir);
	}
    }

  NavDisposeReply (&reply);
  return noErr;
}

pascal OSStatus
tracing_browse_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  NavDialogOptions dialogOptions;
  TTRACING *tracing_t = (TTRACING *) inUserData;
  NavReplyRecord reply;
  OSStatus err;
  FSSpec file;

  NavGetDefaultDialogOptions (&dialogOptions);
  STRCPY (dialogOptions.windowTitle + 1, "Choose your trace library ...");
  dialogOptions.windowTitle[0] = STRLEN ("Choose your trace library ...");
  STRCPY (dialogOptions.savedFileName + 1, "ODBC Trace Library");
  dialogOptions.savedFileName[0] = STRLEN ("ODBC Trace Library");

  err =
      NavChooseFile (NULL, &reply, &dialogOptions,
      NewNavEventUPP (admin_nav_events), NULL, NULL, NULL, NULL);

  if (!err && reply.validRecord)
    {
      /* Get and transform the file descriptor from the reply */
      err =
	  AEGetNthPtr (&reply.selection, 1, typeFSS, NULL, NULL, &file,
	  sizeof (file), NULL);
      if (!err)
	{
          char file_path[PATH_MAX] = { '\0' };
          FSRef ref;

          if( file.name[0] == 0 )
            err = FSMakeFSSpec(file.vRefNum, file.parID, file.name, &file);
          if( err == noErr )
            err = FSpMakeFSRef(&file, &ref);

          err = FSRefMakePath(&ref, file_path, PATH_MAX); // translate the FSRef into a path
          if (err == noErr)
            {
	      /* Display the constructed string re. the file choosen */
	      SetControlData (tracing_t->tracelib_entry, 0,
	          kControlEditTextTextTag, STRLEN (file_path), file_path);
	      DrawOneControl (tracing_t->tracelib_entry);
            }
	}
    }

  NavDisposeReply (&reply);
  return noErr;
}

pascal OSStatus
admin_ok_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TDSNCHOOSER *dsnchoose_t = (inUserData) ? ((void **) inUserData)[0] : NULL;
  TDRIVERCHOOSER *driverchoose_t =
      (inUserData) ? ((void **) inUserData)[1] : NULL;
  TTRACING *tracing_t = (inUserData) ? ((void **) inUserData)[2] : NULL;
  TCOMPONENT *component_t = (inUserData) ? ((void **) inUserData)[3] : NULL;
  TCONNECTIONPOOLING *connectionpool_t =
      (inUserData) ? ((void **) inUserData)[4] : NULL;
  WindowRef mainwnd = (inUserData) ? ((void **) inUserData)[5] : NULL;
  CFStringRef strRef;
  wchar_t *prov;
  Size strl;

  if (dsnchoose_t)
    {
      dsnchoose_t->udsnlist = dsnchoose_t->sdsnlist = dsnchoose_t->dir_list =
	  NULL;
      dsnchoose_t->fadd = dsnchoose_t->fremove = dsnchoose_t->ftest =
	  dsnchoose_t->fconfigure = NULL;
      dsnchoose_t->uadd = dsnchoose_t->uremove = dsnchoose_t->utest =
	  dsnchoose_t->uconfigure = NULL;
      dsnchoose_t->sadd = dsnchoose_t->sremove = dsnchoose_t->stest =
	  dsnchoose_t->sconfigure = NULL;
      dsnchoose_t->file_list = dsnchoose_t->file_entry =
	  dsnchoose_t->dir_combo = NULL;
      dsnchoose_t->type_dsn = -1;
      dsnchoose_t->dsn = NULL;
    }

  if (driverchoose_t)
    driverchoose_t->driverlist = NULL;

  if (component_t)
    component_t->componentlist = NULL;

  if (tracing_t)
    {
      if (tracing_t->changed)
        tracing_start_clicked (NULL, NULL, tracing_t);
      tracing_t->logfile_entry = tracing_t->tracelib_entry = NULL;
      tracing_t->b_start_stop = tracing_t->trace_rb = NULL;
    }

  if (connectionpool_t)
    {
      if (connectionpool_t->changed)
	{
	  UWORD configMode;

          SQLGetConfigMode(&configMode);
	  /* Write keywords for tracing in the ini file */
          SQLSetConfigMode(ODBC_SYSTEM_DSN);
	  SQLWritePrivateProfileString ("ODBC Connection Pooling", "PerfMon",
	      (GetControlValue (connectionpool_t->perfmon_rb) ==
		  1) ? "1" : "0", "odbcinst.ini");
	  GetControlData (connectionpool_t->retwait_entry, 0,
	      kControlEditTextCFStringTag, sizeof (CFStringRef), &strRef, &strl);
          prov = (wchar_t*)convert_CFString_to_char(strRef);
          if(prov)
            {
              SQLSetConfigMode(ODBC_SYSTEM_DSN);
	      SQLWritePrivateProfileString ("ODBC Connection Pooling",
	        "Retry Wait", (char*)prov, "odbcinst.ini");
              free(prov);
            }
          CFRelease(strRef);
          SQLSetConfigMode(configMode);
	}

      connectionpool_t->perfmon_rb = connectionpool_t->retwait_entry = NULL;
      connectionpool_t->driverlist = NULL;
      connectionpool_t->mainwnd = NULL;
    }

  if (mainwnd)
    {
      ((void **) inUserData)[5] = NULL;
      DisposeWindow (mainwnd);
	}

  return noErr;
}

pascal OSStatus
admin_cancel_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TDSNCHOOSER *dsnchoose_t = (inUserData) ? ((void **) inUserData)[0] : NULL;
  TDRIVERCHOOSER *driverchoose_t =
      (inUserData) ? ((void **) inUserData)[1] : NULL;
  TTRACING *tracing_t = (inUserData) ? ((void **) inUserData)[2] : NULL;
  TCOMPONENT *component_t = (inUserData) ? ((void **) inUserData)[3] : NULL;
  TCONNECTIONPOOLING *connectionpool_t =
      (inUserData) ? ((void **) inUserData)[4] : NULL;
  WindowRef mainwnd = (inUserData) ? ((void **) inUserData)[5] : NULL;

  if (dsnchoose_t)
    {
      dsnchoose_t->udsnlist = dsnchoose_t->sdsnlist = dsnchoose_t->dir_list =
	  NULL;
      dsnchoose_t->fadd = dsnchoose_t->fremove = dsnchoose_t->ftest =
	  dsnchoose_t->fconfigure = NULL;
      dsnchoose_t->uadd = dsnchoose_t->uremove = dsnchoose_t->utest =
	  dsnchoose_t->uconfigure = NULL;
      dsnchoose_t->sadd = dsnchoose_t->sremove = dsnchoose_t->stest =
	  dsnchoose_t->sconfigure = NULL;
      dsnchoose_t->file_list = dsnchoose_t->file_entry =
	  dsnchoose_t->dir_combo = NULL;
      dsnchoose_t->type_dsn = -1;
      dsnchoose_t->dsn = NULL;
    }

  if (driverchoose_t)
    driverchoose_t->driverlist = NULL;

  if (component_t)
    component_t->componentlist = NULL;

  if (tracing_t)
    {
      tracing_t->logfile_entry = tracing_t->tracelib_entry =
	  tracing_t->b_start_stop = NULL;
      tracing_t->trace_rb = NULL;
    }

  if (connectionpool_t)
    {
      connectionpool_t->perfmon_rb = connectionpool_t->retwait_entry = NULL;
      connectionpool_t->driverlist = NULL;
      connectionpool_t->mainwnd = NULL;
    }

  if (mainwnd)
    {
      ((void **) inUserData)[5] = NULL;
      DisposeWindow (mainwnd);
	}

  return noErr;
}

void
DisplayTabControlNumber (int index, ControlRef tabControl, WindowRef wnd,
    int *tabs)
{
  ControlRef userPane;
  ControlID controlID;
  UInt16 i;

  controlID.signature = TABS_SIGNATURE;

  for (i = 1; i <= tabs[0]; i++)
    {
      controlID.id = tabs[i];
      GetControlByID (wnd, &controlID, &userPane);
      SetControlVisibility (userPane, i == index, true);
    }

  DrawOneControl (tabControl);
}

pascal OSStatus
admin_switch_page (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  int tabs[] = { NUMBER_TAB, UDSN_TAB, SDSN_TAB, FDSN_TAB, DRIVER_TAB, POOL_TAB,
      TRACE_TAB, ABOUT_TAB };
  TDSNCHOOSER *dsnchoose_t = (inUserData) ? ((void **) inUserData)[0] : NULL;
  TDRIVERCHOOSER *driverchoose_t =
      (inUserData) ? ((void **) inUserData)[1] : NULL;
  TTRACING *tracing_t = (inUserData) ? ((void **) inUserData)[2] : NULL;
  TCOMPONENT *component_t = (inUserData) ? ((void **) inUserData)[3] : NULL;
  TCONNECTIONPOOLING *connectionpool_t =
      (inUserData) ? ((void **) inUserData)[4] : NULL;
  WindowRef mainwnd = (inUserData) ? ((void **) inUserData)[5] : NULL;
  BOOL trace = FALSE, traceauto = FALSE, perfmon = FALSE;
  DataBrowserItemID first, last;
  wchar_t tokenstr[4096] = { L'\0' }, tokenstr1[4096] = { L'\0' };
  ControlRef tabControl;
  ControlID controlID;
  CFStringRef strRef;
  struct passwd *pwdent;
  int tab_index;
  OSStatus err;

  /* Search which tab is activated */
  controlID.signature = TABS_SIGNATURE;
  controlID.id = GLOBAL_TAB;
  GetControlByID (mainwnd, &controlID, &tabControl);
  DisplayTabControlNumber (tab_index =
      GetControlValue (tabControl), tabControl, mainwnd, tabs);

  if (tab_index == *(int *) (((void **) inUserData)[6]))
    return noErr;

  ClearKeyboardFocus (mainwnd);

  *(int *) (((void **) inUserData)[6]) = tab_index;

  switch (tab_index)
    {
    /* User DSN panel */
    case 1:
      if (dsnchoose_t)
	{
	  DSNCHOOSER = dsnchoose_t; DSN_nrows = 0;
	  dsnchoose_t->type_dsn = USER_DSN;
	  adddsns_to_list (dsnchoose_t->udsnlist, FALSE, mainwnd);

	  if ((err =
		  GetDataBrowserSelectionAnchor (dsnchoose_t->udsnlist,
		   &first, &last)) == noErr)
	    {
	      if (!first && !last)
		{
		  DeactivateControl (dsnchoose_t->uremove);
		  DeactivateControl (dsnchoose_t->uconfigure);
		  DeactivateControl (dsnchoose_t->utest);
		}
	    }
	}
      break;

    /* System DSN panel */
    case 2:
      if (dsnchoose_t)
	{
	  DSNCHOOSER = dsnchoose_t; DSN_nrows = 0;
	  dsnchoose_t->type_dsn = SYSTEM_DSN;
	  adddsns_to_list (dsnchoose_t->sdsnlist, TRUE, mainwnd);

	  if ((err =
		  GetDataBrowserSelectionAnchor (dsnchoose_t->sdsnlist,
		   &first, &last)) == noErr)
	    {
	      if (!first && !last)
		{
		  DeactivateControl (dsnchoose_t->sremove);
		  DeactivateControl (dsnchoose_t->sconfigure);
		  DeactivateControl (dsnchoose_t->stest);
		}
	    }
	}
      break;

    /* File DSN panel */
    case 3:
      if (dsnchoose_t)
	{
	  DSNCHOOSER = dsnchoose_t; 

	  dsnchoose_t->type_dsn = FILE_DSN;
          addfdsns_to_list (dsnchoose_t, dsnchoose_t->curr_dir, false);

	  if ((err = GetDataBrowserSelectionAnchor (dsnchoose_t->fdsnlist,
		   &first, &last)) == noErr)
	    {
	      if (!first && !last)
		{
		  DeactivateControl (dsnchoose_t->fremove);
		  DeactivateControl (dsnchoose_t->fconfigure);
		  DeactivateControl (dsnchoose_t->ftest);
		}
	    }
	}
      break;

    /* ODBC drivers panel */
    case 4:
      if (driverchoose_t)
	{
	  DRIVERCHOOSER = driverchoose_t;
	  adddrivers_to_list (driverchoose_t->driverlist, mainwnd, TRUE);

	  if ((err =
		  GetDataBrowserSelectionAnchor (driverchoose_t->driverlist,
		      &first, &last)) == noErr)
	    {
	      if (!first && !last)
		{
		  DeactivateControl (driverchoose_t->b_remove);
		  DeactivateControl (driverchoose_t->b_configure);
		}
	    }
	}
      break;

    /* Connection Pooling panel */
    case 5:
      if (!connectionpool_t->changed)
	{
	  /* Get the connection pooling options */
	  SQLGetPrivateProfileString ("ODBC Connection Pooling", "Perfmon",
	      "", (char *)tokenstr, sizeof (tokenstr), "odbcinst.ini");
	  if (!strcasecmp ((char*)tokenstr, "1") || !strcasecmp ((char*)tokenstr, "On"))
	    perfmon = TRUE;
	  SQLGetPrivateProfileString ("ODBC Connection Pooling", "Retry Wait",
	      "", (char*)tokenstr, sizeof (tokenstr), "odbcinst.ini");

	  if (perfmon)
	    SetControlValue (connectionpool_t->perfmon_rb, 1);
	  else
	    SetControlValue (connectionpool_t->perfmon_rb, 2);

          strRef = CFStringCreateWithCString (NULL, (char *)tokenstr,
            kCFStringEncodingASCII);
	  SetControlData (connectionpool_t->retwait_entry, 0,
            kControlEditTextCFStringTag, sizeof(CFStringRef), &strRef);
	  DrawOneControl (connectionpool_t->retwait_entry);
          CFRelease(strRef);

	  connectionpool_t->changed = TRUE;
	}

      addconnectionpool_to_list (connectionpool_t->driverlist, mainwnd);
      break;

    /* Tracing panel */
    case 6:
      if (!tracing_t->changed)
	{
	  int mode = ODBC_SYSTEM_DSN;

	  /* Get the traces options */
          SQLSetConfigMode (mode);
	  SQLGetPrivateProfileStringW (L"ODBC", L"TraceFile", L"", tokenstr,
	      sizeof (tokenstr) / sizeof(wchar_t), NULL);
	  if (tokenstr[0] != L'\0')
	    {
	      /* All users wide */
	      SetControlValue (tracing_t->trace_wide, 2);
	    }
	  else
	    {
	      /* Only for current user */
	      mode = ODBC_USER_DSN;
              SQLSetConfigMode (mode);
	      SetControlValue (tracing_t->trace_wide, 1);
	    }

          SQLSetConfigMode (mode);
	  SQLGetPrivateProfileString ("ODBC", "Trace", "", (char*)tokenstr,
	      sizeof (tokenstr), NULL);
	  if (!strcasecmp ((char*)tokenstr, "1") || !strcasecmp ((char*)tokenstr, "On"))
	    trace = TRUE;

          SQLSetConfigMode (mode);
	  SQLGetPrivateProfileString ("ODBC", "TraceAutoStop", "", (char*)tokenstr,
	      sizeof (tokenstr), NULL);
	  if (!strcasecmp ((char*)tokenstr, "1") || !strcasecmp ((char*)tokenstr, "On"))
	    traceauto = TRUE;

          SQLSetConfigMode (mode);
	  SQLGetPrivateProfileStringW (L"ODBC", L"TraceFile", L"", tokenstr,
	      sizeof (tokenstr) / sizeof(wchar_t), NULL);

          SQLSetConfigMode (mode);
	  SQLGetPrivateProfileStringW (L"ODBC", L"TraceDLL", L"", tokenstr1,
	      sizeof (tokenstr1) / sizeof(wchar_t), NULL);

	  /* Set the widgets */
	  if (trace)
	    {
	      if (!traceauto)
		SetControlValue (tracing_t->trace_rb, 2);
	      else
		SetControlValue (tracing_t->trace_rb, 3);
	    }
	  else
	    SetControlValue (tracing_t->trace_rb, 1);

          if( tokenstr[0] == L'\0' )
            {
              wchar_t *prov;
              pwdent = getpwuid (getuid());
              prov = dm_SQL_A2W (pwdent->pw_dir, SQL_NTS);
              WCSCPY (tokenstr, prov ? prov : L"");
              WCSCAT (tokenstr, L"/sql.log");
            }

          strRef = convert_wchar_to_CFString(tokenstr);
	  SetControlData (tracing_t->logfile_entry, kControlEditTextPart,
	      kControlEditTextCFStringTag, sizeof(CFStringRef), &strRef);
          CFRelease(strRef);
	  DrawOneControl (tracing_t->logfile_entry);

          strRef = convert_wchar_to_CFString(tokenstr1);
	  SetControlData (tracing_t->tracelib_entry, kControlEditTextPart,
	      kControlEditTextCFStringTag, sizeof(CFStringRef), &strRef);
          CFRelease(strRef);
	  DrawOneControl (tracing_t->tracelib_entry);

	  tracing_t->changed = TRUE;
	}
      break;

    /* About panel */
    case 7:
      if (component_t)
	addcomponents_to_list (component_t->componentlist);
      break;
    };

  AdvanceKeyboardFocus (mainwnd);

  return noErr;
}

pascal OSStatus
filedsn_select_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TDSNCHOOSER *dsnchoose_t = (TDSNCHOOSER *) inUserData;

  if (dsnchoose_t)
    {
      MenuRef menu = GetControlPopupMenuHandle(dsnchoose_t->fdir);
      int id = GetControlValue (dsnchoose_t->fdir);
      CFStringRef menuText;
      char str[1024];

      CopyMenuItemTextAsCFString(menu, id, &menuText);
      CFStringGetCString(menuText, str, sizeof(str), kCFStringEncodingUTF8);
      CFRelease(menuText);

      addfdsns_to_list (dsnchoose_t, str, true);
    }

  return noErr;
}


void
create_administrator (HWND hwnd)
{
  int tabs[] =
      { NUMBER_TAB, UDSN_TAB, SDSN_TAB, FDSN_TAB, DRIVER_TAB, POOL_TAB, TRACE_TAB,
	ABOUT_TAB };
  EventTypeSpec controlSpec = { kEventClassControl, kEventControlHit };
  ControlRef tabControl, control;
  ControlID controlID;
  IBNibRef nibRef;
  OSStatus err;
  WindowPtr admin;
  void *inparams[7];
  TDSNCHOOSER dsnchoose_t;
  TDRIVERCHOOSER driverchoose_t;
  TCOMPONENT component_t;
  TTRACING tracing_t;
  TCONNECTIONPOOLING connectionpool_t;
  RgnHandle cursorRgn = NULL;
  EventRecord event;
  int current_tab = -1;

  CFStringRef libname = NULL;
  CFBundleRef bundle;
  CFURLRef liburl;
  SInt16 rscId = -1, oldId = CurResFile();

  FDSN_nrows = 0;
  Components_nrows = 0;
  DSN_nrows = 0;
  ConnectionPool_nrows = 0;
  Drivers_nrows = 0;

  if (hwnd == NULL)
    return;

  /* Load the ressource data file */
  bundle = CFBundleGetBundleWithIdentifier (CFSTR("org.iodbc.adm"));
  if (bundle)
    {
      liburl = CFBundleCopyExecutableURL (bundle);

      if ((libname = CFURLCopyFileSystemPath (liburl, kCFURLPOSIXPathStyle)))
        {
          FSRef ref;
          char *pr1 = convert_CFString_to_char(libname);

          if(pr1)
            {
              char forkName[2048];
              int i;

              STRCPY (forkName, pr1);
              free(pr1);

              for(i=STRLEN(forkName)-1 ; i>0 && forkName[i]!='/' ; i--);
              if (i != 0)
                {
                  forkName[i] = '\0';
                }

              for(i=STRLEN(forkName)-1 ; i>0 && forkName[i]!='/' ; i--);
              if(i != 0)
                {
                  STRCPY (forkName + i + 1, "Resources/iODBCadm.rsrc");
                  err = FSPathMakeRef (forkName, &ref, FALSE);
                  err = FSOpenResourceFile (&ref, 0, NULL, fsRdPerm, &rscId);
                }
            }
          CFRelease (libname);
        }
    }

  if(rscId != -1)
    UseResFile (rscId);

  /* Search the bundle for a .nib file named 'odbcadmin'. */
  err =
      CreateNibReferenceWithCFBundle (CFBundleGetBundleWithIdentifier (CFSTR
	("org.iodbc.adm")), CFSTR ("odbcadmin"), &nibRef);
  if (err == noErr)
    {
      /* Nib found ... so create the window */
      err = CreateWindowFromNib (nibRef, CFSTR ("Window"), &admin);
      if (err != noErr)
	goto error;
      /* And no need anymore the nib */
      DisposeNibReference (nibRef);
    }
  else
    goto error;

  /* Find the tab control . */
  GETCONTROLBYID (controlID, TABS_SIGNATURE, GLOBAL_TAB, admin, tabControl);
  /* Create for each tab a structure */

  /* TDSNCHOOSER */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, ULIST_CNTL, admin,
    dsnchoose_t.udsnlist);
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, SLIST_CNTL, admin,
    dsnchoose_t.sdsnlist);
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, FLIST_CNTL, admin,
    dsnchoose_t.fdsnlist);
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, UADD_CNTL, admin,
    dsnchoose_t.uadd);
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, SADD_CNTL, admin,
    dsnchoose_t.sadd);
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, FADD_CNTL, admin,
    dsnchoose_t.fadd);
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, UDEL_CNTL, admin,
    dsnchoose_t.uremove);
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, SDEL_CNTL, admin,
    dsnchoose_t.sremove);
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, FDEL_CNTL, admin,
    dsnchoose_t.fremove);
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, UTST_CNTL, admin,
    dsnchoose_t.utest);
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, STST_CNTL, admin,
    dsnchoose_t.stest);
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, FTST_CNTL, admin,
    dsnchoose_t.ftest);
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, UCFG_CNTL, admin,
    dsnchoose_t.uconfigure);
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, SCFG_CNTL, admin,
      dsnchoose_t.sconfigure);
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, FCFG_CNTL, admin,
      dsnchoose_t.fconfigure);
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, FDIR_CNTL, admin,
      dsnchoose_t.fdir);
  dsnchoose_t.type_dsn = USER_DSN;
  dsnchoose_t.mainwnd = admin;

  /* TDRIVERCHOOSER */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, DLIST_CNTL, admin,
    driverchoose_t.driverlist);
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, DADD_CNTL, admin,
    driverchoose_t.b_add);
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, DDEL_CNTL, admin,
    driverchoose_t.b_remove);
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, DCFG_CNTL, admin,
    driverchoose_t.b_configure);
  driverchoose_t.mainwnd = admin;
  /* TCONNECTIONPOOLING */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, PLIST_CNTL, admin,
    connectionpool_t.driverlist);
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, PPERF_CNTL, admin,
    connectionpool_t.perfmon_rb);
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, PRETR_CNTL, admin,
    connectionpool_t.retwait_entry);
  connectionpool_t.changed = false;
  connectionpool_t.mainwnd = admin;

  /* TTRACING */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, TLOG_CNTL, admin,
    tracing_t.logfile_entry);
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, TTRLIB_CNTL, admin,
    tracing_t.tracelib_entry);
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, TOPTI_CNTL, admin,
    tracing_t.trace_rb);
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, TWIDE_CNTL, admin,
    tracing_t.trace_wide);
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, TSTART_CNTL, admin,
    tracing_t.b_start_stop);
  tracing_t.changed = false;

  /* TCOMPONENT */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, CLIST_CNTL, admin,
    component_t.componentlist);
  /* Install an event handler on the tab control. */
  InstallEventHandler (GetControlEventTarget (tabControl),
      NewEventHandlerUPP (admin_switch_page), 1, &controlSpec, inparams,
      NULL);
  /* Install an event handler on the OK button */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, OKBUT_CNTL, admin, control);
  InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (admin_ok_clicked), 1, &controlSpec, inparams, NULL);
  /* Install an event handler on the Cancel button */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, CANCELBUT_CNTL, admin, control);
  InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (admin_cancel_clicked), 1, &controlSpec, inparams,
      NULL);
  /* Install an event handler on the Apply button of the tracing panel */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, TSTART_CNTL, admin, control);
  InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (tracing_start_clicked), 1, &controlSpec, &tracing_t,
      NULL);
  /* Install an event handler on the Browse button of the tracing panel (log file) */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, TLOGB_CNTL, admin, control);
  InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (tracing_logfile_choosen), 1, &controlSpec,
      &tracing_t, NULL);
  /* Install an event handler on the Browse button of the tracing panel (lib file) */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, TLIBB_CNTL, admin, control);
  InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (tracing_browse_clicked), 1, &controlSpec,
      &tracing_t, NULL);

  /* Install an event handler on the Add button of the user DSNs */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, UADD_CNTL, admin, control);
  InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (userdsn_add_clicked), 1, &controlSpec, &dsnchoose_t,
      NULL);
  /* Install an event handler on the Remove button of the user DSNs */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, UDEL_CNTL, admin, control);
  InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (userdsn_remove_clicked), 1, &controlSpec,
      &dsnchoose_t, NULL);
  /* Install an event handler on the Configure button of the user DSNs */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, UCFG_CNTL, admin, control);
  InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (userdsn_configure_clicked), 1, &controlSpec,
      &dsnchoose_t, NULL);
  /* Install an event handler on the Test button of the user DSNs */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, UTST_CNTL, admin, control);
  InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (userdsn_test_clicked), 1, &controlSpec,
      &dsnchoose_t, NULL);

  /* Install an event handler on the Add button of the system DSNs */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, SADD_CNTL, admin, control);
  InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (systemdsn_add_clicked), 1, &controlSpec,
      &dsnchoose_t, NULL);
  /* Install an event handler on the Remove button of the system DSNs */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, SDEL_CNTL, admin, control);
  InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (systemdsn_remove_clicked), 1, &controlSpec,
      &dsnchoose_t, NULL);
  /* Install an event handler on the Configure button of the system DSNs */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, SCFG_CNTL, admin, control);
  InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (systemdsn_configure_clicked), 1, &controlSpec,
      &dsnchoose_t, NULL);
  /* Install an event handler on the Test button of the system DSNs */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, STST_CNTL, admin, control);
  InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (systemdsn_test_clicked), 1, &controlSpec,
      &dsnchoose_t, NULL);

  /* Install an event handler on the Add button of the system DSNs */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, FADD_CNTL, admin, control);
  InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (filedsn_add_clicked), 1, &controlSpec,
      &dsnchoose_t, NULL);
  /* Install an event handler on the Remove button of the system DSNs */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, FDEL_CNTL, admin, control);
  InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (filedsn_remove_clicked), 1, &controlSpec,
      &dsnchoose_t, NULL);
  /* Install an event handler on the Select dir menu of the File DSNs */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, FDIR_CNTL, admin, control);
  InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (filedsn_select_clicked), 1, &controlSpec,
      &dsnchoose_t, NULL);
  /* Install an event handler on the Configure button of the system DSNs */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, FCFG_CNTL, admin, control);
  InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (filedsn_configure_clicked), 1, &controlSpec,
      &dsnchoose_t, NULL);
  /* Install an event handler on the Test button of the system DSNs */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, FTST_CNTL, admin, control);
  InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (filedsn_test_clicked), 1, &controlSpec,
      &dsnchoose_t, NULL);
  /* Install an event handler on the SetDir button of the system DSNs */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, FSETDIR_CNTL, admin, control);
  InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (filedsn_setdir_clicked), 1, &controlSpec,
      &dsnchoose_t, NULL);


  /* Install an event handler on the Add button of the driver panel */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, DADD_CNTL, admin, control);
  InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (driver_add_clicked), 1, &controlSpec,
      &driverchoose_t, NULL);
  /* Install an event handler on the Remove button of the driver panel */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, DDEL_CNTL, admin, control);
  InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (driver_remove_clicked), 1, &controlSpec,
      &driverchoose_t, NULL);
  /* Install an event handler on the Configure button of the driver panel */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, DCFG_CNTL, admin, control);
  InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (driver_configure_clicked), 1, &controlSpec,
      &driverchoose_t, NULL);

  inparams[0] = &dsnchoose_t;
  inparams[1] = &driverchoose_t;
  inparams[2] = &tracing_t;
  inparams[3] = &component_t;
  inparams[4] = &connectionpool_t;
  inparams[5] = admin;
  inparams[6] = &current_tab;

  SQLSetConfigMode (ODBC_BOTH_DSN);
  if (!SQLGetPrivateProfileString("ODBC", "FileDSNPath", "", 
      dsnchoose_t.curr_dir, sizeof(dsnchoose_t.curr_dir), "odbcinst.ini"))
    {
      char tmp[1024];
      snprintf(dsnchoose_t.curr_dir, sizeof(dsnchoose_t.curr_dir),
        "%s/Documents", get_home(tmp, sizeof(tmp)));
/*        "%s"DEFAULT_FILEDSNPATH, get_home(tmp, sizeof(tmp))); */
    }

  /* Force to go on the first tab */
  DisplayTabControlNumber (1, tabControl, admin, tabs);
  admin_switch_page (NULL, NULL, inparams);
  AdvanceKeyboardFocus (admin);
  /* Show the window */
  ShowWindow (admin);
  SetPort ((GrafPtr) GetWindowPort (admin));

  while (inparams[5])
    WaitNextEvent (everyEvent, &event, 60L, cursorRgn);

  if(rscId != -1)
    {
      CloseResFile (rscId);
      UseResFile (oldId);
    }

  return;

error:
  fprintf (stderr, "Can't load Window. Err: %d\n", (int) err);
  if(rscId != -1)
    {
      CloseResFile (rscId);
      UseResFile (oldId);
    }
  return;
}
