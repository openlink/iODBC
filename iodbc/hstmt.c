/*
 *  hstmt.c
 *
 *  $Id$
 *
 *  Query statement object management functions
 *
 *  The iODBC driver manager.
 *
 *  Copyright (C) 1995 Ke Jin <kejin@empress.com>
 *  Copyright (C) 1996-2022 OpenLink Software <iodbc@openlinksw.com>
 *  All Rights Reserved.
 *
 *  This software is released under the terms of either of the following
 *  licenses:
 *
 *      - GNU Library General Public License (see LICENSE.LGPL)
 *      - The BSD License (see LICENSE.BSD).
 *
 *  Note that the only valid version of the LGPL license as far as this
 *  project is concerned is the original GNU Library General Public License
 *  Version 2, dated June 1991.
 *
 *  While not mandated by the BSD license, any patches you make to the
 *  iODBC source code may be contributed back into the iODBC project
 *  at your discretion. Contributions will benefit the Open Source and
 *  Data Access community as a whole. Submissions may be made at:
 *
 *      http://www.iodbc.org
 *
 *
 *  GNU Library Generic Public License Version 2
 *  ============================================
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; only
 *  Version 2 of the License dated June 1991.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 *  The BSD License
 *  ===============
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *  3. Neither the name of OpenLink Software Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL OPENLINK OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <iodbc.h>

#include <isql.h>
#include <isqlext.h>

#include "unicode.h"

#include "dlproc.h"

#include "herr.h"
#if (ODBCVER >= 0x0300)
#include "hdesc.h"
#endif
#include "henv.h"
#include "hdbc.h"
#include "hstmt.h"

#include "itrace.h"

#if (ODBCVER >= 0x300)
static const SQLINTEGER desc_attrs[4] = 
{
  SQL_ATTR_APP_ROW_DESC,
  SQL_ATTR_APP_PARAM_DESC,
  SQL_ATTR_IMP_ROW_DESC,
  SQL_ATTR_IMP_PARAM_DESC
};
#endif

#define XFREE(V)  if (V) { free(V); V = NULL; }

SQLRETURN
SQLAllocStmt_Internal (
    SQLHDBC hdbc,
    SQLHSTMT * phstmt)
{
  CONN (pdbc, hdbc);
  STMT (pstmt, NULL);
  HPROC hproc2 = SQL_NULL_HPROC;
  HPROC hproc3 = SQL_NULL_HPROC;
  SQLRETURN retcode = SQL_SUCCESS;
  int i;
  SQLUINTEGER odbc_ver = ((GENV_t *) pdbc->genv)->odbc_ver;
  SQLUINTEGER dodbc_ver = ((ENV_t *) pdbc->henv)->dodbc_ver;

  if (phstmt == NULL)
    {
      PUSHSQLERR (pdbc->herr, en_S1009);
      return SQL_ERROR;
    }

  /* check state */
  switch (pdbc->state)
    {
    case en_dbc_connected:
    case en_dbc_hstmt:
      break;

    case en_dbc_allocated:
    case en_dbc_needdata:
      PUSHSQLERR (pdbc->herr, en_08003);
      *phstmt = SQL_NULL_HSTMT;

      return SQL_ERROR;

    default:
      return SQL_INVALID_HANDLE;
    }

  pstmt = (STMT_t *) MEM_ALLOC (sizeof (STMT_t));

  if (pstmt == NULL)
    {
      PUSHSQLERR (pdbc->herr, en_S1001);
      *phstmt = SQL_NULL_HSTMT;

      return SQL_ERROR;
    }
  pstmt->rc = 0;

  /*
   *  Initialize this handle
   */
  pstmt->type = SQL_HANDLE_STMT;

  /* initiate the object */
  pstmt->herr = SQL_NULL_HERR;
  pstmt->hdbc = (HSTMT) hdbc;
  pstmt->state = en_stmt_allocated;
  pstmt->cursor_state = en_stmt_cursor_no;
  pstmt->prep_state = 0;
  pstmt->asyn_on = en_NullProc;
  pstmt->need_on = en_NullProc;
  pstmt->stmt_cip = 0;
  pstmt->err_rec = 0;

  memset (pstmt->vars, 0, sizeof (VAR_t) * STMT_VARS_MAX);
  pstmt->vars_inserted = 0;

  /* call driver's function */
  pstmt->rowset_size = 1;
  pstmt->row_bind_type = SQL_BIND_BY_COLUMN;
  pstmt->row_bind_offset = 0;
  pstmt->param_bind_type = SQL_PARAM_BIND_BY_COLUMN;
  pstmt->param_bind_offset = 0;
  pstmt->st_pbinding = NULL;

  pstmt->st_pparam = NULL;
  pstmt->st_nparam = 0;

  pstmt->params_buf = NULL;
  pstmt->rows_buf = NULL;
  pstmt->conv_param_bind_type = SQL_PARAM_BIND_BY_COLUMN;
  pstmt->conv_row_bind_type = SQL_BIND_BY_COLUMN;

#if (ODBCVER >= 0x0300)
  pstmt->row_array_size = 1;
  pstmt->fetch_bookmark_ptr = NULL;
  pstmt->params_processed_ptr = NULL;
  pstmt->paramset_size = 0;
  pstmt->rows_fetched_ptr = NULL;
  if (dodbc_ver == SQL_OV_ODBC2 && odbc_ver == SQL_OV_ODBC3)
    {		    /* if it's a odbc3 app calling odbc2 driver */
      pstmt->row_status_ptr =
	  MEM_ALLOC (sizeof (SQLUINTEGER) * pstmt->row_array_size);
      if (!pstmt->row_status_ptr)
	{
	  PUSHSQLERR (pstmt->herr, en_HY001);
	  *phstmt = SQL_NULL_HSTMT;
	  pstmt->type = 0;
	  MEM_FREE (pstmt);
	  return SQL_ERROR;
	}
      pstmt->row_status_allocated = SQL_TRUE;
    }
  else
    {
      pstmt->row_status_ptr = NULL;
      pstmt->row_status_allocated = SQL_FALSE;
    }
#endif

  hproc2 = _iodbcdm_getproc (pdbc, en_AllocStmt);
#if (ODBCVER >= 0x0300)
  hproc3 = _iodbcdm_getproc (pdbc, en_AllocHandle);
#endif

  if (odbc_ver == SQL_OV_ODBC2 && 
      (  dodbc_ver == SQL_OV_ODBC2
       || (dodbc_ver == SQL_OV_ODBC3 && hproc2 != SQL_NULL_HPROC)))
    hproc3 = SQL_NULL_HPROC;

#if (ODBCVER >= 0x0300)
  if (hproc3)
    {
      CALL_DRIVER (pstmt->hdbc, pdbc, retcode, hproc3,
	  (SQL_HANDLE_STMT, pdbc->dhdbc, &(pstmt->dhstmt)));
    }
  else
#endif
    {
      if (hproc2 == SQL_NULL_HPROC)
	{
	  PUSHSQLERR (pstmt->herr, en_IM001);
	  *phstmt = SQL_NULL_HSTMT;
	  pstmt->type = 0;
	  MEM_FREE (pstmt);

	  return SQL_ERROR;
	}

      CALL_DRIVER (hdbc, pdbc, retcode, hproc2,
	  (pdbc->dhdbc, &(pstmt->dhstmt)));
    }

  if (!SQL_SUCCEEDED (retcode))
    {
      *phstmt = SQL_NULL_HSTMT;
      pstmt->type = 0;
      MEM_FREE (pstmt);

      return retcode;
    }

