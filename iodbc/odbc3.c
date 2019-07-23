/*
 *  odbc3.c
 *
 *  $Id$
 *
 *  ODBC 3.x functions
 *
 *  The iODBC driver manager.
 *
 *  Copyright (C) 1996-2019 by OpenLink Software <iodbc@openlinksw.com>
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


#include "iodbc.h"

#include <sql.h>
#include <sqlext.h>
#include <iodbcext.h>

#if (ODBCVER >= 0x300)
#include <sqlucode.h>

#include <unicode.h>

#include <dlproc.h>

#include <herr.h>
#include <henv.h>
#include <hdesc.h>
#include <hdbc.h>
#include <hstmt.h>

#include <itrace.h>


/*
 * SQL_ATTR_CONNECTION_POOLING value
 */
SQLINTEGER _iodbcdm_attr_connection_pooling = SQL_CP_DEFAULT;


/*
 *  Externals
 */
SQLRETURN SQLAllocEnv_Internal (SQLHENV * phenv, int odbc_ver);
SQLRETURN SQLFreeEnv_Internal (SQLHENV henv);
SQLRETURN SQLAllocConnect_Internal (SQLHENV henv, SQLHDBC * phdbc);
SQLRETURN SQLFreeConnect_Internal (SQLHDBC hdbc, int ver);
SQLRETURN SQLAllocStmt_Internal (SQLHDBC hdbc, SQLHSTMT * phstmt);
SQLRETURN SQLFreeStmt_Internal (SQLHSTMT hstmt, SQLUSMALLINT fOption);
SQLRETURN SQLTransact_Internal (SQLHENV henv, SQLHDBC hdbc, SQLUSMALLINT fType);

SQLRETURN
SQLAllocDesc_Internal (
    SQLHDBC hdbc,
    SQLHDESC * phdesc)
{
  CONN (pdbc, hdbc);
  SQLRETURN retcode = SQL_SUCCESS;
  HPROC hproc = SQL_NULL_HPROC;
  DESC_t *new_desc;
  SQLUINTEGER odbc_ver;
  SQLUINTEGER dodbc_ver;

  /* check state */
  switch (pdbc->state)
    {
    case en_dbc_connected:
    case en_dbc_hstmt:
      break;

    case en_dbc_allocated:
    case en_dbc_needdata:
      PUSHSQLERR (pdbc->herr, en_08003);
      *phdesc = SQL_NULL_HSTMT;

      return SQL_ERROR;

    default:
      return SQL_INVALID_HANDLE;
    }

  odbc_ver = ((GENV_t *) pdbc->genv)->odbc_ver;
  dodbc_ver = (pdbc->henv != SQL_NULL_HENV) ? ((ENV_t *) pdbc->henv)->dodbc_ver : odbc_ver;

  if (odbc_ver == SQL_OV_ODBC2 || dodbc_ver == SQL_OV_ODBC2)
    {
      PUSHSQLERR (pdbc->herr, en_HYC00);
      return SQL_ERROR;
    }

  if (phdesc == NULL)
    {
      PUSHSQLERR (pdbc->herr, en_HY009);
      return SQL_ERROR;
    }

  hproc = _iodbcdm_getproc (pdbc, en_AllocHandle);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pdbc->herr, en_IM001);
      return SQL_ERROR;
    }

  new_desc = (DESC_t *) MEM_ALLOC (sizeof (DESC_t));
  if (!new_desc)
    {
      PUSHSQLERR (pdbc->herr, en_HY001);
      return SQL_ERROR;
    }
  memset (new_desc, 0, sizeof (DESC_t));
  CALL_DRIVER (pdbc, pdbc, retcode, hproc, (SQL_HANDLE_DESC, pdbc->dhdbc, &new_desc->dhdesc));

  if (!SQL_SUCCEEDED (retcode))
    {
      MEM_FREE (new_desc);
      return SQL_ERROR;
    }

  new_desc->type = SQL_HANDLE_DESC;
  new_desc->hdbc = pdbc;
  new_desc->hstmt = NULL;
  new_desc->herr = NULL;
  new_desc->desc_cip = 0;
  *phdesc = new_desc;

  new_desc->next = (DESC_t *) pdbc->hdesc;
  pdbc->hdesc = new_desc;

  return SQL_SUCCESS;
}


RETCODE SQL_API
SQLAllocHandle_Internal (
    SQLSMALLINT	  handleType,
    SQLHANDLE 	  inputHandle,
    SQLHANDLE	* outputHandlePtr)
{
  switch (handleType)
    {
    case SQL_HANDLE_ENV:
      return SQLAllocEnv_Internal (outputHandlePtr, 0);

    case SQL_HANDLE_DBC:
      {
	GENV (genv, inputHandle);

	if (!IS_VALID_HENV (genv))
	  {
	    return SQL_INVALID_HANDLE;
	  }
	CLEAR_ERRORS (genv);
	if (genv->odbc_ver == 0)
	  {
	    PUSHSQLERR (genv->herr, en_HY010);
	    return SQL_ERROR;
	  }

	return SQLAllocConnect_Internal (inputHandle, outputHandlePtr);
      }

    case SQL_HANDLE_STMT:
      {
	CONN (con, inputHandle);

	if (!IS_VALID_HDBC (con))
	  {
	    return SQL_INVALID_HANDLE;
	  }
	CLEAR_ERRORS (con);

        return SQLAllocStmt_Internal (inputHandle, outputHandlePtr);
      }

    case SQL_HANDLE_DESC:
      {
	CONN (con, inputHandle);

	if (!IS_VALID_HDBC (con))
	  {
	    return SQL_INVALID_HANDLE;
	  }
	CLEAR_ERRORS (con);

        return SQLAllocDesc_Internal (inputHandle, outputHandlePtr);
      }

    default:
      if (IS_VALID_HDBC (inputHandle))
	{
	  CONN (con, inputHandle);
	  PUSHSQLERR (con->herr, en_HY092);
	  return SQL_ERROR;
	}
      else if (IS_VALID_HENV (inputHandle))
	{
	  GENV (genv, inputHandle);
	  PUSHSQLERR (genv->herr, en_HY092);
	  return SQL_ERROR;
	}
      return SQL_INVALID_HANDLE;
    }
}


RETCODE SQL_API
SQLAllocHandle (
    SQLSMALLINT	  handleType,
    SQLHANDLE	  inputHandle,
    SQLHANDLE	* outputHandlePtr)
{
  int retcode = SQL_SUCCESS;

  if (handleType == SQL_HANDLE_ENV)
    {
      /* 
       *  One time initialization
       */
      Init_iODBC();

      ODBC_LOCK ();

      retcode = SQLAllocEnv_Internal (outputHandlePtr, 0);

      /*
       * Start tracing
       */
      TRACE (trace_SQLAllocHandle (TRACE_ENTER,
	    handleType, inputHandle, outputHandlePtr));

      TRACE (trace_SQLAllocHandle (TRACE_LEAVE,
	    handleType, inputHandle, outputHandlePtr));

      ODBC_UNLOCK ();

      return retcode;
    }

  ODBC_LOCK ();

  TRACE (trace_SQLAllocHandle (TRACE_ENTER,
	handleType, inputHandle, outputHandlePtr));

  retcode =
      SQLAllocHandle_Internal (handleType, inputHandle, outputHandlePtr);

  TRACE (trace_SQLAllocHandle (TRACE_LEAVE,
	handleType, inputHandle, outputHandlePtr));

  ODBC_UNLOCK ();

  return retcode;
}


RETCODE SQL_API
SQLAllocHandleStd (
  SQLSMALLINT handleType,
  SQLHANDLE inputHandle,
  SQLHANDLE * outputHandlePtr)
{
  int retcode = SQL_SUCCESS;

  if (handleType == SQL_HANDLE_ENV)
    {
      /* 
       *  One time initialization
       */
      Init_iODBC();

      ODBC_LOCK ();

      retcode = SQLAllocEnv_Internal (outputHandlePtr, SQL_OV_ODBC3);

      /*
       * Start tracing
       */
      TRACE (trace_SQLAllocHandle (TRACE_ENTER,
	    handleType, inputHandle, outputHandlePtr));

      TRACE (trace_SQLAllocHandle (TRACE_LEAVE,
	    handleType, inputHandle, outputHandlePtr));

      ODBC_UNLOCK ();

      return retcode;
    }

  ODBC_LOCK ();

  TRACE (trace_SQLAllocHandle (TRACE_ENTER,
	handleType, inputHandle, outputHandlePtr));

  retcode =
      SQLAllocHandle_Internal (handleType, inputHandle, outputHandlePtr);

  TRACE (trace_SQLAllocHandle (TRACE_LEAVE,
	handleType, inputHandle, outputHandlePtr));

  ODBC_UNLOCK ();

  return retcode;
}


/**** SQLFreeHandle ****/

extern unsigned long _iodbc_env_counter;

static RETCODE
_SQLFreeHandle_ENV (
  SQLSMALLINT		  handleType,
  SQLHANDLE		  handle)
{
  int retcode = SQL_SUCCESS;

  ODBC_LOCK ();

  TRACE (trace_SQLFreeHandle (TRACE_ENTER, handleType, handle));

  retcode = SQLFreeEnv_Internal ((SQLHENV) handle);

  TRACE (trace_SQLFreeHandle (TRACE_LEAVE, handleType, handle));

  MEM_FREE (handle);

  /*
   *  Close trace after last environment handle is freed
   */
  if (--_iodbc_env_counter == 0)
    trace_stop();

  ODBC_UNLOCK ();

  return retcode;
}


static RETCODE
_SQLFreeHandle_DBC (
  SQLSMALLINT		  handleType,
  SQLHANDLE		  handle)
{
  ENTER_HDBC ((SQLHDBC) handle, 1,
      trace_SQLFreeHandle (TRACE_ENTER, handleType, handle));

  retcode = SQLFreeConnect_Internal ((SQLHDBC) handle, 3);

  LEAVE_HDBC ((SQLHDBC) handle, 1,
      trace_SQLFreeHandle (TRACE_LEAVE, handleType, handle);
      MEM_FREE (handle)
  );
}

static RETCODE
_SQLFreeHandle_STMT (
  SQLSMALLINT		  handleType,
  SQLHANDLE		  handle)
{
  ENTER_STMT ((SQLHSTMT) handle,
      trace_SQLFreeHandle (TRACE_ENTER, handleType, handle));

  retcode = SQLFreeStmt_Internal ((SQLHSTMT) handle, SQL_DROP);

  LEAVE_STMT ((SQLHSTMT) handle,
      trace_SQLFreeHandle (TRACE_LEAVE, handleType, handle);
      _iodbcdm_dropstmt ((SQLHSTMT) handle);
  );
}

static RETCODE
_SQLFreeHandle_DESC (
  SQLSMALLINT		  handleType,
  SQLHANDLE		  handle)
{
  DESC (pdesc, handle);
  CONN (pdbc, pdesc->hdbc);
  HPROC hproc;
  RETCODE retcode = SQL_SUCCESS;
  DESC_t *curr_desc;

  if (IS_VALID_HSTMT (pdesc->hstmt))
    {				/* the desc handle is implicit */
      PUSHSQLERR (pdesc->herr, en_HY017);
      return SQL_ERROR;
    }
  CLEAR_ERRORS (pdesc);

  /* remove it from the dbc's list */
  curr_desc = (DESC_t *) pdbc->hdesc;
  while (curr_desc)
    {
      if (curr_desc == pdesc)
	{
	  pdbc->hdesc = pdesc->next;
	  break;
	}
      if (curr_desc->next == pdesc)
	{
	  curr_desc->next = pdesc->next;
	  break;
	}
      curr_desc = curr_desc->next;
    }
  if (!curr_desc)
    return SQL_INVALID_HANDLE;

  /* and call the driver's function */
  hproc = SQL_NULL_HPROC;
  if (pdesc->dhdesc)
    {				/* the driver has descriptors */
      hproc = _iodbcdm_getproc (pdbc, en_FreeHandle);
      if (hproc == SQL_NULL_HPROC)
	{
	  PUSHSQLERR (pdesc->herr, en_IM001);
	  retcode = SQL_ERROR;
	}
      else
	CALL_DRIVER (pdbc, pdesc, retcode, hproc,
	    (handleType, pdesc->dhdesc));
    }

  _iodbcdm_freesqlerrlist (pdesc->herr);

  /* invalidate the handle */
  pdesc->type = 0;

  return retcode;
}


RETCODE SQL_API
SQLFreeHandle (
  SQLSMALLINT		  handleType,
  SQLHANDLE		  handle)
{
  switch (handleType)
    {
    case SQL_HANDLE_ENV:
      return _SQLFreeHandle_ENV (handleType, handle);

    case SQL_HANDLE_DBC:
      return _SQLFreeHandle_DBC (handleType, handle);

    case SQL_HANDLE_STMT:
      return _SQLFreeHandle_STMT (handleType, handle);

    case SQL_HANDLE_DESC:
      {
	ENTER_DESC ((SQLHDESC) handle,
	    trace_SQLFreeHandle (TRACE_ENTER, handleType, handle));

	retcode = _SQLFreeHandle_DESC (handleType, handle);

	LEAVE_DESC ((SQLHDESC) handle,
	    trace_SQLFreeHandle (TRACE_LEAVE, handleType, handle);
	    MEM_FREE (handle);
        );
      }
      break;

    default:
      if (IS_VALID_HDBC (handle))
	{
	  CONN (con, handle);
	  PUSHSQLERR (con->herr, en_HY092);
	  return SQL_ERROR;
	}
      else if (IS_VALID_HENV (handle))
	{
	  GENV (genv, handle);
	  PUSHSQLERR (genv->herr, en_HY092);
	  return SQL_ERROR;
	}
    }

  return SQL_INVALID_HANDLE;
}


/**** SQLSetEnvAttr ****/

static RETCODE 
SQLSetEnvAttr_Internal (SQLHENV environmentHandle,
    SQLINTEGER Attribute,
    SQLPOINTER ValuePtr,
    SQLINTEGER StringLength)
{
  GENV (genv, environmentHandle);

  StringLength = StringLength; /*UNUSED*/

  if (genv != NULL && genv->hdbc)
    {
      PUSHSQLERR (genv->herr, en_HY010);
      return SQL_ERROR;
    }

  switch (Attribute)
    {
    case SQL_ATTR_CONNECTION_POOLING:
      switch ((SQLINTEGER) (SQLULEN) ValuePtr)
	{
	case SQL_CP_OFF:
	case SQL_CP_ONE_PER_DRIVER:
	case SQL_CP_ONE_PER_HENV:
          _iodbcdm_attr_connection_pooling = (SQLINTEGER) (SQLULEN) ValuePtr;
	  return SQL_SUCCESS;

	default:
	  if (genv)
	    PUSHSQLERR (genv->herr, en_HY024);
	  return SQL_ERROR;
	}

    case SQL_ATTR_CP_MATCH:
      switch ((SQLINTEGER) (SQLULEN) ValuePtr)
	{
	case SQL_CP_STRICT_MATCH:
	case SQL_CP_RELAXED_MATCH:
          genv->cp_match = (SQLINTEGER) (SQLULEN) ValuePtr;
	  return SQL_SUCCESS;

	default:
	  PUSHSQLERR (genv->herr, en_HY024);
	  return SQL_ERROR;
	}

    case SQL_ATTR_ODBC_VERSION:
      switch ((SQLINTEGER) (SQLULEN) ValuePtr)
	{
	case SQL_OV_ODBC2:
	case SQL_OV_ODBC3:
	  genv->odbc_ver = (SQLINTEGER) (SQLULEN) ValuePtr;
	  return (SQL_SUCCESS);

	default:
	  PUSHSQLERR (genv->herr, en_HY024);
	  return SQL_ERROR;
	}

    case SQL_ATTR_OUTPUT_NTS:
      switch ((SQLINTEGER) (SQLULEN) ValuePtr)
	{
	case SQL_TRUE:
	  return SQL_SUCCESS;

	case SQL_FALSE:
	  PUSHSQLERR (genv->herr, en_HYC00);
	  return SQL_ERROR;

	default:
	  PUSHSQLERR (genv->herr, en_HY024);
	  return SQL_ERROR;
	}

    case SQL_ATTR_APP_UNICODE_TYPE:
      switch ((SQLINTEGER)(SQLULEN) ValuePtr)
        {
        case SQL_DM_CP_UTF16:
          genv->conv.dm_cp = CP_UTF16;
          break;
        case SQL_DM_CP_UCS4:
          genv->conv.dm_cp = CP_UCS4;
          break;
        case SQL_DM_CP_UTF8:
          genv->conv.dm_cp = CP_UTF8;
          break;
        default:
	  PUSHSQLERR (genv->herr, en_HY024);
	  return SQL_ERROR;
        }
      DPRINTF ((stderr,
      "DEBUG: SQLSetEnvAttr DiverManager AppUnicodeType=%s\n",
        genv->conv.dm_cp==CP_UCS4?"UCS4":(genv->conv.dm_cp==CP_UTF16?"UTF16":"UTF8")));

      break;

    default:
      PUSHSQLERR (genv->herr, en_HY092);
      return SQL_ERROR;
    }
}


