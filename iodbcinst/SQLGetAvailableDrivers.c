/*
 *  SQLGetAvailableDrivers.c
 *
 *  $Id$
 *
 *  Get a list of all available drivers
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

BOOL
GetAvailableDrivers (LPCSTR lpszInfFile, LPSTR lpszBuf, WORD cbBufMax,
    WORD FAR *pcbBufOut, BOOL infFile)
{
  int ret = 0, sect_len = 0;
  WORD curr = 0;
  BOOL retcode = FALSE;
  PCONFIG pCfg;
  char *szId;

  if (!lpszBuf || !cbBufMax)
    {
      PUSH_ERROR (ODBC_ERROR_INVALID_BUFF_LEN);
      goto quit;
    }

  /* Open the file to read from */
  if (_iodbcdm_cfg_init (&pCfg, lpszInfFile, FALSE))
    {
      PUSH_ERROR (ODBC_ERROR_COMPONENT_NOT_FOUND);
      goto quit;
    }

  /* Get the ODBC Drivers section */
#ifdef WIN32
  if (_iodbcdm_cfg_find (pCfg, "ODBC 32 bit Drivers", NULL))
#else
  if (_iodbcdm_cfg_find (pCfg, "ODBC Drivers", NULL))
#endif
    {
      PUSH_ERROR (ODBC_ERROR_COMPONENT_NOT_FOUND);
      goto done;
    }

  while (curr < cbBufMax && 0 == _iodbcdm_cfg_nextentry (pCfg))
    {
      if (_iodbcdm_cfg_section (pCfg))
	break;

      if (_iodbcdm_cfg_define (pCfg) && pCfg->id)
	{
	  szId = pCfg->id;
	  while (infFile && *szId == '"')
	    szId++;

	  sect_len = STRLEN (szId);
	  if (!sect_len)
	    {
	      PUSH_ERROR (ODBC_ERROR_INVALID_INF);
	      goto done;
	    }

	  while (infFile && *(szId + sect_len - 1) == '"')
	    sect_len -= 1;

	  sect_len = sect_len > cbBufMax - curr ? cbBufMax - curr : sect_len;

	  if (sect_len)
	    memmove (lpszBuf + curr, szId, sect_len);
	  else
	    {
	      PUSH_ERROR (ODBC_ERROR_INVALID_INF);
	      goto done;
	    }

	  curr += sect_len;
	  lpszBuf[curr++] = 0;
	}
    }

  if (curr < cbBufMax)
    lpszBuf[curr + 1] = 0;
  if (pcbBufOut)
    *pcbBufOut = curr;
  retcode = TRUE;

done:
  _iodbcdm_cfg_done (pCfg);

quit:
  return retcode;
}


BOOL INSTAPI
SQLGetAvailableDrivers (LPCSTR lpszInfFile, LPSTR lpszBuf, WORD cbBufMax,
    WORD FAR *pcbBufOut)
{
  BOOL retcode = FALSE;
  char path[1024] = { 0 };
  WORD lenBufOut;

  /* Get from the user files */
  CLEAR_ERROR ();

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

  retcode =
      GetAvailableDrivers (lpszInfFile, lpszBuf, cbBufMax, &lenBufOut, FALSE);

  if (pcbBufOut)
    *pcbBufOut = lenBufOut;

  wSystemDSN = USERDSN_ONLY;
  configMode = ODBC_BOTH_DSN;
  return retcode;
}
