/*
 *  hdbc.c
 *
 *  $Id$
 *
 *  Data source connect object management functions
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
#include <hdbc.h>
#include <hstmt.h>

#include <itrace.h>
#include <stdio.h>

SQLRETURN SQL_API
SQLAllocConnect (
    SQLHENV henv,
    SQLHDBC FAR * phdbc)
{
  GENV (genv, henv);
  DBC_t FAR *pdbc;

  if (!IS_VALID_HENV (genv))
    {
      return SQL_INVALID_HANDLE;
    }
  CLEAR_ERRORS (genv);

  if (phdbc == NULL)
    {
      PUSHSQLERR (genv->herr, en_S1009);

      return SQL_ERROR;
    }

  pdbc = (DBC_t FAR *) MEM_ALLOC (sizeof (DBC_t));

  if (pdbc == NULL)
    {
      *phdbc = SQL_NULL_HDBC;

      PUSHSQLERR (genv->herr, en_S1001);

      return SQL_ERROR;
    }
  pdbc->rc = 0;

  /*
   *  Initialize this handle
   */
  pdbc->type = SQL_HANDLE_DBC;

  /* insert this dbc entry into the link list */
  pdbc->next = genv->hdbc;
  genv->hdbc = pdbc;
#if (ODBCVER >= 0x0300)
  if (genv->odbc_ver == 0)
    genv->odbc_ver = SQL_OV_ODBC2;
  pdbc->hdesc = NULL;
#endif
  pdbc->genv = genv;

  pdbc->henv = SQL_NULL_HENV;
  pdbc->hstmt = SQL_NULL_HSTMT;
  pdbc->herr = SQL_NULL_HERR;
  pdbc->dhdbc = SQL_NULL_HDBC;
  pdbc->state = en_dbc_allocated;
  pdbc->trace = 0;
  pdbc->tstm = NULL;
  pdbc->tfile = NULL;

  /* set connect options to default values */
  pdbc->access_mode = SQL_MODE_DEFAULT;
  pdbc->autocommit = SQL_AUTOCOMMIT_DEFAULT;
  pdbc->current_qualifier = NULL;
  pdbc->login_timeout = 0UL;
  pdbc->odbc_cursors = SQL_CUR_DEFAULT;
  pdbc->packet_size = 0UL;
  pdbc->quiet_mode = (UDWORD) NULL;
  pdbc->txn_isolation = SQL_TXN_READ_UNCOMMITTED;
  pdbc->cb_commit = (SWORD) SQL_CB_DELETE;
  pdbc->cb_rollback = (SWORD) SQL_CB_DELETE;

  *phdbc = (SQLHDBC) pdbc;

  return SQL_SUCCESS;
}


SQLRETURN SQL_API
SQLFreeConnect (SQLHDBC hdbc)
{
  GENV_t FAR *genv;
  CONN (pdbc, hdbc);
  DBC_t FAR *tpdbc;

  if (!IS_VALID_HDBC (pdbc))
    {
      return SQL_INVALID_HANDLE;
    }
  CLEAR_ERRORS (pdbc);

  /* check state */
  if (pdbc->state != en_dbc_allocated)
    {
      PUSHSQLERR (pdbc->herr, en_S1010);

      return SQL_ERROR;
    }

  genv = (GENV_t FAR *) pdbc->genv;

  for (tpdbc = (DBC_t FAR *) genv->hdbc; tpdbc != NULL; tpdbc = tpdbc->next)
    {
      if (pdbc == tpdbc)
	{
	  genv->hdbc = pdbc->next;
	  break;
	}

      if (pdbc == tpdbc->next)
	{
	  tpdbc->next = pdbc->next;
	  break;
	}
    }

  /* free this dbc */
  _iodbcdm_driverunload (pdbc);

  if (pdbc->tfile)
    {
      MEM_FREE (pdbc->tfile);
    }

  SQLSetConnectOption ((SQLHDBC) pdbc, SQL_OPT_TRACE, SQL_OPT_TRACE_OFF);

  /*
   *  Invalidate this handle
   */
  pdbc->type = 0;

  MEM_FREE (pdbc);

  return SQL_SUCCESS;
}


