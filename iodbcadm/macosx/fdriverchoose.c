/*
 *  fdriverchooser.c
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

extern UInt32 Drivers_nrows;
extern CFStringRef Drivers_array[4][100];

#define	NDCONTINUE_CNTL	1000
#define	NDBACK_CNTL	1001
#define NDCANCEL_CNTL	1002
#define NDADVANCED_CNTL	1003
#define NDLIST_CNTL	2000
#define NDDSN_CNTL	2001
#define NDBROWSE_CNTL   2002
#define NDMESS_CNTL	2003

#define GLOBAL_TAB	128
#define NDDRV_TAB	129
#define NDNAME_TAB	130
#define NDFINISH_TAB	131


#define MESSITEM_ID     'OPLs'
#define MESSLINE_ID	'mess'

CFStringRef Mess_array[512];
UInt32 Mess_nrows;

void SQL_API _iodbcdm_messagebox (HWND hwnd, LPCSTR szDSN, LPCSTR szText);


static pascal OSStatus
mess_getset_item (ControlRef browser,
    DataBrowserItemID itemID, DataBrowserPropertyID property,
    DataBrowserItemDataRef itemData, Boolean changeValue)
{
  OSStatus err = noErr;

  if (!changeValue)
    switch (property)
      {
      case MESSLINE_ID:
        if (Mess_nrows)
	  SetDataBrowserItemDataText (itemData,
	    Mess_array[itemID - MESSITEM_ID - 1]);
	break;

      case kDataBrowserItemIsActiveProperty:
	if (Mess_nrows && itemID > MESSITEM_ID && itemID <= MESSITEM_ID + Mess_nrows)
	  err = SetDataBrowserItemDataBooleanValue (itemData, true);
	break;

      case kDataBrowserItemIsEditableProperty:
	err = SetDataBrowserItemDataBooleanValue (itemData, true);
	break;

      case kDataBrowserItemIsContainerProperty:
	err = SetDataBrowserItemDataBooleanValue (itemData,
	    itemID == MESSITEM_ID);
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
    {
      err = errDataBrowserPropertyNotSupported;
    }

  return err;
} 


static void UpdateMessagePage(TFDRIVERCHOOSER *choose_t)
{
  int i;
  CFStringRef driver_str;
  DataBrowserItemID item = MESSITEM_ID + 1;
  DataBrowserCallbacks dbCallbacks;
  ControlRef widget = choose_t->mess_entry;
  LPSTR curr;
  DataBrowserItemID first, last;
  OSStatus err;
  char dsn[1024];
  char path[1024];
  Size len;

  /* Install an event handler on the component databrowser */
  dbCallbacks.version = kDataBrowserLatestCallbacks;
  InitDataBrowserCallbacks (&dbCallbacks);
  dbCallbacks.u.v1.itemDataCallback = NewDataBrowserItemDataUPP (mess_getset_item);
  SetDataBrowserCallbacks (widget, &dbCallbacks);
  /* Begin the draw of the data browser */
  SetDataBrowserTarget (widget, MESSITEM_ID);

  for(i = 0; i < Mess_nrows; i++, item++)
    {
      if (Mess_array[i])
        CFRelease(Mess_array[i]);
      RemoveDataBrowserItems (widget, MESSITEM_ID, 1, &item, MESSLINE_ID);
    }

  Mess_nrows = 0;

  Mess_array[Mess_nrows] = CFStringCreateWithCString (NULL, "File Data Source", kCFStringEncodingUTF8);
  Mess_nrows++;

  GetControlData (choose_t->dsn_entry, 0, kControlEditTextTextTag, 
        sizeof (dsn), dsn, &len);
  dsn[len] = '\0';
  if (strchr (dsn, '/') != NULL)
    snprintf(path, sizeof(path), "Filename: %s", dsn);
  else
    snprintf(path, sizeof(path), "Filename: %s/%s", choose_t->curr_dir, dsn);

  Mess_array[Mess_nrows] = CFStringCreateWithCString (NULL, path, kCFStringEncodingUTF8);
  Mess_nrows++;

  driver_str = CFSTR("");
  if ((err = GetDataBrowserSelectionAnchor (choose_t->driverlist, &first,
	  &last)) == noErr)
    {
      if (first > DBITEM_ID && first <= DBITEM_ID + Drivers_nrows)
        driver_str = Drivers_array[0][first - DBITEM_ID - 1];
    }
  
  Mess_array[Mess_nrows] = CFStringCreateMutable(NULL, 0);
  CFStringAppend(Mess_array[Mess_nrows], CFSTR("Driver: "));
  CFStringAppend(Mess_array[Mess_nrows], driver_str);
  Mess_nrows++;

  Mess_array[Mess_nrows] = CFStringCreateWithCString (NULL, "Driver-specific Keywords:", kCFStringEncodingUTF8);
  Mess_nrows++;

  if (choose_t->attrs)
    {
      for (curr = choose_t->attrs; *curr; curr += (STRLEN (curr) + 1))
        {
          if (!strncasecmp (curr, "PWD=", STRLEN ("PWD=")))
            {
	        continue;
	    }

          Mess_array[Mess_nrows] = CFStringCreateWithCString (NULL, curr, kCFStringEncodingUTF8);
          Mess_nrows++;
        }
    }

  item = MESSITEM_ID + 1;
  for(i = 0; i < Mess_nrows; i++)
    {
      AddDataBrowserItems (widget, MESSITEM_ID, 1, &item, MESSLINE_ID);
      item++;
    }

  ActivateControl (widget);
  DrawOneControl (widget);
  /* Remove the DataBrowser callback */
  SetDataBrowserCallbacks (NULL, &dbCallbacks);
}


