/*
 *  SQLInstallDriver.c
 *
 *  $Id$
 *
 *  Install a driver
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

#include "misc.h"
#include "inifile.h"
#include "iodbc_error.h"

#ifdef _MAC
# include <getfpn.h>
#endif

#if !defined(WINDOWS) && !defined(WIN32) && !defined(OS2) && !defined(macintosh)
# include <pwd.h>
# include <unistd.h>
# include <sys/types.h>
# include <sys/stat.h>
# define UNIX_PWD
#endif


BOOL
InstallDriverPath (LPSTR lpszPath, WORD cbPathMax, WORD FAR *pcbPathOut,
    LPSTR envname)
{
#ifdef _MAC
  OSErr result;
  long fldrDid;
  short fldrRef;
#endif
  BOOL retcode = FALSE;
  char *ptr;

  lpszPath[cbPathMax - 1] = 0;

#if	!defined(UNIX_PWD)
#ifdef _MAC
  result = FindFolder (kOnSystemDisk, kExtensionFolderType, kDontCreateFolder,
      &fldrRef, &fldrDid);
  if (result != noErr)
    {
      PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
      goto quit;
    }

  ptr = get_full_pathname (fldrDid, fldrRef);
  STRNCPY (lpszPath, ptr, cbPathMax - 1);
  free (ptr);

  if (STRLEN (ptr) >= cbPathMax)
    {
      PUSH_ERROR (ODBC_ERROR_INVALID_BUFF_LEN);
      goto quit;
    }
  else
    goto done;
#else
  /*
   *  On Windows, there is only one place to look
   */
  if (GetWindowsDirectory ((LPSTR) buf, cbPathMax) >= cbPathMax)
    {
      PUSH_ERROR (ODBC_ERROR_INVALID_BUFF_LEN);
      goto quit;
    }
  else
    goto done;
#endif
#else

  /*
   *  1. Check $ODBCDRIVERS environment variable
   */
  if ((ptr = getenv (envname)))
    if (access (ptr, R_OK | W_OK | X_OK) == 0)
      {
	STRNCPY (lpszPath, ptr, cbPathMax - 1);
	if (STRLEN (ptr) >= cbPathMax)
	  {
	    PUSH_ERROR (ODBC_ERROR_INVALID_BUFF_LEN);
	    goto quit;
	  }
	else
	  goto done;
      }

  /*
   * 2. Check /usr/local/lib and /usr/lib
   */
#ifdef _BE
  STRNCPY (lpszPath, "/boot/beos/system/lib", cbPathMax - 1);
  if (STRLEN (lpszPath) != STRLEN ("/boot/beos/system/lib"))
#else
  STRNCPY (lpszPath, "/usr/local/lib", cbPathMax - 1);
  if (STRLEN (lpszPath) != STRLEN ("/usr/local/lib"))
#endif
    {
      PUSH_ERROR (ODBC_ERROR_INVALID_BUFF_LEN);
      goto quit;
    }
  if (access (lpszPath, R_OK | W_OK | X_OK) == 0)
    goto done;

#ifdef _BE
  STRNCPY (lpszPath, "/boot/home/config/lib", cbPathMax - 1);
  if (STRLEN (lpszPath) != STRLEN ("/boot/home/config/lib"))
#else
  STRNCPY (lpszPath, "/usr/lib", cbPathMax - 1);
  if (STRLEN (lpszPath) != STRLEN ("/usr/lib"))
#endif
    {
      PUSH_ERROR (ODBC_ERROR_INVALID_BUFF_LEN);
      goto quit;
    }
  if (access (lpszPath, R_OK | W_OK | X_OK) == 0)
    goto done;

  /*
   *  3. Check either $HOME
   */
  if (!(ptr = getenv ("HOME")))
    {
      ptr = (char *) getpwuid (getuid ());
      if (ptr)
	ptr = ((struct passwd *) ptr)->pw_dir;
    }

  if (ptr)
    {
#ifdef _BE
      sprintf (lpszPath, "%s/config/lib", ptr);
#else
      sprintf (lpszPath, "%s/lib", ptr);
#endif
      if (access (lpszPath, R_OK | W_OK | X_OK) == 0)
	goto done;
    }

  if (!mkdir (lpszPath, 0755))
    goto done;

#endif

  SQLPostInstallerError (ODBC_ERROR_GENERAL_ERR,
      "Cannot retrieve a directory where to install the driver or translator.");
  goto quit;

done:
  retcode = TRUE;

quit:
  if (pcbPathOut)
    *pcbPathOut = STRLEN (lpszPath);
  return retcode;
}


BOOL INSTAPI
SQLInstallDriver (LPCSTR lpszInfFile, LPCSTR lpszDriver, LPSTR lpszPath,
    WORD cbPathMax, WORD FAR *pcbPathOut)
{
  PCONFIG pCfg = NULL, pOdbcCfg = NULL;
  int ret = 0, sect_len = 0;
  WORD curr = 0;
  BOOL retcode = FALSE;
  char *szId;

  /* Check input parameters */
  CLEAR_ERROR ();
  if (!lpszDriver || !STRLEN (lpszDriver))
    {
      PUSH_ERROR (ODBC_ERROR_INVALID_PARAM_SEQUENCE);
      goto quit;
    }

  if (!lpszPath || !cbPathMax)
    {
      PUSH_ERROR (ODBC_ERROR_INVALID_BUFF_LEN);
      goto quit;
    }

  /* Write the out path */
  if (!InstallDriverPath (lpszPath, cbPathMax, pcbPathOut, "ODBCDRIVERS"))
    goto quit;

  /* Else go through user/system odbcinst.ini */
  switch (configMode)
    {
    case ODBC_BOTH_DSN:
    case ODBC_USER_DSN:
      wSystemDSN = USERDSN_ONLY;
      break;

    case ODBC_SYSTEM_DSN:
      wSystemDSN = SYSTEMDSN_ONLY;
      break;
    };

  if (_iodbcdm_cfg_search_init (&pCfg, "odbcinst.ini", TRUE))
    {
      PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
      goto quit;
    }

  if (_iodbcdm_cfg_search_init (&pOdbcCfg, "odbc.ini", TRUE))
    {
      PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
      pOdbcCfg = NULL;
      goto done;
    }

  if (lpszInfFile)
    {
      if (!install_from_ini (pCfg, pOdbcCfg, (char *) lpszInfFile,
	      (char *) lpszDriver, TRUE))
	{
	  PUSH_ERROR (ODBC_ERROR_INVALID_INF);
	  goto done;
	}
    }
  else if (!install_from_string (pCfg, pOdbcCfg, (char *) lpszDriver, TRUE))
    {
      PUSH_ERROR (ODBC_ERROR_INVALID_KEYWORD_VALUE);
      goto done;
    }

  if (_iodbcdm_cfg_commit (pCfg) || _iodbcdm_cfg_commit (pOdbcCfg))
    {
      PUSH_ERROR (ODBC_ERROR_GENERAL_ERR);
      goto done;
    }

  retcode = TRUE;

done:
  _iodbcdm_cfg_done (pCfg);
  if (pOdbcCfg)
    _iodbcdm_cfg_done (pOdbcCfg);

quit:
  wSystemDSN = USERDSN_ONLY;
  configMode = ODBC_BOTH_DSN;

  return retcode;
}
