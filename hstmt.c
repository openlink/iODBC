/*
 *  hstmt.c
 *
 *  $Id$
 *
 *  Query statement object management functions
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
#if (ODBCVER >= 0x0300)
#include <hdesc.h>
#endif
#include <henv.h>
#include <hdbc.h>
#include <hstmt.h>

#include <itrace.h>

#if (ODBCVER >= 0x300)
static const SQLINTEGER desc_attrs[4] = 
{
  SQL_ATTR_APP_ROW_DESC,
  SQL_ATTR_APP_PARAM_DESC,
  SQL_ATTR_IMP_ROW_DESC,
  SQL_ATTR_IMP_PARAM_DESC
};
#endif


SQLRETURN SQL_API 
SQLAllocStmt (
    SQLHDBC hdbc,
    SQLHSTMT FAR * phstmt)
{
  CONN (pdbc, hdbc);
  STMT_t FAR *pstmt = NULL;
  HPROC hproc = SQL_NULL_HPROC;
  SQLRETURN retcode = SQL_SUCCESS;

  if (!IS_VALID_HDBC (pdbc))
    {
      return SQL_INVALID_HANDLE;
    }
  CLEAR_ERRORS(pdbc);

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

  pstmt = (STMT_t FAR *) MEM_ALLOC (sizeof (STMT_t));

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

  /* call driver's function */

