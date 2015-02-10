/*
 *  dsnchooser.c
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

extern wchar_t* convert_CFString_to_wchar(const CFStringRef str);
extern CFStringRef convert_wchar_to_CFString(wchar_t *str);
extern void __create_message (HWND hwnd, SQLPOINTER dsn, SQLPOINTER text,
  SQLCHAR waMode, AlertType id);
extern char *get_home(char *buf, size_t size);


extern UInt32 FDSN_nrows;

UInt32 DSN_nrows;
CFStringRef DSN_array[3][200];
extern CFStringRef FDSN_array[];
extern char FDSN_type[];

TDSNCHOOSER *DSNCHOOSER = NULL;

static pascal OSStatus
dsn_getset_item (ControlRef browser,
    DataBrowserItemID itemID, DataBrowserPropertyID property,
    DataBrowserItemDataRef itemData, Boolean changeValue)
{
  OSStatus err = noErr;

  if (!changeValue)
    switch (property)
      {
      case DBNAME_ID:
	SetDataBrowserItemDataText (itemData,
	    DSN_array[0][itemID - DBITEM_ID - 1]);
	break;

      case DSNDESC_ID:
	SetDataBrowserItemDataText (itemData,
	    DSN_array[1][itemID - DBITEM_ID - 1]);
	break;

      case DSNDRV_ID:
	SetDataBrowserItemDataText (itemData,
	    DSN_array[2][itemID - DBITEM_ID - 1]);
	break;

      case kDataBrowserItemIsActiveProperty:
	if (itemID > DBITEM_ID && itemID <= DBITEM_ID + DSN_nrows)
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
dsn_notification_item (ControlRef browser,
    DataBrowserItemID itemID, DataBrowserItemNotification message)
{
  static Boolean ignore_next = false, selected = false;

  switch (message)
    {
    case kDataBrowserItemSelected:
      switch (DSNCHOOSER->type_dsn)
	{
	case USER_DSN:
	  ActivateControl (DSNCHOOSER->uremove);
	  DrawOneControl (DSNCHOOSER->uremove);
	  ActivateControl (DSNCHOOSER->utest);
	  DrawOneControl (DSNCHOOSER->utest);
	  ActivateControl (DSNCHOOSER->uconfigure);
	  DrawOneControl (DSNCHOOSER->uconfigure);
	  break;

	case SYSTEM_DSN:
	  ActivateControl (DSNCHOOSER->sremove);
	  DrawOneControl (DSNCHOOSER->sremove);
	  ActivateControl (DSNCHOOSER->stest);
	  DrawOneControl (DSNCHOOSER->stest);
	  ActivateControl (DSNCHOOSER->sconfigure);
	  DrawOneControl (DSNCHOOSER->sconfigure);
	  break;
	};

      if (selected)
	ignore_next = true;
      else
	selected = true;
      break;

    case kDataBrowserItemDeselected:
      if (!ignore_next)
	{
	  switch (DSNCHOOSER->type_dsn)
	    {
	    case USER_DSN:
	      DeactivateControl (DSNCHOOSER->uremove);
	      DrawOneControl (DSNCHOOSER->uremove);
	      DeactivateControl (DSNCHOOSER->utest);
	      DrawOneControl (DSNCHOOSER->utest);
	      DeactivateControl (DSNCHOOSER->uconfigure);
	      DrawOneControl (DSNCHOOSER->uconfigure);
	      break;

	    case SYSTEM_DSN:
	      DeactivateControl (DSNCHOOSER->sremove);
	      DrawOneControl (DSNCHOOSER->sremove);
	      DeactivateControl (DSNCHOOSER->stest);
	      DrawOneControl (DSNCHOOSER->stest);
	      DeactivateControl (DSNCHOOSER->sconfigure);
	      DrawOneControl (DSNCHOOSER->sconfigure);
	      break;
	    };
	}
      else
	{
	  ignore_next = false;
	  selected = true;
	}
      break;

    case kDataBrowserItemDoubleClicked:
      switch (DSNCHOOSER->type_dsn)
	{
	case USER_DSN:
	  userdsn_configure_clicked (NULL, NULL, DSNCHOOSER);
	  break;
	case SYSTEM_DSN:
	  systemdsn_configure_clicked (NULL, NULL, DSNCHOOSER);
	  break;
	};
      break;
    };
}

void
adddsns_to_list (ControlRef widget, BOOL systemDSN, WindowRef dlg)
{
  DataBrowserItemID item = DBITEM_ID + 1;
  DataBrowserCallbacks dbCallbacks;
  wchar_t dsnname[1024], dsndesc[1024];
  ThemeDrawingState outState = NULL;
  UInt16 colSize[3] = { 100, 100, 150 };
  SInt16 outBaseline;
  SQLSMALLINT len;
  SQLRETURN ret;
  Point ioBound;
  HENV henv;
  int i;

  if (!widget)
    return;

  GetThemeDrawingState (&outState);

  /* Install an event handler on the component databrowser */
  dbCallbacks.version = kDataBrowserLatestCallbacks;
  InitDataBrowserCallbacks (&dbCallbacks);
  dbCallbacks.u.v1.itemNotificationCallback =
      NewDataBrowserItemNotificationUPP (dsn_notification_item);
  /* On Mac OS X 10.0.x : clientDataCallback */
  dbCallbacks.u.v1.itemDataCallback =
      NewDataBrowserItemDataUPP (dsn_getset_item);
  SetDataBrowserCallbacks (widget, &dbCallbacks);
  /* Begin the draw of the data browser */
  SetDataBrowserTarget (widget, DBITEM_ID);

  /* Make the clean up */
  for (i = 0; i < DSN_nrows; i++, item++)
    {
      CFRelease (DSN_array[0][i]);
      DSN_array[0][i] = NULL;
      CFRelease (DSN_array[1][i]);
      DSN_array[1][i] = NULL;
      CFRelease (DSN_array[2][i]);
      DSN_array[2][i] = NULL;
      RemoveDataBrowserItems (widget, DBITEM_ID, 1, &item, DBNAME_ID);
    }

  /* Global Initialization */
  DSN_nrows = 0;
  item = DBITEM_ID + 1;

  /* Create a HENV to get the list of data sources then */
  ret = SQLAllocHandle (SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
  if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
    {
      _iodbcdm_nativeerrorbox (dlg, henv, SQL_NULL_HANDLE, SQL_NULL_HANDLE);
      goto end;
    }

  /* Set the version ODBC API to use */
  SQLSetEnvAttr (henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER) SQL_OV_ODBC3,
      SQL_IS_UINTEGER);

  /* Get the list of datasources */
  ret = SQLDataSourcesW (henv,
      systemDSN ? SQL_FETCH_FIRST_SYSTEM : SQL_FETCH_FIRST_USER,
      dsnname, sizeof (dsnname)/sizeof(wchar_t), &len,
      dsndesc, sizeof (dsndesc)/sizeof(wchar_t), NULL);
  if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO && ret != SQL_NO_DATA)
    {
      _iodbcdm_nativeerrorbox (dlg, henv, SQL_NULL_HANDLE, SQL_NULL_HANDLE);
      goto error;
    }

  while (ret != SQL_NO_DATA)
    {
      DSN_array[0][DSN_nrows] = convert_wchar_to_CFString(dsnname);
      if (dsndesc[0] == L'\0')
        {
          SQLSetConfigMode (ODBC_BOTH_DSN);
	  SQLGetPrivateProfileStringW (L"Default", L"Driver", L"", dsndesc,
	    sizeof (dsndesc)/sizeof(wchar_t), L"odbc.ini");
        }
      DSN_array[2][DSN_nrows] =
        convert_wchar_to_CFString(WCSLEN (dsndesc) ? dsndesc : L"-");

      /* Get the description */
      SQLSetConfigMode (systemDSN ? ODBC_SYSTEM_DSN : ODBC_USER_DSN);
      SQLGetPrivateProfileStringW (dsnname, L"Description", L"", dsndesc,
	  sizeof (dsndesc)/sizeof(wchar_t), L"odbc.ini");
      DSN_array[1][DSN_nrows] = convert_wchar_to_CFString(WCSLEN (dsndesc) ? dsndesc : L"-");

      for(i = 0 ; i < 3 ; i++)
        {
          GetThemeTextDimensions (DSN_array[i][DSN_nrows], kThemeSystemFont,
            kThemeStateActive, false, &ioBound, &outBaseline);
          if(colSize[i] < ioBound.h) colSize[i] = ioBound.h;
        }

      AddDataBrowserItems (widget, DBITEM_ID, 1, &item, DBNAME_ID);
      item++;
      DSN_nrows++;

      /* Process next one */
      ret = SQLDataSourcesW (henv, SQL_FETCH_NEXT, dsnname,
	  sizeof (dsnname)/sizeof(wchar_t), &len, dsndesc,
          sizeof (dsndesc)/sizeof(wchar_t), NULL);
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
  SetDataBrowserTableViewNamedColumnWidth (widget, DSNDESC_ID, colSize[1] + 20);
  SetDataBrowserTableViewNamedColumnWidth (widget, DSNDRV_ID, colSize[2] + 20);
  DrawOneControl (widget);
  /* Remove the DataBrowser callback */
  SetDataBrowserCallbacks (NULL, &dbCallbacks);
  if(outState) DisposeThemeDrawingState (outState);
}

