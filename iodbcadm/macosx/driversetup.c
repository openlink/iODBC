/*
 *  driversetup.c
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
#include "getfpn.h"
#include <unicode.h>

#define DSDESCRIPTION_CNTL	6007
#define DSDRIVER_CNTL		6008
#define DSDRVBROWSE_CNTL	6009
#define DSSETUP_CNTL		6010
#define DSSTPBROWSE_CNTL	6011
#define DSSYSUSER_CNTL		6012
#define DSLIST_CNTL		6006
#define DSKEYWORD_CNTL		6004
#define DSVALUE_CNTL		6005
#define DSADD_CNTL		6002
#define DSUPDATE_CNTL		6003
#define DSOK_CNTL		6000
#define DSCANCEL_CNTL		6001

#define DRUSER_CNTL		8002
#define DRSYSTEM_CNTL		8003
#define DRDSN_CNTL		8004
#define DROK_CNTL		8000
#define DRCANCEL_CNTL		8001

#define DSKEYWORD_ID		'keyw'
#define DSVALUE_ID		'valu'

extern wchar_t* convert_CFString_to_wchar(const CFStringRef str);
extern CFStringRef convert_wchar_to_CFString(wchar_t *str);

static int STRCONN_NB_TOKENS = 3;

UInt32 DRIVERSETUP_nrows;
CFStringRef DRIVERSETUP_array[2][100];

TDRIVERSETUP *DRIVERSETUP = NULL;

static pascal OSStatus
driversetup_getset_item (ControlRef browser,
    DataBrowserItemID itemID, DataBrowserPropertyID property,
    DataBrowserItemDataRef itemData, Boolean changeValue)
{
  OSStatus err = noErr;

  if (!changeValue)
    switch (property)
      {
      case DSKEYWORD_ID:
	SetDataBrowserItemDataText (itemData,
	    DRIVERSETUP_array[0][itemID - DBITEM_ID - 1]);
	break;

      case DSVALUE_ID:
	SetDataBrowserItemDataText (itemData,
	    DRIVERSETUP_array[1][itemID - DBITEM_ID - 1]);
	break;

      case kDataBrowserItemIsActiveProperty:
	if (itemID > DBITEM_ID && itemID <= DBITEM_ID + DRIVERSETUP_nrows)
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

static void
driversetup_notification_item (ControlRef browser,
    DataBrowserItemID itemID, DataBrowserItemNotification message)
{
  static Boolean ignore_next = false, selected = false;

  switch (message)
    {
    case kDataBrowserItemSelected:
      if (DRIVERSETUP)
	{
	  ActivateControl (DRIVERSETUP->bupdate);
	  DrawOneControl (DRIVERSETUP->bupdate);
	  SetControlData (DRIVERSETUP->key_entry, 0, kControlEditTextCFStringTag,
	      sizeof(CFStringRef), &DRIVERSETUP_array[0][itemID - DBITEM_ID - 1]);
	  DrawOneControl (DRIVERSETUP->key_entry);
	  SetControlData (DRIVERSETUP->value_entry, 0, kControlEditTextCFStringTag,
              sizeof(CFStringRef), &DRIVERSETUP_array[1][itemID - DBITEM_ID - 1]);
	  DrawOneControl (DRIVERSETUP->value_entry);
	}

      if (selected)
	ignore_next = true;
      else
	selected = true;
      break;

    case kDataBrowserItemDeselected:
      if (!ignore_next && DRIVERSETUP)
	{
	  DeactivateControl (DRIVERSETUP->bupdate);
	  DrawOneControl (DRIVERSETUP->bupdate);
	  SetControlData (DRIVERSETUP->key_entry, 0,
              kControlEditTextTextTag, 0, "");
	  DrawOneControl (DRIVERSETUP->key_entry);
	  SetControlData (DRIVERSETUP->value_entry, 0,
	      kControlEditTextTextTag, 0, "");
	  DrawOneControl (DRIVERSETUP->value_entry);
	  selected = false;
	}
      else
	{
	  ignore_next = false;
	  selected = true;
	}
      break;
    };
}

static void
addkeywords_to_list (ControlRef widget, LPWSTR attrs,
    TDRIVERSETUP * driversetup_t)
{
  DataBrowserItemID item = DBITEM_ID + 1;
  DataBrowserCallbacks dbCallbacks;
  ThemeDrawingState outState = NULL;
  UInt16 colSize[2] = { 150, 250 };
  SInt16 outBaseline;
  Point ioBound;
  wchar_t *curr, *cour;
  CFStringRef strRef;
  int i;

  if (!widget)
    return;

  GetThemeDrawingState (&outState);

  /* Install an event handler on the component databrowser */
  dbCallbacks.version = kDataBrowserLatestCallbacks;
  InitDataBrowserCallbacks (&dbCallbacks);
  dbCallbacks.u.v1.itemNotificationCallback =
      NewDataBrowserItemNotificationUPP (driversetup_notification_item);
  /* On Mac OS X 10.0.x : clientDataCallback */
  dbCallbacks.u.v1.itemDataCallback =
      NewDataBrowserItemDataUPP (driversetup_getset_item);
  SetDataBrowserCallbacks (widget, &dbCallbacks);
  /* Begin the draw of the data browser */
  SetDataBrowserTarget (widget, DBITEM_ID);

  /* Make the clean up */
  for (i = 0; i < DRIVERSETUP_nrows; i++)
    {
      CFRelease (DRIVERSETUP_array[0][i]);
      DRIVERSETUP_array[0][i] = NULL;
      CFRelease (DRIVERSETUP_array[1][i]);
      DRIVERSETUP_array[1][i] = NULL;
      RemoveDataBrowserItems (widget, DBITEM_ID, 1, &item, DSKEYWORD_ID);
    }

  /* Global Initialization */
  DRIVERSETUP_nrows = 0;
  item = DBITEM_ID + 1;

  for (curr = (LPWSTR) attrs; *curr != L'\0' ; curr += (WCSLEN (curr) + 1))
    {
      if (!wcsncasecmp (curr, L"Driver=", WCSLEN (L"Driver=")))
        {
          strRef = convert_wchar_to_CFString(curr + WCSLEN (L"Driver="));
          SetControlData (driversetup_t->driver_entry, 0, kControlEditTextCFStringTag,
            sizeof(CFStringRef), &strRef);
          CFRelease(strRef);
          DrawOneControl (driversetup_t->driver_entry);
          continue;
        }

      if (!wcsncasecmp (curr, L"Setup=", WCSLEN (L"Setup=")))
        {
          strRef = convert_wchar_to_CFString(curr + WCSLEN (L"Setup="));
          SetControlData (driversetup_t->setup_entry, 0, kControlEditTextCFStringTag,
            sizeof(CFStringRef), &strRef);
          CFRelease(strRef);
          DrawOneControl (driversetup_t->setup_entry);
          continue;
        }

      if ((cour = wcschr (curr, L'=')))
        {
          *cour = L'\0';
          DRIVERSETUP_array[0][DRIVERSETUP_nrows] =
            convert_wchar_to_CFString(curr);
          *cour = L'=';
          DRIVERSETUP_array[1][DRIVERSETUP_nrows] =
            convert_wchar_to_CFString(cour + 1);
        }
      else
        DRIVERSETUP_array[0][DRIVERSETUP_nrows] =
          convert_wchar_to_CFString(L"");

      for(i = 0 ; i < 2 ; i++)
        {
          GetThemeTextDimensions (DRIVERSETUP_array[i][DRIVERSETUP_nrows], kThemeSystemFont,
            kThemeStateActive, false, &ioBound, &outBaseline);
          if(colSize[i] < ioBound.h) colSize[i] = ioBound.h;
        }

      AddDataBrowserItems (widget, DBITEM_ID, 1, &item, DSKEYWORD_ID);
      item++;
      DRIVERSETUP_nrows++;
    }

  ActivateControl (widget);
  /* Resize the columns to have a good look */
  SetDataBrowserTableViewNamedColumnWidth (widget, DSKEYWORD_ID, colSize[0] + 20);
  SetDataBrowserTableViewNamedColumnWidth (widget, DSVALUE_ID, colSize[1] + 20);
  DrawOneControl (widget);
  /* Remove the DataBrowser callback */
  SetDataBrowserCallbacks (NULL, &dbCallbacks);
  if(outState) DisposeThemeDrawingState (outState);
}

