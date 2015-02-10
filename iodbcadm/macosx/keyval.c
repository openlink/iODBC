/*
 *  keyval.c
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

#define GSVERIFYCONN_CNTL 4008
#define GSLIST_CNTL	  4006
#define GSKEYWORD_CNTL	  4004
#define GSVALUE_CNTL	  4005
#define GSADD_CNTL	  4002
#define GSUPDATE_CNTL	  4003
#define GSOK_CNTL	  4000
#define GSCANCEL_CNTL	  4001


UInt32 KEYVAL_nrows;
CFStringRef KEYVAL_array[2][256];

TKEYVAL *KEYVAL = NULL;


static pascal OSStatus
keyval_getset_item (ControlRef browser,
    DataBrowserItemID itemID, DataBrowserPropertyID property,
    DataBrowserItemDataRef itemData, Boolean changeValue)
{
  OSStatus err = noErr;

  if (!changeValue)
    switch (property)
      {
      case GSKEYWORD_ID:
	SetDataBrowserItemDataText (itemData,
	    KEYVAL_array[0][itemID - DBITEM_ID - 1]);
	break;

      case GSVALUE_ID:
	SetDataBrowserItemDataText (itemData,
	    KEYVAL_array[1][itemID - DBITEM_ID - 1]);
	break;

      case kDataBrowserItemIsActiveProperty:
	if (itemID > DBITEM_ID && itemID <= DBITEM_ID + KEYVAL_nrows)
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
keyval_notification_item (ControlRef browser,
    DataBrowserItemID itemID, DataBrowserItemNotification message)
{
  static Boolean ignore_next = false, selected = false;

  switch (message)
    {
    case kDataBrowserItemSelected:
      if (KEYVAL)
	{
	  ActivateControl (KEYVAL->bupdate);
	  DrawOneControl (KEYVAL->bupdate);
	  SetControlData (KEYVAL->key_entry, 0, kControlEditTextCFStringTag,
	      sizeof(CFStringRef), &KEYVAL_array[0][itemID - DBITEM_ID - 1]);
	  DrawOneControl (KEYVAL->key_entry);
	  SetControlData (KEYVAL->value_entry, 0, kControlEditTextCFStringTag,
	      sizeof(CFStringRef), &KEYVAL_array[1][itemID - DBITEM_ID - 1]);
	  DrawOneControl (KEYVAL->value_entry);
	}

      if (selected)
	ignore_next = true;
      else
	selected = true;
      break;

    case kDataBrowserItemDeselected:
      if (!ignore_next && KEYVAL)
	{
	  DeactivateControl (KEYVAL->bupdate);
	  DrawOneControl (KEYVAL->bupdate);
	  SetControlData (KEYVAL->key_entry, 0, kControlEditTextTextTag, 0,
	      "");
	  DrawOneControl (KEYVAL->key_entry);
	  SetControlData (KEYVAL->value_entry, 0, kControlEditTextTextTag,
	      0, "");
	  DrawOneControl (KEYVAL->value_entry);
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
addkeywords_to_list (ControlRef widget,
    LPCSTR attrs,
    TKEYVAL * keyval_t)
{
  DataBrowserItemID item = DBITEM_ID + 1;
  DataBrowserCallbacks dbCallbacks;
  ThemeDrawingState outState = NULL;
  UInt16 colSize[2] = { 150, 250 };
  SInt16 outBaseline;
  Point ioBound;
  char *curr, *cour;
  int i;

  if (!widget)
    return;

  GetThemeDrawingState (&outState);

  /* Install an event handler on the component databrowser */
  dbCallbacks.version = kDataBrowserLatestCallbacks;
  InitDataBrowserCallbacks (&dbCallbacks);
  dbCallbacks.u.v1.itemNotificationCallback =
      NewDataBrowserItemNotificationUPP (keyval_notification_item);
  /* On Mac OS X 10.0.x : this is clientDataCallback */
  dbCallbacks.u.v1.itemDataCallback =
      NewDataBrowserItemDataUPP (keyval_getset_item);
  SetDataBrowserCallbacks (widget, &dbCallbacks);
  /* Begin the draw of the data browser */
  SetDataBrowserTarget (widget, DBITEM_ID);

  /* Make the clean up */
  for (i = 0; i < KEYVAL_nrows; i++)
    {
      CFRelease (KEYVAL_array[0][i]);
      KEYVAL_array[0][i] = NULL;
      CFRelease (KEYVAL_array[1][i]);
      KEYVAL_array[1][i] = NULL;
    }

  /* Global Initialization */
  KEYVAL_nrows = 0;
  item = DBITEM_ID + 1;

  for (curr = (LPSTR) attrs; *curr; curr += (STRLEN (curr) + 1))
    {
      if (!strncasecmp (curr, "DSN=", STRLEN ("DSN=")) ||
          !strncasecmp (curr, "Driver=", STRLEN ("Driver=")) ||
          !strncasecmp (curr, "Description=", STRLEN ("Description=")))
        continue;

      if ((cour = strchr ((char*)curr, '=')))
        {
          *cour = '\0';
          KEYVAL_array[0][KEYVAL_nrows] =
            CFStringCreateWithCString (NULL, curr, kCFStringEncodingUTF8);
          *cour = '=';
          KEYVAL_array[1][KEYVAL_nrows] =
            CFStringCreateWithCString (NULL, ((char*)cour) + 1, kCFStringEncodingUTF8);
        }
      else
        KEYVAL_array[0][KEYVAL_nrows] =
          CFStringCreateWithCString (NULL, "", kCFStringEncodingUTF8);

      for(i = 0 ; i < 2 ; i++)
        {
          GetThemeTextDimensions (KEYVAL_array[i][KEYVAL_nrows], kThemeSystemFont,
            kThemeStateActive, false, &ioBound, &outBaseline);
          if(colSize[i] < ioBound.h) colSize[i] = ioBound.h;
        }

      AddDataBrowserItems (widget, DBITEM_ID, 1, &item, GSKEYWORD_ID);
      item++;
      KEYVAL_nrows++;
    }

  ActivateControl (widget);
  /* Resize the columns to have a good look */
  SetDataBrowserTableViewNamedColumnWidth (widget, GSKEYWORD_ID, colSize[0] + 20);
  SetDataBrowserTableViewNamedColumnWidth (widget, GSVALUE_ID, colSize[1] + 20);
  DrawOneControl (widget);
  /* Remove the DataBrowser callback */
  SetDataBrowserCallbacks (NULL, &dbCallbacks);
}


