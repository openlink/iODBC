/*
 *  filedsn.c
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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <pwd.h>


#include <sqlucode.h>
#include <sqltypes.h>
#include <unicode.h>
#include <dlproc.h>

#include "gui.h"

#define MAX_ROWS 1024
UInt32 FDSN_nrows;
CFStringRef FDSN_array[MAX_ROWS];
char  FDSN_type[MAX_ROWS];

extern TDSNCHOOSER *DSNCHOOSER;

static Boolean _CheckDriverLoginDlg (char *drv);

void create_error (HWND hwnd, LPCSTR dsn, LPCSTR text, LPCSTR errmsg);
BOOL create_confirm (HWND hwnd, LPCSTR dsn, LPCSTR text);
void _iodbcdm_nativeerrorbox (HWND hwnd, HENV henv, HDBC hdbc, HSTMT hstmt);
void SQL_API _iodbcdm_messagebox (HWND hwnd, LPCSTR szDSN, LPCSTR szText);

static pascal OSStatus
fdsn_getset_item (ControlRef browser,
    DataBrowserItemID itemID, DataBrowserPropertyID property,
    DataBrowserItemDataRef itemData, Boolean changeValue)
{
  static IconRef folderIcon = 0;
  static IconRef dsnIcon = 0;
  OSStatus err = noErr;

  if (!changeValue)
    switch (property)
      {
      case DBNAME_ID:
        if (folderIcon == 0)
          GetIconRef(kOnSystemDisk, kSystemIconsCreator, kGenericFolderIcon, &folderIcon);
        if (dsnIcon == 0)
          GetIconRef(kOnSystemDisk, kSystemIconsCreator, kGenericDocumentIcon, &dsnIcon);
	
	SetDataBrowserItemDataText (itemData, FDSN_array[itemID - DBITEM_ID - 1]);
	if (FDSN_type[itemID - DBITEM_ID - 1])
          SetDataBrowserItemDataIcon(itemData, dsnIcon);
	else
          SetDataBrowserItemDataIcon(itemData, folderIcon);
	break;

      case kDataBrowserItemIsActiveProperty:
	if (itemID > DBITEM_ID && itemID <= DBITEM_ID + FDSN_nrows)
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
fdsn_notification_item (ControlRef browser,
    DataBrowserItemID itemID, DataBrowserItemNotification message)
{
  static Boolean ignore_next = false, selected = false;

  switch (message)
    {
    case kDataBrowserItemSelected:
      ActivateControl (DSNCHOOSER->fremove);
      DrawOneControl (DSNCHOOSER->fremove);
      ActivateControl (DSNCHOOSER->ftest);
      DrawOneControl (DSNCHOOSER->ftest);
      ActivateControl (DSNCHOOSER->fconfigure);
      DrawOneControl (DSNCHOOSER->fconfigure);

      if (selected)
	ignore_next = true;
      else
	selected = true;
      break;

    case kDataBrowserItemDeselected:
      if (!ignore_next)
	{
	  DeactivateControl (DSNCHOOSER->fremove);
	  DrawOneControl (DSNCHOOSER->fremove);
	  DeactivateControl (DSNCHOOSER->ftest);
	  DrawOneControl (DSNCHOOSER->ftest);
	  DeactivateControl (DSNCHOOSER->fconfigure);
	  DrawOneControl (DSNCHOOSER->fconfigure);
	}
      else
	{
	  ignore_next = false;
	  selected = true;
	}
      break;

    case kDataBrowserItemDoubleClicked:
      filedsn_configure_clicked (NULL, NULL, DSNCHOOSER);
      break;
    };
}


static void fill_dir_menu(TDSNCHOOSER *dsnchoose_t, char *path)
{
  char *curr_dir, *prov, *dir;
  MenuRef items_m;
  int i = -1;
  CFStringRef str;
  ControlRef f_select = dsnchoose_t->fdir;

  if (!path || !(prov = strdup (path)))
    return;

  if (prov[strlen(prov) - 1] == '/' && strlen(prov) > 1)
    prov[strlen(prov) - 1] = 0;

  items_m = GetControlPopupMenuHandle (f_select);
  DeleteMenuItems (items_m, 1, CountMenuItems (items_m));

  /* Add the root directory */
  AppendMenuItemTextWithCFString(items_m, CFSTR("/"), 0, 0, NULL);

  if (strlen(prov) > 1)
    for (curr_dir = prov, dir = NULL; curr_dir;
          curr_dir = strchr (curr_dir + 1, '/'))
      {
        if (strchr (curr_dir + 1, '/'))
	  {
	    dir = strchr (curr_dir + 1, '/');
	    *dir = 0;
	  }

        str = CFStringCreateWithCString(NULL, prov, kCFStringEncodingUTF8);
        AppendMenuItemTextWithCFString(items_m, str, 0, 0, NULL);

        if (dir)
	  *dir = '/';
      }

  i = CountMenuItems (items_m);
  SetControlMaximum (f_select, i);
  SetControlValue (f_select, i);
  strncpy(dsnchoose_t->curr_dir, prov, sizeof(dsnchoose_t->curr_dir));
}