#if (ODBCVER >= 0x0300)
  /* get the descriptors */
  memset (&pstmt->imp_desc, 0, sizeof (pstmt->imp_desc));
  memset (&pstmt->desc, 0, sizeof (pstmt->desc));

  if (dodbc_ver == SQL_OV_ODBC2)
    {
      /* 
       *  this is an ODBC2 driver - so alloc dummy implicit desc handles  
       *  (dhdesc = NULL) 
       */
      for (i = 0; i < 4; i++)
	{
	  pstmt->imp_desc[i] = (DESC_t *) MEM_ALLOC (sizeof (DESC_t));
	  if (pstmt->imp_desc[i] == NULL)
	    {
	      PUSHSQLERR (pdbc->herr, en_HY001);

	      goto alloc_stmt_failed;
	    }
	  memset (pstmt->imp_desc[i], 0, sizeof (DESC_t));
	  pstmt->imp_desc[i]->type = SQL_HANDLE_DESC;
	  pstmt->imp_desc[i]->hstmt = pstmt;
	  pstmt->imp_desc[i]->dhdesc = NULL;
	  pstmt->imp_desc[i]->hdbc = hdbc;
	  pstmt->imp_desc[i]->herr = NULL;
	}
    }
  else
    {				/* the ODBC3 driver */
      if (((ENV_t *) pdbc->henv)->unicode_driver)
	hproc3 = _iodbcdm_getproc (pdbc, en_GetStmtAttrW);
      else
        {
	  hproc3 = _iodbcdm_getproc (pdbc, en_GetStmtAttr);
	  if (hproc3 == SQL_NULL_HPROC)
	    hproc3 = _iodbcdm_getproc (pdbc, en_GetStmtAttrA);
	}

      if (hproc3 == SQL_NULL_HPROC)
	{			/* with no GetStmtAttr ! */
	  PUSHSQLERR (pdbc->herr, en_HYC00);
	  goto alloc_stmt_failed;
	}
      else
	{			/* get the implicit descriptors */
	  RETCODE rc1;

	  for (i = 0; i < 4; i++)
	    {
	      int desc_type = 0;

	      switch (i)
		{
		case APP_ROW_DESC:
		  desc_type = SQL_ATTR_APP_ROW_DESC;
		  break;
		case APP_PARAM_DESC:
		  desc_type = SQL_ATTR_APP_PARAM_DESC;
		  break;
		case IMP_ROW_DESC:
		  desc_type = SQL_ATTR_IMP_ROW_DESC;
		  break;
		case IMP_PARAM_DESC:
		  desc_type = SQL_ATTR_IMP_PARAM_DESC;
		  break;
		}

	      pstmt->imp_desc[i] = (DESC_t *) MEM_ALLOC (sizeof (DESC_t));
	      if (pstmt->imp_desc[i] == NULL)
		{		/* memory allocation error */
		  PUSHSQLERR (pdbc->herr, en_HY001);

		  goto alloc_stmt_failed;
		}
	      memset (pstmt->imp_desc[i], 0, sizeof (DESC_t));
	      pstmt->imp_desc[i]->type = SQL_HANDLE_DESC;
	      pstmt->imp_desc[i]->hdbc = hdbc;
	      pstmt->imp_desc[i]->hstmt = *phstmt;
	      pstmt->imp_desc[i]->herr = NULL;
	      CALL_DRIVER (hdbc, pstmt, rc1, hproc3,
		  (pstmt->dhstmt, desc_type, &pstmt->imp_desc[i]->dhdesc, 0,
		      NULL));
	      if (rc1 != SQL_SUCCESS && rc1 != SQL_SUCCESS_WITH_INFO)
		{		/* no descriptor returned from the driver */
		  pdbc->rc = SQL_ERROR;

		  goto alloc_stmt_failed;
		}
	    }
	}
    }
#endif

  /* insert into list */
  pstmt->next = (STMT_t *) pdbc->hstmt;
  pdbc->hstmt = pstmt;

  *phstmt = (SQLHSTMT) pstmt;

  /* state transition */
  pdbc->state = en_dbc_hstmt;

  return SQL_SUCCESS;

  /*
   *  If statement allocation has failed, we need to make sure the driver
   *  handle is also destroyed
   */
alloc_stmt_failed:

  /*
   *  Deallocate any descriptors
   */
  for (i = 0; i < 4; i++)
    {
      if (pstmt->imp_desc[i])
       {
	  pstmt->imp_desc[i]->type = 0;
	  MEM_FREE (pstmt->imp_desc[i]);
       }
    }

  /*
   *  Tell the driver to remove the statement handle
   */
  hproc2 = SQL_NULL_HPROC;
  hproc3 = SQL_NULL_HPROC;

  hproc2 = _iodbcdm_getproc (pstmt->hdbc, en_FreeStmt);
#if (ODBCVER >= 0x0300)
  hproc3 = _iodbcdm_getproc (pstmt->hdbc, en_FreeHandle);
#endif

  if (odbc_ver == SQL_OV_ODBC2 && 
      (  dodbc_ver == SQL_OV_ODBC2
       || (dodbc_ver == SQL_OV_ODBC3 && hproc2 != SQL_NULL_HPROC)))
    hproc3 = SQL_NULL_HPROC;

#if (ODBCVER >= 0x0300)
  if (hproc3 != SQL_NULL_HPROC)
    {
      CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc3,
	  (SQL_HANDLE_STMT, pstmt->dhstmt));
    }
  else
#endif
    {
      if (hproc2 == SQL_NULL_HPROC)
        {
          PUSHSQLERR (pdbc->herr, en_IM001);
          return SQL_ERROR;
        }

      CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc2,
	  (pstmt->dhstmt, SQL_DROP));
    }

  /*
   *  Invalidate and free the statement handle
   */
  pstmt->type = 0;
  MEM_FREE (pstmt);

  return SQL_ERROR;
}


SQLRETURN SQL_API 
SQLAllocStmt (
    SQLHDBC hdbc,
    SQLHSTMT * phstmt)
{
  ENTER_HDBC (hdbc, 1,
    trace_SQLAllocStmt (TRACE_ENTER, hdbc, phstmt));

  retcode = SQLAllocStmt_Internal (hdbc, phstmt);

  LEAVE_HDBC (hdbc, 1,
    trace_SQLAllocStmt (TRACE_LEAVE, hdbc, phstmt));
}


SQLRETURN 
_iodbcdm_dropstmt (HSTMT hstmt)
{
  STMT (pstmt, hstmt);
  STMT (tpstmt, NULL);
  CONN (pdbc, NULL);

  if (!IS_VALID_HSTMT (pstmt))
    {
      return SQL_INVALID_HANDLE;
    }
  CLEAR_ERRORS (pstmt);

  pdbc = (DBC_t *) (pstmt->hdbc);

  for (tpstmt = (STMT_t *) pdbc->hstmt;
      tpstmt != NULL;
      tpstmt = tpstmt->next)
    {
      if (tpstmt == pstmt)
	{
	  pdbc->hstmt = (HSTMT) pstmt->next;
	  break;
	}

      if (tpstmt->next == pstmt)
	{
	  tpstmt->next = pstmt->next;
	  break;
	}
    }

  if (tpstmt == NULL)
    {
      return SQL_INVALID_HANDLE;
    }

#if (ODBCVER >= 0x0300)
  if (pstmt->row_status_allocated == SQL_TRUE && pstmt->row_status_ptr)
    MEM_FREE(pstmt->row_status_ptr);
  
  /* drop the implicit descriptors */
  if (pstmt->imp_desc[0])
    {
      int i;
      for (i = 0; i < 4; i++)
	{
	  _iodbcdm_freesqlerrlist (pstmt->imp_desc[i]->herr);
	  pstmt->imp_desc[i]->type = 0;
	  MEM_FREE(pstmt->imp_desc[i]);
	}
    }
#endif

  MEM_FREE(pstmt->params_buf);
  MEM_FREE(pstmt->rows_buf);

  if (pstmt->vars_inserted > 0)
    _iodbcdm_FreeStmtVars(pstmt);

  _iodbcdm_FreeStmtParams(pstmt);

  /*
   *  Invalidate this handle
   */
  pstmt->type = 0;

  MEM_FREE (pstmt);

  return SQL_SUCCESS;
}


