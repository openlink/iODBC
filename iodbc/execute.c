/*
 *  execute.c
 *
 *  $Id$
 *
 *  Invoke a query
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

#include <iodbc.h>

#include <sql.h>
#include <sqlext.h>

#include <dlproc.h>

#include <herr.h>
#include <henv.h>
#include <hdbc.h>
#include <hstmt.h>

#include <itrace.h>

static void
do_cursoropen (STMT_t FAR * pstmt)
{
  SQLRETURN retcode;
  SWORD ncol;

  pstmt->state = en_stmt_executed;

  retcode = _iodbcdm_NumResultCols ((SQLHSTMT) pstmt, &ncol);

  if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
    {
      if (ncol)
	{
	  pstmt->state = en_stmt_cursoropen;
	  pstmt->cursor_state = en_stmt_cursor_opened;
	}
      else
	{
	  pstmt->state = en_stmt_executed;
	  pstmt->cursor_state = en_stmt_cursor_no;
	}
    }
}


SQLRETURN SQL_API
SQLExecute (SQLHSTMT hstmt)
{
  STMT (pstmt, hstmt);
  HPROC hproc = SQL_NULL_HPROC;
  SQLRETURN retcode;

  int sqlstat = en_00000;

  ENTER_STMT (pstmt);

  /* check state */
  if (pstmt->asyn_on == en_NullProc)
    {
      switch (pstmt->state)
	{
	case en_stmt_allocated:
	  sqlstat = en_S1010;
	  break;

	case en_stmt_executed:
	  if (!pstmt->prep_state)
	    {
	      sqlstat = en_S1010;
	    }
	  break;

	case en_stmt_cursoropen:
	  if (!pstmt->prep_state)
	    {
	      sqlstat = en_S1010;
	    }
	  break;

	case en_stmt_fetched:
	case en_stmt_xfetched:
	  if (!pstmt->prep_state)
	    {
	      sqlstat = en_S1010;
	    }
	  else
	    {
	      sqlstat = en_24000;
	    }
	  break;

	case en_stmt_needdata:
	case en_stmt_mustput:
	case en_stmt_canput:
	  sqlstat = en_S1010;
	  break;

	default:
	  break;
	}
    }
  else if (pstmt->asyn_on != en_Execute)
    {
      sqlstat = en_S1010;
    }

  if (sqlstat == en_00000)
    {
      hproc = _iodbcdm_getproc (pstmt->hdbc, en_Execute);

      if (hproc == SQL_NULL_HPROC)
	{
	  sqlstat = en_IM001;
	}
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      LEAVE_STMT (pstmt, SQL_ERROR);
    }

  CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc, en_Execute,
      (pstmt->dhstmt));

  /* stmt state transition */
  if (pstmt->asyn_on == en_Execute)
    {
      switch (retcode)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
	case SQL_NEED_DATA:
	case SQL_ERROR:
	  pstmt->asyn_on = en_NullProc;
	  break;

	case SQL_STILL_EXECUTING:
	default:
	  LEAVE_STMT (pstmt, retcode);
	}
    }

  switch (pstmt->state)
    {
    case en_stmt_prepared:
      switch (retcode)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
	  do_cursoropen (pstmt);
	  break;

	case SQL_NEED_DATA:
	  pstmt->state = en_stmt_needdata;
	  pstmt->need_on = en_Execute;
	  break;

	case SQL_STILL_EXECUTING:
	  pstmt->asyn_on = en_Execute;
	  break;

	default:
	  break;
	}
      break;

    case en_stmt_executed:
      switch (retcode)
	{
	case SQL_ERROR:
	  pstmt->state = en_stmt_allocated;
	  pstmt->cursor_state = en_stmt_cursor_no;
	  pstmt->prep_state = 0;
	  break;

	case SQL_NEED_DATA:
	  pstmt->state = en_stmt_needdata;
	  pstmt->need_on = en_Execute;
	  break;

	case SQL_STILL_EXECUTING:
	  pstmt->asyn_on = en_Execute;
	  break;

	default:
	  break;
	}
      break;

    default:
      break;
    }

  LEAVE_STMT (pstmt, retcode);
}


