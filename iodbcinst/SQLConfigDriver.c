/*
 *  SQLConfigDriver.c
 *
 *  $Id$
 *
 *  Load the appropriate driver setup DLL and calls the ConfigDriver
 *  function.
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

#ifdef _MACX
#  include <Carbon/Carbon.h>
#endif

#include "dlf.h"
#include "inifile.h"
#include "misc.h"
#include "iodbc_error.h"

#ifndef WIN32
#include <unistd.h>
#define CALL_CONFIG_DRIVER(driverpath) \
  if ((handle = DLL_OPEN((driverpath))) != NULL) \
	{ \
		if ((pConfigDriver = (pConfigDriverFunc)DLL_PROC(handle, "ConfigDriver")) != NULL) \
		{ \
			if (pConfigDriver (hwndParent, fRequest, lpszDriver, lpszArgs, lpszMsg, cbMsgMax, pcbMsgOut))  \
	  	{ \
	    	DLL_CLOSE(handle); \
	    	retcode = TRUE; \
	    	goto done; \
	  	} \
			else \
			{ \
				PUSH_ERROR(ODBC_ERROR_REQUEST_FAILED); \
	    	DLL_CLOSE(handle); \
	    	retcode = FALSE; \
	    	goto done; \
			} \
		} \
		DLL_CLOSE(handle); \
	}
#endif

BOOL INSTAPI
SQLConfigDriver (HWND hwndParent, WORD fRequest, LPCSTR lpszDriver,
    LPCSTR lpszArgs, LPSTR lpszMsg, WORD cbMsgMax, WORD FAR *pcbMsgOut)
{
  PCONFIG pCfg;
  BOOL retcode = FALSE;
  void *handle;
  pConfigDriverFunc pConfigDriver;
#ifdef _MACX
  CFStringRef libname;
  CFBundleRef bundle;
  CFURLRef liburl;
  char name[1024] = { 0 };
#endif

  /* Check input parameters */
  CLEAR_ERROR ();
  if (!lpszDriver || !STRLEN (lpszDriver))
    {
      PUSH_ERROR (ODBC_ERROR_INVALID_NAME);
      goto quit;
    }

  /* Map the request User/System */
  if (fRequest < ODBC_INSTALL_DRIVER || fRequest > ODBC_CONFIG_DRIVER_MAX)
    {
      PUSH_ERROR (ODBC_ERROR_INVALID_REQUEST_TYPE);
      goto quit;
    }

  /* Get it from the user odbcinst file */
  wSystemDSN = USERDSN_ONLY;
  if (!_iodbcdm_cfg_search_init (&pCfg, "odbcinst.ini", TRUE))
    {
      if (!_iodbcdm_cfg_find (pCfg, (char *) lpszDriver, "Setup"))
	CALL_CONFIG_DRIVER (pCfg->value);
      if (!_iodbcdm_cfg_find (pCfg, (char *) lpszDriver, "Driver"))
	CALL_CONFIG_DRIVER (pCfg->value);
      if (!access (lpszDriver, X_OK))
	CALL_CONFIG_DRIVER (lpszDriver);
      if (!_iodbcdm_cfg_find (pCfg, "Default", "Setup"))
	CALL_CONFIG_DRIVER (pCfg->value);
      if (!_iodbcdm_cfg_find (pCfg, "Default", "Driver"))
	CALL_CONFIG_DRIVER (pCfg->value);
    }

  /* Get it from the system odbcinst file */
  if (pCfg)
    {
      _iodbcdm_cfg_done (pCfg);
      pCfg = NULL;
    }
  wSystemDSN = SYSTEMDSN_ONLY;
  if (!_iodbcdm_cfg_search_init (&pCfg, "odbcinst.ini", TRUE))
    {
      if (!_iodbcdm_cfg_find (pCfg, (char *) lpszDriver, "Setup"))
	CALL_CONFIG_DRIVER (pCfg->value);
      if (!_iodbcdm_cfg_find (pCfg, (char *) lpszDriver, "Driver"))
	CALL_CONFIG_DRIVER (pCfg->value);
      if (!access (lpszDriver, X_OK))
	CALL_CONFIG_DRIVER (lpszDriver);
      if (!_iodbcdm_cfg_find (pCfg, "Default", "Setup"))
	CALL_CONFIG_DRIVER (pCfg->value);
      if (!_iodbcdm_cfg_find (pCfg, "Default", "Driver"))
	CALL_CONFIG_DRIVER (pCfg->value);
    }

  /* The last ressort, a proxy driver */
#ifdef _MACX
  bundle = CFBundleGetBundleWithIdentifier (CFSTR ("org.iodbc.core"));
  if (bundle)
    {
      /* Search for the drvproxy library */
      liburl =
	  CFBundleCopyResourceURL (bundle, CFSTR ("iODBCdrvproxy.bundle"),
	  NULL, NULL);
      if (liburl
	  && (libname =
	      CFURLCopyFileSystemPath (liburl, kCFURLPOSIXPathStyle)))
	{
	  CFStringGetCString (libname, name, sizeof (name),
	      kCFStringEncodingASCII);
	  strcat (name, "/Contents/MacOS/iODBCdrvproxy");
	  CALL_CONFIG_DRIVER (name);
	}
      if (liburl)
	CFRelease (liburl);
      if (libname)
	CFRelease (libname);
      CFRelease (bundle);
    }
#else
  CALL_CONFIG_DRIVER ("libdrvproxy.so");
#endif

  /* Error : ConfigDriver could no be found */
  PUSH_ERROR (ODBC_ERROR_LOAD_LIB_FAILED);

done:
  if (pCfg)
    _iodbcdm_cfg_done (pCfg);

quit:
  wSystemDSN = USERDSN_ONLY;
  configMode = ODBC_BOTH_DSN;
  if (pcbMsgOut)
    *pcbMsgOut = 0;

  return retcode;
}