static void
parse_attribute_line (TDRIVERSETUP * driversetup_t, LPWSTR driver,
    LPWSTR attrs, BOOL add, BOOL user)
{
  CFStringRef strRef;

  if (driver)
    {
      strRef = convert_wchar_to_CFString(driver);
      SetControlData (driversetup_t->name_entry, 0, kControlEditTextCFStringTag,
        sizeof(CFStringRef), &strRef);
      CFRelease(strRef);

      /* Activate the radio box re. the set */
      SetControlValue (driversetup_t->sysuser_rb, user ? 1 : 2);
      /* Activate controls re. the add or setup */
      if (add)
      {
	DeactivateControl (driversetup_t->name_entry);
	ActivateControl (driversetup_t->sysuser_rb);
      }
      else
      {
	ActivateControl (driversetup_t->name_entry);
	DeactivateControl (driversetup_t->sysuser_rb);
      }
      DrawOneControl (driversetup_t->name_entry);
    }

  addkeywords_to_list (driversetup_t->key_list, attrs, driversetup_t);
}

pascal OSStatus
driversetup_drvbrowse_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  NavDialogOptions dialogOptions;
  TDRIVERSETUP *driversetup_t = (TDRIVERSETUP *) inUserData;
  NavReplyRecord reply;
  OSStatus err;
  FSSpec file;

  NavGetDefaultDialogOptions (&dialogOptions);
  err =
      NavChooseFile (NULL, &reply, &dialogOptions, NULL, NULL, NULL, NULL,
      NULL);

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
	      SetControlData (driversetup_t->driver_entry, 0,
	          kControlEditTextTextTag, STRLEN (file_path), file_path);
	      DrawOneControl (driversetup_t->driver_entry);
	    }
	}
    }

  NavDisposeReply (&reply);
  return noErr;
}