pascal OSStatus
userdsn_add_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TDSNCHOOSER *choose_t = (TDSNCHOOSER *) inUserData;
  wchar_t drv[1024] = { L'\0' };
  DataBrowserItemID first, last;
  OSStatus err;
  int sqlstat;

  if (choose_t)
    {
      SQLSetConfigMode (ODBC_USER_DSN);
      /* Try first to get the driver name */
      if (_iodbcdm_drvchoose_dialboxw (choose_t->mainwnd, drv,
        sizeof (drv) / sizeof(wchar_t), &sqlstat) == SQL_SUCCESS)
	{
	  SQLSetConfigMode (ODBC_USER_DSN);
          if (!SQLConfigDataSourceW (choose_t->mainwnd, ODBC_ADD_DSN,
            drv + WCSLEN (L"DRIVER="), L"\0\0"))
            {
              _iodbcdm_errorboxw (choose_t->mainwnd, NULL,
                L"An error occurred when trying to add the DSN : ");
              goto done;
            }

	  adddsns_to_list (choose_t->udsnlist, FALSE, choose_t->mainwnd);
	}

    done:
      if ((err = GetDataBrowserSelectionAnchor (choose_t->udsnlist,
        &first, &last)) == noErr)
	{
	  if (!first && !last)
	    {
	      DeactivateControl (choose_t->uremove);
	      DeactivateControl (choose_t->uconfigure);
	      DeactivateControl (choose_t->utest);
	    }
	}
    }

  return noErr;
}