SQLRETURN SQL_API
SQLExecDirect (SQLHSTMT hstmt, SQLCHAR FAR * szSqlStr, SQLINTEGER cbSqlStr)
{
  STMT (pstmt, hstmt);
  HPROC hproc = SQL_NULL_HPROC;

  int sqlstat = en_00000;
  SQLRETURN retcode = SQL_SUCCESS;

  ENTER_STMT (pstmt);

  /* check arguments */
  if (szSqlStr == NULL)
    {
      sqlstat = en_S1009;
    }
  else if (cbSqlStr < 0 && cbSqlStr != SQL_NTS)
    {
      sqlstat = en_S1090;
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      LEAVE_STMT (pstmt, SQL_ERROR);
    }

  /* check state */
  if (pstmt->asyn_on == en_NullProc)
    {
      switch (pstmt->state)
	{
	case en_stmt_fetched:
	case en_stmt_xfetched:
	  sqlstat = en_24000;
	  break;

	case en_stmt_needdata:
	case en_stmt_mustput:
	case en_stmt_canput:
	  sqlstat = en_S1010;
	  break;

	default:
	  break;
	}
    }
  else if (pstmt->asyn_on != en_ExecDirect)
    {
      sqlstat = en_S1010;
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pstmt->herr, sqlstat);

      LEAVE_STMT (pstmt, SQL_ERROR);
    }

  hproc = _iodbcdm_getproc (pstmt->hdbc, en_ExecDirect);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);

      LEAVE_STMT (pstmt, SQL_ERROR);
    }

  CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc, en_ExecDirect,
      (pstmt->dhstmt, szSqlStr, cbSqlStr));

  /* stmt state transition */
  if (pstmt->asyn_on == en_ExecDirect)
    {
      switch (retcode)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
	case SQL_NEED_DATA:
	case SQL_ERROR:
	  pstmt->asyn_on = en_NullProc;
	  break;

	case SQL_STILL_EXECUTING:
	default:
	  LEAVE_STMT (pstmt, retcode);
	}
    }

  if (pstmt->state <= en_stmt_executed)
    {
      switch (retcode)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
	  do_cursoropen (pstmt);
	  break;

	case SQL_NEED_DATA:
	  pstmt->state = en_stmt_needdata;
	  pstmt->need_on = en_ExecDirect;
	  break;

	case SQL_STILL_EXECUTING:
	  pstmt->asyn_on = en_ExecDirect;
	  break;

	case SQL_ERROR:
	  pstmt->state = en_stmt_allocated;
	  pstmt->cursor_state = en_stmt_cursor_no;
	  pstmt->prep_state = 0;
	  break;

	default:
	  break;
	}
    }

  LEAVE_STMT (pstmt, retcode);
}