pascal OSStatus fdriverchooser_ok_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData);

pascal OSStatus
fdriverchooser_switch_page (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TFDRIVERCHOOSER *choose_t = (TFDRIVERCHOOSER *) inUserData;
  int tabs[] = { 3, NDDRV_TAB, NDNAME_TAB, NDFINISH_TAB };
  ControlRef tabControl;
  ControlID controlID;
  int tab_index;
  CFStringRef str;
  char dsn[1024];
  Size len;
  DataBrowserItemID first, last;
  Boolean selected;

  /* Search which tab is activated */
  controlID.signature = TABS_SIGNATURE;
  controlID.id = GLOBAL_TAB;
  GetControlByID (choose_t->mainwnd, &controlID, &tabControl);
  DisplayTabControlNumber (tab_index =
      GetControlValue (tabControl), tabControl, choose_t->mainwnd, tabs);

  /* Is the panel has been changed */
  if(choose_t->tab_number == tab_index)
    return noErr;

  ClearKeyboardFocus (choose_t->mainwnd);

  selected = false;
  /* Get the selection */
  if (GetDataBrowserSelectionAnchor(choose_t->driverlist, &first, &last) == noErr 
      && first > DBITEM_ID && first <= DBITEM_ID + Drivers_nrows)
    {
      selected = true;
    }

  switch ((choose_t->tab_number = tab_index))
    {
    case 1:
      DeactivateControl (choose_t->back_button);
      DrawOneControl (choose_t->back_button);
      str = CFStringCreateWithCString(NULL, "Continue", kCFStringEncodingUTF8);
      SetControlTitleWithCFString(choose_t->continue_button, str);
      CFRelease(str);
      break;
    case 2:
      if (!selected)
        {
          _iodbcdm_messagebox (choose_t->mainwnd, NULL, "Driver wasn't selected!");
          SetControlValue (tabControl, 1);
          DisplayTabControlNumber (choose_t->tab_number = 1, tabControl, choose_t->mainwnd, tabs);
          return noErr;
        }
      ActivateControl (choose_t->back_button);
      DrawOneControl (choose_t->back_button);
      str = CFStringCreateWithCString(NULL, "Continue", kCFStringEncodingUTF8);
      SetControlTitleWithCFString(choose_t->continue_button, str);
      CFRelease(str);
      break;
    case 3:
      if (!selected)
        {
          _iodbcdm_messagebox (choose_t->mainwnd, NULL, "Driver wasn't selected!");
          SetControlValue (tabControl, 1);
          DisplayTabControlNumber (choose_t->tab_number = 1, tabControl, choose_t->mainwnd, tabs);
          return noErr;
        }
      GetControlData (choose_t->dsn_entry, 0, kControlEditTextTextTag, 
          sizeof (dsn), dsn, &len);
      if (strlen(dsn) < 1)
        {
          _iodbcdm_messagebox (choose_t->mainwnd, NULL, "Enter File DSN Name...");
          SetControlValue (tabControl, 2);
          DisplayTabControlNumber (choose_t->tab_number = 2, tabControl, choose_t->mainwnd, tabs);
          return noErr;
        }
      ActivateControl (choose_t->back_button);
      DrawOneControl (choose_t->back_button);
      str = CFStringCreateWithCString(NULL, "Finish", kCFStringEncodingUTF8);
      SetControlTitleWithCFString(choose_t->continue_button, str);
      CFRelease(str);
      UpdateMessagePage(choose_t);
      break;      
    };

  ClearKeyboardFocus (choose_t->mainwnd);
  AdvanceKeyboardFocus (choose_t->mainwnd);

  return noErr;
}