pascal OSStatus
userdsn_remove_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TDSNCHOOSER *choose_t = (TDSNCHOOSER *) inUserData;
  DataBrowserItemID first, last;
  wchar_t *szDSN, *szDriver;
  wchar_t dsn[1024] = { L'\0' };
  OSStatus err;

  if (choose_t)
    {
      /* Retrieve the DSN name */
      if ((err = GetDataBrowserSelectionAnchor (choose_t->udsnlist,
        &first, &last)) == noErr)
	{
	  if (first > DBITEM_ID && first <= DBITEM_ID + DSN_nrows)
	    {
	      /* Get the DSN */
              szDSN = convert_CFString_to_wchar(DSN_array[0][first - DBITEM_ID - 1]);
              if(szDSN)
                {
	          /* Get the driver */
                  szDriver =
                    convert_CFString_to_wchar(DSN_array[2][first - DBITEM_ID - 1]);
                  if(szDriver)
                    {
                      if (create_confirmw (choose_t->mainwnd, szDSN,
                        L"Are you sure you want to remove this DSN ?"))
                        {
                          /* Call the right function */
                          WCSCPY(dsn, L"DSN="); WCSCAT(dsn, szDSN);
                          dsn[WCSLEN(dsn)+1] = L'\0';
                          if (!SQLConfigDataSourceW (choose_t->mainwnd,
                            ODBC_REMOVE_DSN, szDriver, dsn))
                            _iodbcdm_errorboxw (choose_t->mainwnd, szDSN,
                              L"An error occurred when trying to remove the DSN ");
                        }

                      adddsns_to_list (choose_t->udsnlist, FALSE, choose_t->mainwnd);
                      free(szDriver);
	            }
                  free(szDSN);
                }
            }
	}

      if ((err = GetDataBrowserSelectionAnchor (choose_t->udsnlist,
        &first, &last)) == noErr)
	{
	  if (!first && !last)
	    {
	      DeactivateControl (choose_t->uremove);
	      DeactivateControl (choose_t->uconfigure);
	      DeactivateControl (choose_t->utest);
	    }
	}
    }

  return noErr;
}

pascal OSStatus
userdsn_configure_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TDSNCHOOSER *choose_t = (TDSNCHOOSER *) inUserData;
  wchar_t connstr[4096] = { L'\0' }, tokenstr[4096] = { L'\0' };
  wchar_t *szDSN, *szDriver;
  DataBrowserItemID first, last;
  int size = sizeof (connstr)/sizeof(wchar_t);
  wchar_t *curr, *cour;
  OSStatus err;
  DWORD error;

  if (choose_t)
    {
      /* Retrieve the DSN name */
      if ((err = GetDataBrowserSelectionAnchor (choose_t->udsnlist,
        &first, &last)) == noErr)
	{
	  if (first > DBITEM_ID && first <= DBITEM_ID + DSN_nrows)
	    {
	      /* Get the DSN */
              szDSN =
                convert_CFString_to_wchar(DSN_array[0][first - DBITEM_ID - 1]);
              if(szDSN)
                {
	          /* Get the driver */
                  szDriver =
                    convert_CFString_to_wchar(DSN_array[2][first - DBITEM_ID - 1]);
                  if(szDriver)
                    {
                      /* Call the right function */
                      WCSCPY(connstr, L"DSN="); WCSCAT(connstr, szDSN);
                      connstr[WCSLEN(connstr)+1] = L'\0';
                      size -= (WCSLEN (connstr) + 1);

                      SQLSetConfigMode (ODBC_USER_DSN);
                      if (!SQLGetPrivateProfileStringW (szDSN, NULL, L"",
                        tokenstr, sizeof (tokenstr)/sizeof(wchar_t), NULL))
                        {
                          _iodbcdm_errorboxw (choose_t->mainwnd, szDSN,
                            L"An error occurred when trying to configure the DSN ");
                          goto done;
                        }

                      for (curr = tokenstr, cour = connstr + WCSLEN (connstr) + 1;
                        *curr != L'\0' ; curr += (WCSLEN (curr) + 1), cour += (WCSLEN (cour) + 1))
                        {
                          WCSCPY (cour, curr);
                          cour[WCSLEN (curr)] = L'=';
                          SQLSetConfigMode (ODBC_USER_DSN);
                          SQLGetPrivateProfileStringW (szDSN, curr, L"", cour +
                            WCSLEN (curr) + 1, size - WCSLEN (curr) - 1, NULL);
                          size -= (WCSLEN (cour) + 1);
                        }

                      *cour = L'\0';

                      if (!SQLConfigDataSourceW (choose_t->mainwnd, ODBC_CONFIG_DSN,
                        szDriver, connstr))
                        {
                          if (SQLInstallerErrorW (1, &error, connstr,
                            sizeof (connstr)/sizeof(wchar_t), NULL) !=
                            SQL_NO_DATA && error != ODBC_ERROR_REQUEST_FAILED)
                            create_errorw (choose_t->mainwnd, szDSN,
                              L"An error occurred when trying to configure the DSN : ",
                              connstr);
                            goto done;
                        }

	             adddsns_to_list (choose_t->udsnlist, FALSE, choose_t->mainwnd);
                     free(szDriver);
	          }
                free(szDSN);
	      }
	    }
          }

    done:
      if ((err = GetDataBrowserSelectionAnchor (choose_t->udsnlist,
        &first, &last)) == noErr)
	{
	  if (!first && !last)
	    {
	      DeactivateControl (choose_t->uremove);
	      DeactivateControl (choose_t->uconfigure);
	      DeactivateControl (choose_t->utest);
	    }
	}
    }

  return noErr;
}