void
addfdsns_to_list (TDSNCHOOSER *dsnchoose_t, char *path, Boolean b_reset)
{
  DataBrowserItemID item = DBITEM_ID + 1;
  DataBrowserCallbacks dbCallbacks;
  ThemeDrawingState outState = NULL;
  UInt16 colSize[3] = { 400, 100, 150 };
  SInt16 outBaseline;
  Point ioBound;
  int i;
  DIR *dir;
  char *path_buf;
  struct dirent *dir_entry;
  struct stat fstat;
  int b_added;
  ControlRef widget;
  WindowRef dlg;

  if (!dsnchoose_t || !path)
    return; 

  widget = dsnchoose_t->fdsnlist;
  dlg = dsnchoose_t->mainwnd;

  GetThemeDrawingState (&outState);

  /* Install an event handler on the component databrowser */
  dbCallbacks.version = kDataBrowserLatestCallbacks;
  InitDataBrowserCallbacks (&dbCallbacks);
  dbCallbacks.u.v1.itemNotificationCallback =
      NewDataBrowserItemNotificationUPP (fdsn_notification_item);
  /* On Mac OS X 10.0.x : clientDataCallback */
  dbCallbacks.u.v1.itemDataCallback =
      NewDataBrowserItemDataUPP (fdsn_getset_item);
  SetDataBrowserCallbacks (widget, &dbCallbacks);
  /* Begin the draw of the data browser */
  SetDataBrowserTarget (widget, DBITEM_ID);

  /* Make the clean up */
  for (i = 0; i < FDSN_nrows; i++, item++)
    {
      CFRelease (FDSN_array[i]);
      FDSN_array[i] = NULL;
      FDSN_type[i] = 0;
      RemoveDataBrowserItems (widget, DBITEM_ID, 1, &item, DBNAME_ID);
    }

  ActivateControl (widget);
  DrawOneControl (widget);

  /* Global Initialization */
  FDSN_nrows = 0;
  item = DBITEM_ID + 1;

  if ((dir = opendir (path)))
    {
      while ((dir_entry = readdir (dir)) && FDSN_nrows < MAX_ROWS)
	{
	  asprintf (&path_buf, "%s/%s", path, dir_entry->d_name);
	  b_added = 0;

	  if (stat ((LPCSTR) path_buf, &fstat) >= 0 && S_ISDIR (fstat.st_mode))
	    {
	      if (dir_entry->d_name && dir_entry->d_name[0] != '.') 
	        {
                  FDSN_array[FDSN_nrows] = CFStringCreateWithCString(NULL, dir_entry->d_name, kCFStringEncodingUTF8);
                  FDSN_type[FDSN_nrows] = 0;
                  b_added = 1;
	        }
	    }
	  else if (stat ((LPCSTR) path_buf, &fstat) >= 0 && !S_ISDIR (fstat.st_mode)
	           && strstr (dir_entry->d_name, ".dsn"))
	    {
              FDSN_array[FDSN_nrows] = CFStringCreateWithCString(NULL, dir_entry->d_name, kCFStringEncodingUTF8);
              FDSN_type[FDSN_nrows] = 1;
              b_added = 1;
	    }

	  if (path_buf)
	    free (path_buf);

	  if (b_added)
	    {
              GetThemeTextDimensions (FDSN_array[FDSN_nrows], kThemeSystemFont,
                kThemeStateActive, false, &ioBound, &outBaseline);
              if(colSize[0] < ioBound.h) colSize[0] = ioBound.h;

              AddDataBrowserItems (widget, DBITEM_ID, 1, &item, DBNAME_ID);
              item++;
              FDSN_nrows++;
            }
	}

      /* Close the directory entry */
      closedir (dir);
    }
  else
    create_error (NULL, NULL, "Error during accessing directory information",
	strerror (errno));

  ActivateControl (widget);
  /* Resize the columns to have a good look */
  SetDataBrowserTableViewNamedColumnWidth (widget, DBNAME_ID, colSize[0] + 20);
  DrawOneControl (widget);
  /* Remove the DataBrowser callback */
  SetDataBrowserCallbacks (NULL, &dbCallbacks);
  if(outState) DisposeThemeDrawingState (outState);

  if (b_reset)
    SetDataBrowserScrollPosition(widget, 0, 0);

  fill_dir_menu(dsnchoose_t, path);
}