SQLRETURN
SQLFreeStmt_Internal (
    SQLHSTMT hstmt,
    SQLUSMALLINT fOption)
{
  STMT (pstmt, hstmt);
  CONN (pdbc, pstmt->hdbc);
  HPROC hproc2 = SQL_NULL_HPROC;
  HPROC hproc3 = SQL_NULL_HPROC;
  SQLRETURN retcode = SQL_SUCCESS;
  SQLUINTEGER odbc_ver = ((GENV_t *) pdbc->genv)->odbc_ver;
  SQLUINTEGER dodbc_ver = ((ENV_t *) pdbc->henv)->dodbc_ver;

  /* check option */
  switch (fOption)
    {
    case SQL_DROP:
    case SQL_CLOSE:
    case SQL_UNBIND:
    case SQL_RESET_PARAMS:
      break;

    default:
      PUSHSQLERR (pstmt->herr, en_S1092);
      return SQL_ERROR;
    }

  /* check state */
  if (pstmt->state >= en_stmt_needdata || pstmt->asyn_on != en_NullProc)
    {
      PUSHSQLERR (pstmt->herr, en_S1010);
      return SQL_ERROR;
    }


  hproc2 = _iodbcdm_getproc (pstmt->hdbc, en_FreeStmt);
#if (ODBCVER >= 0x0300)
  hproc3 = _iodbcdm_getproc (pstmt->hdbc, en_FreeHandle);
#endif

  if (odbc_ver == SQL_OV_ODBC2 && 
      (  dodbc_ver == SQL_OV_ODBC2
       || (dodbc_ver == SQL_OV_ODBC3 && hproc2 != SQL_NULL_HPROC)))
    hproc3 = SQL_NULL_HPROC;

#if (ODBCVER >= 0x0300)
  if (fOption == SQL_DROP && hproc3 != SQL_NULL_HPROC)
    {
      CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc3,
	      (SQL_HANDLE_STMT, pstmt->dhstmt));
    }
#endif
  else
    {
      if (hproc2 == SQL_NULL_HPROC)
	{
	  PUSHSQLERR (pstmt->herr, en_IM001);
	  return SQL_ERROR;
	}

      CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc2,
	  (pstmt->dhstmt, fOption));
    }

  if (!SQL_SUCCEEDED (retcode))
    {
      return retcode;
    }

  /* state transition */
  switch (fOption)
    {
    case SQL_DROP:
      /* delete this object (ignore return) */
      _iodbcdm_RemoveBind (pstmt);
      _iodbcdm_FreeStmtParams(pstmt);
#if 0
      _iodbcdm_dropstmt (pstmt);	/* Delayed until last moment */
#endif
      break;

    case SQL_CLOSE:
      pstmt->cursor_state = en_stmt_cursor_no;
      /* This means cursor name set by
       * SQLSetCursorName() call will also 
       * be erased.
       */

      switch (pstmt->state)
	{
	case en_stmt_allocated:
	case en_stmt_prepared:
	  break;

	case en_stmt_executed_with_info:
	case en_stmt_executed:
	case en_stmt_cursoropen:
	case en_stmt_fetched:
	case en_stmt_xfetched:
	  if (pstmt->prep_state)
	    {
	      pstmt->state = en_stmt_prepared;
	    }
	  else
	    {
	      pstmt->state = en_stmt_allocated;
	    }
	  break;

	default:
	  break;
	}
      break;

    case SQL_UNBIND:
      _iodbcdm_RemoveBind (pstmt);
      break;
    case SQL_RESET_PARAMS:
      _iodbcdm_FreeStmtParams(pstmt);
      break;
    default:
      break;
    }

  return retcode;
}


SQLRETURN SQL_API
SQLFreeStmt (
    SQLHSTMT hstmt,
    SQLUSMALLINT fOption)
{
  ENTER_STMT (hstmt,
    trace_SQLFreeStmt (TRACE_ENTER, hstmt, fOption));

  retcode = SQLFreeStmt_Internal (hstmt, fOption);

  LEAVE_STMT (hstmt,
    trace_SQLFreeStmt (TRACE_LEAVE, hstmt, fOption);
    if (fOption == SQL_DROP)
	_iodbcdm_dropstmt (hstmt);
  );
}


static SQLRETURN
SQLSetStmtOption_Internal (
    SQLHSTMT hstmt,
    SQLUSMALLINT fOption,
    SQLUINTEGER vParam)
{
  STMT (pstmt, hstmt);
  HPROC hproc2 = SQL_NULL_HPROC;
  HPROC hproc3 = SQL_NULL_HPROC;
  sqlstcode_t sqlstat = en_00000;
  SQLRETURN retcode;
  CONN (pdbc, pstmt->hdbc);
  SQLUINTEGER odbc_ver = ((GENV_t *) pdbc->genv)->odbc_ver;
  SQLUINTEGER dodbc_ver = ((ENV_t *) pdbc->henv)->dodbc_ver;

#if (ODBCVER < 0x0300)
  /* check option */
  if (				/* fOption < SQL_STMT_OPT_MIN || */
      fOption > SQL_STMT_OPT_MAX)
    {
      PUSHSQLERR (pstmt->herr, en_S1092);
      return SQL_ERROR;
    }
#endif	/* ODBCVER < 0x0300 */

  if (fOption == SQL_CONCURRENCY
      || fOption == SQL_CURSOR_TYPE
      || fOption == SQL_SIMULATE_CURSOR
      || fOption == SQL_USE_BOOKMARKS)
    {
      if (pstmt->asyn_on != en_NullProc)
	{
	  if (pstmt->prep_state)
	    {
	      sqlstat = en_S1011;
	    }
	}
      else
	{
	  switch (pstmt->state)
	     {
	     case en_stmt_prepared:
	       sqlstat = en_S1011;
	       break;

	     case en_stmt_executed_with_info:
	     case en_stmt_executed:
	     case en_stmt_cursoropen:
	     case en_stmt_fetched:
	     case en_stmt_xfetched:
	       sqlstat = en_24000;
	       break;

	     case en_stmt_needdata:
	     case en_stmt_mustput:
	     case en_stmt_canput:
	       if (pstmt->prep_state)
		 {
		   sqlstat = en_S1011;
		 }
	       break;

	     default:
	       break;
	     }
	}
    }
  else
    {
      if (pstmt->asyn_on != en_NullProc)
	{
	  if (!pstmt->prep_state)
	    {
	      sqlstat = en_S1010;
	    }
	}
      else
	{
	  if (pstmt->state >= en_stmt_needdata)
	    {
	      sqlstat = en_S1010;
	    }
	}
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }


  hproc2 = _iodbcdm_getproc (pstmt->hdbc, en_SetStmtOption);
#if (ODBCVER >= 0x0300)
  hproc3 = _iodbcdm_getproc (pstmt->hdbc, en_SetStmtAttr);
#endif

  if (odbc_ver == SQL_OV_ODBC2 && 
      (  dodbc_ver == SQL_OV_ODBC2
       || (dodbc_ver == SQL_OV_ODBC3 && hproc2 != SQL_NULL_HPROC)))
    hproc3 = SQL_NULL_HPROC;

#if (ODBCVER >= 0x0300)
  if (hproc3 != SQL_NULL_HPROC)
    {
      switch ((int)fOption)
	{
	/* ODBC integer attributes */   
	  case SQL_ATTR_ASYNC_ENABLE:
	  case SQL_ATTR_CONCURRENCY:
	  case SQL_ATTR_CURSOR_TYPE:
	  case SQL_ATTR_KEYSET_SIZE:
	  case SQL_ATTR_MAX_LENGTH:
	  case SQL_ATTR_MAX_ROWS:
	  case SQL_ATTR_NOSCAN:
	  case SQL_ATTR_QUERY_TIMEOUT:
	  case SQL_ATTR_RETRIEVE_DATA:
	  case SQL_ATTR_ROW_BIND_TYPE:
	  case SQL_ATTR_ROW_NUMBER:
	  case SQL_ATTR_SIMULATE_CURSOR:
	  case SQL_ATTR_USE_BOOKMARKS:
	      CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc3,
		  (pstmt->dhstmt, fOption, vParam, 0));
	      break;	  

	/* ODBC3 attributes */   
	  case SQL_ATTR_APP_PARAM_DESC:
	  case SQL_ATTR_APP_ROW_DESC:
	  case SQL_ATTR_CURSOR_SCROLLABLE:
	  case SQL_ATTR_CURSOR_SENSITIVITY:
	  case SQL_ATTR_ENABLE_AUTO_IPD:
	  case SQL_ATTR_FETCH_BOOKMARK_PTR:
	  case SQL_ATTR_IMP_PARAM_DESC:
	  case SQL_ATTR_IMP_ROW_DESC:
	  case SQL_ATTR_METADATA_ID:
	  case SQL_ATTR_PARAM_BIND_OFFSET_PTR:
	  case SQL_ATTR_PARAM_BIND_TYPE:
	  case SQL_ATTR_PARAM_STATUS_PTR:
	  case SQL_ATTR_PARAMS_PROCESSED_PTR:
	  case SQL_ATTR_PARAMSET_SIZE:
	  case SQL_ATTR_ROW_ARRAY_SIZE:
	  case SQL_ATTR_ROW_BIND_OFFSET_PTR:
	  case SQL_ATTR_ROW_OPERATION_PTR:
	  case SQL_ATTR_ROW_STATUS_PTR:
	  case SQL_ATTR_ROWS_FETCHED_PTR:
	      PUSHSQLERR (pstmt->herr, en_IM001);
	      return SQL_ERROR;

	  default:
	      CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc3,
		  (pstmt->dhstmt, fOption, vParam, SQL_NTS));
	}
    }
  else