pascal OSStatus
userdsn_test_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TDSNCHOOSER *choose_t = (TDSNCHOOSER *) inUserData;
  wchar_t connstr[4096] = { L'\0' }, outconnstr[4096] = { L'\0' };
  wchar_t *szDSN = NULL, *szDriver = NULL;
  DataBrowserItemID first, last;
  HENV henv;
  HDBC hdbc;
  SWORD buflen;
  OSStatus err;

  if (choose_t)
    {
      /* Retrieve the DSN name */
      if ((err = GetDataBrowserSelectionAnchor (choose_t->udsnlist,
        &first, &last)) == noErr)
	{
	  if (first > DBITEM_ID && first <= DBITEM_ID + DSN_nrows)
	    {
	      /* Get the DSN */
              szDSN = convert_CFString_to_wchar(DSN_array[0][first - DBITEM_ID - 1]);
              if(szDSN)
                {
                  /* Get the driver */
                  szDriver = convert_CFString_to_wchar(DSN_array[2][first - DBITEM_ID - 1]);
                  if(szDriver)
                    {
	              /* Make the connection */
#if (ODBCVER < 0x300)
	              if (SQLAllocEnv (&henv) != SQL_SUCCESS)
#else
	              if (SQLAllocHandle (SQL_HANDLE_ENV, NULL, &henv) != SQL_SUCCESS)
#endif
		        {
		          _iodbcdm_nativeerrorbox (choose_t->mainwnd, henv,
		            SQL_NULL_HDBC, SQL_NULL_HSTMT);
		          goto done;
		        }

#if (ODBCVER < 0x300)
                      if (SQLAllocConnect (henv, &hdbc) != SQL_SUCCESS)
#else
	              SQLSetEnvAttr (henv, SQL_ATTR_ODBC_VERSION,
		        (SQLPOINTER) SQL_OV_ODBC3, SQL_IS_UINTEGER);
	              if (SQLAllocHandle (SQL_HANDLE_DBC, henv, &hdbc) != SQL_SUCCESS)
#endif
		        {
		          _iodbcdm_nativeerrorbox (choose_t->mainwnd, henv, hdbc,
                            SQL_NULL_HSTMT);
		          SQLFreeEnv (henv);
		          goto done;
		        }

                      WCSCPY(connstr, L"DSN=");
                      WCSCAT(connstr, szDSN);

                      SQLSetConfigMode (ODBC_USER_DSN);
                      if (SQLDriverConnectW (hdbc, choose_t->mainwnd, connstr, SQL_NTS,
                        outconnstr, sizeof (outconnstr) / sizeof(wchar_t), &buflen,
                        SQL_DRIVER_PROMPT) != SQL_SUCCESS)
                        {
                          _iodbcdm_nativeerrorbox (choose_t->mainwnd, henv, hdbc,
                            SQL_NULL_HSTMT);
                        }
                      else
                        {
                            __create_message (choose_t->mainwnd, szDSN,
                                L"The connection DSN was tested successfully, and can be used at this time.",
                                'W', kAlertNoteAlert);
                          SQLDisconnect (hdbc);
                        }

#if (ODBCVER < 0x300)
	              SQLFreeConnect (hdbc);
	              SQLFreeEnv (henv);
#else
	              SQLFreeHandle (SQL_HANDLE_DBC, hdbc);
	              SQLFreeHandle (SQL_HANDLE_ENV, henv);
#endif
	            }
	        }
            }
        }

    done:
      if(szDSN) free(szDSN);
      if(szDriver) free(szDriver);

      if ((err = GetDataBrowserSelectionAnchor (choose_t->udsnlist,
        &first, &last)) == noErr)
	{
	  if (!first && !last)
	    {
	      DeactivateControl (choose_t->uremove);
	      DeactivateControl (choose_t->uconfigure);
	      DeactivateControl (choose_t->utest);
	    }
	}
    }

  return noErr;
}

pascal OSStatus
systemdsn_add_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TDSNCHOOSER *choose_t = (TDSNCHOOSER *) inUserData;
  wchar_t drv[1024] = { L'\0' };
  DataBrowserItemID first, last;
  OSStatus err;
  int sqlstat;

  if (choose_t)
    {
      /* Try first to get the driver name */
      if (_iodbcdm_drvchoose_dialboxw (choose_t->mainwnd, drv,
        sizeof (drv) / sizeof(wchar_t), &sqlstat) == SQL_SUCCESS)
	{
          if (!SQLConfigDataSourceW (choose_t->mainwnd, ODBC_ADD_SYS_DSN,
            drv + WCSLEN (L"DRIVER="), L"\0\0"))
            {
              _iodbcdm_errorboxw (choose_t->mainwnd, NULL,
                L"An error occurred when trying to add the DSN : ");
              goto done;
            }

	  adddsns_to_list (choose_t->sdsnlist, TRUE, choose_t->mainwnd);
	}

    done:
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
    }

  return noErr;
}

pascal OSStatus
systemdsn_remove_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TDSNCHOOSER *choose_t = (TDSNCHOOSER *) inUserData;
  DataBrowserItemID first, last;
  wchar_t *szDSN, *szDriver;
  wchar_t dsn[1024] = { L'\0' };
  OSStatus err;

  if (choose_t)
    {
      /* Retrieve the DSN name */
      if ((err = GetDataBrowserSelectionAnchor (choose_t->sdsnlist,
        &first, &last)) == noErr)
	{
	  if (first > DBITEM_ID && first <= DBITEM_ID + DSN_nrows)
	    {
	      /* Get the DSN */
              szDSN = convert_CFString_to_wchar(DSN_array[0][first - DBITEM_ID - 1]);
              if(szDSN)
                {
	          /* Get the driver */
                  szDriver =
                    convert_CFString_to_wchar(DSN_array[2][first - DBITEM_ID - 1]);
                  if(szDriver)
                    {
                      if (create_confirmw (choose_t->mainwnd, szDSN,
                        L"Are you sure you want to remove this DSN ?"))
                        {
                          /* Call the right function */
                          WCSCPY(dsn, L"DSN="); WCSCAT(dsn, szDSN);
                          dsn[WCSLEN(dsn)+1] = L'\0';
                          if (!SQLConfigDataSourceW (choose_t->mainwnd,
                            ODBC_REMOVE_SYS_DSN, szDriver, dsn))
                            _iodbcdm_errorboxw (choose_t->mainwnd, szDSN,
                                L"An error occurred when trying to remove the DSN ");
                        }

                      adddsns_to_list (choose_t->sdsnlist, TRUE, choose_t->mainwnd);
                      free(szDriver);
	            }
                  free(szDSN);
                }
            }
	}

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
    }

  return noErr;
}