pascal OSStatus
keyval_add_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TKEYVAL *keyval_t = (TKEYVAL *) inUserData;
  DataBrowserItemID item = DBITEM_ID + 1;
  DataBrowserCallbacks dbCallbacks;
  ThemeDrawingState outState = NULL;
  UInt16 colSize[2] = { 150, 250 };
  SInt16 outBaseline;
  Point ioBound;
  CFStringRef data[2];
  Size len;
  int i = 0, j;

  if (keyval_t)
    {
      GetThemeDrawingState (&outState);

      GetControlData (keyval_t->key_entry, 0, kControlEditTextCFStringTag,
	  sizeof (CFStringRef), &data[0], &len);
      if (CFStringGetLength(data[0]))
	{
	  GetControlData (keyval_t->value_entry, 0, kControlEditTextCFStringTag,
            sizeof (CFStringRef), &data[1], &len);

	  /* Try to see if the keyword already exists */
	  for (i = 0; i < KEYVAL_nrows; i++, item++)
	    if (CFStringCompare (data[0], KEYVAL_array[0][i],
		    0) == kCFCompareEqualTo)
	      goto done;

	  /* Install an event handler on the component databrowser */
	  dbCallbacks.version = kDataBrowserLatestCallbacks;
	  InitDataBrowserCallbacks (&dbCallbacks);
	  dbCallbacks.u.v1.itemNotificationCallback =
	      NewDataBrowserItemNotificationUPP (keyval_notification_item);
          /* On Mac OS X 10.0.x : this is clientDataCallback */
	  dbCallbacks.u.v1.itemDataCallback =
	      NewDataBrowserItemDataUPP (keyval_getset_item);
	  SetDataBrowserCallbacks (keyval_t->key_list, &dbCallbacks);
	  /* Begin the draw of the data browser */
	  SetDataBrowserTarget (keyval_t->key_list, DBITEM_ID);

          /* An update operation */
          if(i<KEYVAL_nrows)
            {
              CFRelease (KEYVAL_array[1][i]);
              KEYVAL_array[1][i] = data[1];
              UpdateDataBrowserItems (keyval_t->key_list, DBITEM_ID, 1, &item,
                GSKEYWORD_ID, kDataBrowserItemNoProperty);
            }
          else if(len)
            {
              KEYVAL_array[0][i] = data[0];
              KEYVAL_array[1][i] = data[1];
              AddDataBrowserItems (keyval_t->key_list, DBITEM_ID, 1, &item,
                GSKEYWORD_ID);
              KEYVAL_nrows++;
            }

          for(j = 0 ; j < KEYVAL_nrows ; j++)
            {
              for(i = 0 ; i < 2 ; i++)
                {
                  GetThemeTextDimensions (KEYVAL_array[i][j], kThemeSystemFont,
                    kThemeStateActive, false, &ioBound, &outBaseline);
                  if(colSize[i] < ioBound.h) colSize[i] = ioBound.h;
                }
            }

	  ActivateControl (keyval_t->key_list);
	  /* Resize the columns to have a good look */
          SetDataBrowserTableViewNamedColumnWidth (keyval_t->key_list, GSKEYWORD_ID, colSize[0] + 20);
          SetDataBrowserTableViewNamedColumnWidth (keyval_t->key_list, GSVALUE_ID, colSize[1] + 20);
	  DrawOneControl (keyval_t->key_list);
	  /* Remove the DataBrowser callback */
	  SetDataBrowserCallbacks (NULL, &dbCallbacks);
	}

done:
      SetControlData (keyval_t->key_entry, 0, kControlEditTextTextTag, 0,
	  "");
      DrawOneControl (keyval_t->key_entry);
      SetControlData (keyval_t->value_entry, 0, kControlEditTextTextTag, 0,
	  "");
      DrawOneControl (keyval_t->value_entry);
      DeactivateControl (KEYVAL->bupdate);
      DrawOneControl (KEYVAL->bupdate);
    }

  return noErr;
}