#endif
    {
      if (hproc2 == SQL_NULL_HPROC)
	{
	  PUSHSQLERR (pstmt->herr, en_IM001);
	  return SQL_ERROR;
	}

      CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc2,
	  (pstmt->dhstmt, fOption, vParam));
    }

  if (SQL_SUCCEEDED (retcode))
    {
      if (fOption == SQL_ROWSET_SIZE || fOption == SQL_ATTR_ROW_ARRAY_SIZE)
        {
          pstmt->rowset_size = vParam;
          if (retcode == SQL_SUCCESS_WITH_INFO)
            {
              SQLUINTEGER data;
              if (SQLGetStmtOption_Internal (hstmt, SQL_ROWSET_SIZE, &data) 
                   == SQL_SUCCESS)
                pstmt->rowset_size = data;
            }
        }
      if (fOption == SQL_BIND_TYPE)
        pstmt->row_bind_type = vParam;
    }
  return retcode;
}


SQLRETURN SQL_API 
SQLSetStmtOption (
  SQLHSTMT		  hstmt,
  SQLUSMALLINT		  fOption,
  SQLULEN		  vParam)
{
  ENTER_STMT (hstmt,
    trace_SQLSetStmtOption (TRACE_ENTER, hstmt, fOption, vParam));

  retcode = SQLSetStmtOption_Internal (hstmt, fOption, vParam);

  LEAVE_STMT (hstmt,
    trace_SQLSetStmtOption (TRACE_LEAVE, hstmt, fOption, vParam));
}


#if ODBCVER >= 0x0300
SQLRETURN SQL_API 
SQLSetStmtOptionA (
  SQLHSTMT		  hstmt,
  SQLUSMALLINT		  fOption,
  SQLULEN		  vParam)
{
  ENTER_STMT (hstmt,
    trace_SQLSetStmtOption (TRACE_ENTER, hstmt, fOption, vParam));

  retcode = SQLSetStmtOption_Internal (hstmt, fOption, vParam);

  LEAVE_STMT (hstmt,
    trace_SQLSetStmtOption (TRACE_LEAVE, hstmt, fOption, vParam));
}
#endif


SQLRETURN
SQLGetStmtOption_Internal (
    SQLHSTMT hstmt,
    SQLUSMALLINT fOption,
    SQLPOINTER pvParam)
{
  STMT (pstmt, hstmt);
  HPROC hproc2 = SQL_NULL_HPROC;
  HPROC hproc3 = SQL_NULL_HPROC;
  sqlstcode_t sqlstat = en_00000;
  SQLRETURN retcode;
  CONN (pdbc, pstmt->hdbc);
  SQLUINTEGER odbc_ver = ((GENV_t *) pdbc->genv)->odbc_ver;
  SQLUINTEGER dodbc_ver = ((ENV_t *) pdbc->henv)->dodbc_ver;


#if (ODBCVER < 0x0300)
  /* check option */
  if (				/* fOption < SQL_STMT_OPT_MIN || */
      fOption > SQL_STMT_OPT_MAX)
    {
      PUSHSQLERR (pstmt->herr, en_S1092);
      return SQL_ERROR;
    }
#endif /* ODBCVER < 0x0300 */

  /* check state */
  if (pstmt->state >= en_stmt_needdata || pstmt->asyn_on != en_NullProc)
    {
      sqlstat = en_S1010;
    }
  else
    {
      switch (pstmt->state)
	{
	case en_stmt_allocated:
	case en_stmt_prepared:
	case en_stmt_executed_with_info:
	case en_stmt_executed:
	case en_stmt_cursoropen:
	  if (fOption == SQL_ROW_NUMBER || fOption == SQL_GET_BOOKMARK)
	    {
	      sqlstat = en_24000;
	    }
	  break;

	default:
	  break;
	}
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      return SQL_ERROR;
    }

  hproc2 = _iodbcdm_getproc (pstmt->hdbc, en_GetStmtOption);
#if (ODBCVER >= 0x0300)
  hproc3 = _iodbcdm_getproc (pstmt->hdbc, en_GetStmtAttr);
#endif

  if (odbc_ver == SQL_OV_ODBC2 && 
      (  dodbc_ver == SQL_OV_ODBC2
       || (dodbc_ver == SQL_OV_ODBC3 && hproc2 != SQL_NULL_HPROC)))
    hproc3 = SQL_NULL_HPROC;

#if (ODBCVER >= 0x0300)
  if (hproc3 != SQL_NULL_HPROC)
    {
      switch ((int)fOption)
	{
	  /* ODBC integer attributes */
	case SQL_ATTR_ASYNC_ENABLE:
	case SQL_ATTR_CONCURRENCY:
	case SQL_ATTR_CURSOR_TYPE:
	case SQL_ATTR_KEYSET_SIZE:
	case SQL_ATTR_MAX_LENGTH:
	case SQL_ATTR_MAX_ROWS:
	case SQL_ATTR_NOSCAN:
	case SQL_ATTR_QUERY_TIMEOUT:
	case SQL_ATTR_RETRIEVE_DATA:
	case SQL_ATTR_ROW_BIND_TYPE:
	case SQL_ATTR_ROW_NUMBER:
	case SQL_ATTR_SIMULATE_CURSOR:
	case SQL_ATTR_USE_BOOKMARKS:
	  CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc3,
	      (pstmt->dhstmt, fOption, pvParam, 0, NULL));
	  break;

	  /* ODBC3 attributes */
	case SQL_ATTR_APP_PARAM_DESC:
	case SQL_ATTR_APP_ROW_DESC:
	case SQL_ATTR_CURSOR_SCROLLABLE:
	case SQL_ATTR_CURSOR_SENSITIVITY:
	case SQL_ATTR_ENABLE_AUTO_IPD:
	case SQL_ATTR_FETCH_BOOKMARK_PTR:
	case SQL_ATTR_IMP_PARAM_DESC:
	case SQL_ATTR_IMP_ROW_DESC:
	case SQL_ATTR_METADATA_ID:
	case SQL_ATTR_PARAM_BIND_OFFSET_PTR:
	case SQL_ATTR_PARAM_BIND_TYPE:
	case SQL_ATTR_PARAM_STATUS_PTR:
	case SQL_ATTR_PARAMS_PROCESSED_PTR:
	case SQL_ATTR_PARAMSET_SIZE:
	case SQL_ATTR_ROW_ARRAY_SIZE:
	case SQL_ATTR_ROW_BIND_OFFSET_PTR:
	case SQL_ATTR_ROW_OPERATION_PTR:
	case SQL_ATTR_ROW_STATUS_PTR:
	case SQL_ATTR_ROWS_FETCHED_PTR:
	  PUSHSQLERR (pstmt->herr, en_IM001);
	  return SQL_ERROR;

	default:
	  CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc3,
	      (pstmt->dhstmt, fOption, pvParam, SQL_MAX_OPTION_STRING_LENGTH,
		  NULL));
	  break;
	}
    }
  else
