/*
 *  gensetup.c
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

#define GSDSN_CNTL	4007
#define GSCOMMENT_CNTL	4008
#define GSLIST_CNTL	4006
#define GSKEYWORD_CNTL	4004
#define GSVALUE_CNTL	4005
#define GSADD_CNTL	4002
#define GSUPDATE_CNTL	4003
#define GSOK_CNTL	4000
#define GSCANCEL_CNTL	4001

static char *STRCONN = "DSN=%s\0Description=%s\0\0";
static int STRCONN_NB_TOKENS = 2;

UInt32 DSNSETUP_nrows;
CFStringRef DSNSETUP_array[2][100];

TGENSETUP *DSNSETUP = NULL;

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

static pascal OSStatus
dsnsetup_getset_item (ControlRef browser,
    DataBrowserItemID itemID, DataBrowserPropertyID property,
    DataBrowserItemDataRef itemData, Boolean changeValue)
{
  OSStatus err = noErr;

  if (!changeValue)
    switch (property)
      {
      case GSKEYWORD_ID:
	SetDataBrowserItemDataText (itemData,
	    DSNSETUP_array[0][itemID - DBITEM_ID - 1]);
	break;

      case GSVALUE_ID:
	SetDataBrowserItemDataText (itemData,
	    DSNSETUP_array[1][itemID - DBITEM_ID - 1]);
	break;

      case kDataBrowserItemIsActiveProperty:
	if (itemID > DBITEM_ID && itemID <= DBITEM_ID + DSNSETUP_nrows)
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
dsnsetup_notification_item (ControlRef browser,
    DataBrowserItemID itemID, DataBrowserItemNotification message)
{
  static Boolean ignore_next = false, selected = false;

  switch (message)
    {
    case kDataBrowserItemSelected:
      if (DSNSETUP)
	{
	  ActivateControl (DSNSETUP->bupdate);
	  DrawOneControl (DSNSETUP->bupdate);
	  SetControlData (DSNSETUP->key_entry, 0, kControlEditTextCFStringTag,
	      sizeof(CFStringRef), &DSNSETUP_array[0][itemID - DBITEM_ID - 1]);
	  DrawOneControl (DSNSETUP->key_entry);
	  SetControlData (DSNSETUP->value_entry, 0, kControlEditTextCFStringTag,
	      sizeof(CFStringRef), &DSNSETUP_array[1][itemID - DBITEM_ID - 1]);
	  DrawOneControl (DSNSETUP->value_entry);
	}

      if (selected)
	ignore_next = true;
      else
	selected = true;
      break;

    case kDataBrowserItemDeselected:
      if (!ignore_next && DSNSETUP)
	{
	  DeactivateControl (DSNSETUP->bupdate);
	  DrawOneControl (DSNSETUP->bupdate);
	  SetControlData (DSNSETUP->key_entry, 0, kControlEditTextTextTag, 0,
	      "");
	  DrawOneControl (DSNSETUP->key_entry);
	  SetControlData (DSNSETUP->value_entry, 0, kControlEditTextTextTag,
	      0, "");
	  DrawOneControl (DSNSETUP->value_entry);
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
    TGENSETUP * gensetup_t)
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
      NewDataBrowserItemNotificationUPP (dsnsetup_notification_item);
  /* On Mac OS X 10.0.x : this is clientDataCallback */
  dbCallbacks.u.v1.itemDataCallback =
      NewDataBrowserItemDataUPP (dsnsetup_getset_item);
  SetDataBrowserCallbacks (widget, &dbCallbacks);
  /* Begin the draw of the data browser */
  SetDataBrowserTarget (widget, DBITEM_ID);

  /* Make the clean up */
  for (i = 0; i < DSNSETUP_nrows; i++)
    {
      CFRelease (DSNSETUP_array[0][i]);
      DSNSETUP_array[0][i] = NULL;
      CFRelease (DSNSETUP_array[1][i]);
      DSNSETUP_array[1][i] = NULL;
    }

  /* Global Initialization */
  DSNSETUP_nrows = 0;
  item = DBITEM_ID + 1;

  for (curr = (LPSTR) attrs; *curr; curr += (STRLEN (curr) + 1))
    {
      if (!strncasecmp (curr, "Description=", STRLEN ("Description=")))
        {
          SetControlData (gensetup_t->comment_entry, 0,
            kControlEditTextTextTag, STRLEN (curr + STRLEN ("Description=")),
            curr + STRLEN ("Description="));
          DrawOneControl (gensetup_t->comment_entry);
        }

      if (!strncasecmp (curr, "DSN=", STRLEN ("DSN=")) ||
          !strncasecmp (curr, "Driver=", STRLEN ("Driver=")) ||
          !strncasecmp (curr, "Description=", STRLEN ("Description=")))
        continue;

      if ((cour = strchr (curr, '=')))
        {
          *cour = '\0';
          DSNSETUP_array[0][DSNSETUP_nrows] =
            CFStringCreateWithCString (NULL, curr, kCFStringEncodingUTF8);
          *cour = '=';
          DSNSETUP_array[1][DSNSETUP_nrows] =
            CFStringCreateWithCString (NULL, cour + 1, kCFStringEncodingUTF8);
        }
      else
        DSNSETUP_array[0][DSNSETUP_nrows] =
          CFStringCreateWithCString (NULL, "", kCFStringEncodingUTF8);

      for(i = 0 ; i < 2 ; i++)
        {
          GetThemeTextDimensions (DSNSETUP_array[i][DSNSETUP_nrows], kThemeSystemFont,
            kThemeStateActive, false, &ioBound, &outBaseline);
          if(colSize[i] < ioBound.h) colSize[i] = ioBound.h;
        }

      AddDataBrowserItems (widget, DBITEM_ID, 1, &item, GSKEYWORD_ID);
      item++;
      DSNSETUP_nrows++;
    }

  ActivateControl (widget);
  /* Resize the columns to have a good look */
  SetDataBrowserTableViewNamedColumnWidth (widget, GSKEYWORD_ID, colSize[0] + 20);
  SetDataBrowserTableViewNamedColumnWidth (widget, GSVALUE_ID, colSize[1] + 20);
  DrawOneControl (widget);
  /* Remove the DataBrowser callback */
  SetDataBrowserCallbacks (NULL, &dbCallbacks);
}