pascal OSStatus
systemdsn_configure_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TDSNCHOOSER *choose_t = (TDSNCHOOSER *) inUserData;
  wchar_t connstr[4096] = { L'\0' }, tokenstr[4096] = { L'\0' };
  wchar_t *szDSN, *szDriver;
  DataBrowserItemID first, last;
  int size = sizeof (connstr)/sizeof(wchar_t);
  wchar_t *curr, *cour;
  OSStatus err;
  DWORD error;

  if (choose_t)
    {
      /* Retrieve the DSN name */
      if ((err = GetDataBrowserSelectionAnchor (choose_t->sdsnlist,
        &first, &last)) == noErr)
	{
	  if (first > DBITEM_ID && first <= DBITEM_ID + DSN_nrows)
	    {
	      /* Get the DSN */
              szDSN =
                convert_CFString_to_wchar(DSN_array[0][first - DBITEM_ID - 1]);
              if(szDSN)
                {
	          /* Get the driver */
                  szDriver =
                    convert_CFString_to_wchar(DSN_array[2][first - DBITEM_ID - 1]);
                  if(szDriver)
                    {
                      /* Call the right function */
                      WCSCPY(connstr, L"DSN="); WCSCAT(connstr, szDSN);
                      connstr[WCSLEN(connstr)+1] = L'\0';
                      size -= (WCSLEN (connstr) + 1);

                      SQLSetConfigMode (ODBC_SYSTEM_DSN);
                      if (!SQLGetPrivateProfileStringW (szDSN, NULL, L"",
                        tokenstr, sizeof (tokenstr)/sizeof(wchar_t), NULL))
                        {
                          _iodbcdm_errorboxw (choose_t->mainwnd, szDSN,
                            L"An error occurred when trying to configure the DSN ");
                          goto done;
                        }

                      for (curr = tokenstr, cour = connstr + WCSLEN (connstr) + 1;
                        *curr != L'\0' ; curr += (WCSLEN (curr) + 1), cour += (WCSLEN (cour) + 1))
                        {
                          WCSCPY (cour, curr);
                          cour[WCSLEN (curr)] = L'=';
                          SQLSetConfigMode (ODBC_SYSTEM_DSN);
                          SQLGetPrivateProfileStringW (szDSN, curr, L"", cour +
                            WCSLEN (curr) + 1, size - WCSLEN (curr) - 1, NULL);
                          size -= (WCSLEN (cour) + 1);
                        }

                      *cour = L'\0';

                      if (!SQLConfigDataSourceW (choose_t->mainwnd,
                        ODBC_CONFIG_SYS_DSN, szDriver, connstr))
                        {
                          if (SQLInstallerErrorW (1, &error, connstr,
                            sizeof (connstr)/sizeof(wchar_t), NULL) != SQL_NO_DATA && error != ODBC_ERROR_REQUEST_FAILED)
                            create_errorw (choose_t->mainwnd, szDSN,
                              L"An error occurred when trying to configure the DSN : ",
                              connstr);
                          goto done;
                        }

	             adddsns_to_list (choose_t->sdsnlist, TRUE, choose_t->mainwnd);
                     free(szDriver);
	          }
                free(szDSN);
	      }
	    }
          }

    done:
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
    }

  return noErr;
}

pascal OSStatus
systemdsn_test_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TDSNCHOOSER *choose_t = (TDSNCHOOSER *) inUserData;
  wchar_t connstr[4096] = { L'\0' }, outconnstr[4096] = { L'\0' };
  wchar_t *szDSN = NULL, *szDriver = NULL;
  DataBrowserItemID first, last;
  HENV henv;
  HDBC hdbc;
  SWORD buflen;
  OSStatus err;

  if (choose_t)
    {
      /* Retrieve the DSN name */
      if ((err = GetDataBrowserSelectionAnchor (choose_t->sdsnlist,
        &first, &last)) == noErr)
	{
	  if (first > DBITEM_ID && first <= DBITEM_ID + DSN_nrows)
	    {
	      /* Get the DSN */
              szDSN = convert_CFString_to_wchar(DSN_array[0][first - DBITEM_ID - 1]);
              if(szDSN)
                {
                  /* Get the driver */
                  szDriver = convert_CFString_to_wchar(DSN_array[2][first - DBITEM_ID - 1]);
                  if(szDriver)
                    {
	              /* Make the connection */
#if (ODBCVER < 0x300)
	              if (SQLAllocEnv (&henv) != SQL_SUCCESS)
#else
	              if (SQLAllocHandle (SQL_HANDLE_ENV, NULL, &henv) != SQL_SUCCESS)
#endif
		        {
		          _iodbcdm_nativeerrorbox (choose_t->mainwnd, henv,
		            SQL_NULL_HDBC, SQL_NULL_HSTMT);
		          goto done;
		        }

#if (ODBCVER < 0x300)
                      if (SQLAllocConnect (henv, &hdbc) != SQL_SUCCESS)
#else
	              SQLSetEnvAttr (henv, SQL_ATTR_ODBC_VERSION,
		        (SQLPOINTER) SQL_OV_ODBC3, SQL_IS_UINTEGER);
	              if (SQLAllocHandle (SQL_HANDLE_DBC, henv, &hdbc) != SQL_SUCCESS)
#endif
		        {
		          _iodbcdm_nativeerrorbox (choose_t->mainwnd, henv, hdbc,
                            SQL_NULL_HSTMT);
		          SQLFreeEnv (henv);
		          goto done;
		        }

                      WCSCPY(connstr, L"DSN=");
                      WCSCAT(connstr, szDSN);

                      SQLSetConfigMode (ODBC_SYSTEM_DSN);
                      if (SQLDriverConnectW (hdbc, choose_t->mainwnd, connstr, SQL_NTS,
                        outconnstr, sizeof (outconnstr)/sizeof(wchar_t), &buflen,
                        SQL_DRIVER_PROMPT) != SQL_SUCCESS)
                        {
                          _iodbcdm_nativeerrorbox (choose_t->mainwnd, henv, hdbc,
                            SQL_NULL_HSTMT);
                        }
                      else
                        {
                            __create_message (choose_t->mainwnd, szDSN,
                                L"The connection DSN was tested successfully, and can be used at this time.",
                                'W', kAlertNoteAlert);
                          SQLDisconnect (hdbc);
                        }

#if (ODBCVER < 0x300)
	              SQLFreeConnect (hdbc);
	              SQLFreeEnv (henv);
#else
	              SQLFreeHandle (SQL_HANDLE_DBC, hdbc);
	              SQLFreeHandle (SQL_HANDLE_ENV, henv);
#endif
	            }
	        }
            }
        }

    done:
      if(szDSN) free(szDSN);
      if(szDriver) free(szDriver);

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
    }

  return noErr;
}