#endif
    {
      if (hproc2 == SQL_NULL_HPROC)
        {
          PUSHSQLERR (pstmt->herr, en_IM001);
          return SQL_ERROR;
        }
      CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc2,
	  (pstmt->dhstmt, fOption, pvParam));
    }
  return retcode;
}


SQLRETURN SQL_API 
SQLGetStmtOption (
    SQLHSTMT hstmt,
    SQLUSMALLINT fOption,
    SQLPOINTER pvParam)
{
  ENTER_STMT (hstmt,
    trace_SQLGetStmtOption (TRACE_ENTER, hstmt, fOption, pvParam));

  retcode = SQLGetStmtOption_Internal (hstmt, fOption, pvParam);

  LEAVE_STMT (hstmt,
    trace_SQLGetStmtOption (TRACE_LEAVE, hstmt, fOption, pvParam));
}


#if ODBCVER >= 0x0300
SQLRETURN SQL_API 
SQLGetStmtOptionA (
    SQLHSTMT hstmt,
    SQLUSMALLINT fOption,
    SQLPOINTER pvParam)
{
  ENTER_STMT (hstmt,
    trace_SQLGetStmtOption (TRACE_ENTER, hstmt, fOption, pvParam));

  retcode = SQLGetStmtOption_Internal (hstmt, fOption, pvParam);

  LEAVE_STMT (hstmt,
    trace_SQLGetStmtOption (TRACE_LEAVE, hstmt, fOption, pvParam));
}
#endif


static SQLRETURN 
SQLCancel_Internal (SQLHSTMT hstmt, int stmt_cip)
{
  STMT (pstmt, hstmt);
  HPROC hproc;
  SQLRETURN retcode;


  /* check argument */
  /* check state */

  /* call driver */
  hproc = _iodbcdm_getproc (pstmt->hdbc, en_Cancel);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc, (pstmt->dhstmt));

  /* state transition */
  if (!SQL_SUCCEEDED (retcode))
    {
      return retcode;
    }

  if (stmt_cip == 0) 
    {
      ODBC_LOCK();

      switch (pstmt->state)
        {
        case en_stmt_allocated:
        case en_stmt_prepared:
          break;

        case en_stmt_executed_with_info:
        case en_stmt_executed:
          if (pstmt->prep_state)
	    {
	      pstmt->state = en_stmt_prepared;
	    }
          else
	    {
	      pstmt->state = en_stmt_allocated;
	    }
          break;

        case en_stmt_cursoropen:
        case en_stmt_fetched:
        case en_stmt_xfetched:
          if (pstmt->prep_state)
	    {
	      pstmt->state = en_stmt_prepared;
	    }
          else
	    {
	      pstmt->state = en_stmt_allocated;
	    }
          break;

        case en_stmt_needdata:
        case en_stmt_mustput:
        case en_stmt_canput:
          switch (pstmt->need_on)
	    {
	    case en_ExecDirect:
	      pstmt->state = en_stmt_allocated;
	      break;

	    case en_Execute:
	      pstmt->state = en_stmt_prepared;
	      break;

	    case en_SetPos:
	      pstmt->state = en_stmt_xfetched;
	      break;

	    default:
	      break;
	    }
          pstmt->need_on = en_NullProc;
          break;

        default:
          break;
        }
      
      ODBC_UNLOCK();
    }

  return retcode;
}


SQLRETURN SQL_API 
SQLCancel (SQLHSTMT hstmt)
{
  ENTER_STMT_CANCEL (hstmt,
    trace_SQLCancel (TRACE_ENTER, hstmt));

  retcode = SQLCancel_Internal (hstmt, stmt_cip);

  LEAVE_STMT_CANCEL (hstmt,
    trace_SQLCancel (TRACE_LEAVE, hstmt));
}


void
_iodbcdm_FreeStmtVars(STMT_t *pstmt)
{
  int i;
  VAR_t *p;

  for(i= 0; i < STMT_VARS_MAX; i++)
    {
      p = &pstmt->vars[i];
      if (p->data)
        {
          free(p->data);
          p->data = NULL;
        }
      p->length = 0;
    }
  pstmt->vars_inserted = 0;
}


void
_iodbcdm_FreeStmtParams(STMT_t *pstmt)
{
  PPARM pparm;
  int i, maxpar;

  if (pstmt->st_pparam)
    {
      maxpar = pstmt->st_nparam;
      pparm = pstmt->st_pparam;
      for (i = 0; i < maxpar; i++, pparm++)
        {
          if (pparm->pm_tmp)
            {
              free(pparm->pm_tmp);
              pparm->pm_tmp = NULL;
            }
          if (pparm->pm_tmp_Ind)
            {
              free(pparm->pm_tmp_Ind);
              pparm->pm_tmp_Ind = NULL;
            }
        }
      free (pstmt->st_pparam);
      pstmt->st_pparam = NULL;
    }

  pstmt->st_nparam = 0;
}


void *
_iodbcdm_alloc_var(STMT_t *pstmt, int i, int size)
{
  VAR_t *var;

  if (i > STMT_VARS_MAX - 1)
    return NULL;

  var = &pstmt->vars[i];
  pstmt->vars_inserted = 1;

  if (size == 0)
    {
      MEM_FREE(var->data);
      var->data = NULL;
      var->length = 0;
      return NULL;
    }

  if (var->data == NULL || var->length < size)
    {
      MEM_FREE(var->data);
      var->length = 0;
      if ((var->data = MEM_ALLOC(size)) != NULL)
         var->length = size;
    }

  return var->data;
}