SQLRETURN SQL_API
SQLPutData (SQLHSTMT hstmt, SQLPOINTER rgbValue, SQLINTEGER cbValue)
{
  STMT (pstmt, hstmt);
  HPROC hproc;
  SQLRETURN retcode;

  ENTER_STMT (pstmt);

  /* check argument value */
  if (rgbValue == NULL &&
      (cbValue != SQL_DEFAULT_PARAM && cbValue != SQL_NULL_DATA))
    {
      PUSHSQLERR (pstmt->herr, en_S1009);

      LEAVE_STMT (pstmt, SQL_ERROR);
    }

  /* check state */
  if (pstmt->asyn_on == en_NullProc)
    {
      if (pstmt->state <= en_stmt_xfetched)
	{
	  PUSHSQLERR (pstmt->herr, en_S1010);

	  LEAVE_STMT (pstmt, SQL_ERROR);
	}
    }
  else if (pstmt->asyn_on != en_PutData)
    {
      PUSHSQLERR (pstmt->herr, en_S1010);

      LEAVE_STMT (pstmt, SQL_ERROR);
    }

  /* call driver */
  hproc = _iodbcdm_getproc (pstmt->hdbc, en_PutData);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);

      LEAVE_STMT (pstmt, SQL_ERROR);
    }

  CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc, en_PutData,
      (pstmt->dhstmt, rgbValue, cbValue));

  /* state transition */
  if (pstmt->asyn_on == en_PutData)
    {
      switch (retcode)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
	case SQL_ERROR:
	  pstmt->asyn_on = en_NullProc;
	  break;

	case SQL_STILL_EXECUTING:
	default:
	  LEAVE_STMT (pstmt, retcode);
	}
    }

  /* must in mustput or canput states */
  switch (retcode)
    {
    case SQL_SUCCESS:
    case SQL_SUCCESS_WITH_INFO:
      pstmt->state = en_stmt_canput;
      break;

    case SQL_ERROR:
      switch (pstmt->need_on)
	{
	case en_ExecDirect:
	  pstmt->state = en_stmt_allocated;
	  pstmt->need_on = en_NullProc;
	  break;

	case en_Execute:
	  if (pstmt->prep_state)
	    {
	      pstmt->state = en_stmt_prepared;
	      pstmt->need_on = en_NullProc;
	    }
	  break;

	case en_SetPos:
	  /* Is this possible ???? */
	  pstmt->state = en_stmt_xfetched;
	  break;

	default:
	  break;
	}
      break;

    case SQL_STILL_EXECUTING:
      pstmt->asyn_on = en_PutData;
      break;

    default:
      break;
    }

  LEAVE_STMT (pstmt, retcode);
}


SQLRETURN SQL_API
SQLParamData (SQLHSTMT hstmt, SQLPOINTER FAR * prgbValue)
{
  STMT (pstmt, hstmt);
  HPROC hproc;
  SQLRETURN retcode;

  ENTER_STMT (pstmt);

  /* check argument */

  /* check state */
  if (pstmt->asyn_on == en_NullProc)
    {
      if (pstmt->state <= en_stmt_xfetched)
	{
	  PUSHSQLERR (pstmt->herr, en_S1010);

	  LEAVE_STMT (pstmt, SQL_ERROR);
	}
    }
  else if (pstmt->asyn_on != en_ParamData)
    {
      PUSHSQLERR (pstmt->herr, en_S1010);

      LEAVE_STMT (pstmt, SQL_ERROR);
    }

  /* call driver */
  hproc = _iodbcdm_getproc (pstmt->hdbc, en_ParamData);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);

      LEAVE_STMT (pstmt, SQL_ERROR);
    }

  CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc, en_ParamData,
      (pstmt->dhstmt, prgbValue));

  /* state transition */
  if (pstmt->asyn_on == en_ParamData)
    {
      switch (retcode)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
	case SQL_ERROR:
	  pstmt->asyn_on = en_NullProc;
	  break;

	case SQL_STILL_EXECUTING:
	default:
	  LEAVE_STMT (pstmt, retcode);
	}
    }

  if (pstmt->state < en_stmt_needdata)
    {
      LEAVE_STMT (pstmt, retcode);
    }

  switch (retcode)
    {
    case SQL_ERROR:
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
	  pstmt->cursor_state = en_stmt_cursor_xfetched;
	  break;

	default:
	  break;
	}
      pstmt->need_on = en_NullProc;
      break;

    case SQL_SUCCESS:
    case SQL_SUCCESS_WITH_INFO:
      switch (pstmt->state)
	{
	case en_stmt_needdata:
	  pstmt->state = en_stmt_mustput;
	  break;

	case en_stmt_canput:
	  switch (pstmt->need_on)
	    {
	    case en_SetPos:
	      pstmt->state = en_stmt_xfetched;
	      pstmt->cursor_state = en_stmt_cursor_xfetched;
	      break;

	    case en_ExecDirect:
	    case en_Execute:
	      do_cursoropen (pstmt);
	      break;

	    default:
	      break;
	    }
	  break;

	default:
	  break;
	}
      pstmt->need_on = en_NullProc;
      break;

    case SQL_NEED_DATA:
      pstmt->state = en_stmt_mustput;
      break;

    default:
      break;
    }

  LEAVE_STMT (pstmt, retcode);
}