#if (ODBCVER >= 0x0300)
  pstmt->row_array_size = 1;
  pstmt->rowset_size = 1;
  pstmt->fetch_bookmark_ptr = NULL;
  pstmt->params_processed_ptr = NULL;
  pstmt->paramset_size = 0;
  pstmt->rows_fetched_ptr = NULL;
  if (((ENV_t FAR *)((DBC_t FAR *)pstmt->hdbc)->henv)->dodbc_ver == SQL_OV_ODBC2 && 
      ((GENV_t FAR *)((DBC_t FAR *)pstmt->hdbc)->genv)->odbc_ver == SQL_OV_ODBC3)
    { /* if it's a odbc3 app calling odbc2 driver */
      pstmt->row_status_ptr = MEM_ALLOC(sizeof(SQLUINTEGER) * pstmt->row_array_size);
      if (!pstmt->row_status_ptr)
	{
	  PUSHSQLERR(pstmt->herr, en_HY001);
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

  hproc = _iodbcdm_getproc (pdbc, en_AllocHandle);

  if (hproc)
    {
      CALL_DRIVER (pstmt->hdbc, pdbc, retcode, hproc, en_AllocHandle, 
        (SQL_HANDLE_STMT, pdbc->dhdbc, &(pstmt->dhstmt)));
    }
  else
#endif

    {
      hproc = _iodbcdm_getproc (pdbc, en_AllocStmt);

      if (hproc == SQL_NULL_HPROC)
	{
	  PUSHSQLERR (pstmt->herr, en_IM001);
	  *phstmt = SQL_NULL_HSTMT;
	  pstmt->type = 0;
	  MEM_FREE (pstmt);

	  return SQL_ERROR;
	}

      CALL_DRIVER (hdbc, pdbc, retcode, hproc, en_AllocStmt, 
        (pdbc->dhdbc, &(pstmt->dhstmt)));
    }

  if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
    {
      *phstmt = SQL_NULL_HSTMT;
      pstmt->type = 0;
      MEM_FREE (pstmt);

      return retcode;
    }

#if (ODBCVER >= 0x0300)  
  /* get the descriptors */
  memset(&pstmt->imp_desc, 0, sizeof(pstmt->imp_desc));
  memset(&pstmt->desc, 0, sizeof(pstmt->desc));

  if (((ENV_t *)pdbc->henv)->dodbc_ver == SQL_OV_ODBC2)
    { /* this is an ODBC2 driver - so alloc dummy implicit desc handles  (dhdesc = NULL) */
      int i, i1;

      for (i = 0; i < 4; i++)
	{
	  pstmt->imp_desc[i] = (DESC_t FAR *) MEM_ALLOC (sizeof (DESC_t));
	  memset(pstmt->imp_desc[i], 0, sizeof(DESC_t));
	  if (pstmt->imp_desc[i] == NULL)
	    {
	      for (i1 = 0; i1 < i; i++)
		{
		  pstmt->imp_desc[i1]->type = 0;
		  MEM_FREE(pstmt->imp_desc[i1]);
		}
	      PUSHSQLERR(pdbc->herr, en_HY001);
	      pstmt->type = 0;
	      MEM_FREE(pstmt);
	      return SQL_ERROR;
	    }
	  pstmt->imp_desc[i]->type = SQL_HANDLE_DESC;
	  pstmt->imp_desc[i]->hstmt = pstmt;
	  pstmt->imp_desc[i]->dhdesc = NULL;
	  pstmt->imp_desc[i]->hdbc = hdbc;
	  pstmt->imp_desc[i]->herr = NULL;
	}
    }
  else
    { /* the ODBC3 driver */
      hproc = _iodbcdm_getproc(pdbc, en_GetStmtAttr);
      if (hproc == SQL_NULL_HPROC)
	{  /* with no GetStmtAttr ! */
	  int i;
	  PUSHSQLERR(pdbc->herr, en_HYC00);
	  pstmt->type = 0;
	  MEM_FREE(pstmt);
	  return SQL_ERROR;
	}
      else
	{ /* get the implicit descriptors */
	  int i, i1;
	  RETCODE rc1;

	  for (i = 0; i < 4; i++)
	    {
	      pstmt->imp_desc[i] = (DESC_t FAR *) MEM_ALLOC (sizeof (DESC_t));
	      memset(pstmt->imp_desc[i], 0, sizeof(DESC_t));
	      if (pstmt->imp_desc[i] == NULL)
		{ /* memory allocation error */
		  PUSHSQLERR(pdbc->herr, en_HY001);
		  for (i1 = 0; i1 < i; i++)
		    {
		      pstmt->imp_desc[i1]->type = 0;
		      MEM_FREE(pstmt->imp_desc[i1]);
		    }
		  pstmt->type = 0;
		  MEM_FREE(pstmt);
		  return SQL_ERROR;
		}
	      pstmt->imp_desc[i]->type = SQL_HANDLE_DESC;
	      pstmt->imp_desc[i]->hdbc = hdbc;
	      pstmt->imp_desc[i]->hstmt = *phstmt;
	      pstmt->imp_desc[i]->herr = NULL;
	      CALL_DRIVER(hdbc, pstmt, rc1, hproc, en_GetStmtAttr,
		  (pstmt->dhstmt, SQL_ATTR_IMP_PARAM_DESC, &pstmt->imp_desc[i]->dhdesc, 0, NULL));
	      if (rc1 != SQL_SUCCESS && rc1 != SQL_SUCCESS_WITH_INFO)
		{ /* no descriptor returned from the driver */
		  pstmt->type = 0;
		  MEM_FREE(pstmt);
		  for (i1 = 0; i1 < i + 1; i++)
		    {
		      pstmt->imp_desc[i1]->type = 0;
		      MEM_FREE(pstmt->imp_desc[i1]);
		    }
		  pstmt->type = 0;
		  MEM_FREE(pstmt);
		  pdbc->rc = SQL_ERROR;
		  return SQL_ERROR;
		}
	    }
	}
    }
#endif  
  
  /* insert into list */
  pstmt->next = pdbc->hstmt;
  pdbc->hstmt = pstmt;

  *phstmt = (SQLHSTMT) pstmt;

  /* state transition */
  pdbc->state = en_dbc_hstmt;

  return SQL_SUCCESS;
}


SQLRETURN 
_iodbcdm_dropstmt (HSTMT hstmt)
{
  STMT (pstmt, hstmt);
  STMT_t FAR *tpstmt;
  DBC_t FAR *pdbc;

  if (!IS_VALID_HSTMT (pstmt))
    {
      return SQL_INVALID_HANDLE;
    }
  CLEAR_ERRORS (pstmt);

  pdbc = (DBC_t FAR *) (pstmt->hdbc);

  for (tpstmt = (STMT_t FAR *) pdbc->hstmt;
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
      int i, i1;
      for (i = 0; i < 4; i++)
	{
	  _iodbcdm_freesqlerrlist (pstmt->imp_desc[i]->herr);
	  pstmt->imp_desc[i]->type = 0;
	  MEM_FREE(pstmt->imp_desc[i]);
	}
    }
#endif   

  /*
   *  Invalidate this handle
   */
  pstmt->type = 0;

  MEM_FREE (pstmt);

  return SQL_SUCCESS;
}


SQLRETURN SQL_API 
SQLFreeStmt (
    SQLHSTMT hstmt,
    SQLUSMALLINT fOption)
{
  STMT (pstmt, hstmt);
  DBC_t FAR *pdbc;

  HPROC hproc = SQL_NULL_HPROC;
  SQLRETURN retcode;

  if (!IS_VALID_HSTMT (pstmt))
    {
      return SQL_INVALID_HANDLE;
    }
  CLEAR_ERRORS(pstmt);

  pdbc = (DBC_t FAR *) (pstmt->hdbc);

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

  hproc = SQL_NULL_HPROC;

#if (ODBCVER >= 0x0300)
  if (fOption == SQL_DROP)
    {
      hproc = _iodbcdm_getproc (pstmt->hdbc, en_FreeHandle);

      if (hproc)
	{
	  CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc, en_FreeHandle, 
	    (SQL_HANDLE_STMT, pstmt->dhstmt));
	}
    }
#endif

  if (hproc == SQL_NULL_HPROC)
    {
      hproc = _iodbcdm_getproc (pstmt->hdbc, en_FreeStmt);

      if (hproc == SQL_NULL_HPROC)
	{
	  PUSHSQLERR (pstmt->herr, en_IM001);

	  return SQL_ERROR;
	}

      CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc, en_FreeStmt, 
	(pstmt->dhstmt, fOption));
    }

  if (retcode != SQL_SUCCESS
      && retcode != SQL_SUCCESS_WITH_INFO)
    {
      return retcode;
    }

  /* state transition */
  switch (fOption)
     {
     case SQL_DROP:
       /* delet this object (ignore return) */
       _iodbcdm_dropstmt (pstmt);
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

	  case en_stmt_executed:
	  case en_stmt_cursoropen:
	  case en_stmt_fetched:
	  case en_stmt_xfetched:
	    if (pstmt->prep_state)
	      {
		pstmt->state =
		    en_stmt_prepared;
	      }
	    else
	      {
		pstmt->state =
		    en_stmt_allocated;
	      }
	    break;

	  default:
	    break;
	  }
       break;

     case SQL_UNBIND:
     case SQL_RESET_PARAMS:
     default:
       break;
     }

  return retcode;
}