pascal OSStatus
driversetup_stpbrowse_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  NavDialogOptions dialogOptions;
  TDRIVERSETUP *driversetup_t = (TDRIVERSETUP *) inUserData;
  NavReplyRecord reply;
  OSStatus err;
  FSSpec file;

  NavGetDefaultDialogOptions (&dialogOptions);
  err =
      NavChooseFile (NULL, &reply, &dialogOptions, NULL, NULL, NULL, NULL,
      NULL);

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
	      SetControlData (driversetup_t->setup_entry, 0,
	          kControlEditTextTextTag, STRLEN (file_path), file_path);
	      DrawOneControl (driversetup_t->setup_entry);
	    }
	}
    }

  NavDisposeReply (&reply);
  return noErr;
}

pascal OSStatus
driversetup_add_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TDRIVERSETUP *driversetup_t = (TDRIVERSETUP *) inUserData;
  DataBrowserItemID item = DBITEM_ID + 1;
  DataBrowserCallbacks dbCallbacks;
  ThemeDrawingState outState = NULL;
  UInt16 colSize[2] = { 150, 250 };
  SInt16 outBaseline;
  Point ioBound;
  CFStringRef data[2];
  Size len;
  int i = 0, j;

  if (driversetup_t)
    {
      GetThemeDrawingState (&outState);

      GetControlData (driversetup_t->key_entry, 0, kControlEditTextCFStringTag,
	  sizeof (CFStringRef), &data[0], &len);
      if (CFStringGetLength(data[0]))
	{
	  GetControlData (driversetup_t->value_entry, 0, kControlEditTextCFStringTag,
            sizeof (CFStringRef), &data[1], &len);

	  /* Try to see if the keyword already exists */
	  for (i = 0; i < DRIVERSETUP_nrows; i++, item++)
	    if (CFStringCompare (data[0], DRIVERSETUP_array[0][i], 0)
              == kCFCompareEqualTo) goto done;

	  /* Install an event handler on the component databrowser */
	  dbCallbacks.version = kDataBrowserLatestCallbacks;
	  InitDataBrowserCallbacks (&dbCallbacks);
	  dbCallbacks.u.v1.itemNotificationCallback =
	      NewDataBrowserItemNotificationUPP
	      (driversetup_notification_item);
          /* On Mac OS X 10.0.x : clientDataCallback */
	  dbCallbacks.u.v1.itemDataCallback =
	      NewDataBrowserItemDataUPP (driversetup_getset_item);
	  SetDataBrowserCallbacks (driversetup_t->key_list, &dbCallbacks);
	  /* Begin the draw of the data browser */
	  SetDataBrowserTarget (driversetup_t->key_list, DBITEM_ID);

          if(i<DRIVERSETUP_nrows)
            {
              CFRelease (DRIVERSETUP_array[1][i]);
              DRIVERSETUP_array[1][i] = data[1];
              UpdateDataBrowserItems (driversetup_t->key_list, DBITEM_ID, 1,
		  &item, DSKEYWORD_ID, kDataBrowserItemNoProperty);
            }
          else if(len)
            {
	      /* Add a line */
	      DRIVERSETUP_array[0][i] = data[0];
	      DRIVERSETUP_array[1][i] = data[1];
	      AddDataBrowserItems (driversetup_t->key_list, DBITEM_ID, 1,
		  &item, DSKEYWORD_ID);
	      DRIVERSETUP_nrows++;
            }

          for(j = 0 ; j < DRIVERSETUP_nrows ; j++)
            {
              for(i = 0 ; i < 2 ; i++)
                {
                  GetThemeTextDimensions (DRIVERSETUP_array[i][j], kThemeSystemFont,
                    kThemeStateActive, false, &ioBound, &outBaseline);
                  if(colSize[i] < ioBound.h) colSize[i] = ioBound.h;
                }
            }

	  ActivateControl (driversetup_t->key_list);
	  /* Resize the columns to have a good look */
          SetDataBrowserTableViewNamedColumnWidth (driversetup_t->key_list, DSKEYWORD_ID, colSize[0] + 20);
          SetDataBrowserTableViewNamedColumnWidth (driversetup_t->key_list, DSVALUE_ID, colSize[1] + 20);
	  DrawOneControl (driversetup_t->key_list);
	  /* Remove the DataBrowser callback */
	  SetDataBrowserCallbacks (NULL, &dbCallbacks);
          if(outState) DisposeThemeDrawingState (outState);
	}

done:
      SetControlData (driversetup_t->key_entry, 0, kControlEditTextTextTag, 0,
	  "");
      DrawOneControl (driversetup_t->key_entry);
      SetControlData (driversetup_t->value_entry, 0, kControlEditTextTextTag,
	  0, "");
      DrawOneControl (driversetup_t->value_entry);
      DeactivateControl (driversetup_t->bupdate);
      DrawOneControl (driversetup_t->bupdate);
    }

  return noErr;
}

