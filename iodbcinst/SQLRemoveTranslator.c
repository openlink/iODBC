/*
 *  SQLRemoveTranslator.c
 *
 *  $Id$
 *
 *  These functions intentionally left blank
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
#include "misc.h"
#include "iodbc_error.h"


BOOL INSTAPI
SQLRemoveTranslator (LPCSTR lpszTranslator, LPDWORD lpdwUsageCount)
{
  BOOL retcode = FALSE;
  PCONFIG pCfg;

  /* Check input parameter */
  CLEAR_ERROR ();
  if (!lpszTranslator || !STRLEN (lpszTranslator))
    {
      PUSH_ERROR (ODBC_ERROR_INVALID_NAME);
      goto quit;
    }

  if (_iodbcdm_cfg_search_init (&pCfg, "odbcinst.ini", FALSE))
    {
      PUSH_ERROR (ODBC_ERROR_REQUEST_FAILED);
      goto quit;
    }

  /* deletes the translator */
  _iodbcdm_cfg_write (pCfg, "ODBC Translators", (LPSTR) lpszTranslator, NULL);
  _iodbcdm_cfg_write (pCfg, (LPSTR) lpszTranslator, NULL, NULL);

  if (_iodbcdm_cfg_commit (pCfg))
    {
      PUSH_ERROR (ODBC_ERROR_REQUEST_FAILED);
      goto done;
    }

  retcode = TRUE;

done:
  _iodbcdm_cfg_done (pCfg);

quit:
  return retcode;
}
