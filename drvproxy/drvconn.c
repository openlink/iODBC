/*
 *  drvconn.c
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

#include <herr.h>
#include <dlproc.h>

SQLRETURN SQL_API
_iodbcdm_drvconn_dialbox (HWND hwnd,
    LPSTR szInOutConnStr, DWORD cbInOutConnStr, int FAR * sqlStat)
{
  RETCODE retcode = SQL_ERROR;
  char *szDSN = NULL, *szDriver = NULL, *curr;
  char dsnbuf[SQL_MAX_DSN_LENGTH + 1];
  int driver_type = -1, flags = 0, i;

  /* Check input parameters */
  if (!hwnd || !szInOutConnStr || cbInOutConnStr < 1)
    goto quit;

  /* Check if the DSN is already set or DRIVER */
  for (curr = szInOutConnStr; *curr; curr += (STRLEN (curr) + 1))
    {
      if (!strncasecmp (curr, "DSN=", STRLEN ("DSN=")))
	szDSN = curr + STRLEN ("DSN=");
      if (!strncasecmp (curr, "DRIVER=", STRLEN ("DRIVER=")))
	szDriver = curr + STRLEN ("DRIVER=");
    }

  /* Retrieve the corresponding driver */
  /*if( strstr(szDriver, "OpenLink") || strstr(szDriver, "Openlink") || strstr(szDriver, "oplodbc") )
     driver_type = 0;
     else if( (strstr(szDriver, "Virtuoso") || strstr(szDriver, "virtodbc")) )
     driver_type = 1; */

  switch (driver_type)
    {
    case 0:
      /*curr = create_oplsetup(hwnd, szDSN, szInOutConnStr, TRUE);
         if( curr && curr!=(LPSTR)-1L )
         {
         STRNCPY(szInOutConnStr, curr, (cbInOutConnStr==SQL_NTS) ? STRLEN(curr) : cbInOutConnStr);
         free(curr);
         } */

      break;

    case 1:
      /*if( szFile )
         {
         curr = create_virtsetup(hwnd, szDSN, szInOutConnStr, TRUE);
         if( curr && curr!=(LPSTR)-1L )
         {
         STRNCPY(szInOutConnStr, curr, (cbInOutConnStr==SQL_NTS) ? STRLEN(curr) : cbInOutConnStr);
         free(curr);
         }
         } */
      break;

    default:
      /*if( szFile )
         {
         curr = create_gensetup(hwnd, szDSN, szInOutConnStr, TRUE);
         if( curr && curr!=(LPSTR)-1L )
         {
         STRNCPY(szInOutConnStr, curr, (cbInOutConnStr==SQL_NTS) ? STRLEN(curr) : cbInOutConnStr);
         free(curr);
         }
         } */
      break;
    };

  retcode = SQL_SUCCESS;

quit:
  return retcode;
}
