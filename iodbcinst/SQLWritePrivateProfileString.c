/*
 *  SQLWritePrivateProfileString.c
 *
 *  $Id$
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

#include "inifile.h"
#include "iodbc_error.h"
#include "misc.h"

#ifndef WIN32
BOOL
WritePrivateProfileString (LPCSTR lpszSection, LPCSTR lpszEntry,
    LPCSTR lpszString, LPCSTR lpszFilename)
{
  BOOL retcode = FALSE;
  PCONFIG pCfg = NULL;

  /* Check Input parameters */
  if (lpszSection == NULL || *lpszSection == '\0')
    {
      PUSH_ERROR (ODBC_ERROR_INVALID_REQUEST_TYPE);
      goto fail;
    }

  /* If error during reading the file */
  if (_iodbcdm_cfg_search_init (&pCfg, lpszFilename, TRUE))
    {
      PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
      goto fail;
    }

  /* Check if the section must be deleted */
  if (!lpszEntry)
    {
      _iodbcdm_cfg_write (pCfg, (LPSTR) lpszSection, NULL, NULL);
      goto done;
    }

  /* Check if the entry must be deleted */
  if (!lpszString)
    {
      _iodbcdm_cfg_write (pCfg, (LPSTR) lpszSection, (LPSTR) lpszEntry, NULL);
      goto done;
    }

  /* Else add the entry */
  _iodbcdm_cfg_write (pCfg, (LPSTR) lpszSection, (LPSTR) lpszEntry,
      (LPSTR) lpszString);

done:
  if (!_iodbcdm_cfg_commit (pCfg))
    retcode = TRUE;
  else
    {
      PUSH_ERROR (ODBC_ERROR_REQUEST_FAILED);
      goto fail;
    }

fail:
  if (pCfg)
    _iodbcdm_cfg_done (pCfg);
  return retcode;
}
#endif


BOOL INSTAPI
SQLWritePrivateProfileString (LPCSTR lpszSection, LPCSTR lpszEntry,
    LPCSTR lpszString, LPCSTR lpszFilename)
{
  char pathbuf[1024];
  BOOL retcode = FALSE;

  /* Check input parameters */
  CLEAR_ERROR ();

  /* Else go through user/system odbc.ini */
  switch (configMode)
    {
    case ODBC_USER_DSN:
      wSystemDSN = USERDSN_ONLY;
      if (lpszFilename)
	{
	  retcode =
	      WritePrivateProfileString (lpszSection, lpszEntry, lpszString,
	      lpszFilename);
	  goto quit;
	}
      if (_iodbcadm_getinifile (pathbuf, sizeof (pathbuf), FALSE, TRUE))
	retcode =
	    WritePrivateProfileString (lpszSection, lpszEntry, lpszString,
	    pathbuf);
      goto quit;

    case ODBC_SYSTEM_DSN:
      wSystemDSN = SYSTEMDSN_ONLY;
      if (lpszFilename)
	{
	  retcode =
	      WritePrivateProfileString (lpszSection, lpszEntry, lpszString,
	      lpszFilename);
	  goto quit;
	}
      if (_iodbcadm_getinifile (pathbuf, sizeof (pathbuf), FALSE, TRUE))
	retcode =
	    WritePrivateProfileString (lpszSection, lpszEntry, lpszString,
	    pathbuf);
      goto quit;

    case ODBC_BOTH_DSN:
      wSystemDSN = USERDSN_ONLY;
      if (lpszFilename)
	{
	  retcode =
	      WritePrivateProfileString (lpszSection, lpszEntry, lpszString,
	      lpszFilename);
	  if (!retcode)
	    {
	      CLEAR_ERROR ();
	      wSystemDSN = SYSTEMDSN_ONLY;
	      retcode =
		  WritePrivateProfileString (lpszSection, lpszEntry,
		  lpszString, lpszFilename);
	    }
	  goto quit;
	}
      if (_iodbcadm_getinifile (pathbuf, sizeof (pathbuf), FALSE, TRUE))
	retcode =
	    WritePrivateProfileString (lpszSection, lpszEntry, lpszString,
	    pathbuf);
      else
	{
	  CLEAR_ERROR ();
	  wSystemDSN = SYSTEMDSN_ONLY;
	  if (_iodbcadm_getinifile (pathbuf, sizeof (pathbuf), FALSE, TRUE))
	    retcode =
		WritePrivateProfileString (lpszSection, lpszEntry, lpszString,
		pathbuf);
	}
      goto quit;
    };

  PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
  goto quit;

quit:
  wSystemDSN = USERDSN_ONLY;
  configMode = ODBC_BOTH_DSN;
  return retcode;
}