/** DM => DRV**/
void *
_iodbcdm_conv_var(STMT_t *pstmt, int i, void *pData, int pDataLength,
	CONV_DIRECT direct)
{
  VAR_t *var;
  size_t size;
  int count_alloc = 0;
  CONN (pdbc, pstmt->hdbc);
  ENVR (penv, pdbc->henv);
  DM_CONV *conv = &pdbc->conv;
  IODBC_CHARSET m_charset = (conv) ? conv->dm_cp : CP_DEF;
  IODBC_CHARSET d_charset = (conv) ? conv->drv_cp : CP_DEF;

  if (i > STMT_VARS_MAX - 1 || direct == CD_NONE)
    return NULL;

  var = &pstmt->vars[i];
  pstmt->vars_inserted = 1;

  if (pData == NULL)
    {
      MEM_FREE(var->data);
      var->data = NULL;
      var->length = 0;
      return NULL;
    }


  if (pDataLength == SQL_NTS)
    {
      if (direct == CD_W2A || direct == CD_W2W)
        size = DM_WCSLEN(conv, pData);
      else
        size = strlen((char *) pData);
    }
  else
    size = pDataLength;

  if (direct == CD_W2A )
    count_alloc = size * MB_CUR_MAX + 1;
  else
    count_alloc = (size + 1) * DRV_WCHARSIZE_ALLOC(conv);

  if (var->data == NULL || var->length < count_alloc)
    {
      MEM_FREE(var->data);
      var->length = 0;
      if ((var->data = MEM_ALLOC(count_alloc)) != NULL)
        var->length = count_alloc;
    }

  if (direct == CD_A2W)
    {
      size = dm_conv_A2W((char *)pData, pDataLength, var->data,
      		count_alloc - DRV_WCHARSIZE(conv), d_charset);
      if (d_charset == CP_UTF8)
        *(char*)(var->data + size) = 0;
      else
        DRV_SetWCharAt(conv, var->data, size/DRV_WCHARSIZE(conv), 0);
    }
  else if (direct == CD_W2A)
    {
      size = dm_conv_W2A(pData, pDataLength, (char *)var->data, count_alloc - 1,
		m_charset);
      ((char*)var->data)[size] = '\0';
    }
  else /* CD_W2W*/
    {
      size = dm_conv_W2W(pData, pDataLength, (char *)var->data,
      		count_alloc - DRV_WCHARSIZE(conv), m_charset, d_charset);
      if (d_charset == CP_UTF8)
        *(char*)(var->data + size) = 0;
      else
        DRV_SetWCharAt(conv, var->data, size/DRV_WCHARSIZE(conv), 0);
    }

  return (void *) var->data;

}


SQLRETURN
_iodbcdm_BindColumn (STMT_t *pstmt, BIND_t *pbind)
{
  PBLST pblst;
  PBLST prev;

  /*
   *  Initialize the cell
   */
  if ((pblst = (PBLST) calloc (1, sizeof (TBLST))) == NULL)
    {
      return SQL_ERROR;
    }
  pblst->bl_bind = *pbind;

  /*
   *  First on the list?
   */
  if (pstmt->st_pbinding == NULL)
    {
      pstmt->st_pbinding = pblst;
      return SQL_SUCCESS;
    }

  for (prev = pstmt->st_pbinding; ; prev = prev->bl_nextBind)
    {
      /*
       *  Column already on the linked list?
       */
      if (prev->bl_bind.bn_col == pbind->bn_col)
	{
	  prev->bl_bind = *pbind;
	  MEM_FREE (pblst);
	  return SQL_SUCCESS;
	}
      if (prev->bl_nextBind == NULL)
        break;
    }
  prev->bl_nextBind = pblst;

  return SQL_SUCCESS;
}


/*
 *  Remove a binding from the linked list
 */
int
_iodbcdm_UnBindColumn (STMT_t *pstmt, BIND_t *pbind)
{
  PBLST pNewNextBind;
  PBLST *pBindHistory;

  /*
   *  Anything on the list? No? Nothing to do.
   */
  if (pstmt->st_pbinding == NULL)
    return 0;

  for (pBindHistory = &pstmt->st_pbinding; (*pBindHistory);
      pBindHistory = &(*pBindHistory)->bl_nextBind)
    {
      /*
       *  Column already on the linked list?
       */
      if ((*pBindHistory)->bl_bind.bn_col == pbind->bn_col)
	{
	  pNewNextBind = (*pBindHistory)->bl_nextBind;
	  free (*pBindHistory);
	  (*pBindHistory) = pNewNextBind;
	  return 0;
	}
    }
  return 0;
}


/*
 *  Remove all bindings
 */
void
_iodbcdm_RemoveBind (STMT_t *pstmt)
{
  BIND_t *col;
  PBLST pblst, pnext;

  if (pstmt->st_pbinding)
    {
      for (pblst = pstmt->st_pbinding; pblst; pblst = pnext)
	{
          col = &(pblst->bl_bind);

          MEM_FREE(col->bn_tmp);
          MEM_FREE(col->bn_tmp_Ind);

	  pnext = pblst->bl_nextBind;
	  free (pblst);
	}
      pstmt->st_pbinding = NULL;
    }
}



static void
_iodbcdm_bindConv_A2W_d2m(char *data, SQLLEN *pInd, UDWORD size, DM_CONV *conv)
{
  if (*pInd != SQL_NULL_DATA)
    {
      int count = 0;
      char *buf = calloc(size + 1, sizeof(char));

      if (buf != NULL)
        {
          memcpy(buf, data, size);
          dm_StrCopyOut2_A2W_d2m (conv, (SQLCHAR *)buf, data, size, NULL, &count);
          MEM_FREE(buf);
        }

      if (pInd && *pInd != SQL_NTS)
        *pInd =(SQLLEN)count;
    }
}

static void
_iodbcdm_bindConv_W2A_m2d(char *data, SQLLEN *pInd, UDWORD size, DM_CONV *conv)
{
  if (*pInd != SQL_NULL_DATA)
    {
      int count = 0;
      char *buf = calloc(size + 1, sizeof(char));

      if (buf != NULL)
        {
          memcpy(buf, data, size);
          dm_StrCopyOut2_W2A_m2d (conv, buf, (SQLCHAR *)data, size, NULL, &count);
          MEM_FREE(buf);
        }

      if (pInd && *pInd != SQL_NTS)
        *pInd =(SQLLEN)count;
    }
}


static void
_iodbcdm_bindConv_W2W_d2m(char *data, SQLLEN *pInd, UDWORD size, DM_CONV *conv)
{
  if (*pInd != SQL_NULL_DATA)
    {
      int count = 0;
      char *buf = calloc(size + WCHAR_MAXSIZE, sizeof(char));

      if (buf != NULL)
        {
          memcpy(buf, data, size);
          dm_StrCopyOut2_W2W_d2m (conv, buf, data, size, NULL, &count);
          MEM_FREE(buf);
        }

      if (pInd && *pInd != SQL_NTS)
        *pInd =(SQLLEN)count;
    }
}

static void
_iodbcdm_bindConv_W2W_m2d(char *data, SQLLEN *pInd, UDWORD size, DM_CONV *conv)
{
  if (*pInd != SQL_NULL_DATA)
    {
      int count = 0;
      char *buf = calloc(size + WCHAR_MAXSIZE, sizeof(char));

      if (buf != NULL)
        {
          memcpy(buf, data, size);
          dm_StrCopyOut2_W2W_m2d (conv, buf, data, size, NULL, &count);
          MEM_FREE(buf);
        }

      if (pInd && *pInd != SQL_NTS)
        *pInd =(SQLLEN)count;
    }
}


static SQLLEN
GetColSize (BIND_t *col)
{
  SQLLEN elementSize;


  if (col->bn_type == SQL_C_CHAR
      || col->bn_type == SQL_C_BINARY
      || col->bn_type == SQL_C_WCHAR)
    {
      elementSize = col->bn_size;
    }
  else				/* fixed length types */
    {
      elementSize = _iodbcdm_OdbcCTypeSize(col->bn_type);
    }

  return elementSize;
}


