/*
 *  isql.h
 *
 *  $Id$
 *
 *  iODBC defines
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
#ifndef _ISQL_H
#define _ISQL_H

/*
 *  Set default specification to ODBC 2.50
 */
#ifndef ODBCVER
#define ODBCVER				0x0250
#endif

#ifndef _ISQLTYPES_H
#include "isqltypes.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  Useful Constants
 */
#define SQL_SPEC_MAJOR			2
#define SQL_SPEC_MINOR			50
#define SQL_SPEC_STRING			"02.50"

#define SQL_SQLSTATE_SIZE		5
#define SQL_MAX_MESSAGE_LENGTH		512
#define SQL_MAX_DSN_LENGTH		32
#define SQL_MAX_OPTION_STRING_LENGTH	256

/*
 *  Handle types
 */
#define SQL_HANDLE_ENV			1
#define SQL_HANDLE_DBC			2
#define SQL_HANDLE_STMT			3
#if (ODBCVER >= 0x0300)
#define SQL_HANDLE_DESC			4
#endif

/*
 *  Function return codes
 */
#define SQL_INVALID_HANDLE		(-2)
#define SQL_ERROR			(-1)
#define SQL_SUCCESS 			0
#define SQL_SUCCESS_WITH_INFO		1
#define SQL_NO_DATA_FOUND		100


/*
 *  Standard SQL datatypes, using ANSI type numbering
 */
#define SQL_UNKNOWN_TYPE		0
#define SQL_CHAR			1
#define SQL_NUMERIC 			2
#define SQL_DECIMAL 			3
#define SQL_INTEGER 			4
#define SQL_SMALLINT			5
#define SQL_FLOAT			6
#define SQL_REAL			7
#define SQL_DOUBLE			8
#define SQL_VARCHAR 			12

#define SQL_TYPE_NULL			0
#define SQL_TYPE_MIN			SQL_CHAR
#define SQL_TYPE_MAX			SQL_VARCHAR


/*
 *  C datatype to SQL datatype mapping
 */
#define SQL_C_CHAR			SQL_CHAR
#define SQL_C_LONG			SQL_INTEGER
#define SQL_C_SHORT			SQL_SMALLINT
#define SQL_C_FLOAT			SQL_REAL
#define SQL_C_DOUBLE			SQL_DOUBLE
#define SQL_C_DEFAULT 			99


/*
 *  NULL status constants.
 */
#define SQL_NO_NULLS			0
#define SQL_NULLABLE			1
#define SQL_NULLABLE_UNKNOWN		2


/*
 *  Special length values
 */
#define SQL_NULL_DATA			(-1)
#define SQL_DATA_AT_EXEC		(-2)
#define SQL_NTS 			(-3)


/*
 *  SQLFreeStmt
 */
#define SQL_CLOSE			0
#define SQL_DROP			1
#define SQL_UNBIND			2
#define SQL_RESET_PARAMS		3


/*
 *  SQLTransact
 */
#define SQL_COMMIT			0
#define SQL_ROLLBACK			1


/*
 *  SQLColAttributes
 */
#define SQL_COLUMN_COUNT		0
#define SQL_COLUMN_NAME			1
#define SQL_COLUMN_TYPE			2
#define SQL_COLUMN_LENGTH		3
#define SQL_COLUMN_PRECISION		4
#define SQL_COLUMN_SCALE		5
#define SQL_COLUMN_DISPLAY_SIZE		6
#define SQL_COLUMN_NULLABLE		7
#define SQL_COLUMN_UNSIGNED		8
#define SQL_COLUMN_MONEY		9
#define SQL_COLUMN_UPDATABLE		10
#define SQL_COLUMN_AUTO_INCREMENT	11
#define SQL_COLUMN_CASE_SENSITIVE	12
#define SQL_COLUMN_SEARCHABLE		13
#define SQL_COLUMN_TYPE_NAME		14
#define SQL_COLUMN_TABLE_NAME		15
#define SQL_COLUMN_OWNER_NAME		16
#define SQL_COLUMN_QUALIFIER_NAME	17
#define SQL_COLUMN_LABEL		18

#define SQL_COLATT_OPT_MAX		SQL_COLUMN_LABEL
#define	SQL_COLATT_OPT_MIN		SQL_COLUMN_COUNT
#define SQL_COLUMN_DRIVER_START		1000


/*
 *  SQLColAttributes : SQL_COLUMN_UPDATABLE
 */
#define SQL_ATTR_READONLY		0
#define SQL_ATTR_WRITE			1
#define SQL_ATTR_READWRITE_UNKNOWN	2


/*
 *  SQLColAttributes : SQL_COLUMN_SEARCHABLE
 */
#define SQL_UNSEARCHABLE		0
#define SQL_LIKE_ONLY			1
#define SQL_ALL_EXCEPT_LIKE 		2
#define SQL_SEARCHABLE			3


/*
 *  NULL Handles
 */