pascal OSStatus
dsnchooser_ok_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TDSNCHOOSER *dsnchoose_t = (inUserData) ? ((void **) inUserData)[0] : NULL;
  WindowRef mainwnd = (inUserData) ? ((void **) inUserData)[1] : NULL;
  DataBrowserItemID first, last;
  OSStatus err;

  if (dsnchoose_t)
    {
      dsnchoose_t->dsn = NULL;
      dsnchoose_t->fdsn = NULL;

      switch (dsnchoose_t->type_dsn)
	{
	case USER_DSN:
	  /* Get the selection */
	  if ((err = GetDataBrowserSelectionAnchor (dsnchoose_t->udsnlist,
            &first, &last)) == noErr)
	    {
	      if (first > DBITEM_ID && first <= DBITEM_ID + DSN_nrows)
		{
		  /* Get the driver name */
		  dsnchoose_t->dsn = 
                    convert_CFString_to_wchar(DSN_array[0][first - DBITEM_ID - 1]);
                }
	    }
	  break;

	case SYSTEM_DSN:
	  /* Get the selection */
	  if ((err = GetDataBrowserSelectionAnchor (dsnchoose_t->sdsnlist,
		   &first, &last)) == noErr)
	    {
	      if (first > DBITEM_ID && first <= DBITEM_ID + DSN_nrows)
		{
		  /* Get the driver name */
		  dsnchoose_t->dsn = 
                    convert_CFString_to_wchar(DSN_array[0][first - DBITEM_ID - 1]);
		}
	    }
	  break;

	case FILE_DSN:
	  /* Get the selection */
	  if ((err = GetDataBrowserSelectionAnchor (dsnchoose_t->fdsnlist,
		   &first, &last)) == noErr)
	    {
              char str[1024];
              char *path;

	      if (first > DBITEM_ID && first <= DBITEM_ID + FDSN_nrows)
		{

                  if (FDSN_type[first - DBITEM_ID - 1] != 0)
                    {
                      CFStringGetCString(FDSN_array[first - DBITEM_ID - 1], str, sizeof(str), kCFStringEncodingUTF8);
                      asprintf (&path, "%s/%s", (dsnchoose_t->curr_dir ? dsnchoose_t->curr_dir : ""), str);

		      /* Get the driver name */
		      dsnchoose_t->fdsn = dm_SQL_U8toW(path, SQL_NTS);
		      if (path)
		        free(path);
		    }
		}
	    }
	  break;

	default:
	  break;
	};

      dsnchoose_t->udsnlist = dsnchoose_t->sdsnlist = dsnchoose_t->fdsnlist = NULL;
      dsnchoose_t->uadd = dsnchoose_t->uremove = NULL;
      dsnchoose_t->utest = dsnchoose_t->uconfigure = NULL;
      dsnchoose_t->sadd = dsnchoose_t->sremove = NULL;
      dsnchoose_t->stest = dsnchoose_t->sconfigure = NULL;
      dsnchoose_t->fadd = dsnchoose_t->fremove = NULL;
      dsnchoose_t->ftest = dsnchoose_t->fconfigure = NULL;
      dsnchoose_t->fdir = dsnchoose_t->dir_list = dsnchoose_t->file_list = NULL;
      dsnchoose_t->file_entry = dsnchoose_t->dir_combo = NULL;
      dsnchoose_t->mainwnd = NULL;
    }

  if (mainwnd)
    {
      DisposeWindow (mainwnd);
	  dsnchoose_t->mainwnd = NULL;
    }

  DSN_nrows = 0;
  FDSN_nrows = 0;

  return noErr;
}