static BOOL
test_driver_connect (TDSNCHOOSER *choose_t, char *connstr)
{
  HENV henv;
  HDBC hdbc;

#if (ODBCVER < 0x300)
  if (SQLAllocEnv (&henv) != SQL_SUCCESS)
#else
  if (SQLAllocHandle (SQL_HANDLE_ENV, NULL, &henv) != SQL_SUCCESS)
#endif
    {
      _iodbcdm_nativeerrorbox (choose_t->mainwnd,
	  henv, SQL_NULL_HDBC, SQL_NULL_HSTMT);
      return FALSE;
    }

#if (ODBCVER < 0x300)
  if (SQLAllocConnect (henv, &hdbc) != SQL_SUCCESS)
#else
  SQLSetEnvAttr (henv, SQL_ATTR_ODBC_VERSION,
      (SQLPOINTER) SQL_OV_ODBC3, SQL_IS_UINTEGER);
  if (SQLAllocHandle (SQL_HANDLE_DBC, henv, &hdbc) != SQL_SUCCESS)
#endif
    {
      _iodbcdm_nativeerrorbox (choose_t->mainwnd, henv, hdbc, SQL_NULL_HSTMT);
      SQLFreeEnv (henv);
      return FALSE;
    }

  SQLSetConfigMode (ODBC_BOTH_DSN);

  if (SQLDriverConnect (hdbc, choose_t->mainwnd, connstr, SQL_NTS,
          NULL, 0, NULL, SQL_DRIVER_PROMPT) != SQL_SUCCESS)
    {
      _iodbcdm_nativeerrorbox (choose_t->mainwnd, henv, hdbc, SQL_NULL_HSTMT);
      SQLFreeEnv (henv);
      return FALSE;
    }
  else
    {
      SQLDisconnect (hdbc);
    }

#if (ODBCVER < 0x300)
  SQLFreeConnect (hdbc);
  SQLFreeEnv (henv);
#else
  SQLFreeHandle (SQL_HANDLE_DBC, hdbc);
  SQLFreeHandle (SQL_HANDLE_ENV, henv);
#endif

  return TRUE;
}