pascal OSStatus
fdriverchooser_back_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TFDRIVERCHOOSER *choose_t = (TFDRIVERCHOOSER *) inUserData;
  int tabs[] = { 3, NDDRV_TAB, NDNAME_TAB, NDFINISH_TAB };
  CFStringRef str;

  if (choose_t)
    {
      ClearKeyboardFocus (choose_t->mainwnd);

      str = CFStringCreateWithCString(NULL, "Continue", kCFStringEncodingUTF8);
      SetControlTitleWithCFString(choose_t->continue_button, str);
      CFRelease(str);

      /* Go to the next tab */
      if (choose_t->tab_number <= 1)
        {
          choose_t->tab_number = tabs[0];
          ActivateControl (choose_t->back_button);
        }
      else
        {
          choose_t->tab_number--;
          
          if (choose_t->tab_number == 1)
            DeactivateControl (choose_t->back_button);
          else
            ActivateControl (choose_t->back_button);
        }

      DrawOneControl (choose_t->back_button);

      SetControlValue (choose_t->tab_panel, choose_t->tab_number);
      DisplayTabControlNumber (choose_t->tab_number, choose_t->tab_panel,
          choose_t->mainwnd, tabs);

      ClearKeyboardFocus (choose_t->mainwnd);
      AdvanceKeyboardFocus (choose_t->mainwnd);
    }

  return noErr;
}

pascal OSStatus
fdriverchooser_continue_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TFDRIVERCHOOSER *choose_t = (TFDRIVERCHOOSER *) inUserData;
  int tabs[] = { 3, NDDRV_TAB, NDNAME_TAB, NDFINISH_TAB };
  CFStringRef str;
  ControlRef tabControl;
  ControlID controlID;

  if (choose_t)
    {
      if (choose_t->tab_number == tabs[0])
        return fdriverchooser_ok_clicked(inHandlerRef, inEvent, inUserData);
      
      ClearKeyboardFocus (choose_t->mainwnd);

      /* Go to the next tab */
      if (choose_t->tab_number >= tabs[0])
        {
          choose_t->tab_number = 1;
          DeactivateControl (choose_t->back_button);
        }
      else
        {
          char dsn[1024];
          Size len;

          if (choose_t->tab_number == 2)
            {
              controlID.signature = TABS_SIGNATURE;
              controlID.id = GLOBAL_TAB;
              GetControlByID (choose_t->mainwnd, &controlID, &tabControl);
              
              GetControlData (choose_t->dsn_entry, 0, kControlEditTextTextTag, 
                sizeof (dsn), dsn, &len);
              if (strlen(dsn) < 1)
                {
                  _iodbcdm_messagebox (choose_t->mainwnd, NULL, "Enter File DSN Name...");
                  SetControlValue (tabControl, 2);
                  DisplayTabControlNumber (2, tabControl, choose_t->mainwnd, tabs);
                  return noErr;
                }
            }
          
          choose_t->tab_number++;

          ActivateControl (choose_t->back_button);
          
          if (choose_t->tab_number == tabs[0])
            {
              str = CFStringCreateWithCString(NULL, "Finish", kCFStringEncodingUTF8);
              SetControlTitleWithCFString(choose_t->continue_button, str);
              CFRelease(str);
              UpdateMessagePage(choose_t);
            }
          else
            {
              str = CFStringCreateWithCString(NULL, "Continue", kCFStringEncodingUTF8);
              SetControlTitleWithCFString(choose_t->continue_button, str);
              CFRelease(str);
            }
        }

      DrawOneControl (choose_t->back_button);

      SetControlValue (choose_t->tab_panel, choose_t->tab_number);
      DisplayTabControlNumber (choose_t->tab_number, choose_t->tab_panel,
          choose_t->mainwnd, tabs);

      ClearKeyboardFocus (choose_t->mainwnd);
      AdvanceKeyboardFocus (choose_t->mainwnd);
    }

  return noErr;
}




