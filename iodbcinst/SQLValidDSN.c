/*
 *  SQLValidDSN.c
 *
 *  $Id$
 *
 *  Validate a DSN name
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

#include <iodbc.h>
#include <iodbcinst.h>

#include "iodbc_error.h"

#define INVALID_CHARS	"[]{}(),;?*=!@\\"

BOOL
ValidDSN (LPCSTR lpszDSN)
{
  char *currp = (char *) lpszDSN;

  while (*currp)
    {
      if (strchr (INVALID_CHARS, *currp))
	return FALSE;
      else
	currp += 1;
    }

  return TRUE;
}


BOOL INSTAPI
SQLValidDSN (LPCSTR lpszDSN)
{
  BOOL retcode = FALSE;

  /* Check dsn */
  CLEAR_ERROR ();
  if (!lpszDSN || !strlen (lpszDSN) || strlen (lpszDSN) >= SQL_MAX_DSN_LENGTH)
    {
      PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
      goto quit;
    }

  retcode = ValidDSN (lpszDSN);

quit:
  return retcode;
}