SQLRETURN SQL_API
SQLSetConnectOption (
    SQLHDBC hdbc,
    SQLUSMALLINT fOption,
    SQLUINTEGER vParam)
{
  CONN (pdbc, hdbc);
  STMT_t FAR *pstmt;
  HPROC hproc = SQL_NULL_HPROC;
  int sqlstat = en_00000;
  SQLRETURN retcode = SQL_SUCCESS;

  if (!IS_VALID_HDBC (pdbc))
    {
      return SQL_INVALID_HANDLE;
    }
  CLEAR_ERRORS (pdbc);

#if (ODBCVER < 0x0300)
  /* check option */
  if (fOption < SQL_CONN_OPT_MIN ||
      (fOption > SQL_CONN_OPT_MAX && fOption < SQL_CONNECT_OPT_DRVR_START))
    {
      PUSHSQLERR (pdbc->herr, en_S1092);

      return SQL_ERROR;
    }
#endif

  /* check state of connection handle */
  switch (pdbc->state)
    {
    case en_dbc_allocated:
      if (fOption == SQL_TRANSLATE_DLL || fOption == SQL_TRANSLATE_OPTION)
	{
	  /* This two options are only meaningful
	   * for specified driver. So, has to be
	   * set after a dirver has been loaded.
	   */
	  sqlstat = en_08003;
	  break;
	}

      if (fOption >= SQL_CONNECT_OPT_DRVR_START && pdbc->henv == SQL_NULL_HENV)
	/* An option only meaningful for drivers
	 * is passed before loading a driver.
	 * We classify this as an invalid option error.
	 * This is not documented by MS SDK guide.
	 */
	{
	  sqlstat = en_S1092;
	  break;
	}
      break;

    case en_dbc_needdata:
      sqlstat = en_S1010;
      break;

    case en_dbc_connected:
    case en_dbc_hstmt:
      if (fOption == SQL_ODBC_CURSORS)
	{
	  sqlstat = en_08002;
	}
      break;

    default:
      break;
    }

  /* check state of statement handle(s) */
  for (pstmt = (STMT_t FAR *) pdbc->hstmt;
      pstmt != NULL && sqlstat == en_00000;
      pstmt = (STMT_t FAR *) pstmt->next)
    {
      if (pstmt->state >= en_stmt_needdata || pstmt->asyn_on != en_NullProc)
	{
	  sqlstat = en_S1010;
	}
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pdbc->herr, sqlstat);

      return SQL_ERROR;
    }

#if (ODBCVER >= 0x0300)
  if (fOption == SQL_OPT_TRACE || fOption == SQL_ATTR_TRACE)
#else
  if (fOption == SQL_OPT_TRACE)
#endif
    /* tracing flag can be set before and after connect 
     * and only meaningful for driver manager(actually
     * there is only one tracing file under one global
     * environment).
     */
    {
      switch (vParam)
	{
	case SQL_OPT_TRACE_ON:
	  if (pdbc->tfile == NULL)
	    {
	      pdbc->tfile = (char FAR *) MEM_ALLOC (1 +
		  STRLEN (SQL_OPT_TRACE_FILE_DEFAULT));

	      if (pdbc->tfile == NULL)
		{
		  PUSHSQLERR (pdbc->herr, en_S1001);

		  return SQL_ERROR;
		}

	      STRCPY (pdbc->tfile, SQL_OPT_TRACE_FILE_DEFAULT);
	    }

	  if (pdbc->tstm == NULL)
	    {

#if	defined(stderr) && defined(stdout)
	      if (STREQ (pdbc->tfile, "stderr"))
		{
		  pdbc->tstm = stderr;
		}
	      else if (STREQ (pdbc->tfile, "stdout"))
		{
		  pdbc->tstm = stdout;
		}
	      else
#endif

		{
		  pdbc->tstm
		      = fopen (pdbc->tfile, "a+");
		}

	      if (pdbc->tstm)
		{
		  pdbc->trace = 1;
		}
	      else
		{
		  pdbc->trace = 0;

		  sqlstat = en_IM013;
		  retcode = SQL_ERROR;
		}
	    }
	  break;

	case SQL_OPT_TRACE_OFF:
	  if (pdbc->trace && pdbc->tstm)
	    {

#if	defined(stderr) && defined(stdout)
	      if (stderr != (FILE FAR *) (pdbc->tstm)
		  && stdout != (FILE FAR *) (pdbc->tstm))
#endif

		{
		  fclose (pdbc->tstm);
		}
	    }
	  pdbc->tstm = NULL;
	  pdbc->trace = 0;
	  break;

	default:
	  PUSHSQLERR (pdbc->herr, en_S1009);
	  retcode = SQL_ERROR;
	}

      if (sqlstat != en_00000)
	{
	  PUSHSQLERR (pdbc->herr, sqlstat);
	}

      return retcode;
    }