pascal OSStatus
keyval_update_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TKEYVAL *keyval_t = (TKEYVAL *) inUserData;
  DataBrowserItemID item = DBITEM_ID + 1, first, last;
  DataBrowserCallbacks dbCallbacks;
  ThemeDrawingState outState = NULL;
  UInt16 colSize[2] = { 150, 250 };
  SInt16 outBaseline;
  Point ioBound;
  CFStringRef data[2];
  Size len;
  int i = 0, j;

  if (keyval_t)
    {
      GetThemeDrawingState (&outState);

      GetControlData (keyval_t->key_entry, 0, kControlEditTextCFStringTag,
	  sizeof (CFStringRef), &data[0], &len);
      if (CFStringGetLength(data[0]))
	{
	  GetControlData (keyval_t->value_entry, 0, kControlEditTextCFStringTag,
              sizeof (CFStringRef), &data[1], &len);

          if(GetDataBrowserSelectionAnchor (keyval_t->key_list, &first, &last) == noErr)
            {
              i = first - DBITEM_ID - 1;
              item += i;
            }
          else i = 0;

	  /* Install an event handler on the component databrowser */
	  dbCallbacks.version = kDataBrowserLatestCallbacks;
	  InitDataBrowserCallbacks (&dbCallbacks);
	  dbCallbacks.u.v1.itemNotificationCallback =
	      NewDataBrowserItemNotificationUPP (keyval_notification_item);
          /* On Mac OS X 10.0.x : this is clientDataCallback */
	  dbCallbacks.u.v1.itemDataCallback =
	      NewDataBrowserItemDataUPP (keyval_getset_item);
	  SetDataBrowserCallbacks (keyval_t->key_list, &dbCallbacks);
	  /* Begin the draw of the data browser */
	  SetDataBrowserTarget (keyval_t->key_list, DBITEM_ID);

          /* An update operation */
          if(i<KEYVAL_nrows)
            {
              CFRelease (KEYVAL_array[0][i]);
              CFRelease (KEYVAL_array[1][i]);
              KEYVAL_array[0][i] = data[0];
              KEYVAL_array[1][i] = data[1];
              UpdateDataBrowserItems (keyval_t->key_list, DBITEM_ID, 1,
                  &item, GSKEYWORD_ID, kDataBrowserItemNoProperty);
            }

          for(j = 0 ; j < KEYVAL_nrows ; j++)
            {
              for(i = 0 ; i < 2 ; i++)
                {
                  GetThemeTextDimensions (KEYVAL_array[i][j], kThemeSystemFont,
                    kThemeStateActive, false, &ioBound, &outBaseline);
                  if(colSize[i] < ioBound.h) colSize[i] = ioBound.h;
                }
            }

	  ActivateControl (keyval_t->key_list);
	  /* Resize the columns to have a good look */
          SetDataBrowserTableViewNamedColumnWidth (keyval_t->key_list, GSKEYWORD_ID, colSize[0] + 20);
          SetDataBrowserTableViewNamedColumnWidth (keyval_t->key_list, GSVALUE_ID, colSize[1] + 20);
	  DrawOneControl (keyval_t->key_list);
	  /* Remove the DataBrowser callback */
	  SetDataBrowserCallbacks (NULL, &dbCallbacks);
	}

      SetControlData (keyval_t->key_entry, 0, kControlEditTextTextTag, 0,
	  "");
      DrawOneControl (keyval_t->key_entry);
      SetControlData (keyval_t->value_entry, 0, kControlEditTextTextTag, 0,
	  "");
      DrawOneControl (keyval_t->value_entry);
      DeactivateControl (KEYVAL->bupdate);
      DrawOneControl (KEYVAL->bupdate);
    }

  return noErr;
}