static void
parse_attribute_line (TGENSETUP * gensetup_t,
    LPCSTR dsn,
    LPCSTR attrs,
    BOOL add)
{
  if (dsn)
    {
      SetControlData (gensetup_t->dsn_entry, 0, kControlEditTextTextTag,
        STRLEN (dsn), (UInt8 *) dsn);
      if (add)
	DeactivateControl (gensetup_t->dsn_entry);
      else
	ActivateControl (gensetup_t->dsn_entry);
      DrawOneControl (gensetup_t->dsn_entry);
    }

  addkeywords_to_list (gensetup_t->key_list, attrs, gensetup_t);
}

pascal OSStatus
gensetup_add_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TGENSETUP *gensetup_t = (TGENSETUP *) inUserData;
  DataBrowserItemID item = DBITEM_ID + 1;
  DataBrowserCallbacks dbCallbacks;
  ThemeDrawingState outState = NULL;
  UInt16 colSize[2] = { 150, 250 };
  SInt16 outBaseline;
  Point ioBound;
  CFStringRef data[2];
  Size len;
  int i = 0, j;

  if (gensetup_t)
    {
      GetThemeDrawingState (&outState);

      GetControlData (gensetup_t->key_entry, 0, kControlEditTextCFStringTag,
	  sizeof (CFStringRef), &data[0], &len);
      if (CFStringGetLength(data[0]))
	{
	  GetControlData (gensetup_t->value_entry, 0, kControlEditTextCFStringTag,
            sizeof (CFStringRef), &data[1], &len);

	  /* Try to see if the keyword already exists */
	  for (i = 0; i < DSNSETUP_nrows; i++, item++)
	    if (CFStringCompare (data[0], DSNSETUP_array[0][i],
		    0) == kCFCompareEqualTo)
	      goto done;

	  /* Install an event handler on the component databrowser */
	  dbCallbacks.version = kDataBrowserLatestCallbacks;
	  InitDataBrowserCallbacks (&dbCallbacks);
	  dbCallbacks.u.v1.itemNotificationCallback =
	      NewDataBrowserItemNotificationUPP (dsnsetup_notification_item);
          /* On Mac OS X 10.0.x : this is clientDataCallback */
	  dbCallbacks.u.v1.itemDataCallback =
	      NewDataBrowserItemDataUPP (dsnsetup_getset_item);
	  SetDataBrowserCallbacks (gensetup_t->key_list, &dbCallbacks);
	  /* Begin the draw of the data browser */
	  SetDataBrowserTarget (gensetup_t->key_list, DBITEM_ID);

          /* An update operation */
          if(i<DSNSETUP_nrows)
            {
              CFRelease (DSNSETUP_array[1][i]);
              DSNSETUP_array[1][i] = data[1];
              UpdateDataBrowserItems (gensetup_t->key_list, DBITEM_ID, 1, &item,
                GSKEYWORD_ID, kDataBrowserItemNoProperty);
            }
          else if(len)
            {
              DSNSETUP_array[0][i] = data[0];
              DSNSETUP_array[1][i] = data[1];
              AddDataBrowserItems (gensetup_t->key_list, DBITEM_ID, 1, &item,
                GSKEYWORD_ID);
              DSNSETUP_nrows++;
            }

          for(j = 0 ; j < DSNSETUP_nrows ; j++)
            {
              for(i = 0 ; i < 2 ; i++)
                {
                  GetThemeTextDimensions (DSNSETUP_array[i][j], kThemeSystemFont,
                    kThemeStateActive, false, &ioBound, &outBaseline);
                  if(colSize[i] < ioBound.h) colSize[i] = ioBound.h;
                }
            }

	  ActivateControl (gensetup_t->key_list);
	  /* Resize the columns to have a good look */
          SetDataBrowserTableViewNamedColumnWidth (gensetup_t->key_list, GSKEYWORD_ID, colSize[0] + 20);
          SetDataBrowserTableViewNamedColumnWidth (gensetup_t->key_list, GSVALUE_ID, colSize[1] + 20);
	  DrawOneControl (gensetup_t->key_list);
	  /* Remove the DataBrowser callback */
	  SetDataBrowserCallbacks (NULL, &dbCallbacks);
	}

done:
      SetControlData (gensetup_t->key_entry, 0, kControlEditTextTextTag, 0,
	  "");
      DrawOneControl (gensetup_t->key_entry);
      SetControlData (gensetup_t->value_entry, 0, kControlEditTextTextTag, 0,
	  "");
      DrawOneControl (gensetup_t->value_entry);
      DeactivateControl (DSNSETUP->bupdate);
      DrawOneControl (DSNSETUP->bupdate);
    }

  return noErr;
}