#if (ODBCVER >= 0x0300)
  if (fOption == SQL_OPT_TRACEFILE || fOption == SQL_ATTR_TRACEFILE)
#else
  if (fOption == SQL_OPT_TRACEFILE)
#endif
    /* Tracing file can be set before and after connect 
     * and only meaningful for driver manager. 
     */
    {
      if (vParam == 0UL || ((char FAR *) vParam)[0] == 0)
	{
	  PUSHSQLERR (pdbc->herr, en_S1009);

	  return SQL_ERROR;
	}

      if (pdbc->tfile && STREQ (pdbc->tfile, vParam))
	{
	  return SQL_SUCCESS;
	}

      if (pdbc->trace)
	{
	  PUSHSQLERR (pdbc->herr, en_IM014);

	  return SQL_ERROR;
	}

      if (pdbc->tfile)
	{
	  MEM_FREE (pdbc->tfile);
	}

      pdbc->tfile = (char FAR *) MEM_ALLOC (1 + STRLEN (vParam));

      if (pdbc->tfile == NULL)
	{
	  PUSHSQLERR (pdbc->herr, en_S1001);

	  return SQL_ERROR;
	}

      STRCPY (pdbc->tfile, vParam);

      return SQL_SUCCESS;
    }

  if (pdbc->state != en_dbc_allocated)
    {
      /* If already connected, then, driver's odbc call
       * will be invoked. Otherwise, we only save the options
       * and delay the setting process until the connection 
       * been established.  
       */

#if (ODBCVER >= 0x0300)
      hproc = _iodbcdm_getproc (pdbc, en_SetConnectAttr);

      if (hproc != SQL_NULL_HPROC)
	{
	  switch (fOption)
	    {
	      /* integer attributes */
	    case SQL_ATTR_ACCESS_MODE:
	    case SQL_ATTR_AUTOCOMMIT:
	    case SQL_ATTR_LOGIN_TIMEOUT:
	    case SQL_ATTR_ODBC_CURSORS:
	    case SQL_ATTR_PACKET_SIZE:
	    case SQL_ATTR_QUIET_MODE:
	    case SQL_ATTR_TRANSLATE_OPTION:
	    case SQL_ATTR_TXN_ISOLATION:
	      CALL_DRIVER (hdbc, pdbc, retcode, hproc, en_SetConnectAttr,
		  (pdbc->dhdbc, fOption, vParam, 0));
	      break;

	      /* ODBC3 defined options */
	    case SQL_ATTR_ASYNC_ENABLE:
	    case SQL_ATTR_AUTO_IPD:
	    case SQL_ATTR_CONNECTION_DEAD:
	    case SQL_ATTR_CONNECTION_TIMEOUT:
	    case SQL_ATTR_METADATA_ID:
	      PUSHSQLERR (pdbc->herr, en_IM001);
	      return SQL_ERROR;

	    default:		/* string & driver defined */

	      CALL_DRIVER (hdbc, pdbc, retcode, hproc, en_SetConnectAttr,
		  (pdbc->dhdbc, fOption, vParam, SQL_NTS));
	    }
	}
      else
#endif
	{
	  hproc = _iodbcdm_getproc (pdbc, en_SetConnectOption);

	  if (hproc == SQL_NULL_HPROC)
	    {
	      PUSHSQLERR (pdbc->herr, en_IM001);

	      return SQL_ERROR;
	    }

	  CALL_DRIVER (hdbc, pdbc, retcode, hproc, en_SetConnectOption,
	      (pdbc->dhdbc, fOption, vParam));
	}

      if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
	{
	  return retcode;
	}
    }

  /* 
   * Now, either driver's odbc call was successed or
   * driver has not been loaded yet. In the first case, we
   * need flip flag for(such as access_mode, autocommit, ...)
   * for our finit state machine. While in the second case, 
   * we need save option values(such as current_qualifier, ...)
   * for delaied setting. So, ...
   */

  /* No matter what state we are(i.e. allocated or connected, ..)
   * we need to flip the flag.
   */
  switch (fOption)
    {
    case SQL_ACCESS_MODE:
      pdbc->access_mode = vParam;
      break;

    case SQL_AUTOCOMMIT:
      pdbc->autocommit = vParam;
      break;
    }

  /* state transition */
  if (pdbc->state != en_dbc_allocated)
    {
      return retcode;
    }

  /* Only 'allocated' state is possible here, and we need to
   * save the options for delaied setting.
   */
  switch (fOption)
    {
    case SQL_CURRENT_QUALIFIER:
      if (pdbc->current_qualifier != NULL)
	{
	  MEM_FREE (pdbc->current_qualifier);
	}

      if (vParam == 0UL)
	{
	  pdbc->current_qualifier = NULL;

	  break;
	}

      pdbc->current_qualifier
	  = (char FAR *) MEM_ALLOC (
	  STRLEN (vParam) + 1);

      if (pdbc->current_qualifier == NULL)
	{
	  PUSHSQLERR (pdbc->herr, en_S1001);
	  return SQL_ERROR;
	}

      STRCPY (pdbc->current_qualifier, vParam);
      break;

    case SQL_LOGIN_TIMEOUT:
      pdbc->login_timeout = vParam;
      break;

    case SQL_ODBC_CURSORS:
      pdbc->odbc_cursors = vParam;
      break;

    case SQL_PACKET_SIZE:
      pdbc->packet_size = vParam;
      break;

    case SQL_QUIET_MODE:
      pdbc->quiet_mode = vParam;
      break;

    case SQL_TXN_ISOLATION:
      pdbc->txn_isolation = vParam;
      break;

    default:
      /* Since we didn't save the option value for delaied
       * setting, we should raise an error here.
       */
      break;
    }

  return retcode;
}


