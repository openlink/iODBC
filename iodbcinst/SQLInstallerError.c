/*
 *  SQLInstallerError.c
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

#include "iodbc_error.h"

LPSTR errortable[] = {
  "",
  "General installer error",
  "Invalid buffer length",
  "Invalid window handle",
  "Invalid string parameter",
  "Invalid type of request",
  "Component not found",
  "Invalid name parameter",
  "Invalid keyword-value pairs",
  "Invalid DSN",
  "Invalid .INF file",
  "Request failed",
  "Invalid install path.",
  "Could not load the driver or translator setup library",
  "Invalid parameter sequence",
  "Invalid log file name.",
  "Operation canceled on user request",
  "Could not increment or decrement the component usage count",
  "Creation of the DSN failed",
  "Error during writing system information",
  "Deletion of the DSN failed",
  "Out of memory",
  "Output string truncated due to a buffer not large enough",
};


RETCODE INSTAPI
SQLInstallerError (WORD iError, DWORD *pfErrorCode, LPSTR lpszErrorMsg,
    WORD cbErrorMsgMax, WORD * pcbErrorMsg)
{
  LPSTR message;
  RETCODE retcode = SQL_ERROR;

  /* Check if the index is valid to retrieve an error */
  if ((iError - 1) > numerrors)
    {
      retcode = SQL_NO_DATA;
      goto quit;
    }

  if (!lpszErrorMsg || !cbErrorMsgMax)
    goto quit;

  lpszErrorMsg[cbErrorMsgMax - 1] = 0;

  /* Copy the message error */
  message = (errormsg[iError - 1]) ?
	errormsg[iError - 1] : errortable[ierror[iError - 1]];

  if (STRLEN (message) >= cbErrorMsgMax - 1)
    {
      STRNCPY (lpszErrorMsg, message, cbErrorMsgMax - 1);
      retcode = SQL_SUCCESS_WITH_INFO;
      goto quit;
    }
  else
    STRCPY (lpszErrorMsg, message);

  if (pfErrorCode)
    *pfErrorCode = ierror[iError - 1];
  if (pcbErrorMsg)
    *pcbErrorMsg = STRLEN (lpszErrorMsg);
  retcode = SQL_SUCCESS;

quit:
  return retcode;
}