pascal OSStatus
driversetup_update_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TDRIVERSETUP *driversetup_t = (TDRIVERSETUP *) inUserData;
  DataBrowserItemID item = DBITEM_ID + 1, first, last;
  DataBrowserCallbacks dbCallbacks;
  ThemeDrawingState outState = NULL;
  UInt16 colSize[2] = { 150, 250 };
  SInt16 outBaseline;
  Point ioBound;
  CFStringRef data[2];
  Size len;
  int i = 0, j;

  if (driversetup_t)
    {
      GetThemeDrawingState (&outState);

      GetControlData (driversetup_t->key_entry, 0, kControlEditTextCFStringTag,
	  sizeof (CFStringRef), &data[0], &len);
      if (CFStringGetLength(data[0]))
	{
	  GetControlData (driversetup_t->value_entry, 0, kControlEditTextCFStringTag,
              sizeof (CFStringRef), &data[1], &len);

          if(GetDataBrowserSelectionAnchor (driversetup_t->key_list, &first, &last) == noErr)
            {
              i = first - DBITEM_ID - 1;
              item += i;
            }
          else i = 0;

	  /* Install an event handler on the component databrowser */
	  dbCallbacks.version = kDataBrowserLatestCallbacks;
	  InitDataBrowserCallbacks (&dbCallbacks);
	  dbCallbacks.u.v1.itemNotificationCallback =
	      NewDataBrowserItemNotificationUPP
	      (driversetup_notification_item);
          /* On Mac OS X 10.0.x : clientDataCallback */
	  dbCallbacks.u.v1.itemDataCallback =
	      NewDataBrowserItemDataUPP (driversetup_getset_item);
	  SetDataBrowserCallbacks (driversetup_t->key_list, &dbCallbacks);
	  /* Begin the draw of the data browser */
	  SetDataBrowserTarget (driversetup_t->key_list, DBITEM_ID);

          /* An update operation */
          if(i<DRIVERSETUP_nrows)
            {
              CFRelease (DRIVERSETUP_array[0][i]);
              CFRelease (DRIVERSETUP_array[1][i]);
              DRIVERSETUP_array[0][i] = data[0];
              DRIVERSETUP_array[1][i] = data[1];
              UpdateDataBrowserItems (driversetup_t->key_list, DBITEM_ID, 1,
                  &item, DSKEYWORD_ID, kDataBrowserItemNoProperty);
            }

          for(j = 0 ; j < DRIVERSETUP_nrows ; j++)
            {
              for(i = 0 ; i < 2 ; i++)
                {
                  GetThemeTextDimensions (DRIVERSETUP_array[i][j], kThemeSystemFont,
                    kThemeStateActive, false, &ioBound, &outBaseline);
                  if(colSize[i] < ioBound.h) colSize[i] = ioBound.h;
                }
            }

	  ActivateControl (driversetup_t->key_list);
	  /* Resize the columns to have a good look */
          SetDataBrowserTableViewNamedColumnWidth (driversetup_t->key_list, DSKEYWORD_ID, colSize[0] + 20);
          SetDataBrowserTableViewNamedColumnWidth (driversetup_t->key_list, DSVALUE_ID, colSize[1] + 20);
	  DrawOneControl (driversetup_t->key_list);
	  /* Remove the DataBrowser callback */
	  SetDataBrowserCallbacks (NULL, &dbCallbacks);
          if(outState) DisposeThemeDrawingState (outState);
	}

      SetControlData (driversetup_t->key_entry, 0, kControlEditTextTextTag, 0,
	  "");
      DrawOneControl (driversetup_t->key_entry);
      SetControlData (driversetup_t->value_entry, 0, kControlEditTextTextTag,
	  0, "");
      DrawOneControl (driversetup_t->value_entry);
      DeactivateControl (driversetup_t->bupdate);
      DrawOneControl (driversetup_t->bupdate);
    }

  return noErr;
}