pascal OSStatus
fdriverchooser_advanced_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TFDRIVERCHOOSER *choose_t = (TFDRIVERCHOOSER *) inUserData;
  DataBrowserItemID first, last;
  OSStatus err;

  if (choose_t)
    {
      /* Get the selection */
      if ((err = GetDataBrowserSelectionAnchor (choose_t->driverlist, &first,
	       &last)) == noErr)
	{
	  if (first > DBITEM_ID && first <= DBITEM_ID + Drivers_nrows)
	    {
	      LPSTR attr_lst = NULL;
	      LPSTR in_attrs = choose_t->attrs ? choose_t->attrs : "\0\0";

              attr_lst = create_keyval(choose_t->mainwnd, in_attrs, &choose_t->verify_conn);
              if (attr_lst)
                {
                  if (choose_t->attrs)
                    free(choose_t->attrs);
                  choose_t->attrs = attr_lst;
                }
	    }
	  else
           _iodbcdm_messagebox (choose_t->mainwnd, NULL, "Driver wasn't selected!");
	}
    }

  return noErr;
}



pascal OSStatus
fdriverchooser_ok_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TFDRIVERCHOOSER *choose_t = (TFDRIVERCHOOSER *) inUserData;
  DataBrowserItemID first, last;
  OSStatus err;
  char dsn[1024];
  char path[1024];
  Size len;

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

      GetControlData (choose_t->dsn_entry, 0, kControlEditTextTextTag, 
        sizeof (dsn), dsn, &len);
      dsn[len] = '\0';

      if (strchr (dsn, '/') != NULL)
        snprintf(path, sizeof(path), "%s", dsn);
      else
        snprintf(path, sizeof(path), "%s/%s", choose_t->curr_dir, dsn);

      choose_t->dsn = strdup(path);

      DisposeWindow (choose_t->mainwnd);
      choose_t->mainwnd = NULL;
      choose_t->driverlist = NULL;
      choose_t->dsn_entry = NULL;
      Drivers_nrows = 0;
      choose_t->ok = TRUE;
    }

  return noErr;
}


pascal OSStatus
fdriverchooser_cancel_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TFDRIVERCHOOSER *choose_t = (TFDRIVERCHOOSER *) inUserData;

  if (choose_t)
    {
      DisposeWindow (choose_t->mainwnd);
	  choose_t->mainwnd = NULL;
      /* No driver choosen ... cancel pressed */
      if (choose_t->driver)
        {
          free(choose_t->driver);
          choose_t->driver = NULL;
        }
      if (choose_t->dsn)
        {
          free(choose_t->dsn);
          choose_t->dsn = NULL;
        }
      if (choose_t->attrs)
        {
          free(choose_t->attrs);
          choose_t->attrs = NULL;
        }
      choose_t->driverlist = NULL;
      Drivers_nrows = 0;
      choose_t->ok = FALSE;
    }

  return noErr;
}