pascal OSStatus
dsnchooser_cancel_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  TDSNCHOOSER *dsnchoose_t = (inUserData) ? ((void **) inUserData)[0] : NULL;
  WindowRef mainwnd = (inUserData) ? ((void **) inUserData)[1] : NULL;

  if (dsnchoose_t)
    {
      dsnchoose_t->udsnlist = dsnchoose_t->sdsnlist = NULL;
      dsnchoose_t->uadd = dsnchoose_t->uremove = NULL;
      dsnchoose_t->utest = dsnchoose_t->uconfigure = NULL;
      dsnchoose_t->sadd = dsnchoose_t->sremove = NULL;
      dsnchoose_t->stest = dsnchoose_t->sconfigure = NULL;
      dsnchoose_t->fadd = dsnchoose_t->fremove = NULL;
      dsnchoose_t->ftest = dsnchoose_t->fconfigure = NULL;
      dsnchoose_t->fdir = dsnchoose_t->dir_list = dsnchoose_t->file_list = NULL;
      dsnchoose_t->file_entry = dsnchoose_t->dir_combo = NULL;
      dsnchoose_t->type_dsn = -1;
      dsnchoose_t->dsn = NULL;
      dsnchoose_t->fdsn = NULL;
      dsnchoose_t->mainwnd = NULL;
    }

  if (mainwnd)
    {
      DisposeWindow (mainwnd);
	  dsnchoose_t->mainwnd = NULL;
    }

  DSN_nrows = 0;
  FDSN_nrows = 0;

  return noErr;
}

pascal OSStatus
dsnchooser_switch_page (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData)
{
  int tabs[] = { 3, UDSN_TAB, SDSN_TAB, FDSN_TAB };
  TDSNCHOOSER *dsnchoose_t = (inUserData) ? ((void **) inUserData)[0] : NULL;
  WindowRef mainwnd = (inUserData) ? ((void **) inUserData)[1] : NULL;
  DataBrowserItemID first, last;
  ControlRef tabControl;
  ControlID controlID;
  int tab_index;
  OSStatus err;

  /* Search which tab is activated */
  controlID.signature = TABS_SIGNATURE;
  controlID.id = GLOBAL_TAB;
  GetControlByID (mainwnd, &controlID, &tabControl);
  DisplayTabControlNumber (tab_index =
      GetControlValue (tabControl), tabControl, mainwnd, tabs);

  if (tab_index == *(int *) (((void **) inUserData)[2]))
    return noErr;

  ClearKeyboardFocus (mainwnd);

  *(int *) (((void **) inUserData)[2]) = tab_index;

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

    };

  AdvanceKeyboardFocus (mainwnd);

  return noErr;
}