static void
filedsn_configure (TDSNCHOOSER *choose_t, char *drv, char *dsn, char *in_attrs,
	BOOL b_add, BOOL verify_conn)
{
  char *connstr = NULL;
  size_t len;			/* current connstr len    */
  size_t add_len;		/* len of appended string */
  LPSTR attrs = NULL, curr, tmp, attr_lst = NULL;
  BOOL b_Save = TRUE;
  
  attrs = in_attrs;

  if (!b_add && !_CheckDriverLoginDlg(drv + STRLEN("DRIVER=")))
    {
      /*  Get DSN name and additional attributes  */
      attr_lst = create_gensetup (choose_t->mainwnd, dsn, in_attrs, 
         b_add, &verify_conn);
      attrs = attr_lst;
    }

  if (!attrs)
    {
      create_error (choose_t->mainwnd, NULL, "Error adding File DSN:",
          strerror (ENOMEM));
      return;
    }
  if (attrs == (LPSTR) - 1L)
    return;


  /* Build the connection string */
  connstr = strdup (drv);
  len = strlen (connstr);
  for (curr = attrs; *curr; curr += (STRLEN (curr) + 1))
    {
      if (!strncasecmp (curr, "DSN=", STRLEN ("DSN=")))
        {
          if (dsn == NULL)
            {
	      /* got dsn name */
              dsn = curr + STRLEN ("DSN=");
            }
	  continue;
	}

      /* append attr */
      add_len = 1 + strlen (curr);			/* +1 for ';' */
      tmp = realloc (connstr, len + add_len + 1);	/* +1 for NUL */
      if (tmp == NULL)
        {
          create_error (choose_t->mainwnd, NULL, "Error adding File DSN:",
	      strerror (errno));
	  goto done;
	}
      connstr = tmp;
      snprintf (connstr + len, add_len + 1, ";%s", curr);
      len += add_len;
    }

  /* Nothing to do if no DSN */
  if (!dsn || STRLEN (dsn) == 0)
    goto done;

  if (verify_conn)
    {
      BOOL ret;

      /* Append SAVEFILE */
      add_len = strlen (";SAVEFILE=") + strlen (dsn);
      tmp = realloc (connstr, len + add_len + 1);		/* +1 for NUL */
      if (tmp == NULL)
        {
          create_error (choose_t->mainwnd, NULL, "Error adding file DSN:",
	      strerror (errno));
          goto done;
        }
      connstr = tmp;
      snprintf (connstr + len, add_len + 1, ";SAVEFILE=%s", dsn);
      len += add_len;

      /* Connect to data source */
      ret = test_driver_connect (choose_t, connstr);
      if (!ret && b_add)
        { 
	  if (create_confirm (choose_t->mainwnd, dsn,
	      "Can't check the connection. Do you want to store the FileDSN without verification ?"))
            b_Save = TRUE;
          else
            b_Save = FALSE;
        }
      else
        b_Save = FALSE;
    }

  if (b_Save)
    {
      char key[512];
      char *p;
      size_t sz;

      if (drv)
        {
	  p = strchr(drv, '=');
          if (!SQLWriteFileDSN (dsn, "ODBC", "DRIVER", p + 1))
            {
              create_error (choose_t->mainwnd, NULL, "Error adding File DSN:",
	          strerror (errno));
	      goto done;
	    }
        }

      for (curr = attrs; *curr; curr += (STRLEN (curr) + 1))
        {
          if (!strncasecmp (curr, "DSN=", STRLEN ("DSN=")))
	    continue;
	  else if (!strncasecmp (curr, "PWD=", STRLEN ("PWD=")))
	    continue;
	  else if (!strncasecmp (curr, "SAVEFILE=", STRLEN ("SAVEFILE=")))
	    continue;
	  else if (!strncasecmp (curr, "FILEDSN=", STRLEN ("FILEDSN=")))
	    continue;

	  p = strchr(curr, '=');
	  sz = p - curr < sizeof(key) ? p - curr : sizeof(key);
	  memset(key, 0, sizeof(key));
	  strncpy(key, curr, sz);

          if (!SQLWriteFileDSN (dsn, "ODBC", key, p + 1))
            {
              create_error (choose_t->mainwnd, NULL, "Error adding File DSN:",
	          strerror (errno));
	      goto done;
	    }
        }
    }

done:
  if (attr_lst != NULL)
    free (attr_lst);
  if (connstr != NULL)
    free (connstr);
}