SQLRETURN SQL_API
SQLGetConnectOption (
    SQLHDBC hdbc,
    SQLUSMALLINT fOption,
    SQLPOINTER pvParam)
{
  CONN (pdbc, hdbc);
  int sqlstat = en_00000;
  HPROC hproc = SQL_NULL_HPROC;
  SQLRETURN retcode;

  if (!IS_VALID_HDBC (pdbc))
    {
      return SQL_INVALID_HANDLE;
    }
  CLEAR_ERRORS (pdbc);

#if (ODBCVER < 0x0300)
  /* check option */
  if (fOption < SQL_CONN_OPT_MIN ||
      (fOption > SQL_CONN_OPT_MAX && fOption < SQL_CONNECT_OPT_DRVR_START))
    {
      PUSHSQLERR (pdbc->herr, en_S1092);

      return SQL_ERROR;
    }
#endif

  /* check state */
  switch (pdbc->state)
    {
    case en_dbc_allocated:
      if (fOption != SQL_ACCESS_MODE
	  && fOption != SQL_AUTOCOMMIT
	  && fOption != SQL_LOGIN_TIMEOUT
	  && fOption != SQL_OPT_TRACE
	  && fOption != SQL_OPT_TRACEFILE)
	{
	  sqlstat = en_08003;
	}
      /* MS ODBC SDK document only
       * allows SQL_ACCESS_MODE
       * and SQL_AUTOCOMMIT in this
       * dbc state. We allow another 
       * two options, because they 
       * are only meaningful for driver 
       * manager.  
       */
      break;

    case en_dbc_needdata:
      sqlstat = en_S1010;
      break;

    default:
      break;
    }

  if (sqlstat != en_00000)
    {
      PUSHSQLERR (pdbc->herr, sqlstat);

      return SQL_ERROR;
    }

  /* Tracing and tracing file options are only 
   * meaningful for driver manager
   */
#if (ODBCVER >= 0x0300)
  if (fOption == SQL_OPT_TRACE || fOption == SQL_ATTR_TRACE)
#else
  if (fOption == SQL_OPT_TRACE)
#endif
    {
      if (pdbc->trace)
	*((UDWORD *) pvParam) = (UDWORD) SQL_OPT_TRACE_ON;
      else
	*((UDWORD *) pvParam) = (UDWORD) SQL_OPT_TRACE_OFF;

      return SQL_SUCCESS;
    }

#if (ODBCVER >= 0x0300)
  if (fOption == SQL_OPT_TRACEFILE || fOption == SQL_ATTR_TRACEFILE)
#else
  if (fOption == SQL_OPT_TRACEFILE)
#endif
    {
      STRCPY (pvParam, pdbc->tfile);

      return SQL_SUCCESS;
    }

  if (pdbc->state != en_dbc_allocated)
    /* if already connected, we will invoke driver's function */
    {

#if (ODBCVER >= 0x0300)

      hproc = _iodbcdm_getproc (pdbc, en_GetConnectAttr);

      if (hproc != SQL_NULL_HPROC)
	{
	  switch (fOption)
	    {
	      /* integer attributes */
	    case SQL_ATTR_ACCESS_MODE:
	    case SQL_ATTR_AUTOCOMMIT:
	    case SQL_ATTR_LOGIN_TIMEOUT:
	    case SQL_ATTR_ODBC_CURSORS:
	    case SQL_ATTR_PACKET_SIZE:
	    case SQL_ATTR_QUIET_MODE:
	    case SQL_ATTR_TRANSLATE_OPTION:
	    case SQL_ATTR_TXN_ISOLATION:
	      CALL_DRIVER (hdbc, pdbc, retcode, hproc, en_GetConnectAttr,
		  (pdbc->dhdbc, fOption, pvParam, 0, NULL));
	      break;

	      /* ODBC3 defined options */
	    case SQL_ATTR_ASYNC_ENABLE:
	    case SQL_ATTR_AUTO_IPD:
	    case SQL_ATTR_CONNECTION_DEAD:
	    case SQL_ATTR_CONNECTION_TIMEOUT:
	    case SQL_ATTR_METADATA_ID:
	      PUSHSQLERR (pdbc->herr, en_IM001);
	      return SQL_ERROR;

	    default:		/* string & driver defined */

	      CALL_DRIVER (hdbc, pdbc, retcode, hproc, en_GetConnectAttr,
		  (pdbc->dhdbc, fOption, pvParam, SQL_MAX_OPTION_STRING_LENGTH, NULL));
	    }
	}
      else
#endif
	{
	  hproc = _iodbcdm_getproc (pdbc, en_GetConnectOption);

	  if (hproc == SQL_NULL_HPROC)
	    {
	      PUSHSQLERR (pdbc->herr, en_IM001);

	      return SQL_ERROR;
	    }

	  CALL_DRIVER (hdbc, pdbc, retcode, hproc, en_GetConnectOption,
	      (pdbc->dhdbc, fOption, pvParam));
	}

      return retcode;
    }

  /* We needn't to handle options which are not allowed 
   * to be *get* at a allocated dbc state(and two tracing
   * options which has been handled and returned). Thus, 
   * there are only two possible cases. 
   */
  switch (fOption)
    {
    case SQL_ACCESS_MODE:
      *((UDWORD *) pvParam) = pdbc->access_mode;
      break;

    case SQL_AUTOCOMMIT:
      *((UDWORD *) pvParam) = pdbc->autocommit;
      break;

    case SQL_LOGIN_TIMEOUT:
      *((UDWORD *) pvParam) = pdbc->login_timeout;
      break;

    default:
      break;
    }

  return SQL_SUCCESS;
}