/** drv => dm **/
void
_iodbcdm_ConvBindData (STMT_t *pstmt)
{
  PBLST ptr;
  BIND_t *col;
  UDWORD i, size, row_size;
  CONN (pdbc, pstmt->hdbc);
  ENVR (penv, pdbc->henv);
  DM_CONV *conv = &pdbc->conv;
  IODBC_CHARSET m_charset = CP_DEF;
  IODBC_CHARSET d_charset = CP_DEF;
  SQLULEN cRows = pstmt->rowset_size;
  SQLUINTEGER bindOffset = pstmt->row_bind_offset;

  /*
   *  Anything on the list? No? Nothing to do.
   */
  if (pstmt->st_pbinding == NULL)
    return ;

  if (cRows == 0)
    cRows = 1;

  if (conv)
    {
      m_charset = conv ? conv->dm_cp: CP_DEF;
      d_charset = conv ? conv->drv_cp: CP_DEF;
    }

  for (ptr = pstmt->st_pbinding; ptr; ptr = ptr->bl_nextBind)
    {
      col = &(ptr->bl_bind);

      for (i = 0; i < cRows; i++)
        {
          void *val_dm = NULL;
          void *val_drv = NULL;
          SQLLEN *pInd_dm = NULL;
          SQLLEN *pInd_drv = NULL;
          SQLLEN len;
          size = GetColSize(col);

          if (pstmt->row_bind_type == SQL_BIND_BY_COLUMN)
            {
              if (col->bn_pInd)
                pInd_dm = &((SQLLEN*)((char*)col->bn_pInd + bindOffset))[i];
              val_dm = (char *) col->bn_data + i * size + bindOffset;

              if (col->rebinded)
                {
                  pInd_drv = &((SQLLEN*)col->bn_conv_pInd)[i];
                  val_drv = (char *) col->bn_conv_data + i * col->bn_conv_size;
                }
            }
          else  /* row-wise binding */
            {
              row_size = pstmt->row_bind_type;
              if (col->bn_pInd)
                pInd_dm = (SQLLEN *) ((char *) col->bn_pInd
	            + i * row_size + bindOffset);

              val_dm = (char *) col->bn_data + i * row_size + bindOffset;

              if (col->rebinded)
                {
                  pInd_drv = (SQLLEN *) ((char *) col->bn_conv_pInd
	              + i * pstmt->conv_row_bind_type);
                  val_drv = (char *) col->bn_conv_data
                      + i * pstmt->conv_row_bind_type;
                }
            }


          if (col->rebinded)
            {
              if (*pInd_drv != SQL_NULL_DATA)
                {
                  if (col->bn_type==SQL_C_WCHAR)
                    {
                      len = (SQLLEN)*pInd_drv;

                      len = dm_conv_W2W(val_drv, len, val_dm, size,
				d_charset, m_charset);
                      if (m_charset == CP_UTF8)
                        *(char*)(val_dm + len) = 0;
                      else
                        DM_SetWCharAt(conv, val_dm, len/DM_WCHARSIZE(conv), 0);

                      if (pInd_dm)
                        *pInd_dm = (*pInd_drv != SQL_NTS)? len: SQL_NTS;
                    }
                  else
                    {
                      memcpy(val_dm, val_drv, size);
                      if (pInd_dm)
                        *pInd_dm = *pInd_drv;
                    }
                }
              else
                {
                  if (pInd_dm)
                    *pInd_dm = *pInd_drv;
                }
            }
          else
            {
              if (col->direct == CD_A2W)
                _iodbcdm_bindConv_A2W_d2m(val_dm, pInd_dm, size, conv);
              else if (col->direct == CD_W2W)
                _iodbcdm_bindConv_W2W_d2m(val_dm, pInd_dm, size, conv);
            }
        }
    }
}



/** dm => drv **/
void
_iodbcdm_ConvBindData_m2d (STMT_t *pstmt)
{
  PBLST ptr;
  BIND_t *col;
  UDWORD i, size, row_size;
  CONN (pdbc, pstmt->hdbc);
  ENVR (penv, pdbc->henv);
  DM_CONV *conv = &pdbc->conv;
  IODBC_CHARSET m_charset = CP_DEF;
  IODBC_CHARSET d_charset = CP_DEF;
  SQLULEN cRows = pstmt->rowset_size;
  SQLUINTEGER bindOffset = pstmt->row_bind_offset;

  /*
   *  Anything on the list? No? Nothing to do.
   */
  if (pstmt->st_pbinding == NULL)
    return ;

  if (cRows == 0)
    cRows = 1;

  if (conv)
    {
      m_charset = conv ? conv->dm_cp: CP_DEF;
      d_charset = conv ? conv->drv_cp: CP_DEF;
    }

  for (ptr = pstmt->st_pbinding; ptr; ptr = ptr->bl_nextBind)
    {
      col = &(ptr->bl_bind);

      for (i = 0; i < cRows; i++)
        {
          void *val_dm = NULL;
          void *val_drv = NULL;
          SQLLEN *pInd_dm = NULL;
          SQLLEN *pInd_drv = NULL;
          SQLLEN len;
          size = GetColSize(col);

          if (pstmt->row_bind_type == SQL_BIND_BY_COLUMN)
            {
              if (col->bn_pInd)
                pInd_dm = &((SQLLEN*)((char*)col->bn_pInd + bindOffset))[i];
              val_dm = (char *) col->bn_data + i * size + bindOffset;

              if (col->rebinded)
                {
                  pInd_drv = &((SQLLEN*)col->bn_conv_pInd)[i];
                  val_drv = (char *) col->bn_conv_data + i * col->bn_conv_size;
                }
            }
          else  /* row-wise binding */
            {
              row_size = pstmt->row_bind_type;
              if (col->bn_pInd)
                pInd_dm = (SQLLEN *) ((char *) col->bn_pInd
	            + i * row_size + bindOffset);

              val_dm = (char *) col->bn_data + i * row_size + bindOffset;

              if (col->rebinded)
                {
                  pInd_drv = (SQLLEN *) ((char *) col->bn_conv_pInd
	              + i * pstmt->conv_row_bind_type);
                  val_drv = (char *) col->bn_conv_data
                      + i * pstmt->conv_row_bind_type;
                }
            }


          if (col->rebinded)
            {
              if (*pInd_dm != SQL_NULL_DATA)
                {
                  if (col->bn_type==SQL_C_WCHAR)
                    {
                      len = (SQLLEN)*pInd_dm;
                      size = size/DM_WCHARSIZE(conv)*DRV_WCHARSIZE(conv);

                      len = dm_conv_W2W(val_dm, len, val_drv, size,
				m_charset, d_charset);
                      if (d_charset == CP_UTF8)
                        *(char*)(val_dm + len) = 0;
                      else
                        DRV_SetWCharAt(conv, val_drv, len/DRV_WCHARSIZE(conv), 0);

                      if (pInd_dm)
                        *pInd_drv = (*pInd_dm != SQL_NTS)? len: SQL_NTS;
                    }
                  else
                    {
                      memcpy(val_drv, val_dm, size);
                      if (pInd_dm)
                        *pInd_drv = *pInd_dm;
                    }
                }
              else
                {
                  if (pInd_dm)
                    *pInd_drv = *pInd_dm;
                }
            }
          else /* convert on place */
            {
              if (col->direct == CD_A2W)
                _iodbcdm_bindConv_W2A_m2d(val_dm, pInd_dm, size, conv);
              else if (col->direct == CD_W2W)
                _iodbcdm_bindConv_W2W_m2d(val_dm, pInd_dm, size, conv);
            }
        }
    }
}