pascal OSStatus
filedsn_configure_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TDSNCHOOSER *choose_t = (TDSNCHOOSER *) inUserData;
  DataBrowserItemID first, last;
  OSStatus err;
  char str[1024], path[1024];
  int id;
  char *drv = NULL;
  char *attrs = NULL;
  char *_attrs = NULL;	/* attr list */
  size_t len = 0;	/* current attr list length (w/o list-terminating NUL) */
  char *p, *p_next;
  WORD read_len;
  char entries[4096];
  char *curr_dir;

  if (!choose_t)
    return noErr;

  curr_dir = choose_t->curr_dir;

  /* Retrieve the DSN name */
  if ((err = GetDataBrowserSelectionAnchor (choose_t->fdsnlist,
       &first, &last)) == noErr)
    {
      if (first > DBITEM_ID && first <= DBITEM_ID + FDSN_nrows)
        {
          id = first - DBITEM_ID - 1;
          CFStringGetCString(FDSN_array[id], str, sizeof(str), kCFStringEncodingUTF8);

          if (*curr_dir == '/' && strlen(curr_dir) == 1)
            snprintf(path, sizeof(path), "/%s", str);
          else
            snprintf(path, sizeof(path), "%s/%s", curr_dir, str);

          if (FDSN_type[id] == 0)  /* Directory */
            {
              addfdsns_to_list (choose_t, path, true);
            }

          else  /* File DSN*/
            {
              /* Get list of entries in .dsn file */
              if (!SQLReadFileDSN (path, "ODBC", NULL,
		       entries, sizeof (entries), &read_len))
                {
                  create_error (choose_t->mainwnd, NULL, "SQLReadFileDSN failed", NULL);
                  goto done;
                }

              /* add params from the .dsn file */
              for (p = entries; *p != '\0'; p = p_next)
                {
                  char *tmp;
                  size_t add_len;		/* length of added attribute */
                  char value[1024];

                  /* get next entry */
                  p_next = strchr (p, ';');
                  if (p_next)
                    *p_next++ = '\0';

                  if (!SQLReadFileDSN (path, "ODBC", p, value, sizeof(value), &read_len))
                    {
                      create_error (choose_t->mainwnd, NULL, "SQLReadFileDSN failed", NULL);
                      goto done;
                    }

                  if (!strcasecmp (p, "DRIVER"))
                    {
                      /* got driver keyword */
                      add_len = strlen ("DRIVER=") + strlen (value) + 1;
                      drv = malloc (add_len);
                      snprintf (drv, add_len, "DRIVER=%s", value);
                      continue;
                    }

                  /* +1 for '=', +1 for NUL */
                  add_len = strlen (p) + 1 + strlen (value) + 1;
                  /* +1 for list-terminating NUL */;
                  tmp = realloc (attrs, len + add_len + 1);
                  if (tmp == NULL)
                    {
                      create_error (choose_t->mainwnd, NULL, "Error adding file DSN:",
                         strerror (errno));
                      goto done;
                    }
                  attrs = tmp;
                  snprintf (attrs + len, add_len, "%s=%s", p, value);
                  len += add_len;
                }

              if (drv == NULL)
                {
                  /* no driver found, probably unshareable file data source */
                  create_error (choose_t->mainwnd, NULL,
            	    "Can't configure file DSN without DRIVER keyword (probably unshareable data source?)", NULL);
                  goto done;
                }

              if (attrs == NULL)
                attrs = "\0\0";
              else
                {
                  /* NUL-terminate the list */
                  attrs[len] = '\0';
                  _attrs = attrs;
                }

              /* Configure file DSN */
              filedsn_configure (choose_t, drv, path, attrs, FALSE, TRUE);
              addfdsns_to_list (choose_t, curr_dir, true);
            }
        }
    }

done:
  if (drv != NULL)
    free (drv);
  if (_attrs != NULL)
    free (_attrs);

  if ((err = GetDataBrowserSelectionAnchor (choose_t->sdsnlist,
        &first, &last)) == noErr)
    {
      if (!first && !last)
        {
          DeactivateControl (choose_t->sremove);
          DeactivateControl (choose_t->sconfigure);
          DeactivateControl (choose_t->stest);
        }
    }
  return noErr;
}