pascal OSStatus
driversetup_ok_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TDRIVERSETUP *driversetup_t = (TDRIVERSETUP *) inUserData;
  int i = 0, size = 0;
  wchar_t *cour, *prov;
  CFStringRef strRef;
  Size len;

  if (driversetup_t)
    {
      /* What is the size of the block to malloc */
      GetControlData (driversetup_t->name_entry, 0,
        kControlEditTextCFStringTag, sizeof (CFStringRef),
        &strRef, &len);
      size += CFStringGetLength(strRef) + 1;
      CFRelease(strRef);
      GetControlData (driversetup_t->driver_entry, 0,
        kControlEditTextCFStringTag, sizeof (CFStringRef),
        &strRef, &len);
      size += CFStringGetLength(strRef) + WCSLEN (L"Driver=") + 1;
      CFRelease(strRef);
      GetControlData (driversetup_t->setup_entry, 0,
        kControlEditTextCFStringTag, sizeof (CFStringRef),
        &strRef, &len);
      size += CFStringGetLength(strRef) + WCSLEN (L"Setup=") + 1;
      CFRelease(strRef);
      size += WCSLEN( (GetControlValue (driversetup_t->sysuser_rb) == 1) ?
          L"USR" : L"SYS" ) + 1;
      /* Malloc it */
      if ((driversetup_t->connstr = (wchar_t *) malloc (++size * sizeof(wchar_t))))
	{
         WCSCPY(driversetup_t->connstr, (GetControlValue (driversetup_t->sysuser_rb)
            == 1) ? L"USR" : L"SYS");

	  for (cour = driversetup_t->connstr + 4; i < STRCONN_NB_TOKENS ; i++)
            {
	      switch (i)
	        {
	        case 0:
                  GetControlData (driversetup_t->name_entry, 0,
                    kControlEditTextCFStringTag, sizeof (CFStringRef),
                    &strRef, &len);
                  prov = convert_CFString_to_wchar(strRef);
                  if(prov)
                    {
                      WCSCPY(cour, prov);
                      free(prov);
                    }
		  break;
	        case 1:
                  GetControlData (driversetup_t->driver_entry, 0,
                    kControlEditTextCFStringTag, sizeof (CFStringRef),
                    &strRef, &len);
                  prov = convert_CFString_to_wchar(strRef);
                  if(prov)
                    {
                      WCSCPY(cour, L"Driver=");
                      WCSCAT(cour, prov);
                      free(prov);
                    }
		  break;
	        case 2:
                  GetControlData (driversetup_t->setup_entry, 0,
                    kControlEditTextCFStringTag, sizeof (CFStringRef),
                    &strRef, &len);
                  prov = convert_CFString_to_wchar(strRef);
                  if(prov)
                    {
                      WCSCPY(cour, L"Setup=");
                      WCSCAT(cour, prov);
                      free(prov);
                    }
		  break;
	        };
            
              CFRelease(strRef);
              cour += (WCSLEN (cour) + 1);
            }

	  for (i = 0; i < DRIVERSETUP_nrows; i++)
	    {
	      cour = driversetup_t->connstr;
	      driversetup_t->connstr =
                (LPWSTR) malloc ((size + CFStringGetLength(DRIVERSETUP_array[0][i]) +
                CFStringGetLength(DRIVERSETUP_array[1][i]) + 2) * sizeof(wchar_t));
	      if (driversetup_t->connstr)
		{
		  memcpy (driversetup_t->connstr, cour, size*sizeof(wchar_t));
                  prov = convert_CFString_to_wchar(DRIVERSETUP_array[0][i]);
                  if(prov)
                    {
                      WCSCPY(driversetup_t->connstr + size - 1, prov);
                      WCSCAT(driversetup_t->connstr + size - 1, L"=");
                      free(prov);
                      prov = convert_CFString_to_wchar(DRIVERSETUP_array[1][i]);
                      if(prov)
                        {
                          WCSCAT(driversetup_t->connstr + size - 1, prov);
                          free(prov);
                        }
                    }
		  free (cour);
		  size += CFStringGetLength(DRIVERSETUP_array[0][i]) +
                    CFStringGetLength(DRIVERSETUP_array[1][i]) + 2;
		}
	      else
		driversetup_t->connstr = cour;
	    }

	  driversetup_t->connstr[size - 1] = L'\0';
	}

      driversetup_t->name_entry = driversetup_t->driver_entry = NULL;
      driversetup_t->sysuser_rb = driversetup_t->setup_entry = NULL;
      driversetup_t->key_list = driversetup_t->filesel = NULL;

      DRIVERSETUP = NULL;
      DRIVERSETUP_nrows = 0;
      DisposeWindow (driversetup_t->mainwnd);
	  driversetup_t->mainwnd = NULL;
    }

  return noErr;
}