#define SQL_NULL_HENV			0
#define SQL_NULL_HDBC			0
#define SQL_NULL_HSTMT			0


/*
 *  Function Prototypes
 */

SQLRETURN SQL_API SQLAllocConnect (
    SQLHENV henv,
    SQLHDBC FAR * phdbc);

SQLRETURN SQL_API SQLAllocEnv (
    SQLHENV FAR * phenv);

SQLRETURN SQL_API SQLAllocStmt (
    SQLHDBC hdbc,
    SQLHSTMT FAR * phstmt);

SQLRETURN SQL_API SQLBindCol (
    SQLHSTMT hstmt,
    SQLUSMALLINT icol,
    SQLSMALLINT fCType,
    SQLPOINTER rgbValue,
    SQLINTEGER cbValueMax,
    SQLINTEGER FAR * pcbValue);

SQLRETURN SQL_API SQLCancel (
    SQLHSTMT hstmt);

SQLRETURN SQL_API SQLColAttributes (
    SQLHSTMT hstmt,
    SQLUSMALLINT icol,
    SQLUSMALLINT fDescType,
    SQLPOINTER rgbDesc,
    SQLSMALLINT cbDescMax,
    SQLSMALLINT FAR * pcbDesc,
    SQLINTEGER FAR * pfDesc);

SQLRETURN SQL_API SQLConnect (
    SQLHDBC hdbc,
    SQLCHAR FAR * szDSN,
    SQLSMALLINT cbDSN,
    SQLCHAR FAR * szUID,
    SQLSMALLINT cbUID,
    SQLCHAR FAR * szAuthStr,
    SQLSMALLINT cbAuthStr);

SQLRETURN SQL_API SQLDescribeCol (
    SQLHSTMT hstmt,
    SQLUSMALLINT icol,
    SQLCHAR FAR * szColName,
    SQLSMALLINT cbColNameMax,
    SQLSMALLINT FAR * pcbColName,
    SQLSMALLINT FAR * pfSqlType,
    SQLUINTEGER FAR * pcbColDef,
    SQLSMALLINT FAR * pibScale,
    SQLSMALLINT FAR * pfNullable);

SQLRETURN SQL_API SQLDisconnect (
    SQLHDBC hdbc);

SQLRETURN SQL_API SQLError (
    SQLHENV henv,
    SQLHDBC hdbc,
    SQLHSTMT hstmt,
    SQLCHAR FAR * szSqlState,
    SQLINTEGER FAR * pfNativeError,
    SQLCHAR FAR * szErrorMsg,
    SQLSMALLINT cbErrorMsgMax,
    SQLSMALLINT FAR * pcbErrorMsg);

SQLRETURN SQL_API SQLExecDirect (
    SQLHSTMT hstmt,
    SQLCHAR FAR * szSqlStr,
    SQLINTEGER cbSqlStr);

SQLRETURN SQL_API SQLExecute (
    SQLHSTMT hstmt);

SQLRETURN SQL_API SQLFetch (
    SQLHSTMT hstmt);

SQLRETURN SQL_API SQLFreeConnect (
    SQLHDBC hdbc);

SQLRETURN SQL_API SQLFreeEnv (
    SQLHENV henv);

SQLRETURN SQL_API SQLFreeStmt (
    SQLHSTMT hstmt,
    SQLUSMALLINT fOption);

SQLRETURN SQL_API SQLGetCursorName (
    SQLHSTMT hstmt,
    SQLCHAR FAR * szCursor,
    SQLSMALLINT cbCursorMax,
    SQLSMALLINT FAR * pcbCursor);

SQLRETURN SQL_API SQLNumResultCols (
    SQLHSTMT hstmt,
    SQLSMALLINT FAR * pccol);

SQLRETURN SQL_API SQLPrepare (
    SQLHSTMT hstmt,
    SQLCHAR FAR * szSqlStr,
    SQLINTEGER cbSqlStr);

SQLRETURN SQL_API SQLRowCount (
    SQLHSTMT hstmt,
    SQLINTEGER FAR * pcrow);

SQLRETURN SQL_API SQLSetCursorName (
    SQLHSTMT hstmt,
    SQLCHAR FAR * szCursor,
    SQLSMALLINT cbCursor);

SQLRETURN SQL_API SQLTransact (
    SQLHENV henv,
    SQLHDBC hdbc,
    SQLUSMALLINT fType);

/*
 *  Depreciated ODBC 1.0 function - Use SQLBindParameter
 */
SQLRETURN SQL_API SQLSetParam (
    SQLHSTMT hstmt,
    SQLUSMALLINT ipar,
    SQLSMALLINT fCType,
    SQLSMALLINT fSqlType,
    SQLUINTEGER cbParamDef,
    SQLSMALLINT ibScale,
    SQLPOINTER rgbValue,
    SQLINTEGER FAR * pcbValue);

#ifdef __cplusplus
}
#endif

#endif