pascal OSStatus
filedsn_add_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TDSNCHOOSER *choose_t = (TDSNCHOOSER *) inUserData;
  DataBrowserItemID first, last;
  OSStatus err;
  SQLCHAR drv[1024] = { 0 };
  LPSTR s, attrs;
  TFDRIVERCHOOSER drvchoose_t;

  if (!choose_t)
    return noErr;

  /* Try first to get the driver name */
  SQLSetConfigMode (ODBC_USER_DSN);

  drvchoose_t.attrs = NULL;
  drvchoose_t.dsn = NULL;
  drvchoose_t.driver = NULL;
  drvchoose_t.curr_dir = choose_t->curr_dir;

  create_fdriverchooser (choose_t->mainwnd, &drvchoose_t);

  /* Check output parameters */
  if (drvchoose_t.ok)
    {
      if (sizeof(drv) > WCSLEN(drvchoose_t.driver) + strlen("DRIVER="))
	{
          s = strcpy(drv, "DRIVER=");
          s += strlen("DRIVER=");
          dm_strcpy_W2A(s, drvchoose_t.driver);
          attrs = drvchoose_t.attrs;

          filedsn_configure(choose_t, drv, drvchoose_t.dsn, 
          	attrs ? attrs :"\0\0", TRUE, drvchoose_t.verify_conn);
          addfdsns_to_list (choose_t, choose_t->curr_dir, true);
	}
    }

  if (drvchoose_t.driver)
    free (drvchoose_t.driver);
  if (drvchoose_t.attrs)
    free (drvchoose_t.attrs);
  if (drvchoose_t.dsn)
    free (drvchoose_t.dsn);

  if ((err = GetDataBrowserSelectionAnchor (choose_t->fdsnlist,
        &first, &last)) == noErr)
    {
      if (!first && !last)
        {
          DeactivateControl (choose_t->fremove);
          DeactivateControl (choose_t->fconfigure);
          DeactivateControl (choose_t->ftest);
        }
    }

  return noErr;
}


pascal OSStatus
filedsn_remove_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TDSNCHOOSER *choose_t = (TDSNCHOOSER *) inUserData;
  DataBrowserItemID first, last;
  OSStatus err;

  if (!choose_t)
    return noErr;

  /* Retrieve the DSN name */
  if ((err = GetDataBrowserSelectionAnchor (choose_t->fdsnlist,
        &first, &last)) == noErr)
    {
      if (first > DBITEM_ID && first <= DBITEM_ID + FDSN_nrows)
        {
          char str[1024];
          char *path;

          if (FDSN_type[first - DBITEM_ID - 1] == 0)
            return noErr;

          /* Get the DSN */
          CFStringGetCString(FDSN_array[first - DBITEM_ID - 1], str, sizeof(str), kCFStringEncodingUTF8);
          asprintf (&path, "%s/%s", choose_t->curr_dir, str);

          if (path)
            {
              if (create_confirm (choose_t->mainwnd, path,
                   "Are you sure you want to remove this File DSN ?"))
                {
                  /* Call the right function */
                  if (unlink(path) < 0)
                    {
                      create_error (choose_t->mainwnd, NULL, 
                        "Error removing file DSN:", strerror (errno));
                    }
                }
              free(path);
              addfdsns_to_list (choose_t, choose_t->curr_dir, true);
            }
        }
    }

  if ((err = GetDataBrowserSelectionAnchor (choose_t->fdsnlist,
        &first, &last)) == noErr)
    {
      if (!first && !last)
        {
          DeactivateControl (choose_t->fremove);
          DeactivateControl (choose_t->fconfigure);
          DeactivateControl (choose_t->ftest);
        }
    }

  return noErr;
}