RETCODE SQL_API
SQLSetEnvAttr (
  SQLHENV		  EnvironmentHandle,
  SQLINTEGER		  Attribute,
  SQLPOINTER		  ValuePtr,
  SQLINTEGER		  StringLength)
{
  if (Attribute == SQL_ATTR_CONNECTION_POOLING)
    {
	SQLRETURN retcode = SQL_SUCCESS;

	/* ENTER_HENV without EnvironmentHandle check */
	ODBC_LOCK ();
	TRACE (trace_SQLSetEnvAttr (TRACE_ENTER,
	    EnvironmentHandle,
	    Attribute,
	    ValuePtr, StringLength));

        retcode = SQLSetEnvAttr_Internal (
	      EnvironmentHandle,
	      Attribute,
	      ValuePtr, StringLength);

	/* LEAVE_HENV without done: label */
	TRACE (trace_SQLSetEnvAttr (TRACE_LEAVE,
	    EnvironmentHandle,
	    Attribute,
	    ValuePtr, StringLength));
	ODBC_UNLOCK ();
	return retcode;
    }
  else
    {
      ENTER_HENV (EnvironmentHandle,
        trace_SQLSetEnvAttr (TRACE_ENTER,
	    EnvironmentHandle,
	    Attribute,
	    ValuePtr, StringLength));

      retcode = SQLSetEnvAttr_Internal (
	    EnvironmentHandle,
	    Attribute,
	    ValuePtr, StringLength);

      LEAVE_HENV (EnvironmentHandle,
        trace_SQLSetEnvAttr (TRACE_LEAVE,
	    EnvironmentHandle,
	    Attribute,
	    ValuePtr, StringLength));
      return retcode;
   }
}


static RETCODE
SQLGetEnvAttr_Internal (
  SQLHENV		  environmentHandle,
  SQLINTEGER		  Attribute,
  SQLPOINTER		  ValuePtr,
  SQLINTEGER		  BufferLength,
  SQLINTEGER		* StringLengthPtr)
{
  GENV (genv, environmentHandle);
  SQLRETURN retcode = SQL_SUCCESS;

  if (Attribute != SQL_ATTR_CONNECTION_POOLING &&
      Attribute != SQL_ATTR_CP_MATCH &&
      Attribute != SQL_ATTR_ODBC_VERSION && 
      Attribute != SQL_ATTR_OUTPUT_NTS &&
      Attribute != SQL_ATTR_WCHAR_SIZE)
    {
      PUSHSQLERR (genv->herr, en_HY092);
      return SQL_ERROR;
    }

  /* ODBC DM env attributes */
  if (Attribute == SQL_ATTR_ODBC_VERSION)
    {
      if (ValuePtr)
	*((SQLINTEGER *) ValuePtr) = genv->odbc_ver;
      return SQL_SUCCESS;
    }
  if (Attribute == SQL_ATTR_CONNECTION_POOLING)
    {
      if (ValuePtr)
	*((SQLUINTEGER *) ValuePtr) = _iodbcdm_attr_connection_pooling;
      return SQL_SUCCESS;
    }
  if (Attribute == SQL_ATTR_CP_MATCH)
    {
      if (ValuePtr)
	*((SQLUINTEGER *) ValuePtr) = genv->cp_match;
      return SQL_SUCCESS;
    }
  if (Attribute == SQL_ATTR_OUTPUT_NTS)
    {
      if (ValuePtr)
	*((SQLINTEGER *) ValuePtr) = SQL_TRUE;
      return SQL_SUCCESS;
    }
  if (Attribute == SQL_ATTR_WCHAR_SIZE)
    {
      if (ValuePtr)
	*((SQLINTEGER *) ValuePtr) = sizeof(wchar_t);
       return SQL_SUCCESS;
     }

  /* fall back to the first driver */
  if (IS_VALID_HDBC (genv->hdbc))
    {
      CONN (con, genv->hdbc);
      HPROC hproc = _iodbcdm_getproc (con, en_GetEnvAttr);
      if (hproc != SQL_NULL_HPROC)
	{
	  ENVR (env, con->henv);
	  CALL_DRIVER (con, genv, retcode, hproc,
	      (env->dhenv, Attribute, ValuePtr, BufferLength,
		  StringLengthPtr));
	  return retcode;
	}
      else
	{			/* possibly an ODBC2 driver */
	  PUSHSQLERR (genv->herr, en_IM001);
	  return SQL_ERROR;
	}
    }
  else
    {
      switch ((SQLINTEGER) Attribute)
	{
	case SQL_ATTR_CONNECTION_POOLING:
	  if (ValuePtr)
	    *((SQLINTEGER *) ValuePtr) = SQL_CP_OFF;
	  break;

	case SQL_ATTR_CP_MATCH:
	  if (ValuePtr)
	    *((SQLINTEGER *) ValuePtr) = SQL_CP_STRICT_MATCH;
	  break;

	case SQL_ATTR_ODBC_VERSION:
	  if (ValuePtr)
	    *((SQLINTEGER *) ValuePtr) = genv->odbc_ver;
	  break;
	}
    }
  return SQL_SUCCESS;
}


RETCODE SQL_API
SQLGetEnvAttr (
  SQLHENV		  EnvironmentHandle,
  SQLINTEGER		  Attribute,
  SQLPOINTER		  ValuePtr,
  SQLINTEGER		  BufferLength,
  SQLINTEGER		* StringLengthPtr)
{
  if (Attribute == SQL_ATTR_CONNECTION_POOLING)
    {
	SQLRETURN retcode = SQL_SUCCESS;

	/* ENTER_HENV without EnvironmentHandle check */
	ODBC_LOCK ();
	TRACE (trace_SQLGetEnvAttr (TRACE_ENTER,
	    EnvironmentHandle,
	    Attribute,
	    ValuePtr, BufferLength, StringLengthPtr));

        retcode = SQLGetEnvAttr_Internal (
	      EnvironmentHandle,
	      Attribute,
	      ValuePtr, BufferLength, StringLengthPtr);

	/* LEAVE_HENV without done: label */
	TRACE (trace_SQLGetEnvAttr (TRACE_LEAVE,
	    EnvironmentHandle,
	    Attribute,
	    ValuePtr, BufferLength, StringLengthPtr));
	ODBC_UNLOCK ();
	return retcode;
    }
  else
    {
      ENTER_HENV (EnvironmentHandle,
        trace_SQLGetEnvAttr (TRACE_ENTER,
  	    EnvironmentHandle,
	    Attribute,
	    ValuePtr, BufferLength, StringLengthPtr));

      retcode = SQLGetEnvAttr_Internal (
  	    EnvironmentHandle,
	    Attribute,
	    ValuePtr, BufferLength, StringLengthPtr);

      LEAVE_HENV (EnvironmentHandle,
        trace_SQLGetEnvAttr (TRACE_LEAVE,
  	    EnvironmentHandle,
	    Attribute,
	    ValuePtr, BufferLength, StringLengthPtr));
    }
}


RETCODE SQL_API
SQLGetStmtAttr_Internal (
  SQLHSTMT		  statementHandle,
  SQLINTEGER		  Attribute,
  SQLPOINTER		  ValuePtr,
  SQLINTEGER		  BufferLength,
  SQLINTEGER		* StringLengthPtr,
  SQLCHAR		  waMode)
{
  STMT (stmt, statementHandle);
  CONN (pdbc, stmt->hdbc);
  ENVR (penv, pdbc->henv);
  HPROC hproc = SQL_NULL_HPROC;
  SQLRETURN retcode = SQL_SUCCESS;

  waMode = waMode; /*UNUSED*/

  switch (Attribute)
    {
    case SQL_ATTR_IMP_PARAM_DESC:

      if (ValuePtr)
	{
	  if (IS_VALID_HDESC (stmt->desc[IMP_PARAM_DESC]))
	    *((SQLHANDLE *) ValuePtr) = (SQLHANDLE *) stmt->desc[IMP_PARAM_DESC];
	  else if (IS_VALID_HDESC (stmt->imp_desc[IMP_PARAM_DESC]))
	    *((SQLHANDLE *) ValuePtr) = (SQLHANDLE *) stmt->imp_desc[IMP_PARAM_DESC];
	  else
	    {
	      PUSHSQLERR (stmt->herr, en_IM001);
	      return SQL_ERROR;
	    }
	}
      if (StringLengthPtr)
	*StringLengthPtr = SQL_IS_POINTER;
      return SQL_SUCCESS;

    case SQL_ATTR_APP_PARAM_DESC:

      if (ValuePtr)
	{
	  if (IS_VALID_HDESC (stmt->desc[APP_PARAM_DESC]))
	    *((SQLHANDLE *) ValuePtr) = (SQLHANDLE *) stmt->desc[APP_PARAM_DESC];
	  else if (IS_VALID_HDESC (stmt->imp_desc[APP_PARAM_DESC]))
	    *((SQLHANDLE *) ValuePtr) = (SQLHANDLE *) stmt->imp_desc[APP_PARAM_DESC];
	  else
	    {
	      PUSHSQLERR (stmt->herr, en_IM001);
	      return SQL_ERROR;
	    }
	}
      if (StringLengthPtr)
	*StringLengthPtr = SQL_IS_POINTER;
      return SQL_SUCCESS;

    case SQL_ATTR_IMP_ROW_DESC:

      if (ValuePtr)
	{
	  if (IS_VALID_HDESC (stmt->desc[IMP_ROW_DESC]))
	    *((SQLHANDLE *) ValuePtr) = (SQLHANDLE *) stmt->desc[IMP_ROW_DESC];
	  else if (IS_VALID_HDESC (stmt->imp_desc[IMP_ROW_DESC]))
	    *((SQLHANDLE *) ValuePtr) = (SQLHANDLE *) stmt->imp_desc[IMP_ROW_DESC];
	  else
	    {
	      PUSHSQLERR (stmt->herr, en_IM001);
	      return SQL_ERROR;
	    }
	}
      if (StringLengthPtr)
	*StringLengthPtr = SQL_IS_POINTER;
      return SQL_SUCCESS;

    case SQL_ATTR_APP_ROW_DESC:

      if (ValuePtr)
	{
	  if (IS_VALID_HDESC (stmt->desc[APP_ROW_DESC]))
	    *((SQLHANDLE *) ValuePtr) = (SQLHANDLE *) stmt->desc[APP_ROW_DESC];
	  else if (IS_VALID_HDESC (stmt->imp_desc[APP_ROW_DESC]))
	    *((SQLHANDLE *) ValuePtr) = (SQLHANDLE *) stmt->imp_desc[APP_ROW_DESC];
	  else
	    {
	      PUSHSQLERR (stmt->herr, en_IM001);
	      return SQL_ERROR;
	    }
	}
      if (StringLengthPtr)
	*StringLengthPtr = SQL_IS_POINTER;
      return SQL_SUCCESS;

    case SQL_ATTR_ROW_ARRAY_SIZE:
      if (((ENV_t *) ((DBC_t *) stmt->hdbc)->henv)->dodbc_ver == SQL_OV_ODBC3)
	{
          CALL_UDRIVER(stmt->hdbc, stmt, retcode, hproc, 
            penv->unicode_driver, en_GetStmtAttr, (stmt->dhstmt, Attribute, 
            ValuePtr, BufferLength, StringLengthPtr));
          if (hproc == SQL_NULL_HPROC)
            {
	      PUSHSQLERR (stmt->herr, en_IM001);
	      return SQL_ERROR;
            }
	  return retcode;
	}
      else
	{			/* an ODBC2 driver */
	  if (ValuePtr)
	    *((SQLUINTEGER *) ValuePtr) = stmt->row_array_size;
	  return SQL_SUCCESS;
	}

    case SQL_ATTR_ENABLE_AUTO_IPD:
    case SQL_ATTR_CURSOR_SENSITIVITY:
    case SQL_ATTR_CURSOR_SCROLLABLE:
    case SQL_ATTR_PARAM_BIND_TYPE:
    case SQL_ATTR_PARAM_OPERATION_PTR:
    case SQL_ATTR_PARAM_STATUS_PTR:
    case SQL_ATTR_PARAM_BIND_OFFSET_PTR:
    case SQL_ATTR_ROW_BIND_OFFSET_PTR:
    case SQL_ATTR_ROW_OPERATION_PTR:

      CALL_UDRIVER(stmt->hdbc, stmt, retcode, hproc, 
        penv->unicode_driver, en_GetStmtAttr, (stmt->dhstmt, Attribute, 
        ValuePtr, BufferLength, StringLengthPtr));
      if (hproc == SQL_NULL_HPROC)
        {
          PUSHSQLERR (stmt->herr, en_IM001);
	  return SQL_ERROR;
        }
      return retcode;

    case SQL_ATTR_FETCH_BOOKMARK_PTR:

      if (((ENV_t *) ((DBC_t *) stmt->hdbc)->henv)->dodbc_ver == SQL_OV_ODBC3)
	{
          CALL_UDRIVER(stmt->hdbc, stmt, retcode, hproc, 
            penv->unicode_driver, en_GetStmtAttr, (stmt->dhstmt, Attribute, 
            ValuePtr, BufferLength, StringLengthPtr));
          if (hproc == SQL_NULL_HPROC)
            {
	      PUSHSQLERR (stmt->herr, en_IM001);
	      return SQL_ERROR;
            }
	  return retcode;
	}
      else
	{			/* an ODBC2 driver */
	  if (ValuePtr)
	    *((SQLPOINTER *) ValuePtr) = stmt->fetch_bookmark_ptr;
	  return SQL_SUCCESS;
	}

    case SQL_ATTR_ROWS_FETCHED_PTR:

      if (((ENV_t *) ((DBC_t *) stmt->hdbc)->henv)->dodbc_ver == SQL_OV_ODBC3)
	{
          CALL_UDRIVER(stmt->hdbc, stmt, retcode, hproc, 
            penv->unicode_driver, en_GetStmtAttr, (stmt->dhstmt, Attribute, 
            ValuePtr, BufferLength, StringLengthPtr));
          if (hproc == SQL_NULL_HPROC)
            {
	      PUSHSQLERR (stmt->herr, en_IM001);
	      return SQL_ERROR;
            }
	  return retcode;
	}
      else
	{			/* an ODBC2 driver */
	  if (ValuePtr)
	    *((SQLPOINTER *) ValuePtr) = stmt->rows_fetched_ptr;
	  return SQL_SUCCESS;
	}

    case SQL_ATTR_METADATA_ID:

      if (((ENV_t *) ((DBC_t *) stmt->hdbc)->henv)->dodbc_ver == SQL_OV_ODBC3)
	{
          CALL_UDRIVER(stmt->hdbc, stmt, retcode, hproc, 
            penv->unicode_driver, en_GetStmtAttr, (stmt->dhstmt, Attribute, 
            ValuePtr, BufferLength, StringLengthPtr));
          if (hproc == SQL_NULL_HPROC)
            {
	      PUSHSQLERR (stmt->herr, en_IM001);
	      return SQL_ERROR;
            }
	  return retcode;
	}
      else
	{			/* an ODBC2 driver */
	  if (ValuePtr)
	    *((SQLUINTEGER *) ValuePtr) = SQL_FALSE;
	  return SQL_SUCCESS;
	}

    case SQL_ATTR_PARAMS_PROCESSED_PTR:

      if (((ENV_t *) ((DBC_t *) stmt->hdbc)->henv)->dodbc_ver == SQL_OV_ODBC3)
	{
          CALL_UDRIVER(stmt->hdbc, stmt, retcode, hproc, 
            penv->unicode_driver, en_GetStmtAttr, (stmt->dhstmt, Attribute, 
            ValuePtr, BufferLength, StringLengthPtr));
          if (hproc == SQL_NULL_HPROC)
            {
	      PUSHSQLERR (stmt->herr, en_IM001);
	      return SQL_ERROR;
            }
	  return retcode;
	}
      else
	{			/* an ODBC2 driver */
	  if (ValuePtr)
	    *((SQLUINTEGER **) ValuePtr) = (SQLUINTEGER *) stmt->params_processed_ptr;
	  return SQL_SUCCESS;
	}

    case SQL_ATTR_PARAMSET_SIZE:

      if (((ENV_t *) ((DBC_t *) stmt->hdbc)->henv)->dodbc_ver == SQL_OV_ODBC3)
	{
          CALL_UDRIVER(stmt->hdbc, stmt, retcode, hproc, 
            penv->unicode_driver, en_GetStmtAttr, (stmt->dhstmt, Attribute, 
            ValuePtr, BufferLength, StringLengthPtr));
          if (hproc == SQL_NULL_HPROC)
            {
	      PUSHSQLERR (stmt->herr, en_IM001);
	      return SQL_ERROR;
            }
	  if (ValuePtr)
	    stmt->paramset_size = *((SQLUINTEGER *) ValuePtr);
	  return retcode;
	}
      else
	{			/* an ODBC2 driver */
	  if (ValuePtr)
	    *((SQLUINTEGER *) ValuePtr) = stmt->paramset_size;
	  return SQL_SUCCESS;
	}

    case SQL_ATTR_ROW_STATUS_PTR:

      if (((ENV_t *) ((DBC_t *) stmt->hdbc)->henv)->dodbc_ver == SQL_OV_ODBC3)
	{
          CALL_UDRIVER(stmt->hdbc, stmt, retcode, hproc, 
            penv->unicode_driver, en_GetStmtAttr, (stmt->dhstmt, Attribute, 
            ValuePtr, BufferLength, StringLengthPtr));
          if (hproc == SQL_NULL_HPROC)
            {
	      PUSHSQLERR (stmt->herr, en_IM001);
	      return SQL_ERROR;
            }
	  return retcode;
	}
      else
	{			/* an ODBC2 driver */
	  if (ValuePtr)
	    *((SQLUINTEGER **) ValuePtr) = stmt->row_status_allocated == SQL_FALSE ?
		(SQLUINTEGER *) stmt->row_status_ptr :
		NULL;
	  return SQL_SUCCESS;
	}

    case SQL_ATTR_ASYNC_ENABLE:
    case SQL_ATTR_MAX_ROWS:
    case SQL_ATTR_QUERY_TIMEOUT:
    case SQL_ATTR_CONCURRENCY:
    case SQL_ROWSET_SIZE:
    case SQL_ATTR_CURSOR_TYPE:
    case SQL_ATTR_KEYSET_SIZE:
    case SQL_ATTR_NOSCAN:
    case SQL_ATTR_RETRIEVE_DATA:
    case SQL_ATTR_ROW_BIND_TYPE:
    case SQL_ATTR_ROW_NUMBER:
    case SQL_ATTR_SIMULATE_CURSOR:
    case SQL_ATTR_USE_BOOKMARKS:
    case SQL_ATTR_MAX_LENGTH:

      if (((ENV_t *) ((DBC_t *) stmt->hdbc)->henv)->dodbc_ver == SQL_OV_ODBC3)
	{
          CALL_UDRIVER(stmt->hdbc, stmt, retcode, hproc, 
            penv->unicode_driver, en_GetStmtAttr, (stmt->dhstmt, Attribute, 
            ValuePtr, BufferLength, StringLengthPtr));
          if (hproc == SQL_NULL_HPROC)
            {
	      PUSHSQLERR (stmt->herr, en_IM001);
	      return SQL_ERROR;
            }
	  return retcode;
	}
      else
	{			/* an ODBC2 driver */
	  if ((hproc = _iodbcdm_getproc (stmt->hdbc, en_GetStmtOption)) 
	     != SQL_NULL_HPROC)
	    {
	      CALL_DRIVER (stmt->hdbc, stmt, retcode, hproc,
		  (stmt->dhstmt, Attribute, ValuePtr));
	      return retcode;
	    }
	  else
	  if ((hproc = _iodbcdm_getproc (stmt->hdbc, en_GetStmtOptionA)) 
	     != SQL_NULL_HPROC)
	    {
	      CALL_DRIVER (stmt->hdbc, stmt, retcode, hproc,
		  (stmt->dhstmt, Attribute, ValuePtr));
	      return retcode;
	    }
	  else
	    {
	      PUSHSQLERR (stmt->herr, en_IM001);
	      return SQL_ERROR;
	    }
	}

    default:
      CALL_UDRIVER(stmt->hdbc, stmt, retcode, hproc, 
        penv->unicode_driver, en_GetStmtAttr, (stmt->dhstmt, Attribute, ValuePtr, 
        BufferLength, StringLengthPtr));
      if (hproc == SQL_NULL_HPROC)
        {
          PUSHSQLERR (stmt->herr, en_IM001);
	  return SQL_ERROR;
        }
      return retcode;
    }
  return SQL_SUCCESS;
}


