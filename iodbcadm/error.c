/*
 *  error.c
 *
 *  $Id$
 *
 *  The data_sources dialog for SQLDriverConnect and a login box procedures
 *
 *  The iODBC driver manager.
 *
 *  Copyright (C) 2001 by OpenLink Software <iodbc@openlinksw.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "gui.h"

void SQL_API
_iodbcdm_errorbox (
    HWND hwnd,
    LPCSTR szDSN,
    LPCSTR szText)
{
  char msg[4096];

  if (SQLInstallerError (1, NULL, msg, sizeof (msg), NULL) == SQL_SUCCESS)
    create_error (hwnd, szDSN, szText, msg);
}


void SQL_API
_iodbcdm_messagebox (
    HWND hwnd,
    LPCSTR szDSN,
    LPCSTR szText)
{
  create_message (hwnd, szDSN, szText);
}