pascal OSStatus
filedsn_test_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TDSNCHOOSER *choose_t = (TDSNCHOOSER *) inUserData;
  DataBrowserItemID first, last;
  OSStatus err;
  char connstr[4096] = { 0 };

  if (!choose_t)
    return noErr;

  /* Retrieve the DSN name */
  if ((err = GetDataBrowserSelectionAnchor (choose_t->fdsnlist,
        &first, &last)) == noErr)
    {
      if (first > DBITEM_ID && first <= DBITEM_ID + FDSN_nrows)
        {
          char str[1024];
          char *path;

          if (FDSN_type[first - DBITEM_ID - 1] == 0)
            return noErr;

          /* Get the DSN */
          CFStringGetCString(FDSN_array[first - DBITEM_ID - 1], str, sizeof(str), kCFStringEncodingUTF8);
          asprintf (&path, "%s/%s", choose_t->curr_dir, str);
          if (path)
            {
              /* Create connection string and connect to data source */
              snprintf (connstr, sizeof (connstr), "FILEDSN=%s", path);
              if (test_driver_connect(choose_t, connstr))
                {
                  _iodbcdm_messagebox (choose_t->mainwnd, path,
                    "The connection DSN was tested successfully, and can be used at this time.");
                }

              free(path);
            }
        }
    }

  if ((err = GetDataBrowserSelectionAnchor (choose_t->fdsnlist,
        &first, &last)) == noErr)
    {
      if (!first && !last)
        {
          DeactivateControl (choose_t->fremove);
          DeactivateControl (choose_t->fconfigure);
          DeactivateControl (choose_t->ftest);
        }
    }

  return noErr;
}


pascal OSStatus
filedsn_setdir_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TDSNCHOOSER *choose_t = (TDSNCHOOSER *) inUserData;
  char msg[4096];

  if (!choose_t)
    return noErr;

  /* confirm setting a directory */
  snprintf (msg, sizeof (msg),
      "Are you sure that you want to make '%s' the default file DSN directory?",
      choose_t->curr_dir);
  if (!create_confirm (choose_t->mainwnd, NULL, msg))
    return noErr;

  /* write FileDSNPath value */
  if (!SQLWritePrivateProfileString ("ODBC", "FileDSNPath",
	   choose_t->curr_dir, "odbcinst.ini"))
    {
      create_error (choose_t->mainwnd, NULL,
	  "Error setting default file DSN directory", NULL);
      return noErr;
    }
  return noErr;
}



#define CHECK_DRVCONN_DIALBOX(path) \
  { \
    if ((handle = DLL_OPEN(path)) != NULL) \
      { \
        if (DLL_PROC(handle, "_iodbcdm_drvconn_dialboxw") != NULL) \
          { \
            DLL_CLOSE(handle); \
            retVal = TRUE; \
            goto quit; \
          } \
        else \
          { \
            if (DLL_PROC(handle, "_iodbcdm_drvconn_dialbox") != NULL) \
              { \
                DLL_CLOSE(handle); \
                retVal = TRUE; \
                goto quit; \
              } \
          } \
        DLL_CLOSE(handle); \
      } \
  }



static Boolean
_CheckDriverLoginDlg (
    char *drv
)
{
  char drvbuf[4096] = { L'\0'};
  HDLL handle;
  Boolean retVal = false;

  if (!drv)
    return false;

  SQLSetConfigMode (ODBC_USER_DSN);
  if (!access (drv, X_OK))
    { CHECK_DRVCONN_DIALBOX (drv); }
  if (SQLGetPrivateProfileString (drv, "Driver", "", drvbuf,
    sizeof (drvbuf), "odbcinst.ini"))
    { CHECK_DRVCONN_DIALBOX (drvbuf); }
  if (SQLGetPrivateProfileString (drv, "Setup", "", drvbuf,
    sizeof (drvbuf), "odbcinst.ini"))
    { CHECK_DRVCONN_DIALBOX (drvbuf); }

  SQLSetConfigMode (ODBC_SYSTEM_DSN);
  if (!access (drv, X_OK))
    { CHECK_DRVCONN_DIALBOX (drv); }
  if (SQLGetPrivateProfileString (drv, "Driver", "", drvbuf,
    sizeof (drvbuf), "odbcinst.ini"))
    { CHECK_DRVCONN_DIALBOX (drvbuf); }
  if (SQLGetPrivateProfileString (drv, "Setup", "", drvbuf,
    sizeof (drvbuf), "odbcinst.ini"))
    { CHECK_DRVCONN_DIALBOX (drvbuf); }

quit:
  return retVal;
}