pascal OSStatus
gensetup_update_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TGENSETUP *gensetup_t = (TGENSETUP *) inUserData;
  DataBrowserItemID item = DBITEM_ID + 1, first, last;
  DataBrowserCallbacks dbCallbacks;
  ThemeDrawingState outState = NULL;
  UInt16 colSize[2] = { 150, 250 };
  SInt16 outBaseline;
  Point ioBound;
  CFStringRef data[2];
  Size len;
  int i = 0, j;

  if (gensetup_t)
    {
      GetThemeDrawingState (&outState);

      GetControlData (gensetup_t->key_entry, 0, kControlEditTextCFStringTag,
	  sizeof (CFStringRef), &data[0], &len);
      if (CFStringGetLength(data[0]))
	{
	  GetControlData (gensetup_t->value_entry, 0, kControlEditTextCFStringTag,
              sizeof (CFStringRef), &data[1], &len);

          if(GetDataBrowserSelectionAnchor (gensetup_t->key_list, &first, &last) == noErr)
            {
              i = first - DBITEM_ID - 1;
              item += i;
            }
          else i = 0;

	  /* Install an event handler on the component databrowser */
	  dbCallbacks.version = kDataBrowserLatestCallbacks;
	  InitDataBrowserCallbacks (&dbCallbacks);
	  dbCallbacks.u.v1.itemNotificationCallback =
	      NewDataBrowserItemNotificationUPP (dsnsetup_notification_item);
          /* On Mac OS X 10.0.x : this is clientDataCallback */
	  dbCallbacks.u.v1.itemDataCallback =
	      NewDataBrowserItemDataUPP (dsnsetup_getset_item);
	  SetDataBrowserCallbacks (gensetup_t->key_list, &dbCallbacks);
	  /* Begin the draw of the data browser */
	  SetDataBrowserTarget (gensetup_t->key_list, DBITEM_ID);

          /* An update operation */
          if(i<DSNSETUP_nrows)
            {
              CFRelease (DSNSETUP_array[0][i]);
              CFRelease (DSNSETUP_array[1][i]);
              DSNSETUP_array[0][i] = data[0];
              DSNSETUP_array[1][i] = data[1];
              UpdateDataBrowserItems (gensetup_t->key_list, DBITEM_ID, 1,
                  &item, GSKEYWORD_ID, kDataBrowserItemNoProperty);
            }

          for(j = 0 ; j < DSNSETUP_nrows ; j++)
            {
              for(i = 0 ; i < 2 ; i++)
                {
                  GetThemeTextDimensions (DSNSETUP_array[i][j], kThemeSystemFont,
                    kThemeStateActive, false, &ioBound, &outBaseline);
                  if(colSize[i] < ioBound.h) colSize[i] = ioBound.h;
                }
            }

	  ActivateControl (gensetup_t->key_list);
	  /* Resize the columns to have a good look */
          SetDataBrowserTableViewNamedColumnWidth (gensetup_t->key_list, GSKEYWORD_ID, colSize[0] + 20);
          SetDataBrowserTableViewNamedColumnWidth (gensetup_t->key_list, GSVALUE_ID, colSize[1] + 20);
	  DrawOneControl (gensetup_t->key_list);
	  /* Remove the DataBrowser callback */
	  SetDataBrowserCallbacks (NULL, &dbCallbacks);
	}

      SetControlData (gensetup_t->key_entry, 0, kControlEditTextTextTag, 0,
	  "");
      DrawOneControl (gensetup_t->key_entry);
      SetControlData (gensetup_t->value_entry, 0, kControlEditTextTextTag, 0,
	  "");
      DrawOneControl (gensetup_t->value_entry);
      DeactivateControl (DSNSETUP->bupdate);
      DrawOneControl (DSNSETUP->bupdate);
    }

  return noErr;
}