SQLRETURN SQL_API
SQLNumParams (SQLHSTMT hstmt, SQLSMALLINT FAR * pcpar)
{
  STMT (pstmt, hstmt);
  HPROC hproc;
  SQLRETURN retcode;

  ENTER_STMT (pstmt);

  /* check argument */
  if (!pcpar)
    {
      LEAVE_STMT (pstmt, SQL_SUCCESS);
    }

  /* check state */
  if (pstmt->asyn_on == en_NullProc)
    {
      switch (pstmt->state)
	{
	case en_stmt_allocated:
	case en_stmt_needdata:
	case en_stmt_mustput:
	case en_stmt_canput:
	  PUSHSQLERR (pstmt->herr, en_S1010);
	  LEAVE_STMT (pstmt, SQL_ERROR);

	default:
	  break;
	}
    }
  else if (pstmt->asyn_on != en_NumParams)
    {
      PUSHSQLERR (pstmt->herr, en_S1010);

      LEAVE_STMT (pstmt, SQL_ERROR);
    }

  /* call driver */
  hproc = _iodbcdm_getproc (pstmt->hdbc, en_NumParams);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);

      LEAVE_STMT (pstmt, SQL_ERROR);
    }

  CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc, en_NumParams,
      (pstmt->dhstmt, pcpar));

  /* state transition */
  if (pstmt->asyn_on == en_NumParams)
    {
      switch (retcode)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
	case SQL_ERROR:
	  break;

	default:
	  LEAVE_STMT (pstmt, retcode);
	}
    }

  if (retcode == SQL_STILL_EXECUTING)
    {
      pstmt->asyn_on = en_NumParams;
    }

  LEAVE_STMT (pstmt, retcode);
}


SQLRETURN SQL_API
SQLDescribeParam (SQLHSTMT hstmt,
    SQLUSMALLINT ipar,
    SQLSMALLINT FAR * pfSqlType,
    SQLUINTEGER FAR * pcbColDef,
    SQLSMALLINT FAR * pibScale, SQLSMALLINT FAR * pfNullable)
{
  STMT (pstmt, hstmt);
  HPROC hproc;
  SQLRETURN retcode;

  ENTER_STMT (pstmt);

  /* check argument */
  if (ipar == 0)
    {
      PUSHSQLERR (pstmt->herr, en_S1093);

      LEAVE_STMT (pstmt, SQL_ERROR);
    }

  /* check state */
  if (pstmt->asyn_on == en_NullProc)
    {
      switch (pstmt->state)
	{
	case en_stmt_allocated:
	case en_stmt_needdata:
	case en_stmt_mustput:
	case en_stmt_canput:
	  PUSHSQLERR (pstmt->herr, en_S1010);
	  LEAVE_STMT (pstmt, SQL_ERROR);

	default:
	  break;
	}
    }
  else if (pstmt->asyn_on != en_DescribeParam)
    {
      PUSHSQLERR (pstmt->herr, en_S1010);

      LEAVE_STMT (pstmt, SQL_ERROR);
    }

  /* call driver */
  hproc = _iodbcdm_getproc (pstmt->hdbc, en_DescribeParam);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);

      LEAVE_STMT (pstmt, SQL_ERROR);
    }

  CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc, en_DescribeParam,
      (pstmt->dhstmt, ipar, pfSqlType, pcbColDef, pibScale, pfNullable));

  /* state transition */
  if (pstmt->asyn_on == en_DescribeParam)
    {
      switch (retcode)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
	case SQL_ERROR:
	  break;

	default:
	  LEAVE_STMT (pstmt, retcode);
	}
    }

  if (retcode == SQL_STILL_EXECUTING)
    {
      pstmt->asyn_on = en_DescribeParam;
    }

  LEAVE_STMT (pstmt, retcode);
}
