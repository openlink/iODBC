/*
 *  henv.c
 *
 *  $Id$
 *
 *  Environment object management functions
 *
 *  The iODBC driver manager.
 *  
 *  Copyright (C) 1995 by Ke Jin <kejin@empress.com> 
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

#include <sql.h>
#include <sqlext.h>

#include <dlproc.h>

#include <herr.h>
#include <henv.h>

#include <itrace.h>

SQLRETURN SQL_API
SQLAllocEnv (SQLHENV FAR * phenv)
{
  GENV_t FAR *genv;

  genv = (GENV_t *) MEM_ALLOC (sizeof (GENV_t));

  if (genv == NULL)
    {
      *phenv = SQL_NULL_HENV;

      return SQL_ERROR;
    }
  genv->rc = 0;

  /*
   *  Initialize this handle
   */
  genv->type = SQL_HANDLE_ENV;
  genv->henv = SQL_NULL_HENV;	/* driver's env list */
  genv->hdbc = SQL_NULL_HDBC;	/* driver's dbc list */
  genv->herr = SQL_NULL_HERR;	/* err list          */
#if (ODBCVER >= 0x300)
  genv->odbc_ver = SQL_OV_ODBC2;
#endif

  *phenv = (SQLHENV) genv;

  return SQL_SUCCESS;
}


SQLRETURN SQL_API
SQLFreeEnv (SQLHENV henv)
{
  GENV (genv, henv);

  if (!IS_VALID_HENV (genv))
    {
      return SQL_INVALID_HANDLE;
    }
  CLEAR_ERRORS (genv);

  if (genv->hdbc != SQL_NULL_HDBC)
    {
      PUSHSQLERR (genv->herr, en_S1010);

      return SQL_ERROR;
    }

  /*
   *  Invalidate this handle
   */
  genv->type = 0;

  MEM_FREE (genv);

  return SQL_SUCCESS;
}