pascal OSStatus
gensetup_ok_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TGENSETUP *gensetup_t = (TGENSETUP *) inUserData;
  char *cour, *curr;
  int i = 0, size = 0;
  char msg[1024], msg1[1024];
  Size len;

  if (gensetup_t)
    {
      /* What is the size of the block to malloc */
      GetControlDataSize (gensetup_t->dsn_entry, 0, kControlEditTextTextTag,
        &len);
      size += len + STRLEN ("DSN=") + 1;
      GetControlDataSize (gensetup_t->comment_entry, 0,
        kControlEditTextTextTag, &len);
      size += len + STRLEN ("Description=") + 1;
      /* Malloc it */
      if ((gensetup_t->connstr = (char *) malloc (++size)))
        {
          for (curr = STRCONN, cour = gensetup_t->connstr;
            i < STRCONN_NB_TOKENS; i++, curr += (STRLEN (curr) + 1))
            switch (i)
              {
                case 0:
                  GetControlData (gensetup_t->dsn_entry, 0,
                    kControlEditTextTextTag, sizeof (msg), msg, &len);
                  msg[len] = '\0';
                  sprintf (cour, curr, msg);
                  cour += (STRLEN (cour) + 1);
                  break;
                case 1:
                  GetControlData (gensetup_t->comment_entry, 0,
                    kControlEditTextTextTag, sizeof (msg), msg, &len);
                  msg[len] = '\0';
                  sprintf (cour, curr, msg);
                    cour += (STRLEN (cour) + 1);
                  break;
                };

              for (i = 0; i < DSNSETUP_nrows; i++)
                {
                  CFStringGetCString (DSNSETUP_array[0][i], msg, sizeof (msg),
                    kCFStringEncodingUTF8);
                  CFStringGetCString (DSNSETUP_array[1][i], msg1, sizeof (msg1),
                    kCFStringEncodingUTF8);

                  cour = (char *) gensetup_t->connstr;
                  gensetup_t->connstr =
                    (LPSTR) malloc (size + STRLEN (msg) + STRLEN (msg1) + 2);
                  if (gensetup_t->connstr)
                    {
                      memcpy (gensetup_t->connstr, cour, size);
                      sprintf (((char*)gensetup_t->connstr) + size - 1, "%s=%s", msg,
                        msg1);
                      free (cour);
                      size += STRLEN (msg) + STRLEN (msg1) + 2;
                    }
                  else
                    gensetup_t->connstr = cour;
                }

              ((char*)gensetup_t->connstr)[size - 1] = '\0';
            }

      DisposeWindow (gensetup_t->mainwnd);
	  gensetup_t->mainwnd = NULL;
      gensetup_t->dsn_entry = gensetup_t->comment_entry = NULL;
      gensetup_t->key_list = NULL;
      DSNSETUP = NULL;
    }

  return noErr;
}