RETCODE SQL_API
SQLGetStmtAttr (
  SQLHSTMT		  statementHandle,
  SQLINTEGER		  Attribute,
  SQLPOINTER		  ValuePtr,
  SQLINTEGER		  BufferLength,
  SQLINTEGER		* StringLengthPtr)
{
  ENTER_STMT (statementHandle,
    trace_SQLGetStmtAttr (TRACE_ENTER,
    	statementHandle,
	Attribute,
	ValuePtr, BufferLength, StringLengthPtr));

  retcode = SQLGetStmtAttr_Internal(statementHandle, Attribute, ValuePtr,
    BufferLength, StringLengthPtr, 'A');

  LEAVE_STMT (statementHandle,
    trace_SQLGetStmtAttr (TRACE_LEAVE,
    	statementHandle,
	Attribute,
	ValuePtr, BufferLength, StringLengthPtr));
}


RETCODE SQL_API
SQLGetStmtAttrA (
  SQLHSTMT		  statementHandle,
  SQLINTEGER		  Attribute,
  SQLPOINTER		  ValuePtr,
  SQLINTEGER		  BufferLength,
  SQLINTEGER		* StringLengthPtr)
{
  ENTER_STMT (statementHandle,
    trace_SQLGetStmtAttr (TRACE_ENTER,
    	statementHandle,
	Attribute,
	ValuePtr, BufferLength, StringLengthPtr));

  retcode = SQLGetStmtAttr_Internal(statementHandle, Attribute, ValuePtr,
    BufferLength, StringLengthPtr, 'A');

  LEAVE_STMT (statementHandle,
    trace_SQLGetStmtAttr (TRACE_LEAVE,
    	statementHandle,
	Attribute,
	ValuePtr, BufferLength, StringLengthPtr));
}


RETCODE SQL_API
SQLGetStmtAttrW (
  SQLHSTMT		  statementHandle,
  SQLINTEGER 		  Attribute,
  SQLPOINTER 		  ValuePtr,
  SQLINTEGER 		  BufferLength,
  SQLINTEGER		* StringLengthPtr)
{
  ENTER_STMT (statementHandle,
    trace_SQLGetStmtAttrW (TRACE_ENTER,
    	statementHandle,
	Attribute,
	ValuePtr, BufferLength, StringLengthPtr));

  retcode = SQLGetStmtAttr_Internal (
  	statementHandle, 
	Attribute, 
	ValuePtr, BufferLength, StringLengthPtr, 
	'W');

  LEAVE_STMT (statementHandle,
    trace_SQLGetStmtAttrW (TRACE_LEAVE,
    	statementHandle,
	Attribute,
	ValuePtr, BufferLength, StringLengthPtr));
}


/**** SQLSetStmtAttr ****/