static SQLRETURN
_iodbcdm_transact (
    HDBC hdbc,
    UWORD fType)
{
  CONN (pdbc, hdbc);
  STMT_t FAR *pstmt;
  HPROC hproc;
  SQLRETURN retcode;

  /* check state */
  switch (pdbc->state)
    {
    case en_dbc_allocated:
    case en_dbc_needdata:
      PUSHSQLERR (pdbc->herr, en_08003);
      return SQL_ERROR;

    case en_dbc_connected:
      return SQL_SUCCESS;

    case en_dbc_hstmt:
    default:
      break;
    }

  for (pstmt = (STMT_t FAR *) (pdbc->hstmt);
      pstmt != NULL;
      pstmt = pstmt->next)
    {
      if (pstmt->state >= en_stmt_needdata
	  || pstmt->asyn_on != en_NullProc)
	{
	  PUSHSQLERR (pdbc->herr, en_S1010);

	  return SQL_ERROR;
	}
    }
  hproc = _iodbcdm_getproc (pdbc, en_Transact);

  if (hproc != SQL_NULL_HPROC)
    {
      CALL_DRIVER (hdbc, pdbc, retcode, hproc, en_Transact,
	  (SQL_NULL_HENV, pdbc->dhdbc, fType));
    }
  else
    {
#if (ODBCVER >= 0x300)
      hproc = _iodbcdm_getproc (pdbc, en_EndTran);
      if (hproc != SQL_NULL_HPROC)
	{
	  CALL_DRIVER (pdbc, pdbc, retcode, hproc, en_EndTran,
	      (SQL_HANDLE_DBC, pdbc->dhdbc, fType));
	}
      else
	{
	  PUSHSQLERR (pdbc->herr, en_IM001);
	  return SQL_ERROR;
	}
#else
      PUSHSQLERR (pdbc->herr, en_IM001);
      return SQL_ERROR;
#endif
    }

  /* state transition */
  if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
    {
      return retcode;
    }

  pdbc->state = en_dbc_hstmt;

  for (pstmt = (STMT_t FAR *) (pdbc->hstmt);
      pstmt != NULL;
      pstmt = pstmt->next)
    {
      switch (pstmt->state)
	{
	case en_stmt_prepared:
	  if (pdbc->cb_commit == SQL_CB_DELETE
	      || pdbc->cb_rollback == SQL_CB_DELETE)
	    {
	      pstmt->state = en_stmt_allocated;
	      pstmt->prep_state = 0;
	      break;
	    }
	  break;

	case en_stmt_executed:
	case en_stmt_cursoropen:
	case en_stmt_fetched:
	case en_stmt_xfetched:
	  if (!pstmt->prep_state
	      && pdbc->cb_commit != SQL_CB_PRESERVE
	      && pdbc->cb_rollback != SQL_CB_PRESERVE)
	    {
	      pstmt->state = en_stmt_allocated;
	      pstmt->prep_state = 0;
	      pstmt->cursor_state = en_stmt_cursor_no;
	      break;
	    }

	  if (pstmt->prep_state)
	    {
	      if (pdbc->cb_commit == SQL_CB_DELETE
		  || pdbc->cb_rollback == SQL_CB_DELETE)
		{
		  pstmt->state = en_stmt_allocated;
		  pstmt->prep_state = 0;
		  pstmt->cursor_state = en_stmt_cursor_no;
		  break;
		}

	      if (pdbc->cb_commit == SQL_CB_CLOSE
		  || pdbc->cb_rollback == SQL_CB_CLOSE)
		{
		  pstmt->state
		      = en_stmt_prepared;
		  pstmt->cursor_state
		      = en_stmt_cursor_no;
		  break;
		}
	      break;
	    }
	  break;

	default:
	  break;
	}
    }

  return retcode;
}