pascal OSStatus
driversetup_cancel_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TDRIVERSETUP *driversetup_t = (TDRIVERSETUP *) inUserData;

  if (driversetup_t)
    {
      driversetup_t->connstr = (LPWSTR) - 1L;
      
      driversetup_t->name_entry = driversetup_t->driver_entry = NULL;
      driversetup_t->sysuser_rb = driversetup_t->setup_entry = NULL;
      driversetup_t->key_list = driversetup_t->filesel = NULL;

      DRIVERSETUP = NULL;
      DRIVERSETUP_nrows = 0;
      DisposeWindow (driversetup_t->mainwnd);
	  driversetup_t->mainwnd = NULL;
    }

  return noErr;
}

LPSTR create_driversetup (HWND hwnd, LPCSTR driver, LPCSTR attrs,
    BOOL add, BOOL user)
{
  EventTypeSpec controlSpec = { kEventClassControl, kEventControlHit };
  RgnHandle cursorRgn = NULL;
  TDRIVERSETUP driversetup_t;
  ControlID controlID;
  WindowRef wdrvsetup;
  ControlRef control;
  EventRecord event;
  IBNibRef nibRef;
  OSStatus err;

  /* Search the bundle for a .nib file named 'odbcadmin'. */
  err =
      CreateNibReferenceWithCFBundle (CFBundleGetBundleWithIdentifier (CFSTR
	("org.iodbc.adm")), CFSTR ("driversetup"), &nibRef);
  if (err == noErr)
    {
      /* Nib found ... so create the window */
      CreateWindowFromNib (nibRef, CFSTR ("Dialog"), &wdrvsetup);
      DisposeNibReference (nibRef);
      /* Install handlers for the finish button, the cancel */
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, DSDESCRIPTION_CNTL,
	  wdrvsetup, driversetup_t.name_entry);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, DSSYSUSER_CNTL, wdrvsetup,
	  driversetup_t.sysuser_rb);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, DSDRIVER_CNTL, wdrvsetup,
	  driversetup_t.driver_entry);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, DSSETUP_CNTL, wdrvsetup,
	  driversetup_t.setup_entry);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, DSLIST_CNTL, wdrvsetup,
	  driversetup_t.key_list);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, DSKEYWORD_CNTL, wdrvsetup,
	  driversetup_t.key_entry);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, DSVALUE_CNTL, wdrvsetup,
	  driversetup_t.value_entry);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, DSUPDATE_CNTL, wdrvsetup,
	  driversetup_t.bupdate);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, DSADD_CNTL, wdrvsetup,
	  control);
      InstallEventHandler (GetControlEventTarget (control),
	  NewEventHandlerUPP (driversetup_add_clicked), 1, &controlSpec,
	  &driversetup_t, NULL);
      InstallEventHandler (GetControlEventTarget (driversetup_t.bupdate),
	  NewEventHandlerUPP (driversetup_update_clicked), 1, &controlSpec,
	  &driversetup_t, NULL);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, DSOK_CNTL, wdrvsetup,
	  control);
      InstallEventHandler (GetControlEventTarget (control),
	  NewEventHandlerUPP (driversetup_ok_clicked), 1, &controlSpec,
	  &driversetup_t, NULL);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, DSCANCEL_CNTL, wdrvsetup,
	  control);
      InstallEventHandler (GetControlEventTarget (control),
	  NewEventHandlerUPP (driversetup_cancel_clicked), 1, &controlSpec,
	  &driversetup_t, NULL);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, DSDRVBROWSE_CNTL, wdrvsetup,
	  control);
      InstallEventHandler (GetControlEventTarget (control),
	  NewEventHandlerUPP (driversetup_drvbrowse_clicked), 1, &controlSpec,
	  &driversetup_t, NULL);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, DSSTPBROWSE_CNTL, wdrvsetup,
	  control);
      InstallEventHandler (GetControlEventTarget (control),
	  NewEventHandlerUPP (driversetup_stpbrowse_clicked), 1, &controlSpec,
	  &driversetup_t, NULL);
      /* Parse the attributes line */
      driversetup_t.mainwnd = wdrvsetup;
      parse_attribute_line (&driversetup_t, (LPWSTR)driver, (LPWSTR)attrs, add, user);
      AdvanceKeyboardFocus (wdrvsetup);
      /* Show the window and run the loop */
      DeactivateControl (driversetup_t.bupdate);
      DRIVERSETUP = &driversetup_t;
      ShowWindow (wdrvsetup);

      /* The main loop */
      while (driversetup_t.mainwnd)
	WaitNextEvent (everyEvent, &event, 60L, cursorRgn);
    }
  else
    goto error;

  return (LPSTR)driversetup_t.connstr;