RETCODE SQL_API
SQLSetStmtAttr_Internal (
  SQLHSTMT		  statementHandle,
  SQLINTEGER		  Attribute,
  SQLPOINTER		  ValuePtr,
  SQLINTEGER		  StringLength,
  SQLCHAR		  waMode)
{
  STMT (stmt, statementHandle);
  CONN (pdbc, stmt->hdbc);
  ENVR (penv, pdbc->henv);
  HPROC hproc = SQL_NULL_HPROC;
  SQLRETURN retcode = SQL_SUCCESS;

  waMode = waMode; /*UNUSED*/

  if (stmt->state == en_stmt_needdata)
    {
      PUSHSQLERR (stmt->herr, en_HY010);
      return SQL_ERROR;
    }

  switch (Attribute)
    {

    case SQL_ATTR_APP_PARAM_DESC:

      if (ValuePtr == SQL_NULL_HDESC || ValuePtr == stmt->imp_desc[APP_PARAM_DESC])
	{
	  HDESC hdesc = ValuePtr == SQL_NULL_HDESC ? ValuePtr : stmt->imp_desc[APP_PARAM_DESC]->dhdesc;

          CALL_UDRIVER(stmt->hdbc, stmt, retcode, hproc, 
            penv->unicode_driver, en_SetStmtAttr, (stmt->dhstmt, Attribute, 
            hdesc, StringLength));
          if (hproc == SQL_NULL_HPROC)
            {
	      PUSHSQLERR (stmt->herr, en_IM001);
	      return SQL_ERROR;
            }
	  if (!SQL_SUCCEEDED (retcode))
	    return SQL_ERROR;

	  stmt->desc[APP_PARAM_DESC] = SQL_NULL_HDESC;
	  return SQL_SUCCESS;
	}

      if (!IS_VALID_HDESC (ValuePtr))
	{
	  PUSHSQLERR (stmt->herr, en_HY024);
	  return SQL_ERROR;
	}
      else
	{
	  DESC (pdesc, ValuePtr);
	  if (pdesc->hdbc != stmt->hdbc || IS_VALID_HSTMT (pdesc->hstmt))
	    {
	      PUSHSQLERR (stmt->herr, en_HY017);
	      return SQL_ERROR;
	    }

          CALL_UDRIVER(stmt->hdbc, stmt, retcode, hproc, 
            penv->unicode_driver, en_SetStmtAttr, (stmt->dhstmt, Attribute, 
            pdesc->dhdesc, StringLength));
          if (hproc == SQL_NULL_HPROC)
            {
	      PUSHSQLERR (stmt->herr, en_IM001);
	      return SQL_ERROR;
            }
	  if (!SQL_SUCCEEDED (retcode))
	    return SQL_ERROR;

	  stmt->desc[APP_PARAM_DESC] = (DESC_t *) ValuePtr;
	  return SQL_SUCCESS;
	}

    case SQL_ATTR_APP_ROW_DESC:

      if (ValuePtr == SQL_NULL_HDESC || ValuePtr == stmt->imp_desc[APP_ROW_DESC])
	{
	  HDESC hdesc = ValuePtr == SQL_NULL_HDESC ? ValuePtr : stmt->imp_desc[APP_ROW_DESC]->dhdesc;

          CALL_UDRIVER(stmt->hdbc, stmt, retcode, hproc, 
            penv->unicode_driver, en_SetStmtAttr, (stmt->dhstmt, Attribute, 
            hdesc, StringLength));
          if (hproc == SQL_NULL_HPROC)
            {
	      PUSHSQLERR (stmt->herr, en_IM001);
	      return SQL_ERROR;
            }
	  if (!SQL_SUCCEEDED (retcode))
	    return SQL_ERROR;

	  stmt->desc[APP_ROW_DESC] = SQL_NULL_HDESC;
	  return SQL_SUCCESS;
	}

      if (!IS_VALID_HDESC (ValuePtr))
	{
	  PUSHSQLERR (stmt->herr, en_HY024);
	  return SQL_ERROR;
	}
      else
	{
	  DESC (pdesc, ValuePtr);
	  if (pdesc->hdbc != stmt->hdbc || IS_VALID_HSTMT (pdesc->hstmt))
	    {
	      PUSHSQLERR (stmt->herr, en_HY017);
	      return SQL_ERROR;
	    }

          CALL_UDRIVER(stmt->hdbc, stmt, retcode, hproc, 
            penv->unicode_driver, en_SetStmtAttr, (stmt->dhstmt, Attribute, 
            pdesc->dhdesc, StringLength));
          if (hproc == SQL_NULL_HPROC)
            {
	      PUSHSQLERR (stmt->herr, en_IM001);
	      return SQL_ERROR;
            }
	  if (!SQL_SUCCEEDED (retcode))
	    return SQL_ERROR;

	  stmt->desc[APP_ROW_DESC] = (DESC_t *) ValuePtr;
	  return SQL_SUCCESS;
	}

    case SQL_ATTR_CURSOR_SCROLLABLE:
    case SQL_ATTR_CURSOR_SENSITIVITY:
    case SQL_ATTR_ENABLE_AUTO_IPD:
    case SQL_ATTR_METADATA_ID:
    case SQL_ATTR_PARAM_BIND_OFFSET_PTR:
    case SQL_ATTR_PARAM_BIND_TYPE:
    case SQL_ATTR_PARAM_OPERATION_PTR:
    case SQL_ATTR_PARAM_STATUS_PTR:
    case SQL_ATTR_ROW_BIND_OFFSET_PTR:
    case SQL_ATTR_ROW_OPERATION_PTR:


      CALL_UDRIVER(stmt->hdbc, stmt, retcode, hproc, 
        penv->unicode_driver, en_SetStmtAttr, (stmt->dhstmt, Attribute, 
        ValuePtr, StringLength));
      if (hproc == SQL_NULL_HPROC)
        {
          PUSHSQLERR (stmt->herr, en_IM001);
          return SQL_ERROR;
        }
      if (Attribute == SQL_ATTR_PARAM_BIND_TYPE)
        stmt->param_bind_type = (SQLUINTEGER) (SQLULEN) ValuePtr;
      else if (Attribute == SQL_ATTR_PARAM_BIND_OFFSET_PTR)
        stmt->param_bind_offset = (SQLUINTEGER) (SQLULEN) ValuePtr;
      else if (Attribute == SQL_ATTR_ROW_BIND_OFFSET_PTR)
        stmt->row_bind_offset = (SQLUINTEGER) (SQLULEN) ValuePtr;
      return retcode;

    case SQL_ATTR_ROWS_FETCHED_PTR:

      if (((ENV_t *) ((DBC_t *) stmt->hdbc)->henv)->dodbc_ver == SQL_OV_ODBC3)
	{

          CALL_UDRIVER(stmt->hdbc, stmt, retcode, hproc, 
            penv->unicode_driver, en_SetStmtAttr, (stmt->dhstmt, Attribute, 
            ValuePtr, StringLength));
          if (hproc == SQL_NULL_HPROC)
            {
	      PUSHSQLERR (stmt->herr, en_IM001);
	      return SQL_ERROR;
            }

          return retcode;
	}
      else
	{			/* an ODBC2 driver */
	  stmt->rows_fetched_ptr = ValuePtr;
	  return SQL_SUCCESS;
	}

    case SQL_ATTR_FETCH_BOOKMARK_PTR:

      if (((ENV_t *) ((DBC_t *) stmt->hdbc)->henv)->dodbc_ver == SQL_OV_ODBC3)
	{
          CALL_UDRIVER(stmt->hdbc, stmt, retcode, hproc, 
            penv->unicode_driver, en_SetStmtAttr, (stmt->dhstmt, Attribute, 
            ValuePtr, StringLength));
          if (hproc == SQL_NULL_HPROC)
            {
	      PUSHSQLERR (stmt->herr, en_IM001);
	      return SQL_ERROR;
            }

          return retcode;
	}
      else
	{			/* an ODBC2 driver */
	  stmt->fetch_bookmark_ptr = ValuePtr;
	  return SQL_SUCCESS;
	}

    case SQL_ATTR_PARAMS_PROCESSED_PTR:

      if (((ENV_t *) ((DBC_t *) stmt->hdbc)->henv)->dodbc_ver == SQL_OV_ODBC3)
	{
          CALL_UDRIVER(stmt->hdbc, stmt, retcode, hproc, 
            penv->unicode_driver, en_SetStmtAttr, (stmt->dhstmt, Attribute, 
            ValuePtr, StringLength));
          if (hproc == SQL_NULL_HPROC)
            {
	      PUSHSQLERR (stmt->herr, en_IM001);
	      return SQL_ERROR;
            }

          return retcode;
	}
      else
	{			/* an ODBC2 driver */
	  stmt->params_processed_ptr = ValuePtr;
	  return SQL_SUCCESS;
	}

    case SQL_ATTR_PARAMSET_SIZE:

      if (((ENV_t *) ((DBC_t *) stmt->hdbc)->henv)->dodbc_ver == SQL_OV_ODBC3)
	{
          CALL_UDRIVER(stmt->hdbc, stmt, retcode, hproc, 
            penv->unicode_driver, en_SetStmtAttr, (stmt->dhstmt, Attribute, 
            ValuePtr, StringLength));
          if (hproc == SQL_NULL_HPROC)
            {
	      PUSHSQLERR (stmt->herr, en_IM001);
	      return SQL_ERROR;
            }
	  stmt->paramset_size = (SQLUINTEGER) (SQLULEN) ValuePtr;
          return retcode;
	}
      else
	{			/* an ODBC2 driver */
	  stmt->paramset_size = (SQLUINTEGER) (SQLULEN) ValuePtr;
	  return SQL_SUCCESS;
	}

    case SQL_ATTR_ROW_ARRAY_SIZE:

      if (((ENV_t *) ((DBC_t *) stmt->hdbc)->henv)->dodbc_ver == SQL_OV_ODBC3)
	{
          CALL_UDRIVER(stmt->hdbc, stmt, retcode, hproc, 
            penv->unicode_driver, en_SetStmtAttr, (stmt->dhstmt, Attribute, 
            ValuePtr, StringLength));
          if (hproc == SQL_NULL_HPROC)
            {
	      PUSHSQLERR (stmt->herr, en_IM001);
	      return SQL_ERROR;
            }

          if (SQL_SUCCEEDED (retcode))
            {
              stmt->rowset_size = (SQLUINTEGER) (SQLULEN) ValuePtr;
              if (retcode == SQL_SUCCESS_WITH_INFO)
                {
                  SQLUINTEGER data;
                  if (SQLGetStmtAttr_Internal (statementHandle, SQL_ROWSET_SIZE, &data, 
                        0, NULL, 'A') == SQL_SUCCESS)
                    stmt->rowset_size = data;
                }
            }
          return retcode;
	}
      else
	{			/* an ODBC2 driver */
	  if ((SQLUINTEGER) (SQLULEN) ValuePtr < 1)
	    {
	      PUSHSQLERR (stmt->herr, en_HY024);
	      return SQL_ERROR;
	    }
	  stmt->row_array_size = (SQLUINTEGER) (SQLULEN) ValuePtr;

	  /* reallocate the row_status_ptr */
	  if (stmt->row_status_ptr && stmt->row_status_allocated == SQL_TRUE)
	    {
	      MEM_FREE (stmt->row_status_ptr);
	      stmt->row_status_allocated = SQL_FALSE;
	    }
	  stmt->row_status_ptr = MEM_ALLOC (sizeof (SQLUINTEGER) * stmt->row_array_size);
	  if (!stmt->row_status_ptr)
	    {
	      PUSHSQLERR (stmt->herr, en_HY001);
	      return SQL_ERROR;
	    }
	  stmt->row_status_allocated = SQL_TRUE;

	  /*
	   *  Tell the driver the rowset size has changed
	   */
	  if ((hproc = _iodbcdm_getproc (stmt->hdbc, en_SetStmtOption))
	      != SQL_NULL_HPROC)
	    {
	      CALL_DRIVER (stmt->hdbc, stmt, retcode, hproc,
		  (stmt->dhstmt, SQL_ROWSET_SIZE, stmt->row_array_size));

              if (SQL_SUCCEEDED (retcode))
                {
                  stmt->rowset_size = (SQLUINTEGER) (SQLULEN) ValuePtr;
                  if (retcode == SQL_SUCCESS_WITH_INFO)
                    {
                      SQLUINTEGER data;
                      if (SQLGetStmtOption_Internal (statementHandle, SQL_ROWSET_SIZE, 
                            &data) == SQL_SUCCESS)
                        stmt->rowset_size = data;
                    }
                }
	      return retcode;
	    }
	  else
	  if ((hproc = _iodbcdm_getproc (stmt->hdbc, en_SetStmtOptionA))
	      != SQL_NULL_HPROC)
	    {
	      CALL_DRIVER (stmt->hdbc, stmt, retcode, hproc,
		  (stmt->dhstmt, SQL_ROWSET_SIZE, stmt->row_array_size));

              if (SQL_SUCCEEDED (retcode))
                {
                  stmt->rowset_size = (SQLUINTEGER) (SQLULEN) ValuePtr;
                  if (retcode == SQL_SUCCESS_WITH_INFO)
                    {
                      SQLUINTEGER data;
                      if (SQLGetStmtOption_Internal (statementHandle, SQL_ROWSET_SIZE, 
                           &data) == SQL_SUCCESS)
                        stmt->rowset_size = data;
                    }
                }
	      return retcode;
	    }
	  else
	    {
	      PUSHSQLERR (stmt->herr, en_IM001);
	      return SQL_ERROR;
	    }

	  return SQL_SUCCESS;
	}

    case SQL_ATTR_ROW_STATUS_PTR:

      if (((ENV_t *) ((DBC_t *) stmt->hdbc)->henv)->dodbc_ver == SQL_OV_ODBC3)
	{
          CALL_UDRIVER(stmt->hdbc, stmt, retcode, hproc, 
            penv->unicode_driver, en_SetStmtAttr, (stmt->dhstmt, Attribute, 
            ValuePtr, StringLength));
          if (hproc == SQL_NULL_HPROC)
            {
	      PUSHSQLERR (stmt->herr, en_IM001);
	      return SQL_ERROR;
            }

          return retcode;
	}
      else
	{			/* an ODBC2 driver */
	  /* free the implicit row status ptr (if any) */
	  if (stmt->row_status_ptr && stmt->row_status_allocated == SQL_TRUE)
	    {
	      MEM_FREE (stmt->row_status_ptr);
	      stmt->row_status_allocated = SQL_FALSE;
	    }
	  stmt->row_status_ptr = ValuePtr;
	  return SQL_SUCCESS;
	}

    case SQL_ATTR_ASYNC_ENABLE:
    case SQL_ATTR_CONCURRENCY:
    case SQL_ATTR_CURSOR_TYPE:
    case SQL_ATTR_KEYSET_SIZE:
    case SQL_ATTR_MAX_ROWS:
    case SQL_ATTR_NOSCAN:
    case SQL_ATTR_QUERY_TIMEOUT:
    case SQL_ATTR_RETRIEVE_DATA:
    case SQL_ATTR_ROW_BIND_TYPE:
    case SQL_ATTR_ROW_NUMBER:
    case SQL_ATTR_SIMULATE_CURSOR:
    case SQL_ATTR_USE_BOOKMARKS:
    case SQL_ROWSET_SIZE:
    case SQL_ATTR_MAX_LENGTH:

      if (((ENV_t *) ((DBC_t *) stmt->hdbc)->henv)->dodbc_ver == SQL_OV_ODBC3)
	{
          CALL_UDRIVER(stmt->hdbc, stmt, retcode, hproc, 
            penv->unicode_driver, en_SetStmtAttr, (stmt->dhstmt, Attribute, 
            ValuePtr, StringLength));
          if (hproc == SQL_NULL_HPROC)
            {
	      PUSHSQLERR (stmt->herr, en_IM001);
	      return SQL_ERROR;
            }

          if (Attribute == SQL_ATTR_ROW_BIND_TYPE && SQL_SUCCEEDED (retcode))
            stmt->row_bind_type = (SQLUINTEGER) (SQLULEN) ValuePtr;

          return retcode;
	}
      else
	{			/* an ODBC2 driver */
	  if ((hproc = _iodbcdm_getproc (stmt->hdbc, en_SetStmtOption))
	      != SQL_NULL_HPROC)
	    {
	      CALL_DRIVER (stmt->hdbc, stmt, retcode, hproc,
		  (stmt->dhstmt, Attribute, ValuePtr));
              if (Attribute == SQL_ATTR_ROW_BIND_TYPE && SQL_SUCCEEDED (retcode))
                stmt->row_bind_type = (SQLUINTEGER) (SQLULEN) ValuePtr;
	      return retcode;
	    }
	  else
	  if ((hproc = _iodbcdm_getproc (stmt->hdbc, en_SetStmtOptionA))
	      != SQL_NULL_HPROC)
	    {
	      CALL_DRIVER (stmt->hdbc, stmt, retcode, hproc,
		  (stmt->dhstmt, Attribute, ValuePtr));
              if (Attribute == SQL_ATTR_ROW_BIND_TYPE && SQL_SUCCEEDED (retcode))
                stmt->row_bind_type = (SQLUINTEGER) (SQLULEN) ValuePtr;
	      return retcode;
	    }
	  else
	    {
	      PUSHSQLERR (stmt->herr, en_IM001);
	      return SQL_ERROR;
	    }
	}
    default:
      if ((hproc = _iodbcdm_getproc (stmt->hdbc, en_SetStmtOption))
         != SQL_NULL_HPROC)
	{
	  CALL_DRIVER (stmt->hdbc, stmt, retcode, hproc,
	      (stmt->dhstmt, Attribute, ValuePtr));
	  return retcode;
	}
      else
      if ((hproc = _iodbcdm_getproc (stmt->hdbc, en_SetStmtOptionA))
         != SQL_NULL_HPROC)
	{
	  CALL_DRIVER (stmt->hdbc, stmt, retcode, hproc,
	      (stmt->dhstmt, Attribute, ValuePtr));

	  return retcode;
	}
      else
	{
	  PUSHSQLERR (stmt->herr, en_IM001);

	  return SQL_ERROR;
	}
    }

  return SQL_SUCCESS;
}


RETCODE SQL_API
SQLSetStmtAttr (
  SQLHSTMT 		  statementHandle,
  SQLINTEGER		  Attribute,
  SQLPOINTER		  ValuePtr,
  SQLINTEGER		  StringLength)
{
  ENTER_STMT (statementHandle,
    trace_SQLSetStmtAttr (TRACE_ENTER,
    	statementHandle,
	Attribute,
	ValuePtr,
	StringLength));

  retcode =  SQLSetStmtAttr_Internal(
  	statementHandle, 
	Attribute, 
	ValuePtr, 
	StringLength, 
	'A');

  LEAVE_STMT (statementHandle,
    trace_SQLSetStmtAttr (TRACE_LEAVE,
    	statementHandle,
	Attribute,
	ValuePtr,
	StringLength));
}


RETCODE SQL_API
SQLSetStmtAttrA (
  SQLHSTMT		  statementHandle,
  SQLINTEGER		  Attribute,
  SQLPOINTER		  ValuePtr,
  SQLINTEGER		  StringLength)
{
  ENTER_STMT (statementHandle,
    trace_SQLSetStmtAttr (TRACE_ENTER,
    	statementHandle,
	Attribute,
	ValuePtr,
	StringLength));

  retcode =  SQLSetStmtAttr_Internal(
  	statementHandle, 
	Attribute, 
	ValuePtr, 
	StringLength, 
	'A');

  LEAVE_STMT (statementHandle,
    trace_SQLSetStmtAttr (TRACE_LEAVE,
    	statementHandle,
	Attribute,
	ValuePtr,
	StringLength));
}


RETCODE SQL_API
SQLSetStmtAttrW (
  SQLHSTMT		  statementHandle,
  SQLINTEGER		  Attribute,
  SQLPOINTER		  ValuePtr,
  SQLINTEGER		  StringLength)
{
  ENTER_STMT (statementHandle,
    trace_SQLSetStmtAttrW (TRACE_ENTER,
    	statementHandle,
	Attribute,
	ValuePtr,
	StringLength));

  retcode =  SQLSetStmtAttr_Internal(
  	statementHandle, 
	Attribute, 
	ValuePtr, 
	StringLength, 
	'W');

  LEAVE_STMT (statementHandle,
    trace_SQLSetStmtAttrW (TRACE_LEAVE,
    	statementHandle,
	Attribute,
	ValuePtr,
	StringLength));
}


static RETCODE
SQLSetConnectAttr_Internal (
  SQLHDBC		  connectionHandle,
  SQLINTEGER		  Attribute,
  SQLPOINTER		  ValuePtr,
  SQLINTEGER		  StringLength,
  SQLCHAR		  waMode)
{
  CONN (con, connectionHandle);
  ENVR (penv, con->henv);
  HPROC hproc = SQL_NULL_HPROC;
  HPROC hproc2 = SQL_NULL_HPROC;
  SQLRETURN retcode = SQL_SUCCESS;
  void * _ValuePtr = NULL;
  SWORD unicode_driver = (penv ? penv->unicode_driver : 0);
  SQLUINTEGER odbc_ver;
  SQLUINTEGER dodbc_ver;
  CONV_DIRECT conv_direct = CD_NONE;
  DM_CONV *conv = &con->conv;

  odbc_ver = ((GENV_t *) con->genv)->odbc_ver;
  dodbc_ver = (penv != SQL_NULL_HENV) ? penv->dodbc_ver : odbc_ver;

  if (con->state == en_dbc_needdata)
    {
      PUSHSQLERR (con->herr, en_HY010);
      return SQL_ERROR;
    }

  if (unicode_driver && waMode != 'W')
    conv_direct = CD_A2W;
  else if (!unicode_driver && waMode == 'W')
    conv_direct = CD_W2A;
  else if (waMode == 'W' && conv->dm_cp != conv->drv_cp)
    conv_direct = CD_W2W;

  if (conv_direct != CD_NONE)
    {
      switch(Attribute)
        {
        case SQL_ATTR_CURRENT_CATALOG:
        case SQL_ATTR_TRACEFILE:
        case SQL_ATTR_TRANSLATE_LIB:

          if (waMode == 'W')
            StringLength = (StringLength != SQL_NTS) ? (SQLINTEGER) (StringLength / DM_WCHARSIZE(conv)) : SQL_NTS;
          _ValuePtr = conv_text_m2d (conv, ValuePtr, StringLength, conv_direct);

          ValuePtr = _ValuePtr;
          StringLength = SQL_NTS;
          break;
        }
    }

  GET_UHPROC(con, hproc2, en_SetConnectOption, unicode_driver);

  if (dodbc_ver == SQL_OV_ODBC3 &&
      (  odbc_ver == SQL_OV_ODBC3 
       || (odbc_ver == SQL_OV_ODBC2 && hproc2 == SQL_NULL_HPROC)))
    {
      CALL_UDRIVER(con, con, retcode, hproc, unicode_driver,
        en_SetConnectAttr, (con->dhdbc, Attribute, ValuePtr, StringLength));
      if (hproc != SQL_NULL_HPROC)
        {
          if (retcode == SQL_SUCCESS && Attribute == SQL_ATTR_APP_WCHAR_TYPE)
            {
              switch((SQLINTEGER)ValuePtr)
                {
                case CP_UTF16:
                  penv->conv.drv_cp = (IODBC_CHARSET)ValuePtr;
                  break;

                case CP_UTF8:
                  penv->conv.drv_cp = (IODBC_CHARSET)ValuePtr;
                  break;

                case CP_UCS4:
                  penv->conv.drv_cp = (IODBC_CHARSET)ValuePtr;
                  break;
                }
            }
          return retcode;
        }
    }

  switch (Attribute)
    {

    case SQL_ATTR_AUTO_IPD:
      PUSHSQLERR (con->herr, en_HY092);
      return SQL_ERROR;

    default:
      retcode = _iodbcdm_SetConnectOption (con, Attribute, 
      	  (SQLULEN) ValuePtr, waMode);
      return retcode;
    }
}


RETCODE SQL_API
SQLSetConnectAttr (
  SQLHDBC		  connectionHandle,
  SQLINTEGER		  Attribute,
  SQLPOINTER		  ValuePtr,
  SQLINTEGER		  StringLength)
{
  ENTER_HDBC (connectionHandle, 0,
    trace_SQLSetConnectAttr (TRACE_ENTER, 
    	connectionHandle,
	Attribute,
	ValuePtr, StringLength));

  retcode = SQLSetConnectAttr_Internal (
  	connectionHandle, 
	Attribute, 
	ValuePtr, StringLength, 
	'A');

  LEAVE_HDBC (connectionHandle, 0,
    trace_SQLSetConnectAttr (TRACE_LEAVE, 
    	connectionHandle,
	Attribute,
	ValuePtr, StringLength));
}