static pascal void
fdriverchooser_nav_events (NavEventCallbackMessage callBackSelector,
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

pascal OSStatus
fdriverchooser_browse_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  NavDialogOptions dialogOptions;
  TFDRIVERCHOOSER *choose_t = (TFDRIVERCHOOSER *) inUserData;
  NavReplyRecord reply;
  char tokenstr[4096] = { 0 };
  OSStatus err;
  FSSpec file;
  char *dir;
  AEDesc defaultLoc;
  CFStringRef str;
  CFURLRef url;
  Str255 param;
  char path[4096];
  char buf[4096];
  Size len;

  GetControlData (choose_t->dsn_entry, 0, kControlEditTextTextTag, 
        sizeof (buf), buf, &len);
  buf[len] = '\0';

  if (STRLEN(buf)==0)
    STRCPY(path, "xxx.dsn");
  else
    {
      char *s = strrchr(buf,'/');
      STRCPY(path, s?s+1:buf);
    }

  NavGetDefaultDialogOptions (&dialogOptions);
  STRCPY (dialogOptions.windowTitle + 1, "Save as ...");
  dialogOptions.windowTitle[0] = STRLEN ("Save as ...");
  STRCPY (dialogOptions.savedFileName + 1, path);
  dialogOptions.savedFileName[0] = STRLEN (path);

  str = CFStringCreateWithCString(NULL, choose_t->curr_dir, kCFStringEncodingMacRoman);
  url = CFURLCreateWithFileSystemPath(NULL, str, kCFURLHFSPathStyle, TRUE);
  CFRelease(str);
  str = CFURLCopyFileSystemPath(url, kCFURLHFSPathStyle);
  CFStringGetCString(str, path, sizeof(path), kCFStringEncodingUTF8);

  param[0] =STRLEN(path);
  STRCPY(param + 1, path);
  err = FSMakeFSSpec(0, 0, param, &file);

  AECreateDesc(typeFSS, &file, sizeof(file), &defaultLoc);

  dialogOptions.dialogOptionFlags |= kNavAllowPreviews|kNavAllowStationery;

  err = NavPutFile (&defaultLoc, &reply, &dialogOptions,
    NewNavEventUPP (fdriverchooser_nav_events),
    'TEXT', kNavGenericSignature, NULL);

  if (!err && reply.validRecord)
    {
      /* Get and transform the file descriptor from the reply */
      err = AEGetNthPtr (&reply.selection, 1, typeFSS, NULL, NULL, &file,
	  sizeof (file), NULL);
      if (!err)
	{
	  int l;
	  /* Get back some information about the directory */
	  dir = get_full_pathname (file.parID, file.vRefNum);
	  sprintf (tokenstr, "%s/", dir ? dir : "");
	  strncat (tokenstr, &file.name[1], file.name[0]);
	  /* Display the constructed string re. the file choosen */
	  l = STRLEN(tokenstr);

	  if (l == 0)
	    STRCPY(buf, "xxx.dsn");
	  else if (l > 4 && strcmp(tokenstr+l-4, ".dsn")!=0)
	    snprintf(buf, sizeof(buf), "%s.dsn", tokenstr);
	  else
	    STRNCPY(buf, tokenstr, sizeof(buf));

	  buf[sizeof(buf)-1]=0;

	  SetControlData (choose_t->dsn_entry, 0,
	      kControlEditTextTextTag, STRLEN(buf), buf);
	  DrawOneControl (choose_t->dsn_entry);
	  /* Clean up */
	  if (dir)
	    free (dir);
	}
    }

  NavDisposeReply (&reply);
  return noErr;
}