pascal OSStatus
keyval_ok_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TKEYVAL *keyval_t = (TKEYVAL *) inUserData;
  char *cour;
  int i = 0, size = 1;
  char msg[1024], msg1[1024];

  if (keyval_t)
    {
      /* What is the size of the block to malloc */
      keyval_t->connstr = calloc(sizeof(char), 2);
      for (i = 0; i < KEYVAL_nrows; i++)
        {
          CFStringGetCString (KEYVAL_array[0][i], msg, sizeof (msg),
            kCFStringEncodingUTF8);
          CFStringGetCString (KEYVAL_array[1][i], msg1, sizeof (msg1),
            kCFStringEncodingUTF8);

          cour = (char *) keyval_t->connstr;
          keyval_t->connstr =
                (LPSTR) malloc (size + STRLEN (msg) + STRLEN (msg1) + 2);
          if (keyval_t->connstr)
            {
              memcpy (keyval_t->connstr, cour, size);
              sprintf (((char*)keyval_t->connstr) + size - 1, "%s=%s", msg, msg1);
              if (cour)
                free (cour);
              size += STRLEN (msg) + STRLEN (msg1) + 2;
            }
          else
            keyval_t->connstr = cour;
        }

      ((char*)keyval_t->connstr)[size - 1] = '\0';

      keyval_t->verify_conn = GetControlValue (keyval_t->verify_conn_cb) != 0;
      DisposeWindow (keyval_t->mainwnd);
      keyval_t->mainwnd = NULL;
      keyval_t->verify_conn_cb =  NULL;
      keyval_t->key_list = NULL;
      KEYVAL = NULL;
    }

  return noErr;
}