RETCODE SQL_API
SQLSetConnectAttrA (
  SQLHDBC		  connectionHandle,
  SQLINTEGER		  Attribute,
  SQLPOINTER		  ValuePtr,
  SQLINTEGER		  StringLength)
{
  ENTER_HDBC (connectionHandle, 0,
    trace_SQLSetConnectAttr (TRACE_ENTER, 
    	connectionHandle,
	Attribute,
	ValuePtr, StringLength));

  retcode = SQLSetConnectAttr_Internal (
  	connectionHandle, 
	Attribute, 
	ValuePtr, StringLength, 
	'A');

  LEAVE_HDBC (connectionHandle, 0,
    trace_SQLSetConnectAttr (TRACE_LEAVE, 
    	connectionHandle,
	Attribute,
	ValuePtr, StringLength));
}


RETCODE SQL_API
SQLSetConnectAttrW (
  SQLHDBC		  connectionHandle,
  SQLINTEGER		  Attribute,
  SQLPOINTER		  ValuePtr,
  SQLINTEGER		  StringLength)
{
  ENTER_HDBC (connectionHandle, 0,
    trace_SQLSetConnectAttrW (TRACE_ENTER, 
    	connectionHandle,
	Attribute,
	ValuePtr, StringLength));

  retcode = SQLSetConnectAttr_Internal (
  	connectionHandle, 
	Attribute, 
	ValuePtr, StringLength, 
	'W');

  LEAVE_HDBC (connectionHandle, 0,
    trace_SQLSetConnectAttrW (TRACE_LEAVE, 
    	connectionHandle,
	Attribute,
	ValuePtr, StringLength));
}


static RETCODE 
SQLGetConnectAttr_Internal (
  SQLHDBC		  connectionHandle,
  SQLINTEGER		  Attribute,
  SQLPOINTER		  ValuePtr,
  SQLINTEGER		  StringLength,
  SQLINTEGER		* StringLengthPtr,
  SQLCHAR 		  waMode)
{
  CONN (con, connectionHandle);
  ENVR (penv, con->henv);
  HPROC hproc = SQL_NULL_HPROC;
  HPROC hproc2 = SQL_NULL_HPROC;
  RETCODE retcode = SQL_SUCCESS;
  void * _Value = NULL;
  void * valueOut = ValuePtr;
  SWORD unicode_driver = (penv ? penv->unicode_driver : 0);
  SQLUINTEGER odbc_ver;
  SQLUINTEGER dodbc_ver;
  CONV_DIRECT conv_direct = CD_NONE;
  DM_CONV *conv = &con->conv;
  SQLINTEGER _StringLength = StringLength;

  odbc_ver = ((GENV_t *) con->genv)->odbc_ver;
  dodbc_ver = (penv != SQL_NULL_HENV) ? penv->dodbc_ver : odbc_ver;

  if (con->state == en_dbc_needdata)
    {
      PUSHSQLERR (con->herr, en_HY010);
      return SQL_ERROR;
    }

  if (unicode_driver && waMode != 'W')
    conv_direct = CD_A2W;
  else if (!unicode_driver && waMode == 'W')
    conv_direct = CD_W2A;
  else if (waMode == 'W' && conv->dm_cp != conv->drv_cp)
    conv_direct = CD_W2W;

  if (penv && conv_direct != CD_NONE)
    {
      switch(Attribute)
        {
        case SQL_ATTR_CURRENT_CATALOG:
        case SQL_ATTR_TRACEFILE:
        case SQL_ATTR_TRANSLATE_LIB:
          if (conv_direct == CD_A2W || conv_direct == CD_W2W)
            {
              /*ansi<=unicode*/
              if (conv_direct == CD_W2W)
                _StringLength /= DM_WCHARSIZE(conv);

              if ((_Value = malloc((_StringLength + 1) * DRV_WCHARSIZE_ALLOC(conv))) == NULL)
	        {
                  PUSHSQLERR (con->herr, en_HY001);
                  return SQL_ERROR;
                }
              _StringLength *= DRV_WCHARSIZE_ALLOC(conv);
              valueOut = _Value;
            }
          else if (conv_direct == CD_W2A)
            {
              /*unicode<=ansi*/
              if ((_Value = malloc(StringLength + 1)) == NULL)
	        {
                  PUSHSQLERR (con->herr, en_HY001);
                  return SQL_ERROR;
                }
              _StringLength /= DM_WCHARSIZE(conv);
              valueOut = _Value;
            }
          break;
        }
    }

  GET_UHPROC(con, hproc2, en_GetConnectOption, unicode_driver);

  if (dodbc_ver == SQL_OV_ODBC3 &&
      (  odbc_ver == SQL_OV_ODBC3 
       || (odbc_ver == SQL_OV_ODBC2 && hproc2 == SQL_NULL_HPROC)))
    {
      CALL_UDRIVER(con, con, retcode, hproc, unicode_driver,
        en_GetConnectAttr, (con->dhdbc, Attribute, valueOut, _StringLength,
        StringLengthPtr));
    }

  if (hproc != SQL_NULL_HPROC)
    {
      if (ValuePtr && conv_direct != CD_NONE && SQL_SUCCEEDED (retcode))
        {
          int count;
          switch(Attribute)
            {
            case SQL_ATTR_CURRENT_CATALOG:
            case SQL_ATTR_TRACEFILE:
            case SQL_ATTR_TRANSLATE_LIB:
              if (conv_direct == CD_A2W)
                {
                  /*ansi<=unicode*/
                  dm_StrCopyOut2_W2A_d2m (conv, valueOut,
			(SQLCHAR *) ValuePtr,
			StringLength, NULL, &count);
                  if (StringLengthPtr)
                    *StringLengthPtr = (SQLSMALLINT)count;
                }
              else if (conv_direct == CD_W2A)
                {
                  /*unicode<=ansi*/
                  dm_StrCopyOut2_A2W_d2m (conv, (SQLCHAR *) valueOut,
			ValuePtr, StringLength, NULL, &count);
                  if (StringLengthPtr)
                    *StringLengthPtr = (SQLSMALLINT)count;
                }
              else if (conv_direct == CD_W2W)
                {
                  /*unicode<=unicode*/
                  dm_StrCopyOut2_W2W_d2m (conv, valueOut,
			ValuePtr, StringLength, NULL, &count);
                  if (StringLengthPtr)
                    *StringLengthPtr = (SQLSMALLINT)count;
                }
            }
        }
      MEM_FREE(_Value);
      return retcode;
    }
  MEM_FREE(_Value);

  retcode = _iodbcdm_GetConnectOption (con, Attribute, ValuePtr, waMode);
  if (!SQL_SUCCEEDED (retcode))
    return retcode;

  if (StringLengthPtr)
    {
      *StringLengthPtr = 0;
      if (ValuePtr)
	switch (Attribute)
	  {
	  case SQL_ATTR_CURRENT_CATALOG:
	  case SQL_ATTR_TRACEFILE:
	  case SQL_ATTR_TRANSLATE_LIB:
	    if (waMode != 'W')
	      *StringLengthPtr = STRLEN (ValuePtr);
	    else
	      *StringLengthPtr = DM_WCSLEN (conv, ValuePtr) * DM_WCHARSIZE(conv);
	  }
    }
  return retcode;
}


RETCODE SQL_API
SQLGetConnectAttr (
  SQLHDBC 		  connectionHandle,
  SQLINTEGER 		  Attribute,
  SQLPOINTER 		  ValuePtr,
  SQLINTEGER 		  StringLength,
  SQLINTEGER		* StringLengthPtr)
{
  ENTER_HDBC (connectionHandle, 0,
    trace_SQLGetConnectAttr (TRACE_ENTER,
  	connectionHandle, 
	Attribute, 
	ValuePtr, StringLength, StringLengthPtr));

  retcode =  SQLGetConnectAttr_Internal(
  	connectionHandle, 
	Attribute, 
	ValuePtr, StringLength, StringLengthPtr, 
	'A');

  LEAVE_HDBC (connectionHandle, 0,
    trace_SQLGetConnectAttr (TRACE_LEAVE,
  	connectionHandle, 
	Attribute, 
	ValuePtr, StringLength, StringLengthPtr));
}


RETCODE SQL_API
SQLGetConnectAttrA (
  SQLHDBC 		  connectionHandle,
  SQLINTEGER 		  Attribute,
  SQLPOINTER 		  ValuePtr,
  SQLINTEGER 		  StringLength,
  SQLINTEGER		* StringLengthPtr)
{
  ENTER_HDBC (connectionHandle, 0,
    trace_SQLGetConnectAttr (TRACE_ENTER,
  	connectionHandle, 
	Attribute, 
	ValuePtr, StringLength, StringLengthPtr));

  retcode =  SQLGetConnectAttr_Internal(
  	connectionHandle, 
	Attribute, 
	ValuePtr, StringLength, StringLengthPtr, 
	'A');

  LEAVE_HDBC (connectionHandle, 0,
    trace_SQLGetConnectAttr (TRACE_LEAVE,
  	connectionHandle, 
	Attribute, 
	ValuePtr, StringLength, StringLengthPtr));
}


RETCODE SQL_API
SQLGetConnectAttrW (SQLHDBC connectionHandle,
    SQLINTEGER Attribute,
    SQLPOINTER ValuePtr,
    SQLINTEGER StringLength,
    SQLINTEGER * StringLengthPtr)
{
  ENTER_HDBC (connectionHandle, 0,
    trace_SQLGetConnectAttrW (TRACE_ENTER,
  	connectionHandle, 
	Attribute, 
	ValuePtr, StringLength, StringLengthPtr));

  retcode =  SQLGetConnectAttr_Internal(
  	connectionHandle, 
	Attribute, 
	ValuePtr, StringLength, StringLengthPtr, 
	'W');

  LEAVE_HDBC (connectionHandle, 0,
    trace_SQLGetConnectAttrW (TRACE_LEAVE,
  	connectionHandle, 
	Attribute, 
	ValuePtr, StringLength, StringLengthPtr));
}


RETCODE SQL_API
SQLGetDescField_Internal (
  SQLHDESC		  descriptorHandle,
  SQLSMALLINT		  RecNumber,
  SQLSMALLINT		  FieldIdentifier,
  SQLPOINTER		  ValuePtr,
  SQLINTEGER		  BufferLength,
  SQLINTEGER		* StringLengthPtr,
  SQLCHAR 		  waMode)
{
  DESC (desc, descriptorHandle);
  CONN (pdbc, desc->hdbc);
  ENVR (penv, pdbc->henv);
  HPROC hproc = SQL_NULL_HPROC;
  SQLRETURN retcode = SQL_SUCCESS;
  void * valueOut = ValuePtr;
  void * _ValuePtr = NULL;
  CONV_DIRECT conv_direct = CD_NONE;
  DM_CONV *conv = &pdbc->conv;
  SQLINTEGER _BufferLength = BufferLength;

  if (penv->unicode_driver && waMode != 'W')
    conv_direct = CD_A2W;
  else if (!penv->unicode_driver && waMode == 'W')
    conv_direct = CD_W2A;
  else if (waMode == 'W' && conv->dm_cp != conv->drv_cp)
    conv_direct = CD_W2W;

  if (conv_direct != CD_NONE)
    {
      switch(FieldIdentifier)
        {
        case SQL_DESC_BASE_COLUMN_NAME:
        case SQL_DESC_BASE_TABLE_NAME:
        case SQL_DESC_CATALOG_NAME:
        case SQL_DESC_LABEL:
        case SQL_DESC_LITERAL_PREFIX:
        case SQL_DESC_LITERAL_SUFFIX:
        case SQL_DESC_LOCAL_TYPE_NAME:
        case SQL_DESC_NAME:
        case SQL_DESC_SCHEMA_NAME:
        case SQL_DESC_TABLE_NAME:
        case SQL_DESC_TYPE_NAME:
          if (conv_direct == CD_A2W || conv_direct == CD_W2W)
            {
             /*ansi<=unicode*/
              if (conv_direct == CD_W2W)
                _BufferLength /= DM_WCHARSIZE(conv);

              if ((_ValuePtr = malloc((_BufferLength + 1) * DRV_WCHARSIZE_ALLOC(conv))) == NULL)
	        {
                  PUSHSQLERR (desc->herr, en_HY001);
                  return SQL_ERROR;
                }
              _BufferLength *= DRV_WCHARSIZE_ALLOC(conv);
              valueOut = _ValuePtr;
            }
          else if (conv_direct == CD_W2A)
            {
              /*unicode<=ansi*/
              if ((_ValuePtr = malloc(_BufferLength + 1)) == NULL)
	        {
                  PUSHSQLERR (desc->herr, en_HY001);
                  return SQL_ERROR;
                }
              _BufferLength /= DM_WCHARSIZE(conv);
              valueOut = _ValuePtr;
            }
          break;
        }
    }

  CALL_UDRIVER(desc->hdbc, desc, retcode, hproc, penv->unicode_driver,
    en_GetDescField, (desc->dhdesc, RecNumber, FieldIdentifier, valueOut,
    _BufferLength, StringLengthPtr));

  if (hproc == SQL_NULL_HPROC)
    {
      MEM_FREE(_ValuePtr);
      PUSHSQLERR (desc->herr, en_IM001);
      return SQL_ERROR;
    }

  if (ValuePtr && conv_direct != CD_NONE && SQL_SUCCEEDED (retcode))
    {
      int count;
      switch(FieldIdentifier)
        {
        case SQL_DESC_BASE_COLUMN_NAME:
        case SQL_DESC_BASE_TABLE_NAME:
        case SQL_DESC_CATALOG_NAME:
        case SQL_DESC_LABEL:
        case SQL_DESC_LITERAL_PREFIX:
        case SQL_DESC_LITERAL_SUFFIX:
        case SQL_DESC_LOCAL_TYPE_NAME:
        case SQL_DESC_NAME:
        case SQL_DESC_SCHEMA_NAME:
        case SQL_DESC_TABLE_NAME:
        case SQL_DESC_TYPE_NAME:
          if (conv_direct == CD_A2W)
            {
            /* ansi<=unicode*/
              dm_StrCopyOut2_W2A_d2m (conv, valueOut,
		(SQLCHAR *) ValuePtr, BufferLength, NULL, &count);
              if (StringLengthPtr)
                *StringLengthPtr = (SQLSMALLINT)count;
            }
          else if (conv_direct == CD_W2A)
            {
            /* unicode<=ansi*/
              dm_StrCopyOut2_A2W_d2m (conv, (SQLCHAR *) valueOut,
		ValuePtr, BufferLength, NULL, &count);
              if (StringLengthPtr)
                *StringLengthPtr = (SQLSMALLINT)count;
            }
          else if (conv_direct == CD_W2W)
            {
            /* unicode<=unicode*/
              dm_StrCopyOut2_W2W_d2m (conv, valueOut,
		ValuePtr, BufferLength, NULL, &count);
              if (StringLengthPtr)
                *StringLengthPtr = (SQLSMALLINT)count;
            }
          break;
        }
    }

  MEM_FREE(_ValuePtr);

  return retcode;
}


RETCODE SQL_API
SQLGetDescField (
  SQLHDESC		  descriptorHandle,
  SQLSMALLINT		  RecNumber,
  SQLSMALLINT		  FieldIdentifier,
  SQLPOINTER		  ValuePtr,
  SQLINTEGER		  BufferLength,
  SQLINTEGER		* StringLengthPtr)
{
  ENTER_DESC (descriptorHandle,
    trace_SQLGetDescField (TRACE_ENTER,
    	descriptorHandle,
	RecNumber,
	FieldIdentifier,
	ValuePtr, BufferLength, StringLengthPtr));

  retcode = SQLGetDescField_Internal(
  	descriptorHandle, 
	RecNumber, 
	FieldIdentifier, 
	ValuePtr, BufferLength, StringLengthPtr, 
	'A');

  LEAVE_DESC (descriptorHandle,
    trace_SQLGetDescField (TRACE_LEAVE,
    	descriptorHandle,
	RecNumber,
	FieldIdentifier,
	ValuePtr, BufferLength, StringLengthPtr));
}