void
create_dsnchooser (HWND hwnd, TDSNCHOOSER * dsnchoose_t)
{
  int tabs[] = { 3, UDSN_TAB, SDSN_TAB, FDSN_TAB };
  EventTypeSpec controlSpec = { kEventClassControl, kEventControlHit };
  ControlRef tabControl, control;
  RgnHandle cursorRgn = NULL;
  EventRecord event;
  ControlID controlID;
  IBNibRef nibRef;
  OSStatus err;
  WindowPtr dsnchooser;
  void *inparams[3];
  int current_tab = -1;

  CFStringRef libname = NULL;
  CFBundleRef bundle;
  CFURLRef liburl;
  SInt16 rscId = -1, oldId = CurResFile();

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
	("org.iodbc.adm")), CFSTR ("dsnchooser"), &nibRef);
  if (err == noErr)
    {
      /* Nib found ... so create the window */
      err = CreateWindowFromNib (nibRef, CFSTR ("Window"), &dsnchooser);
      if (err != noErr)
	goto error;
      /* And no need anymore the nib */
      DisposeNibReference (nibRef);
    }
  else
    goto error;

  /* Find the tab control . */
  GETCONTROLBYID (controlID, TABS_SIGNATURE, GLOBAL_TAB, dsnchooser,
      tabControl);
      /* Create for each tab a structure */
      /* TDSNCHOOSER */

    GETCONTROLBYID (controlID, CNTL_SIGNATURE, ULIST_CNTL, dsnchooser,
      dsnchoose_t->udsnlist);
    GETCONTROLBYID (controlID, CNTL_SIGNATURE, SLIST_CNTL, dsnchooser,
      dsnchoose_t->sdsnlist);
    GETCONTROLBYID (controlID, CNTL_SIGNATURE, FLIST_CNTL, dsnchooser,
      dsnchoose_t->fdsnlist);

    GETCONTROLBYID (controlID, CNTL_SIGNATURE, UADD_CNTL, dsnchooser, 
      dsnchoose_t->uadd);
    GETCONTROLBYID (controlID, CNTL_SIGNATURE, SADD_CNTL, dsnchooser,
      dsnchoose_t->sadd);
    GETCONTROLBYID (controlID, CNTL_SIGNATURE, FADD_CNTL, dsnchooser,
      dsnchoose_t->fadd);

    GETCONTROLBYID (controlID, CNTL_SIGNATURE, UDEL_CNTL, dsnchooser, 
      dsnchoose_t->uremove);
    GETCONTROLBYID (controlID, CNTL_SIGNATURE, SDEL_CNTL, dsnchooser,
      dsnchoose_t->sremove);
    GETCONTROLBYID (controlID, CNTL_SIGNATURE, FDEL_CNTL, dsnchooser,
      dsnchoose_t->fremove);

    GETCONTROLBYID (controlID, CNTL_SIGNATURE, UTST_CNTL, dsnchooser, 
      dsnchoose_t->utest);
    GETCONTROLBYID (controlID, CNTL_SIGNATURE, STST_CNTL, dsnchooser,
      dsnchoose_t->stest);
    GETCONTROLBYID (controlID, CNTL_SIGNATURE, FTST_CNTL, dsnchooser,
      dsnchoose_t->ftest);

    GETCONTROLBYID (controlID, CNTL_SIGNATURE, UCFG_CNTL, dsnchooser,
      dsnchoose_t->uconfigure);
    GETCONTROLBYID (controlID, CNTL_SIGNATURE, SCFG_CNTL, dsnchooser, 
      dsnchoose_t->sconfigure);
    GETCONTROLBYID (controlID, CNTL_SIGNATURE, FCFG_CNTL, dsnchooser,
      dsnchoose_t->fconfigure);
    GETCONTROLBYID (controlID, CNTL_SIGNATURE, FDIR_CNTL, dsnchooser,
      dsnchoose_t->fdir);
      
    dsnchoose_t->type_dsn = USER_DSN; 
    dsnchoose_t->mainwnd = dsnchooser;

  /* Install an event handler on the tab control. */
  InstallEventHandler (GetControlEventTarget (tabControl),
      NewEventHandlerUPP (dsnchooser_switch_page), 1, &controlSpec, inparams,
      NULL);
  /* Install an event handler on the OK button */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, OKBUT_CNTL, dsnchooser, control)
      InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (dsnchooser_ok_clicked), 1, &controlSpec, inparams,
      NULL);
  /* Install an event handler on the Cancel button */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, CANCELBUT_CNTL, dsnchooser,
      control) InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (dsnchooser_cancel_clicked), 1, &controlSpec,
      inparams, NULL);
  /* Install an event handler on the Add button of the user DSNs */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, UADD_CNTL, dsnchooser, control)
      InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (userdsn_add_clicked), 1, &controlSpec, dsnchoose_t,
      NULL);
  /* Install an event handler on the Remove button of the user DSNs */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, UDEL_CNTL, dsnchooser, control)
      InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (userdsn_remove_clicked), 1, &controlSpec,
      dsnchoose_t, NULL);
  /* Install an event handler on the Configure button of the user DSNs */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, UCFG_CNTL, dsnchooser, control)
      InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (userdsn_configure_clicked), 1, &controlSpec,
      dsnchoose_t, NULL);
  /* Install an event handler on the Test button of the user DSNs */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, UTST_CNTL, dsnchooser, control)
      InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (userdsn_test_clicked), 1, &controlSpec, dsnchoose_t,
      NULL);
  /* Install an event handler on the Add button of the system DSNs */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, SADD_CNTL, dsnchooser, control)
      InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (systemdsn_add_clicked), 1, &controlSpec,
      dsnchoose_t, NULL);
  /* Install an event handler on the Remove button of the system DSNs */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, SDEL_CNTL, dsnchooser, control)
      InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (systemdsn_remove_clicked), 1, &controlSpec,
      dsnchoose_t, NULL);
  /* Install an event handler on the Configure button of the system DSNs */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, SCFG_CNTL, dsnchooser, control)
      InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (systemdsn_configure_clicked), 1, &controlSpec,
      dsnchoose_t, NULL);
  /* Install an event handler on the Test button of the system DSNs */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, STST_CNTL, dsnchooser, control)
      InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (systemdsn_test_clicked), 1, &controlSpec,
      dsnchoose_t, NULL);

  /* Install an event handler on the Add button of the system DSNs */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, FADD_CNTL, dsnchooser, control);
  InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (filedsn_add_clicked), 1, &controlSpec,
      &dsnchoose_t, NULL);
  /* Install an event handler on the Remove button of the system DSNs */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, FDEL_CNTL, dsnchooser, control);
  InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (filedsn_remove_clicked), 1, &controlSpec,
      &dsnchoose_t, NULL);
  /* Install an event handler on the Select dir menu of the File DSNs */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, FDIR_CNTL, dsnchooser, control);
  InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (filedsn_select_clicked), 1, &controlSpec,
      &dsnchoose_t, NULL);
  /* Install an event handler on the Configure button of the system DSNs */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, FCFG_CNTL, dsnchooser, control);
  InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (filedsn_configure_clicked), 1, &controlSpec,
      &dsnchoose_t, NULL);
  /* Install an event handler on the Test button of the system DSNs */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, FTST_CNTL, dsnchooser, control);
  InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (filedsn_test_clicked), 1, &controlSpec,
      &dsnchoose_t, NULL);
  /* Install an event handler on the SetDir button of the system DSNs */
  GETCONTROLBYID (controlID, CNTL_SIGNATURE, FSETDIR_CNTL, dsnchooser, control);
  InstallEventHandler (GetControlEventTarget (control),
      NewEventHandlerUPP (filedsn_setdir_clicked), 1, &controlSpec,
      &dsnchoose_t, NULL);

  inparams[0] = dsnchoose_t;
  inparams[1] = dsnchooser;
  inparams[2] = &current_tab;

  SQLSetConfigMode (ODBC_BOTH_DSN);
  if (!SQLGetPrivateProfileString("ODBC", "FileDSNPath", "", 
      dsnchoose_t->curr_dir, sizeof(dsnchoose_t->curr_dir), "odbcinst.ini"))
    {
      char tmp[1024];
      snprintf(dsnchoose_t->curr_dir, sizeof(dsnchoose_t->curr_dir),
        "%s/Documents", get_home(tmp, sizeof(tmp)));
/*was   "%s"DEFAULT_FILEDSNPATH, get_home(tmp, sizeof(tmp))); */
    }

  /* Force to go on the first tab */
  DisplayTabControlNumber (1, tabControl, dsnchooser, tabs);
  dsnchooser_switch_page (NULL, NULL, inparams);
  AdvanceKeyboardFocus (dsnchooser);
  /* Show the window */
  ShowWindow (dsnchooser);
  SetPort ((GrafPtr) GetWindowPort (dsnchooser));

  /* The main loop */
  while (dsnchoose_t->mainwnd)
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