error:
  fprintf (stderr, "Can't load Window. Err: %d\n", (int) err);
  return (LPSTR)driversetup_t.connstr;
}

pascal OSStatus
driverremove_ok_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TDRIVERREMOVE *driverremove_t = (TDRIVERREMOVE *) inUserData;

  if (driverremove_t)
    {
      driverremove_t->dsns = GetControlValue (driverremove_t->system_cb);
      driverremove_t->dsns = (driverremove_t->dsns<<1) |
          GetControlValue (driverremove_t->user_cb);
      driverremove_t->deletedsn = GetControlValue (driverremove_t->dsn_rb) - 1;

      driverremove_t->user_cb = driverremove_t->system_cb = NULL;
      driverremove_t->dsn_rb = NULL;
      
      DisposeWindow (driverremove_t->mainwnd);
	  driverremove_t->mainwnd = NULL;
    }

  return noErr;
}

pascal OSStatus
driverremove_cancel_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TDRIVERREMOVE *driverremove_t = (TDRIVERREMOVE *) inUserData;

  if (driverremove_t)
    {
      driverremove_t->dsns = -1;
		driverremove_t->deletedsn = FALSE;

      driverremove_t->user_cb = driverremove_t->system_cb = NULL;
      driverremove_t->dsn_rb = NULL;

      DisposeWindow (driverremove_t->mainwnd);
	  driverremove_t->mainwnd = NULL;
    }

  return noErr;
}