RETCODE SQL_API
SQLGetDescFieldA (
  SQLHDESC		  descriptorHandle,
  SQLSMALLINT		  RecNumber,
  SQLSMALLINT		  FieldIdentifier,
  SQLPOINTER		  ValuePtr,
  SQLINTEGER		  BufferLength,
  SQLINTEGER		* StringLengthPtr)
{
  ENTER_DESC (descriptorHandle,
    trace_SQLGetDescField (TRACE_ENTER,
    	descriptorHandle,
	RecNumber,
	FieldIdentifier,
	ValuePtr, BufferLength, StringLengthPtr));

  retcode = SQLGetDescField_Internal (
  	descriptorHandle, 
	RecNumber, 
	FieldIdentifier, 
	ValuePtr, BufferLength, StringLengthPtr, 
	'A');

  LEAVE_DESC (descriptorHandle,
    trace_SQLGetDescField (TRACE_LEAVE,
    	descriptorHandle,
	RecNumber,
	FieldIdentifier,
	ValuePtr, BufferLength, StringLengthPtr));
}


RETCODE SQL_API
SQLGetDescFieldW (
  SQLHDESC		  descriptorHandle,
  SQLSMALLINT		  RecNumber,
  SQLSMALLINT		  FieldIdentifier,
  SQLPOINTER		  ValuePtr,
  SQLINTEGER		  BufferLength,
  SQLINTEGER		* StringLengthPtr)
{
  ENTER_DESC (descriptorHandle,
    trace_SQLGetDescFieldW (TRACE_ENTER,
    	descriptorHandle,
	RecNumber,
	FieldIdentifier,
	ValuePtr, BufferLength, StringLengthPtr));

  retcode = SQLGetDescField_Internal (
  	descriptorHandle, 
	RecNumber, 
	FieldIdentifier, 
	ValuePtr, BufferLength, StringLengthPtr, 
	'W');

  LEAVE_DESC (descriptorHandle,
    trace_SQLGetDescFieldW (TRACE_LEAVE,
    	descriptorHandle,
	RecNumber,
	FieldIdentifier,
	ValuePtr, BufferLength, StringLengthPtr));
}


RETCODE SQL_API
SQLSetDescField_Internal (
  SQLHDESC		  descriptorHandle,
  SQLSMALLINT		  RecNumber,
  SQLSMALLINT		  FieldIdentifier,
  SQLPOINTER		  ValuePtr,
  SQLINTEGER		  BufferLength,
  SQLCHAR		  waMode)
{
  DESC (desc, descriptorHandle);
  CONN (pdbc, desc->hdbc);
  ENVR (penv, pdbc->henv);
  HPROC hproc = SQL_NULL_HPROC;
  SQLRETURN retcode = SQL_SUCCESS;
  void * _ValuePtr = NULL;
  CONV_DIRECT conv_direct = CD_NONE;
  DM_CONV *conv = &pdbc->conv;

  if (penv->unicode_driver && waMode != 'W')
    conv_direct = CD_A2W;
  else if (!penv->unicode_driver && waMode == 'W')
    conv_direct = CD_W2A;
  else if (waMode == 'W' && conv->dm_cp != conv->drv_cp)
    conv_direct = CD_W2W;

  if (conv_direct != CD_NONE)
    {
      switch(FieldIdentifier)
        {
        case SQL_DESC_BASE_COLUMN_NAME:
        case SQL_DESC_BASE_TABLE_NAME:
        case SQL_DESC_CATALOG_NAME:
        case SQL_DESC_LABEL:
        case SQL_DESC_LITERAL_PREFIX:
        case SQL_DESC_LITERAL_SUFFIX:
        case SQL_DESC_LOCAL_TYPE_NAME:
        case SQL_DESC_NAME:
        case SQL_DESC_SCHEMA_NAME:
        case SQL_DESC_TABLE_NAME:
        case SQL_DESC_TYPE_NAME:
          if (waMode == 'W')
            BufferLength = (BufferLength != SQL_NTS) ? (SQLINTEGER) (BufferLength / DM_WCHARSIZE(conv)) : SQL_NTS;
          _ValuePtr = conv_text_m2d (conv, ValuePtr, BufferLength, conv_direct);
          ValuePtr = _ValuePtr;
          BufferLength = SQL_NTS;
          break;
        }
    }

  CALL_UDRIVER(desc->hdbc, desc, retcode, hproc, penv->unicode_driver, 
    en_SetDescField, (desc->dhdesc, RecNumber, FieldIdentifier, ValuePtr,
    BufferLength));

  MEM_FREE(_ValuePtr);

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (desc->herr, en_IM001);
      return SQL_ERROR;
    }

  return retcode;
}


RETCODE SQL_API
SQLSetDescField (
  SQLHDESC		  descriptorHandle,
  SQLSMALLINT		  RecNumber,
  SQLSMALLINT		  FieldIdentifier,
  SQLPOINTER		  ValuePtr,
  SQLINTEGER		  BufferLength)
{
  ENTER_DESC (descriptorHandle,
      trace_SQLSetDescField (TRACE_ENTER,
	  descriptorHandle,
	  RecNumber, FieldIdentifier, ValuePtr, BufferLength));

  retcode = SQLSetDescField_Internal (descriptorHandle,
      RecNumber, FieldIdentifier, ValuePtr, BufferLength, 'A');

  LEAVE_DESC (descriptorHandle,
      trace_SQLSetDescField (TRACE_LEAVE,
	  descriptorHandle,
	  RecNumber, FieldIdentifier, ValuePtr, BufferLength));
}


RETCODE SQL_API
SQLSetDescFieldA (
  SQLHDESC		  descriptorHandle,
  SQLSMALLINT		  RecNumber,
  SQLSMALLINT		  FieldIdentifier,
  SQLPOINTER		  ValuePtr,
  SQLINTEGER		  BufferLength)
{
  ENTER_DESC (descriptorHandle,
      trace_SQLSetDescField (TRACE_ENTER,
	  descriptorHandle,
	  RecNumber, FieldIdentifier, ValuePtr, BufferLength));

  retcode = SQLSetDescField_Internal (descriptorHandle,
      RecNumber, FieldIdentifier, ValuePtr, BufferLength, 'A');

  LEAVE_DESC (descriptorHandle,
      trace_SQLSetDescField (TRACE_LEAVE,
	  descriptorHandle,
	  RecNumber, FieldIdentifier, ValuePtr, BufferLength));
}


RETCODE SQL_API
SQLSetDescFieldW (
  SQLHDESC		  descriptorHandle,
  SQLSMALLINT		  RecNumber,
  SQLSMALLINT		  FieldIdentifier,
  SQLPOINTER		  ValuePtr,
  SQLINTEGER		  BufferLength)
{
  ENTER_DESC (descriptorHandle,
      trace_SQLSetDescFieldW (TRACE_ENTER,
	  descriptorHandle,
	  RecNumber, FieldIdentifier, ValuePtr, BufferLength));

  retcode = SQLSetDescField_Internal (descriptorHandle,
      RecNumber, FieldIdentifier, ValuePtr, BufferLength, 'W');

  LEAVE_DESC (descriptorHandle,
      trace_SQLSetDescFieldW (TRACE_LEAVE,
	  descriptorHandle,
	  RecNumber, FieldIdentifier, ValuePtr, BufferLength));
}


RETCODE SQL_API
SQLGetDescRec_Internal (
  SQLHDESC		  descriptorHandle,
  SQLSMALLINT 		  RecNumber,
  SQLPOINTER 		  Name,
  SQLSMALLINT 		  BufferLength,
  SQLSMALLINT		* StringLengthPtr,
  SQLSMALLINT		* TypePtr,
  SQLSMALLINT 		* SubTypePtr,
  SQLLEN		* LengthPtr,
  SQLSMALLINT		* PrecisionPtr,
  SQLSMALLINT		* ScalePtr,
  SQLSMALLINT		* NullablePtr,
  SQLCHAR 		  waMode)
{
  DESC (desc, descriptorHandle);
  CONN (pdbc, desc->hdbc);
  ENVR (penv, pdbc->henv);
  HPROC hproc = SQL_NULL_HPROC;
  SQLRETURN retcode = SQL_SUCCESS;
  void * nameOut = Name;
  void * _Name = NULL;

  CONV_DIRECT conv_direct = CD_NONE;
  DM_CONV *conv = &pdbc->conv;

  if (penv->unicode_driver && waMode != 'W')
    conv_direct = CD_A2W;
  else if (!penv->unicode_driver && waMode == 'W')
    conv_direct = CD_W2A;
  else if (waMode == 'W' && conv->dm_cp != conv->drv_cp)
    conv_direct = CD_W2W;

  if (conv_direct == CD_A2W || conv_direct == CD_W2W)
    {
      /* ansi<=unicode*/
      if ((_Name = malloc((BufferLength + 1) * DRV_WCHARSIZE_ALLOC(conv))) == NULL)
        {
          PUSHSQLERR (desc->herr, en_HY001);
          return SQL_ERROR;
        }
      nameOut = _Name;
    }
  else if (conv_direct == CD_W2A)
    {
      /* unicode<=ansi*/
      if ((_Name = malloc(BufferLength + 1)) == NULL)
        {
          PUSHSQLERR (desc->herr, en_HY001);
          return SQL_ERROR;
        }
      nameOut = _Name;
    }

  CALL_UDRIVER(desc->hdbc, desc, retcode, hproc, penv->unicode_driver, 
    en_GetDescRec, (desc->dhdesc, RecNumber, nameOut, BufferLength, 
    StringLengthPtr, TypePtr, SubTypePtr, LengthPtr, PrecisionPtr, 
    ScalePtr, NullablePtr));

  if (hproc == SQL_NULL_HPROC)
    {
      MEM_FREE(_Name);
      PUSHSQLERR (desc->herr, en_IM001);
      return SQL_ERROR;
    }

  if (Name && conv_direct != CD_NONE && SQL_SUCCEEDED (retcode))
    {
      int count;
      if (conv_direct == CD_A2W)
        {
        /* ansi<=unicode*/
          dm_StrCopyOut2_W2A_d2m (conv, nameOut, (SQLCHAR *) Name,
              BufferLength, NULL, &count);
          if (StringLengthPtr)
            *StringLengthPtr = (SQLSMALLINT)count;
        }
      else if (conv_direct == CD_W2A)
        {
        /* unicode<=ansi*/
          dm_StrCopyOut2_A2W_d2m (conv, (SQLCHAR *) nameOut, Name,
              BufferLength, NULL, &count);
          if (StringLengthPtr)
            *StringLengthPtr = (SQLSMALLINT)count;
        }
      else if (conv_direct == CD_W2W)
        {
        /* unicode<=unicode*/
          dm_StrCopyOut2_W2W_d2m (conv, nameOut, Name, BufferLength,
              NULL, &count);
          if (StringLengthPtr)
            *StringLengthPtr = (SQLSMALLINT)count;
        }
    }

  MEM_FREE(_Name);

  return retcode;
}


RETCODE SQL_API
SQLGetDescRec (
  SQLHDESC		  descriptorHandle,
  SQLSMALLINT		  RecNumber,
  SQLCHAR		* Name,
  SQLSMALLINT		  BufferLength,
  SQLSMALLINT		* StringLengthPtr,
  SQLSMALLINT		* TypePtr,
  SQLSMALLINT		* SubTypePtr,
  SQLLEN		* LengthPtr,
  SQLSMALLINT		* PrecisionPtr,
  SQLSMALLINT		* ScalePtr,
  SQLSMALLINT		* NullablePtr)
{
  ENTER_DESC (descriptorHandle,
    trace_SQLGetDescRec (TRACE_ENTER,
  	descriptorHandle, 
	RecNumber, 
	Name, BufferLength, StringLengthPtr, 
	TypePtr, 
	SubTypePtr, 
	LengthPtr, 
	PrecisionPtr, 
	ScalePtr, 
	NullablePtr));
    	
  retcode =  SQLGetDescRec_Internal(
  	descriptorHandle, 
	RecNumber, 
	Name, BufferLength, StringLengthPtr, 
	TypePtr, 
	SubTypePtr, 
	LengthPtr, 
	PrecisionPtr, 
	ScalePtr, 
	NullablePtr, 
	'A');

  LEAVE_DESC (descriptorHandle,
    trace_SQLGetDescRec (TRACE_LEAVE,
  	descriptorHandle, 
	RecNumber, 
	Name, BufferLength, StringLengthPtr, 
	TypePtr, 
	SubTypePtr, 
	LengthPtr, 
	PrecisionPtr, 
	ScalePtr, 
	NullablePtr));
}


RETCODE SQL_API
SQLGetDescRecA (
  SQLHDESC		  descriptorHandle,
  SQLSMALLINT		  RecNumber,
  SQLCHAR		* Name,
  SQLSMALLINT		  BufferLength,
  SQLSMALLINT		* StringLengthPtr,
  SQLSMALLINT		* TypePtr,
  SQLSMALLINT		* SubTypePtr,
  SQLLEN		* LengthPtr,
  SQLSMALLINT		* PrecisionPtr,
  SQLSMALLINT		* ScalePtr,
  SQLSMALLINT		* NullablePtr)
{
  ENTER_DESC (descriptorHandle,
    trace_SQLGetDescRec (TRACE_ENTER,
  	descriptorHandle, 
	RecNumber, 
	Name, BufferLength, StringLengthPtr, 
	TypePtr, 
	SubTypePtr, 
	LengthPtr, 
	PrecisionPtr, 
	ScalePtr, 
	NullablePtr));
    	
  retcode =  SQLGetDescRec_Internal(
  	descriptorHandle, 
	RecNumber, 
	Name, BufferLength, StringLengthPtr, 
	TypePtr, 
	SubTypePtr, 
	LengthPtr, 
	PrecisionPtr, 
	ScalePtr, 
	NullablePtr, 
	'A');

  LEAVE_DESC (descriptorHandle,
    trace_SQLGetDescRec (TRACE_LEAVE,
  	descriptorHandle, 
	RecNumber, 
	Name, BufferLength, StringLengthPtr, 
	TypePtr, 
	SubTypePtr, 
	LengthPtr, 
	PrecisionPtr, 
	ScalePtr, 
	NullablePtr));
}


RETCODE SQL_API
SQLGetDescRecW (
  SQLHDESC		  descriptorHandle,
  SQLSMALLINT		  RecNumber,
  SQLWCHAR		* Name,
  SQLSMALLINT		  BufferLength,
  SQLSMALLINT		* StringLengthPtr,
  SQLSMALLINT		* TypePtr,
  SQLSMALLINT		* SubTypePtr,
  SQLLEN		* LengthPtr,
  SQLSMALLINT		* PrecisionPtr,
  SQLSMALLINT		* ScalePtr,
  SQLSMALLINT		* NullablePtr)
{
  ENTER_DESC (descriptorHandle,
    trace_SQLGetDescRecW (TRACE_ENTER,
  	descriptorHandle, 
	RecNumber, 
	Name, BufferLength, StringLengthPtr, 
	TypePtr, 
	SubTypePtr, 
	LengthPtr, 
	PrecisionPtr, 
	ScalePtr, 
	NullablePtr));
    	
  retcode =  SQLGetDescRec_Internal(
  	descriptorHandle, 
	RecNumber, 
	Name, BufferLength, StringLengthPtr, 
	TypePtr, 
	SubTypePtr, 
	LengthPtr, 
	PrecisionPtr, 
	ScalePtr, 
	NullablePtr, 
	'W');

  LEAVE_DESC (descriptorHandle,
    trace_SQLGetDescRecW (TRACE_LEAVE,
  	descriptorHandle, 
	RecNumber, 
	Name, BufferLength, StringLengthPtr, 
	TypePtr, 
	SubTypePtr, 
	LengthPtr, 
	PrecisionPtr, 
	ScalePtr, 
	NullablePtr));
}