pascal OSStatus
keyval_cancel_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TKEYVAL *keyval_t = (TKEYVAL *) inUserData;

  if (keyval_t)
    {
      keyval_t->connstr = NULL;
      keyval_t->verify_conn_cb = NULL;
      keyval_t->key_list = NULL;

      KEYVAL = NULL;
      DisposeWindow (keyval_t->mainwnd);
      keyval_t->mainwnd = NULL;
    }

  return noErr;
}

LPSTR create_keyval (WindowRef wnd, LPCSTR attrs, BOOL *verify_conn)
{
  EventTypeSpec controlSpec = { kEventClassControl, kEventControlHit };
  RgnHandle cursorRgn = NULL;
  TKEYVAL keyval_t;
  ControlID controlID;
  WindowRef wkeyval;
  ControlRef control;
  EventRecord event;
  IBNibRef nibRef;
  OSStatus err;

  /* Search the bundle for a .nib file named 'odbcadmin'. */
  err = CreateNibReferenceWithCFBundle (CFBundleGetBundleWithIdentifier (
          CFSTR ("org.iodbc.adm")), CFSTR ("keyval"), &nibRef);
  if (err == noErr)
    {
      /* Nib found ... so create the window */
      CreateWindowFromNib (nibRef, CFSTR ("Dialog"), &wkeyval);
      DisposeNibReference (nibRef);

      /* Install handlers for the finish button, the cancel */
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, GSVERIFYCONN_CNTL, wkeyval,
	  keyval_t.verify_conn_cb);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, GSLIST_CNTL, wkeyval,
	  keyval_t.key_list);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, GSKEYWORD_CNTL, wkeyval,
	  keyval_t.key_entry);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, GSVALUE_CNTL, wkeyval,
	  keyval_t.value_entry);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, GSUPDATE_CNTL, wkeyval,
	  keyval_t.bupdate);

      GETCONTROLBYID (controlID, CNTL_SIGNATURE, GSADD_CNTL, wkeyval,
	  control);
      InstallEventHandler (GetControlEventTarget (control),
	  NewEventHandlerUPP (keyval_add_clicked), 1, &controlSpec,
	  &keyval_t, NULL);
      InstallEventHandler (GetControlEventTarget (keyval_t.bupdate),
	  NewEventHandlerUPP (keyval_update_clicked), 1, &controlSpec,
	  &keyval_t, NULL);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, GSOK_CNTL, wkeyval,
	  control);
      InstallEventHandler (GetControlEventTarget (control),
	  NewEventHandlerUPP (keyval_ok_clicked), 1, &controlSpec,
	  &keyval_t, NULL);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, GSCANCEL_CNTL, wkeyval,
	  control);
      InstallEventHandler (GetControlEventTarget (control),
	  NewEventHandlerUPP (keyval_cancel_clicked), 1, &controlSpec,
	  &keyval_t, NULL);

      /* Parse the attributes line */
      keyval_t.mainwnd = wkeyval;
      addkeywords_to_list (keyval_t.key_list, attrs, &keyval_t);

      AdvanceKeyboardFocus (wkeyval);
      /* Show the window and run the loop */
      DeactivateControl (keyval_t.bupdate);
      KEYVAL = &keyval_t;
      ShowSheetWindow(wkeyval, wnd);
      /* The main loop */
      while (keyval_t.mainwnd)
        WaitNextEvent (everyEvent, &event, 60L, cursorRgn);

      if (keyval_t.connstr)
        *verify_conn = keyval_t.verify_conn;
    }
  else
    goto error;

  return keyval_t.connstr;

error:
  fprintf (stderr, "Can't load Window. Err: %d\n", (int) err);
  return keyval_t.connstr;
}