static SQLRETURN
_ReBindCol (STMT_t *pstmt, BIND_t *col)
{
  HPROC hproc = SQL_NULL_HPROC;
  SQLRETURN retcode = SQL_SUCCESS;

  hproc = _iodbcdm_getproc (pstmt->hdbc, en_BindCol);
  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);
      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc,
      (pstmt->dhstmt, col->bn_col, col->bn_type, col->bn_conv_data,
       col->bn_conv_size, col->bn_conv_pInd));

  return retcode;
}


SQLRETURN
_iodbcdm_FixColBindData (STMT_t *pstmt)
{
  PBLST ptr;
  BIND_t *col;
  CONN (pdbc, pstmt->hdbc);
  ENVR (penv, pdbc->henv);
  SQLUINTEGER odbc_ver = ((GENV_t *) pdbc->genv)->odbc_ver;
  SQLUINTEGER dodbc_ver = ((ENV_t *) pdbc->henv)->dodbc_ver;
  DM_CONV *conv = &pdbc->conv;
  IODBC_CHARSET m_charset = CP_DEF;
  IODBC_CHARSET d_charset = CP_DEF;
  BOOL needRebind = FALSE;
  SQLLEN sz_mult = 1;
  SQLULEN cRows = pstmt->rowset_size;
  HPROC hproc2 = SQL_NULL_HPROC;
  HPROC hproc3 = SQL_NULL_HPROC;
  SQLRETURN retcode = SQL_SUCCESS;


  /*
   *  Anything on the list? No? Nothing to do.
   */
  if (pstmt->st_pbinding == NULL)
    return SQL_SUCCESS;

  if (cRows == 0)
    cRows = 1;

  if (conv)
    {
      m_charset = conv ? conv->dm_cp: CP_DEF;
      d_charset = conv ? conv->drv_cp: CP_DEF;

      if (m_charset==CP_UTF16 && d_charset==CP_UCS4)
        sz_mult = 2;
      else if (m_charset==CP_UTF8 && d_charset==CP_UCS4)
        sz_mult = 4;
      else if (m_charset==CP_UTF8 && d_charset==CP_UTF16)
        sz_mult = 2;
      else
        sz_mult = 1;
    }

  if (penv->unicode_driver)
    {
      if (conv==NULL || (conv && conv->dm_cp == conv->drv_cp))
        {
          needRebind = FALSE;
        }
      else if ((m_charset==CP_UTF16 && d_charset==CP_UCS4)
             ||(m_charset==CP_UTF8 && d_charset==CP_UTF16)
             ||(m_charset==CP_UTF8 && d_charset==CP_UCS4))
        {
          /* check if we need rebind columns */
          for (ptr = pstmt->st_pbinding; ptr; ptr = ptr->bl_nextBind)
            {
              col = &(ptr->bl_bind);
              if (col->bn_type==SQL_C_WCHAR && col->direct==CD_W2W)
                {
                  needRebind = TRUE;
                  break;
                }
            }
        }
    }

  if (needRebind)
    {
      SQLULEN new_size = 0;
      void *buf = NULL;

      if (pstmt->row_bind_type == SQL_BIND_BY_COLUMN)
        {
          for (ptr = pstmt->st_pbinding; ptr; ptr = ptr->bl_nextBind)
            {
              col = &(ptr->bl_bind);

              XFREE(col->bn_tmp);
              XFREE(col->bn_tmp_Ind);

              col->bn_conv_size = GetColSize(col);
              if (col->bn_type==SQL_C_WCHAR)
                col->bn_conv_size *= sz_mult;

              new_size = cRows * col->bn_conv_size;
              buf = calloc(new_size, sizeof(char));
              if (!buf)
                {
                  PUSHSQLERR (pstmt->herr, en_HY001);
                  return SQL_ERROR;
                }
              col->bn_tmp = col->bn_conv_data = buf;

              buf = calloc(cRows, sizeof(SQLLEN));
              if (!buf)
                {
                  PUSHSQLERR (pstmt->herr, en_HY001);
                  return SQL_ERROR;
                }
              col->bn_tmp_Ind = col->bn_conv_pInd = (SQLLEN*)buf;

              retcode = _ReBindCol(pstmt, col);
              if (!SQL_SUCCEEDED (retcode))
                return retcode;
              col->rebinded = TRUE;

            }

        }
      else  /* row-wise bind*/
        {

          /* calc new_size */
          for (ptr = pstmt->st_pbinding; ptr; ptr = ptr->bl_nextBind)
            {
              col = &(ptr->bl_bind);
              new_size += sizeof(SQLLEN);

              col->bn_conv_size = GetColSize(col);
              if (col->bn_type==SQL_C_WCHAR)
                col->bn_conv_size *= sz_mult;

              new_size += col->bn_conv_size;
            }

          if (pstmt->rows_buf)
            {
              free(pstmt->rows_buf);
              pstmt->rows_buf = NULL;
            }

          buf = calloc((new_size*cRows), sizeof(char));
          if (!buf)
            {
              PUSHSQLERR (pstmt->herr, en_HY001);
              return SQL_ERROR;
            }
          pstmt->rows_buf = buf;
          pstmt->conv_row_bind_type = new_size;

          /***** Set Bind_type in driver to new size *****/
          if (dodbc_ver == SQL_OV_ODBC3)
            {
              CALL_UDRIVER(pstmt->hdbc, pstmt, retcode, hproc3,
                penv->unicode_driver, en_SetStmtAttr, (pstmt->dhstmt,
		(SQLINTEGER)SQL_ATTR_ROW_BIND_TYPE,
                (SQLPOINTER)new_size, 0));
              if (hproc3 == SQL_NULL_HPROC)
                {
	          PUSHSQLERR (pstmt->herr, en_IM001);
	          return SQL_ERROR;
                }
            }
          else
            {
              hproc2 = _iodbcdm_getproc (pstmt->hdbc, en_SetStmtOption);
              if (hproc2 == SQL_NULL_HPROC)
                hproc2 = _iodbcdm_getproc (pstmt->hdbc, en_SetStmtOptionA);

              if (hproc2 == SQL_NULL_HPROC)
                {
	          PUSHSQLERR (pstmt->herr, en_IM001);
	          return SQL_ERROR;
                }

              CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc2,
	          (pstmt->dhstmt,
	           (SQLUSMALLINT)SQL_BIND_TYPE,
	           (SQLUINTEGER)new_size));

            }
          if (!SQL_SUCCEEDED (retcode))
            return retcode;

          /* rebind parameters */
          buf = pstmt->rows_buf;
          for (ptr = pstmt->st_pbinding; ptr; ptr = ptr->bl_nextBind)
            {
              col = &(ptr->bl_bind);

              col->bn_conv_data = buf;
              buf += col->bn_conv_size;

              col->bn_conv_pInd = (SQLLEN*)buf;
              buf += sizeof(SQLLEN);

              retcode = _ReBindCol(pstmt, col);
              if (!SQL_SUCCEEDED (retcode))
                return retcode;
              col->rebinded = TRUE;
            }
        }


      /***** Set ColSet offset *****/
      if (dodbc_ver == SQL_OV_ODBC3)
        {
          CALL_UDRIVER(pstmt->hdbc, pstmt, retcode, hproc3,
            penv->unicode_driver, en_SetStmtAttr, (pstmt->dhstmt,
	    (SQLINTEGER)SQL_ATTR_ROW_BIND_OFFSET_PTR,
            (SQLPOINTER)0, 0));
          if (hproc3 == SQL_NULL_HPROC)
            {
	      PUSHSQLERR (pstmt->herr, en_IM001);
	      return SQL_ERROR;
            }
        }
#if 0
      if (!SQL_SUCCEEDED (retcode))
        return retcode;
#endif
    }

  return SQL_SUCCESS;
}