static RETCODE
SQLSetDescRec_Internal (
  SQLHDESC		  DescriptorHandle,
  SQLSMALLINT		  RecNumber,
  SQLSMALLINT		  Type,
  SQLSMALLINT		  SubType,
  SQLLEN		  Length,
  SQLSMALLINT		  Precision,
  SQLSMALLINT		  Scale,
  SQLPOINTER		  Data,
  SQLLEN		* StringLength,
  SQLLEN		* Indicator)
{
  DESC (desc, DescriptorHandle);
  HPROC hproc;
  RETCODE retcode;

  hproc = _iodbcdm_getproc (desc->hdbc, en_SetDescRec);
  if (!hproc)
    {
      PUSHSQLERR (desc->herr, en_IM001);
      return SQL_ERROR;
    }

  CALL_DRIVER (desc->hdbc, desc, retcode, hproc,
      (desc->dhdesc, RecNumber, Type, SubType, Length, Precision, Scale, 
       Data, StringLength, Indicator));

  return retcode;
}

RETCODE SQL_API
SQLSetDescRec (
  SQLHDESC		  DescriptorHandle,
  SQLSMALLINT		  RecNumber,
  SQLSMALLINT		  Type,
  SQLSMALLINT		  SubType,
  SQLLEN		  Length,
  SQLSMALLINT		  Precision,
  SQLSMALLINT		  Scale,
  SQLPOINTER		  Data,
  SQLLEN		* StringLength,
  SQLLEN		* Indicator)
{
  ENTER_DESC (DescriptorHandle,
    trace_SQLSetDescRec (TRACE_ENTER,
  	DescriptorHandle, RecNumber, Type, SubType, Length, Precision,
  	Scale, Data, StringLength, Indicator));

  retcode = SQLSetDescRec_Internal (
  	DescriptorHandle, RecNumber, Type, SubType, Length, Precision,
  	Scale, Data, StringLength, Indicator);

  LEAVE_DESC (DescriptorHandle,
    trace_SQLSetDescRec (TRACE_LEAVE,
  	DescriptorHandle, RecNumber, Type, SubType, Length, Precision,
  	Scale, Data, StringLength, Indicator));
}


static RETCODE 
SQLCopyDesc_Internal (
  SQLHDESC		  SourceDescHandle,
  SQLHDESC		  TargetDescHandle)
{
  DESC (desc, SourceDescHandle);
  DESC (desc1, TargetDescHandle);
  HPROC hproc;
  RETCODE retcode;

  hproc = _iodbcdm_getproc (desc->hdbc, en_CopyDesc);
  if (!hproc)
    {
      PUSHSQLERR (desc->herr, en_IM001);
      return SQL_ERROR;
    }

  CALL_DRIVER (desc->hdbc, desc, retcode, hproc,
      (desc->dhdesc, desc1->dhdesc));

  return retcode;
}


RETCODE SQL_API
SQLCopyDesc (
  SQLHDESC		  SourceDescHandle,
  SQLHDESC		  TargetDescHandle)
{
  ENTER_DESC (SourceDescHandle,
    trace_SQLCopyDesc (TRACE_ENTER,
    	SourceDescHandle,
	TargetDescHandle));

  retcode = SQLCopyDesc_Internal (
    	SourceDescHandle,
	TargetDescHandle);

  LEAVE_DESC (SourceDescHandle,
    trace_SQLCopyDesc (TRACE_LEAVE,
    	SourceDescHandle,
	TargetDescHandle));
}


RETCODE SQL_API
SQLColAttribute_Internal (
  SQLHSTMT		  statementHandle,
  SQLUSMALLINT		  ColumnNumber,
  SQLUSMALLINT		  FieldIdentifier,
  SQLPOINTER		  CharacterAttributePtr,
  SQLSMALLINT		  BufferLength,
  SQLSMALLINT		* StringLengthPtr,
  SQLLEN		* NumericAttributePtr,
  SQLCHAR		  waMode)
{
  STMT (stmt, statementHandle);
  CONN (pdbc, stmt->hdbc);
  ENVR (penv, pdbc->henv);
  GENV (genv, pdbc->genv);
  HPROC hproc = SQL_NULL_HPROC;
  HPROC hproc2 = SQL_NULL_HPROC;
  SQLRETURN retcode = SQL_SUCCESS;
  void * charAttrOut = CharacterAttributePtr;
  void * _charAttr = NULL;
  SQLUINTEGER odbc_ver;
  SQLUINTEGER dodbc_ver;
  CONV_DIRECT conv_direct = CD_NONE;
  DM_CONV *conv = &pdbc->conv;
  SQLSMALLINT _BufferLength = BufferLength;

  odbc_ver = ((GENV_t *) pdbc->genv)->odbc_ver;
  dodbc_ver = (penv != SQL_NULL_HENV) ? penv->dodbc_ver : odbc_ver;

  if (penv->unicode_driver && waMode != 'W')
    conv_direct = CD_A2W;
  else if (!penv->unicode_driver && waMode == 'W')
    conv_direct = CD_W2A;
  else if (waMode == 'W' && conv->dm_cp != conv->drv_cp)
    conv_direct = CD_W2W;

  if (conv_direct != CD_NONE)
    {
      switch(FieldIdentifier)
        {
        case SQL_COLUMN_NAME:

        case SQL_DESC_BASE_COLUMN_NAME:
        case SQL_DESC_BASE_TABLE_NAME:
        case SQL_DESC_CATALOG_NAME:
        case SQL_DESC_LABEL:
        case SQL_DESC_LITERAL_PREFIX:
        case SQL_DESC_LITERAL_SUFFIX:
        case SQL_DESC_LOCAL_TYPE_NAME:
        case SQL_DESC_NAME:
        case SQL_DESC_SCHEMA_NAME:
        case SQL_DESC_TABLE_NAME:
        case SQL_DESC_TYPE_NAME:
          if (conv_direct == CD_A2W || conv_direct == CD_W2W)
            {
              /*ansi<=unicode*/
              if (conv_direct == CD_W2W)
                _BufferLength /= DM_WCHARSIZE(conv);

              if ((_charAttr = malloc((_BufferLength + 1) * DRV_WCHARSIZE_ALLOC(conv))) == NULL)
                {
                  PUSHSQLERR (stmt->herr, en_HY001);
                  return SQL_ERROR;
                }
              _BufferLength *= DRV_WCHARSIZE_ALLOC(conv);
              charAttrOut = _charAttr;
            }
          else if (conv_direct == CD_W2A)
            {
              /*unicode<=ansi*/
              if ((_charAttr = malloc(BufferLength + 1)) == NULL)
                {
                  PUSHSQLERR (stmt->herr, en_HY001);
                  return SQL_ERROR;
                }
              _BufferLength /= DM_WCHARSIZE(conv);
              charAttrOut = _charAttr;
            }
          break;
        }
    }

  GET_UHPROC(stmt->hdbc, hproc2, en_ColAttributes, penv->unicode_driver);

  if (dodbc_ver == SQL_OV_ODBC3 &&
      (  odbc_ver == SQL_OV_ODBC3 
       || (odbc_ver == SQL_OV_ODBC2 && hproc2 == SQL_NULL_HPROC)))
    {
      CALL_UDRIVER(stmt->hdbc, stmt, retcode, hproc, penv->unicode_driver,
        en_ColAttribute, (stmt->dhstmt, ColumnNumber, FieldIdentifier,
        charAttrOut, _BufferLength, StringLengthPtr, NumericAttributePtr));
    }

  if (hproc != SQL_NULL_HPROC)
    {
    if (SQL_SUCCEEDED (retcode) && FieldIdentifier == SQL_DESC_CONCISE_TYPE)
      {
        SQLINTEGER *ptr = (SQLINTEGER *)NumericAttributePtr;

	/*
	 *  Convert sql type to ODBC version of application
	 */
	if (ptr)
	  *ptr = _iodbcdm_map_sql_type (*ptr, genv->odbc_ver);
      }

      if (CharacterAttributePtr && conv_direct != CD_NONE
          && SQL_SUCCEEDED (retcode))
        {
          int count;
          switch(FieldIdentifier)
            {
            case SQL_COLUMN_NAME:
            case SQL_DESC_BASE_COLUMN_NAME:
            case SQL_DESC_BASE_TABLE_NAME:
            case SQL_DESC_CATALOG_NAME:
            case SQL_DESC_LABEL:
            case SQL_DESC_LITERAL_PREFIX:
            case SQL_DESC_LITERAL_SUFFIX:
            case SQL_DESC_LOCAL_TYPE_NAME:
            case SQL_DESC_NAME:
            case SQL_DESC_SCHEMA_NAME:
            case SQL_DESC_TABLE_NAME:
            case SQL_DESC_TYPE_NAME:
              if (conv_direct == CD_A2W)
                {
                  /*ansi<=unicode*/
                  dm_StrCopyOut2_W2A_d2m (conv, charAttrOut,
			(SQLCHAR *) CharacterAttributePtr,
			BufferLength, NULL, &count);
                  if (StringLengthPtr)
                    *StringLengthPtr = (SQLSMALLINT)count;
                }
              else if (conv_direct == CD_W2A)
                {
                  /*unicode<=ansi*/
                  dm_StrCopyOut2_A2W_d2m (conv, (SQLCHAR *) charAttrOut,
			CharacterAttributePtr, BufferLength, NULL, &count);
                  if (StringLengthPtr)
                    *StringLengthPtr = (SQLSMALLINT)count;
                }
              else if (conv_direct == CD_W2W)
                {
                  /*unicode<=unicode*/
                  dm_StrCopyOut2_W2W_d2m (conv, charAttrOut,
			CharacterAttributePtr, BufferLength, NULL, &count);
                  if (StringLengthPtr)
                    *StringLengthPtr = (SQLSMALLINT)count;
                }
            }
        }
      MEM_FREE(_charAttr);
      return retcode;
    }


  if (ColumnNumber == 0)
    {
      char *szval = "";
      int isSz = 0;
      SQLINTEGER val = 0;

      MEM_FREE(_charAttr);

      switch (FieldIdentifier)
	{
	case SQL_DESC_AUTO_UNIQUE_VALUE:
	case SQL_DESC_CASE_SENSITIVE:
	case SQL_DESC_FIXED_PREC_SCALE:
	case SQL_DESC_UNSIGNED:
	  val = SQL_FALSE;
	  break;

	case SQL_DESC_LABEL:
	case SQL_DESC_CATALOG_NAME:
	case SQL_DESC_LITERAL_PREFIX:
	case SQL_DESC_LITERAL_SUFFIX:
	case SQL_DESC_LOCAL_TYPE_NAME:
	case SQL_DESC_NAME:
	case SQL_DESC_SCHEMA_NAME:
	case SQL_DESC_TABLE_NAME:
	case SQL_DESC_TYPE_NAME:
	  isSz = 1;
	  break;

	case SQL_DESC_CONCISE_TYPE:
	case SQL_DESC_TYPE:
	  val = SQL_BINARY;
	  break;

	case SQL_DESC_COUNT:
	  hproc = _iodbcdm_getproc (stmt->hdbc, en_NumResultCols);
	  if (!hproc)
	    {
	      PUSHSQLERR (stmt->herr, en_IM001);
	      return SQL_ERROR;
	    }
	  CALL_DRIVER (stmt->hdbc, stmt, retcode, hproc,
	      (stmt->dhstmt, NumericAttributePtr));
	  return retcode;

	case SQL_DESC_LENGTH:
	case SQL_DESC_DATETIME_INTERVAL_CODE:
	case SQL_DESC_SCALE:
	  val = 0;
	  break;

	case SQL_DESC_DISPLAY_SIZE:
	  val = 8;
	  break;

	case SQL_DESC_NULLABLE:
	  val = SQL_NO_NULLS;
	  break;

	case SQL_DESC_OCTET_LENGTH:
	case SQL_DESC_PRECISION:
	  val = 4;
	  break;

	case SQL_DESC_SEARCHABLE:
	  val = SQL_PRED_NONE;
	  break;

	case SQL_DESC_UNNAMED:
	  val = SQL_UNNAMED;
	  break;

	case SQL_DESC_UPDATABLE:
	  val = SQL_ATTR_READONLY;
	  break;

	default:
	  PUSHSQLERR (stmt->herr, en_HYC00);
	  return SQL_ERROR;
	}
      if (isSz)
	{
	  int len = STRLEN (szval), len1;
	  len1 = len > BufferLength ? BufferLength - 1 : len;
	  if (CharacterAttributePtr)
	    {
	      STRNCPY (CharacterAttributePtr, szval, len1);
	      ((SQLCHAR *) CharacterAttributePtr)[len1] = 0;
	    }
	  if (StringLengthPtr)
	    *StringLengthPtr = len;
	}
      else
	{
	  if (NumericAttributePtr)
	    *((SQLINTEGER *) NumericAttributePtr) = val;
	}
      return SQL_SUCCESS;
    }
  else
    {				/* all other */
      switch (FieldIdentifier)
	{
	case SQL_DESC_SCALE:
	  FieldIdentifier = SQL_COLUMN_SCALE;
	  break;

	case SQL_DESC_LENGTH:
	  FieldIdentifier = SQL_COLUMN_LENGTH;
	  break;

	case SQL_DESC_PRECISION:
	  FieldIdentifier = SQL_COLUMN_PRECISION;
	  break;

	case SQL_DESC_COUNT:
	  FieldIdentifier = SQL_COLUMN_COUNT;
	  break;

	case SQL_DESC_NAME:
	  FieldIdentifier = SQL_COLUMN_NAME;
	  break;

	case SQL_DESC_NULLABLE:
	  FieldIdentifier = SQL_COLUMN_NULLABLE;
	  break;

	case SQL_DESC_TYPE:
	  FieldIdentifier = SQL_COLUMN_TYPE;
	  break;

	case SQL_DESC_BASE_COLUMN_NAME:
	case SQL_DESC_BASE_TABLE_NAME:
	case SQL_DESC_LITERAL_PREFIX:
	case SQL_DESC_LITERAL_SUFFIX:
	case SQL_DESC_LOCAL_TYPE_NAME:
	case SQL_DESC_NUM_PREC_RADIX:
	case SQL_DESC_OCTET_LENGTH:
	case SQL_DESC_UNNAMED:
          MEM_FREE(_charAttr);
	  PUSHSQLERR (stmt->herr, en_HY091);
	  return SQL_ERROR;
	}

      CALL_UDRIVER(stmt->hdbc, stmt, retcode, hproc, penv->unicode_driver,
        en_ColAttributes, (stmt->dhstmt, ColumnNumber, FieldIdentifier,
        charAttrOut, _BufferLength, StringLengthPtr, NumericAttributePtr));

      if (hproc == SQL_NULL_HPROC)
        {
          MEM_FREE(_charAttr);
          PUSHSQLERR (stmt->herr, en_IM001);
          return SQL_ERROR;
        }    

    if (SQL_SUCCEEDED (retcode) && FieldIdentifier == SQL_DESC_CONCISE_TYPE)
      {
        SQLINTEGER *ptr = (SQLINTEGER *)NumericAttributePtr;

	/*
	 *  Convert sql type to ODBC version of application
	 */
	if (ptr)
	  *ptr = _iodbcdm_map_sql_type (*ptr, genv->odbc_ver);
      }

      if (CharacterAttributePtr && conv_direct != CD_NONE
          && SQL_SUCCEEDED (retcode))
        {
          int count;
          switch(FieldIdentifier)
            {
            case SQL_COLUMN_QUALIFIER_NAME:
            case SQL_COLUMN_NAME:
            case SQL_COLUMN_LABEL:
            case SQL_COLUMN_OWNER_NAME:
            case SQL_COLUMN_TABLE_NAME:
            case SQL_COLUMN_TYPE_NAME:

            case SQL_DESC_BASE_COLUMN_NAME:
            case SQL_DESC_BASE_TABLE_NAME:
            case SQL_DESC_LITERAL_PREFIX:
            case SQL_DESC_LITERAL_SUFFIX:
            case SQL_DESC_LOCAL_TYPE_NAME:
            case SQL_DESC_NAME:
              if (conv_direct == CD_A2W)
                {
                  /*ansi<=unicode*/
                  dm_StrCopyOut2_W2A_d2m (conv, charAttrOut,
			(SQLCHAR *) CharacterAttributePtr,
			BufferLength, NULL, &count);
                  if (StringLengthPtr)
                    *StringLengthPtr = (SQLSMALLINT)count;
                }
              else if (conv_direct == CD_W2A)
                {
                  /*unicode<=ansi*/
                  dm_StrCopyOut2_A2W_d2m (conv, (SQLCHAR *) charAttrOut,
			CharacterAttributePtr, BufferLength, NULL, &count);
                  if (StringLengthPtr)
                    *StringLengthPtr = (SQLSMALLINT)count;
                }
              else if (conv_direct == CD_W2W)
                {
                  /*unicode<=unicode*/
                  dm_StrCopyOut2_W2W_d2m (conv, charAttrOut,
			CharacterAttributePtr, BufferLength, NULL, &count);
                  if (StringLengthPtr)
                    *StringLengthPtr = (SQLSMALLINT)count;
                }
            }
        }
      MEM_FREE(_charAttr);
      return retcode;
    }
}


