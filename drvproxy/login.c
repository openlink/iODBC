/*
 *  login.c
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

#include <config.h>
#include <iodbc.h>
#include <iodbcinst.h>
#include <iodbc_error.h>

#include "gui.h"

SQLRETURN SQL_API
_iodbcdm_loginbox (HWND hwnd,
    LPSTR szInOutConnStr, DWORD cbInOutConnStr, int FAR * sqlStat)
{
  RETCODE retcode = SQL_ERROR;
  char *szUID = NULL, *szPWD = NULL, *szDSN = NULL, *curr;
  TLOGIN log_t;

  /* Check input parameters */
  if (!hwnd || !szInOutConnStr || cbInOutConnStr < 1)
    goto quit;

  /* Check if the user and password are put */
  for (curr = szInOutConnStr; *curr; curr += (STRLEN (curr) + 1))
    {
      if (!strncasecmp (curr, "DSN=", STRLEN ("DSN=")))
	szDSN = curr + STRLEN ("DSN=");
      if (!strncasecmp (curr, "UID=", STRLEN ("UID=")))
	szUID = curr + STRLEN ("UID=");
      if (!strncasecmp (curr, "PWD=", STRLEN ("PWD=")))
	szPWD = curr + STRLEN ("PWD=");
    }

  if (!szUID || !szPWD)
    {
      create_login (hwnd, szUID, szPWD, szDSN, &log_t);

      if (log_t.user && !szUID)
	{
	  sprintf (curr, "UID=%s\0", log_t.user);
	  curr += (STRLEN (curr) + 1);
	  free (log_t.user);
	  *curr = 0;
	}

      if (log_t.pwd)
	{
	  sprintf (curr, "PWD=%s\0", log_t.pwd);
	  curr += (STRLEN (curr) + 1);
	  free (log_t.pwd);
	  *curr = 0;
	}

    }

  retcode = SQL_SUCCESS;

quit:
  return retcode;
}