SQLRETURN SQL_API
SQLTransact (
    SQLHENV henv,
    SQLHDBC hdbc,
    SQLUSMALLINT fType)
{
  GENV (genv, henv);
  CONN (pdbc, hdbc);
  HERR herr;
  SQLRETURN retcode = SQL_SUCCESS;

  if (IS_VALID_HDBC (pdbc))
    {
      CLEAR_ERRORS (pdbc);
      herr = pdbc->herr;
    }
  else if (IS_VALID_HENV (genv))
    {
      CLEAR_ERRORS (genv);
      herr = genv->herr;
    }
  else
    {
      return SQL_INVALID_HANDLE;
    }

  /* check argument */
  if (fType != SQL_COMMIT && fType != SQL_ROLLBACK)
    {
      SQLHENV handle = IS_VALID_HDBC (pdbc) ? ((SQLHENV) pdbc) : genv;
      PUSHSQLERR (herr, en_S1012);

      return SQL_ERROR;
    }

  if (hdbc != SQL_NULL_HDBC)
    {
      retcode = _iodbcdm_transact (pdbc, fType);
    }
  else
    {
      for (pdbc = (DBC_t FAR *) (genv->hdbc);
	  pdbc != NULL;
	  pdbc = pdbc->next)
	{
	  retcode |= _iodbcdm_transact (pdbc, fType);
	}
    }

  if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
    {
      /* fail on one of the connection */
      return SQL_ERROR;
    }

  return retcode;
}