RETCODE SQL_API
SQLColAttribute (
  SQLHSTMT		  statementHandle,
  SQLUSMALLINT		  ColumnNumber,
  SQLUSMALLINT		  FieldIdentifier,
  SQLPOINTER		  CharacterAttributePtr,
  SQLSMALLINT		  BufferLength,
  SQLSMALLINT		* StringLengthPtr,
  SQLLEN		* NumericAttributePtr)
{
  ENTER_STMT (statementHandle,
    trace_SQLColAttribute (TRACE_ENTER, 
    	statementHandle,
	ColumnNumber,
	FieldIdentifier,
	CharacterAttributePtr, BufferLength, StringLengthPtr,
	NumericAttributePtr));

  retcode = SQLColAttribute_Internal (
  	statementHandle, 
	ColumnNumber, 
	FieldIdentifier,
	CharacterAttributePtr, BufferLength, StringLengthPtr, 
	NumericAttributePtr, 'A');

  LEAVE_STMT (statementHandle,
    trace_SQLColAttribute (TRACE_LEAVE, 
    	statementHandle,
	ColumnNumber,
	FieldIdentifier,
	CharacterAttributePtr, BufferLength, StringLengthPtr,
	NumericAttributePtr));
}


RETCODE SQL_API
SQLColAttributeA (
  SQLHSTMT		  statementHandle,
  SQLUSMALLINT		  ColumnNumber,
  SQLUSMALLINT		  FieldIdentifier,
  SQLPOINTER		  CharacterAttributePtr,
  SQLSMALLINT		  BufferLength,
  SQLSMALLINT		* StringLengthPtr,
  SQLLEN		* NumericAttributePtr)
{
  ENTER_STMT (statementHandle,
    trace_SQLColAttribute (TRACE_ENTER, 
    	statementHandle,
	ColumnNumber,
	FieldIdentifier,
	CharacterAttributePtr, BufferLength, StringLengthPtr,
	NumericAttributePtr));

  retcode = SQLColAttribute_Internal (
  	statementHandle, 
	ColumnNumber, 
	FieldIdentifier,
	CharacterAttributePtr, BufferLength, StringLengthPtr, 
	NumericAttributePtr, 'A');

  LEAVE_STMT (statementHandle,
    trace_SQLColAttribute (TRACE_LEAVE, 
    	statementHandle,
	ColumnNumber,
	FieldIdentifier,
	CharacterAttributePtr, BufferLength, StringLengthPtr,
	NumericAttributePtr));
}


RETCODE SQL_API
SQLColAttributeW (
  SQLHSTMT		  statementHandle,
  SQLUSMALLINT		  ColumnNumber,
  SQLUSMALLINT		  FieldIdentifier,
  SQLPOINTER		  CharacterAttributePtr,
  SQLSMALLINT		  BufferLength,
  SQLSMALLINT		* StringLengthPtr,
  SQLLEN		* NumericAttributePtr)
{
  ENTER_STMT (statementHandle,
    trace_SQLColAttributeW (TRACE_ENTER, 
    	statementHandle,
	ColumnNumber,
	FieldIdentifier,
	CharacterAttributePtr, BufferLength, StringLengthPtr,
	NumericAttributePtr));

  retcode = SQLColAttribute_Internal (
  	statementHandle, 
	ColumnNumber, 
	FieldIdentifier,
	CharacterAttributePtr, BufferLength, StringLengthPtr, 
	NumericAttributePtr, 
	'W');

  LEAVE_STMT (statementHandle,
    trace_SQLColAttributeW (TRACE_LEAVE, 
    	statementHandle,
	ColumnNumber,
	FieldIdentifier,
	CharacterAttributePtr, BufferLength, StringLengthPtr,
	NumericAttributePtr));
}


static RETCODE
SQLEndTran_Internal (
  SQLSMALLINT		  handleType,
  SQLHANDLE		  Handle,
  SQLSMALLINT		  completionType)
{
  switch (handleType)
    {
    case SQL_HANDLE_DBC:
    case SQL_HANDLE_ENV:
      break;
    default:
      return SQL_INVALID_HANDLE;
    }

  return SQLTransact_Internal (
      handleType == SQL_HANDLE_ENV ? Handle : SQL_NULL_HENV,
      handleType == SQL_HANDLE_DBC ? Handle : SQL_NULL_HDBC,
      completionType);
}


RETCODE SQL_API
SQLEndTran (
  SQLSMALLINT		  handleType,
  SQLHANDLE		  Handle,
  SQLSMALLINT		  completionType)
{
  SQLRETURN retcode = SQL_SUCCESS;

  ODBC_LOCK ();
  TRACE (trace_SQLEndTran (TRACE_ENTER, handleType, Handle, completionType));

  retcode = SQLEndTran_Internal (handleType, Handle, completionType);

  TRACE (trace_SQLEndTran (TRACE_LEAVE, handleType, Handle, completionType));
  ODBC_UNLOCK ();

  return retcode; 
}


static RETCODE 
SQLBulkOperations_Internal (
  SQLHSTMT		  statementHandle,
  SQLSMALLINT 		  Operation)
{
  STMT (stmt, statementHandle);
  HPROC hproc;
  RETCODE retcode;

  switch (Operation)
    {
    case SQL_ADD:
    case SQL_UPDATE_BY_BOOKMARK:
    case SQL_DELETE_BY_BOOKMARK:
    case SQL_FETCH_BY_BOOKMARK:
      break;
    default:
      PUSHSQLERR (stmt->herr, en_HY092);
      return SQL_ERROR;
    }

  if (Operation == SQL_FETCH_BY_BOOKMARK )
    {
      if ((retcode = _iodbcdm_FixColBindData (stmt)) != SQL_SUCCESS)
        return retcode;
    }

  if (Operation == SQL_ADD || Operation == SQL_UPDATE_BY_BOOKMARK
      || Operation == SQL_DELETE_BY_BOOKMARK)
    {
      _iodbcdm_ConvBindData_m2d (stmt);
    }

  hproc = _iodbcdm_getproc (stmt->hdbc, en_BulkOperations);
  if (hproc)
    {
      CALL_DRIVER (stmt->hdbc, stmt, retcode, hproc,
	  (stmt->dhstmt, Operation));

      if (Operation == SQL_FETCH_BY_BOOKMARK && SQL_SUCCEEDED (retcode))
        _iodbcdm_ConvBindData (stmt);
      return retcode;
    }

  switch (Operation)
    {
    case SQL_ADD:
      retcode = _iodbcdm_SetPos (statementHandle, 
		0, SQL_ADD, SQL_LOCK_NO_CHANGE);
      return retcode;

    default:
      PUSHSQLERR (stmt->herr, en_HYC00);
      return SQL_ERROR;
    }
}


RETCODE SQL_API
SQLBulkOperations (
  SQLHSTMT		  StatementHandle,
  SQLSMALLINT 		  Operation)
{
  ENTER_STMT (StatementHandle,
    trace_SQLBulkOperations (TRACE_ENTER, StatementHandle, Operation));

  retcode = SQLBulkOperations_Internal (StatementHandle, Operation);

  LEAVE_STMT (StatementHandle,
    trace_SQLBulkOperations (TRACE_LEAVE, StatementHandle, Operation));
}


static RETCODE
SQLFetchScroll_Internal (
  SQLHSTMT		  statementHandle,
  SQLSMALLINT		  fetchOrientation,
  SQLLEN		  fetchOffset)
{
  STMT (stmt, statementHandle);
  HPROC hproc = SQL_NULL_HPROC;
  RETCODE retcode;
  CONN (pdbc, stmt->hdbc);
  HPROC hproc2 = SQL_NULL_HPROC;
  SQLUINTEGER odbc_ver = ((GENV_t *) pdbc->genv)->odbc_ver;
  SQLUINTEGER dodbc_ver = ((ENV_t *) pdbc->henv)->dodbc_ver;

  /* check arguments */
  switch (fetchOrientation)
    {
    case SQL_FETCH_NEXT:
    case SQL_FETCH_PRIOR:
    case SQL_FETCH_FIRST:
    case SQL_FETCH_LAST:
    case SQL_FETCH_ABSOLUTE:
    case SQL_FETCH_RELATIVE:
    case SQL_FETCH_BOOKMARK:
      break;

    default:
      PUSHSQLERR (stmt->herr, en_HY092);
      return SQL_ERROR;
    }

  /* check state */
  if (stmt->asyn_on == en_NullProc)
    {
      switch (stmt->state)
	{
	case en_stmt_allocated:
	case en_stmt_prepared:
	case en_stmt_fetched:
	case en_stmt_needdata:
	case en_stmt_mustput:
	case en_stmt_canput:
	  PUSHSQLERR (stmt->herr, en_S1010);
	  return SQL_ERROR;

	default:
	  break;
	}
    }
  else if (stmt->asyn_on != en_FetchScroll)
    {
      PUSHSQLERR (stmt->herr, en_S1010);
      return SQL_ERROR;
    }

  hproc2 = _iodbcdm_getproc (stmt->hdbc, en_ExtendedFetch);

  if (dodbc_ver == SQL_OV_ODBC3 &&
      (  odbc_ver == SQL_OV_ODBC3 
       || (odbc_ver == SQL_OV_ODBC2 && hproc2 == SQL_NULL_HPROC)))
    {
      hproc = _iodbcdm_getproc (stmt->hdbc, en_FetchScroll);
      if (hproc)
        {
          CALL_DRIVER (stmt->hdbc, stmt, retcode, hproc,
	      (stmt->dhstmt, fetchOrientation, fetchOffset));
        }
    }

  if (hproc == SQL_NULL_HPROC)
    {
      if (!stmt->row_status_ptr)
	{
	  PUSHSQLERR (stmt->herr, en_HYC00);
	  return SQL_ERROR;
	}

      if (fetchOrientation == SQL_FETCH_BOOKMARK)
	{
	  if (fetchOffset)
	    {
	      PUSHSQLERR (stmt->herr, en_HYC00);
	      return SQL_ERROR;
	    }
	  retcode = _iodbcdm_ExtendedFetch (statementHandle, fetchOrientation,
	      stmt->fetch_bookmark_ptr ? *((SQLINTEGER *) stmt->fetch_bookmark_ptr)
	      : 0, (SQLULEN *) stmt->rows_fetched_ptr, 
		(SQLUSMALLINT *) stmt->row_status_ptr);
	}
      else
	retcode =
	    _iodbcdm_ExtendedFetch (statementHandle, fetchOrientation,
	    fetchOffset, (SQLULEN *) stmt->rows_fetched_ptr, 
	    (SQLUSMALLINT *) stmt->row_status_ptr);
    }

  /* state transition */
  if (stmt->asyn_on == en_FetchScroll)
    {
      switch (retcode)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
	case SQL_NO_DATA_FOUND:
	case SQL_ERROR:
	  stmt->asyn_on = en_NullProc;
	  break;

	case SQL_STILL_EXECUTING:
	default:
	  return retcode;
	}
    }

  switch (stmt->state)
    {
    case en_stmt_cursoropen:
    case en_stmt_xfetched:
      switch (retcode)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
	case SQL_NO_DATA_FOUND:
	  stmt->state = en_stmt_xfetched;
	  stmt->cursor_state = en_stmt_cursor_xfetched;
	  break;

	case SQL_STILL_EXECUTING:
	  stmt->asyn_on = en_FetchScroll;
	  break;

	default:
	  break;
	}
      break;

    default:
      break;
    }

  return retcode;
}


RETCODE SQL_API
SQLFetchScroll (
  SQLHSTMT		  StatementHandle,
  SQLSMALLINT		  FetchOrientation,
  SQLLEN		  FetchOffset)
{
  ENTER_STMT (StatementHandle,
    trace_SQLFetchScroll (TRACE_ENTER,
    	StatementHandle, 
	FetchOrientation,
	FetchOffset));

  retcode = _iodbcdm_FixColBindData ((STMT_t *) StatementHandle);
  if (retcode != SQL_SUCCESS)
    return retcode;

  retcode = SQLFetchScroll_Internal (
	StatementHandle,
	FetchOrientation,
	FetchOffset);

  if (SQL_SUCCEEDED (retcode))
    _iodbcdm_ConvBindData ((STMT_t *) StatementHandle);

  LEAVE_STMT (StatementHandle,
    trace_SQLFetchScroll (TRACE_LEAVE,
    	StatementHandle, 
	FetchOrientation, 
	FetchOffset));
}


SQLRETURN SQL_API
SQLBindParam (
    SQLHSTMT hstmt,
    SQLUSMALLINT ipar,
    SQLSMALLINT fCType,
    SQLSMALLINT fSqlType,
    SQLULEN cbParamDef,
    SQLSMALLINT ibScale,
    SQLPOINTER rgbValue,
    SQLLEN *pcbValue)
{
  return SQLBindParameter (hstmt, ipar, SQL_PARAM_INPUT, fCType, fSqlType, cbParamDef, ibScale, rgbValue, SQL_MAX_OPTION_STRING_LENGTH, pcbValue);
}


static SQLRETURN
SQLCloseCursor_Internal (SQLHSTMT hstmt)
{
  STMT (pstmt, hstmt);
  CONN (pdbc, pstmt->hdbc);
  HPROC hproc = SQL_NULL_HPROC;
  SQLRETURN retcode = SQL_SUCCESS;
  HPROC hproc2 = SQL_NULL_HPROC;
  SQLUINTEGER odbc_ver = ((GENV_t *) pdbc->genv)->odbc_ver;
  SQLUINTEGER dodbc_ver = ((ENV_t *) pdbc->henv)->dodbc_ver;

  /* check state */
  if (pstmt->state >= en_stmt_needdata || pstmt->asyn_on != en_NullProc)
    {
      PUSHSQLERR (pstmt->herr, en_S1010);

      return SQL_ERROR;
    }

  hproc2 = _iodbcdm_getproc (pstmt->hdbc, en_FreeStmt);

  if (dodbc_ver == SQL_OV_ODBC3 &&
      (  odbc_ver == SQL_OV_ODBC3 
       || (odbc_ver == SQL_OV_ODBC2 && hproc2 == SQL_NULL_HPROC)))
    {
      hproc = _iodbcdm_getproc (pstmt->hdbc, en_CloseCursor);
      if (hproc)
        {
          CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc,
	      (pstmt->dhstmt));
        }
    }

  if (hproc == SQL_NULL_HPROC)
    {
      hproc = _iodbcdm_getproc (pstmt->hdbc, en_FreeStmt);

      if (hproc == SQL_NULL_HPROC)
	{
	  PUSHSQLERR (pstmt->herr, en_IM001);
	  return SQL_ERROR;
	}

      CALL_DRIVER (pstmt->hdbc, pstmt, retcode, hproc,
	  (pstmt->dhstmt, SQL_CLOSE));
    }

  if (retcode != SQL_SUCCESS
      && retcode != SQL_SUCCESS_WITH_INFO)
    {
      return retcode;
    }

  /*
   *  State transition
   */
  pstmt->cursor_state = en_stmt_cursor_no;

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
	pstmt->state = en_stmt_prepared;
      else
	pstmt->state = en_stmt_allocated;
      break;

    default:
      break;
    }

  return retcode;
}


SQLRETURN SQL_API 
SQLCloseCursor (SQLHSTMT hstmt)
{
  ENTER_STMT (hstmt,
    trace_SQLCloseCursor (TRACE_ENTER, hstmt));

  retcode = SQLCloseCursor_Internal (hstmt);

  LEAVE_STMT (hstmt,
    trace_SQLCloseCursor (TRACE_LEAVE, hstmt));
}

#endif	/* ODBCVER >= 0x0300 */