void
create_fdriverchooser (HWND hwnd, TFDRIVERCHOOSER * choose_t)
{
  EventTypeSpec controlSpec = { kEventClassControl, kEventControlHit };
  int tabs[] = { 3, NDDRV_TAB, NDNAME_TAB, NDFINISH_TAB };
  RgnHandle cursorRgn = NULL;
  WindowRef wdrvchooser;
  ControlID controlID;
  EventRecord event;
  IBNibRef nibRef;
  OSStatus err;
  ControlRef control;
  int i;

  choose_t->attrs = NULL;
  choose_t->verify_conn = TRUE;
  choose_t->driver = NULL;
  choose_t->dsn = NULL;
  choose_t->ok = FALSE;

  Mess_nrows = 0;
  memset(&Mess_array, 0, sizeof(Mess_array));

  if (hwnd == NULL)
    return;

  /* Search the bundle for a .nib file named 'odbcadmin'. */
  err =
      CreateNibReferenceWithCFBundle (CFBundleGetBundleWithIdentifier (CFSTR
	("org.iodbc.adm")), CFSTR ("odbcdriverf"), &nibRef);
  if (err == noErr)
    {
      /* Nib found ... so create the window */
      CreateWindowFromNib (nibRef, CFSTR ("Dialog"), &wdrvchooser);
      DisposeNibReference (nibRef);
      /* Set the control into the structure */
      GETCONTROLBYID (controlID, TABS_SIGNATURE, GLOBAL_TAB, wdrvchooser,
	  choose_t->tab_panel);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, NDLIST_CNTL, wdrvchooser,
	  choose_t->driverlist);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, NDCONTINUE_CNTL, wdrvchooser,
	  choose_t->continue_button);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, NDBACK_CNTL, wdrvchooser,
	  choose_t->back_button);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, NDDSN_CNTL, wdrvchooser,
	  choose_t->dsn_entry);
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, NDMESS_CNTL, wdrvchooser,
	  choose_t->mess_entry);
      
      choose_t->mainwnd = wdrvchooser;

      /* Install handlers for the finish button, the cancel */
      GETCONTROLBYID (controlID, CNTL_SIGNATURE, NDCANCEL_CNTL, wdrvchooser,
          control);
      InstallEventHandler (GetControlEventTarget (control),
	  NewEventHandlerUPP (fdriverchooser_cancel_clicked), 1, &controlSpec,
	  choose_t, NULL);
      GETCONTROLBYID(controlID, CNTL_SIGNATURE, NDADVANCED_CNTL, wdrvchooser, 
          control);
      InstallEventHandler (GetControlEventTarget (control),
         NewEventHandlerUPP (fdriverchooser_advanced_clicked), 1, &controlSpec,
	 choose_t, NULL);

      GETCONTROLBYID(controlID, CNTL_SIGNATURE, NDBROWSE_CNTL, wdrvchooser, 
          control);
      InstallEventHandler (GetControlEventTarget (control),
         NewEventHandlerUPP (fdriverchooser_browse_clicked), 1, &controlSpec,
	 choose_t, NULL);

      InstallEventHandler (GetControlEventTarget (choose_t->continue_button),
	  NewEventHandlerUPP (fdriverchooser_continue_clicked), 1, &controlSpec,
	  choose_t, NULL);
      InstallEventHandler (GetControlEventTarget (choose_t->back_button),
	  NewEventHandlerUPP (fdriverchooser_back_clicked), 1, &controlSpec,
	  choose_t, NULL);
      InstallEventHandler (GetControlEventTarget (choose_t->tab_panel),
	  NewEventHandlerUPP (fdriverchooser_switch_page), 1, &controlSpec,
	  choose_t, NULL);

      Drivers_nrows = 0;

      adddrivers_to_list (choose_t->driverlist, choose_t->mainwnd, FALSE);

      /* Show the window and run the loop */
      /* Force to display the first tab */
      DisplayTabControlNumber (choose_t->tab_number = 1, choose_t->tab_panel, wdrvchooser, tabs);
      DeactivateControl (choose_t->back_button);
      DrawOneControl (choose_t->back_button);
      ActivateControl (choose_t->continue_button);
      DrawOneControl (choose_t->continue_button);

      ShowWindow (wdrvchooser);

      /* The main loop */
      while (choose_t->mainwnd)
	WaitNextEvent (everyEvent, &event, 60L, cursorRgn);
    }
  else
    goto error;

  for(i = 0; i < Mess_nrows; i++)
    if (Mess_array[i])
      CFRelease(Mess_array[i]);
  Mess_nrows = 0;

  return;

error:
  choose_t->driver = NULL;
  fprintf (stderr, "Can't load Window. Err: %d\n", (int) err);
  return;
}