SQLRETURN SQL_API 
SQLSetStmtOption (
    SQLHSTMT hstmt,
    SQLUSMALLINT fOption,
    SQLUINTEGER vParam)
{
  STMT (pstmt, hstmt);
  HPROC hproc;
  int sqlstat = en_00000;
  SQLRETURN retcode;

  if (!IS_VALID_HSTMT (pstmt))
    {
      return SQL_INVALID_HANDLE;
    }
  CLEAR_ERRORS(pstmt);

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
#if (ODBCVER >= 0x0300)

  hproc = _iodbcdm_getproc (pstmt->hdbc, en_SetStmtAttr);

  if (hproc != SQL_NULL_HPROC)
    {
      switch (fOption)
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
	      CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc, en_SetStmtAttr,
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
	      PUSHSQLERR(pstmt->herr, en_IM001);
	      return SQL_ERROR;

	  default:
	      CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc, en_SetStmtAttr,
		  (pstmt->dhstmt, fOption, vParam, SQL_NTS));
	}
    }
  else
#endif
    {
      hproc = _iodbcdm_getproc (pstmt->hdbc, en_SetStmtOption);

      if (hproc == SQL_NULL_HPROC)
	{
	  PUSHSQLERR (pstmt->herr, en_IM001);

	  return SQL_ERROR;
	}

      CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc, en_SetStmtOption,
	  (pstmt->dhstmt, fOption, vParam));
    }

  return retcode;
}


SQLRETURN SQL_API 
SQLGetStmtOption (
    SQLHSTMT hstmt,
    SQLUSMALLINT fOption,
    SQLPOINTER pvParam)
{
  STMT (pstmt, hstmt);
  HPROC hproc;
  int sqlstat = en_00000;
  SQLRETURN retcode;

  if (!IS_VALID_HSTMT (pstmt))
    {
      return SQL_INVALID_HANDLE;
    }
  CLEAR_ERRORS(pstmt);

#if (ODBCVER < 0x0300)
  /* check option */
  if (				/* fOption < SQL_STMT_OPT_MIN || */
      fOption > SQL_STMT_OPT_MAX)
    {
      PUSHSQLERR (pstmt->herr, en_S1092);

      return SQL_ERROR;
    }
#endif	/* ODBCVER < 0x0300 */

  /* check state */
  if (pstmt->state >= en_stmt_needdata
      || pstmt->asyn_on != en_NullProc)
    {
      sqlstat = en_S1010;
    }
  else
    {
      switch (pstmt->state)
	 {
	 case en_stmt_allocated:
	 case en_stmt_prepared:
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

#if (ODBCVER >= 0x0300)

  hproc = _iodbcdm_getproc (pstmt->hdbc, en_GetStmtAttr);

  if (hproc != SQL_NULL_HPROC)
    {
      switch (fOption)
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
	      CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc, en_GetStmtAttr,
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
	      PUSHSQLERR(pstmt->herr, en_IM001);
	      return SQL_ERROR;

	  default:
	      CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc, en_GetStmtAttr,
		  (pstmt->dhstmt, fOption, pvParam, SQL_MAX_OPTION_STRING_LENGTH, NULL));
	      break;
	}
    }
  else
#endif
    {
      hproc = _iodbcdm_getproc (pstmt->hdbc, en_GetStmtOption);

      if (hproc == SQL_NULL_HPROC)
	{
	  PUSHSQLERR (pstmt->herr, en_IM001);
	  return SQL_ERROR;
	}

      CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc, en_GetStmtOption,
	  (pstmt->dhstmt, fOption, pvParam));
    }

  return retcode;
}


SQLRETURN SQL_API 
SQLCancel (SQLHSTMT hstmt)
{
  STMT (pstmt, hstmt);
  HPROC hproc;
  SQLRETURN retcode;

  if (!IS_VALID_HSTMT (pstmt))
    {
      return SQL_INVALID_HANDLE;
    }
  CLEAR_ERRORS(pstmt);

  /* check argument */
  /* check state */

  /* call driver */
  hproc = _iodbcdm_getproc (pstmt->hdbc, en_Cancel);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pstmt->herr, en_IM001);

      return SQL_ERROR;
    }

  CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc, en_Cancel, 
    (pstmt->dhstmt));

  /* state transition */
  if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
    {
      return retcode;
    }

  switch (pstmt->state)
     {
     case en_stmt_allocated:
     case en_stmt_prepared:
       break;

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

  return retcode;
}