void create_driverremove (HWND hwnd, TDRIVERREMOVE * driverremove_t)
{
  EventTypeSpec controlSpec = { kEventClassControl, kEventControlHit };
  RgnHandle cursorRgn = NULL;
  ControlID controlID;
  WindowRef wdrvremove;
  ControlRef control;
  EventRecord event;
  IBNibRef nibRef;
  OSStatus err;

  if (hwnd == NULL)
    return;

  /* Search the bundle for a .nib file named 'odbcadmin'. */
  err =
      CreateNibReferenceWithCFBundle (CFBundleGetBundleWithIdentifier (CFSTR
	("org.iodbc.adm")), CFSTR ("driverremove"), &nibRef);
  if (err == noErr)
    {
      /* Nib found ... so create the window */
      CreateWindowFromNib (nibRef, CFSTR ("Dialog"), &wdrvremove);
      DisposeNibReference (nibRef);
      /* Install handlers for the finish button, the cancel */
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, DRUSER_CNTL,
	  wdrvremove, driverremove_t->user_cb) GETCONTROLBYID (controlID,
	  CNTL_SIGNATURE, DRSYSTEM_CNTL, wdrvremove,
	  driverremove_t->system_cb) GETCONTROLBYID (controlID,
	  CNTL_SIGNATURE, DRDSN_CNTL, wdrvremove,
	  driverremove_t->dsn_rb)
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, DROK_CNTL, wdrvremove,
	  control) InstallEventHandler (GetControlEventTarget (control),
	  NewEventHandlerUPP (driverremove_ok_clicked), 1, &controlSpec,
	  driverremove_t, NULL);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, DRCANCEL_CNTL, wdrvremove,
	  control) InstallEventHandler (GetControlEventTarget (control),
	  NewEventHandlerUPP (driverremove_cancel_clicked), 1, &controlSpec,
	  driverremove_t, NULL);
      /* Set the controls */
      SetControlValue (driverremove_t->user_cb, driverremove_t->dsns & 1);
      SetControlValue (driverremove_t->system_cb, driverremove_t->dsns>>1 & 1);
      driverremove_t->mainwnd = wdrvremove;
      ShowWindow (wdrvremove);

      /* The main loop */
      while (driverremove_t->mainwnd)
	WaitNextEvent (everyEvent, &event, 60L, cursorRgn);
    }
  else
    goto error;

  return;

error:
  fprintf (stderr, "Can't load Window. Err: %d\n", (int) err);
  return;
}