pascal OSStatus
gensetup_cancel_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TGENSETUP *gensetup_t = (TGENSETUP *) inUserData;

  if (gensetup_t)
    {
      gensetup_t->connstr = (LPSTR) - 1L;
      gensetup_t->dsn_entry = gensetup_t->comment_entry = NULL;
      gensetup_t->key_list = NULL;

      DSNSETUP = NULL;
      DisposeWindow (gensetup_t->mainwnd);
	  gensetup_t->mainwnd = NULL;
    }

  return noErr;
}

LPSTR create_gensetup (HWND hwnd, LPCSTR dsn,
    LPCSTR attrs, BOOL add)
{
  EventTypeSpec controlSpec = { kEventClassControl, kEventControlHit };
  RgnHandle cursorRgn = NULL;
  TGENSETUP gensetup_t;
  ControlID controlID;
  WindowRef wgensetup;
  ControlRef control;
  EventRecord event;
  IBNibRef nibRef;
  OSStatus err;
  char msg[1024];

  /* Search the bundle for a .nib file named 'odbcadmin'. */
  err =
      CreateNibReferenceWithCFBundle (CFBundleGetBundleWithIdentifier (CFSTR
	("org.iodbc.drvproxy")), CFSTR ("gensetup"), &nibRef);
  if (err == noErr)
    {
      /* Nib found ... so create the window */
      CreateWindowFromNib (nibRef, CFSTR ("Dialog"), &wgensetup);
      DisposeNibReference (nibRef);
      /* Set the title with the DSN */
      if (dsn)
        {
          msg[0] = STRLEN ("Setup of ") + STRLEN(dsn);
          sprintf (msg+1, "Setup of %s", (char*)dsn);
          SetWTitle (wgensetup, msg);
	}
      /* Install handlers for the finish button, the cancel */
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, GSDSN_CNTL, wgensetup,
	  gensetup_t.dsn_entry);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, GSCOMMENT_CNTL, wgensetup,
	  gensetup_t.comment_entry);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, GSLIST_CNTL, wgensetup,
	  gensetup_t.key_list);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, GSKEYWORD_CNTL, wgensetup,
	  gensetup_t.key_entry);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, GSVALUE_CNTL, wgensetup,
	  gensetup_t.value_entry);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, GSUPDATE_CNTL, wgensetup,
	  gensetup_t.bupdate);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, GSADD_CNTL, wgensetup,
	  control);
      InstallEventHandler (GetControlEventTarget (control),
	  NewEventHandlerUPP (gensetup_add_clicked), 1, &controlSpec,
	  &gensetup_t, NULL);
      InstallEventHandler (GetControlEventTarget (gensetup_t.bupdate),
	  NewEventHandlerUPP (gensetup_update_clicked), 1, &controlSpec,
	  &gensetup_t, NULL);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, GSOK_CNTL, wgensetup,
	  control);
      InstallEventHandler (GetControlEventTarget (control),
	  NewEventHandlerUPP (gensetup_ok_clicked), 1, &controlSpec,
	  &gensetup_t, NULL);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, GSCANCEL_CNTL, wgensetup,
	  control);
      InstallEventHandler (GetControlEventTarget (control),
	  NewEventHandlerUPP (gensetup_cancel_clicked), 1, &controlSpec,
	  &gensetup_t, NULL);
      /* Parse the attributes line */
      gensetup_t.mainwnd = wgensetup;
      parse_attribute_line (&gensetup_t, dsn, attrs, add);
      AdvanceKeyboardFocus (wgensetup);
      /* Show the window and run the loop */
      DeactivateControl (gensetup_t.bupdate);
      DSNSETUP = &gensetup_t;
      ShowWindow (wgensetup);
      /* The main loop */
      while (gensetup_t.mainwnd)
	{
	  switch (WaitNextEvent (everyEvent, &event, 60L, cursorRgn))
	    {
	    };
	}
    }
  else
    goto error;

  return gensetup_t.connstr;

error:
  fprintf (stderr, "Can't load Window. Err: %d\n", (int) err);
  return gensetup_t.connstr;
}
