/*
 *  $Id$
 *
 *  The iODBC driver manager.
 *  
 *  Copyright (C) 1996-2015 by OpenLink Software <iodbc@openlinksw.com>
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

#include <config.h>

#define UNICODE
#define SQL_NOUNICODEMAP
#include <iodbcinst.h>
#include <isqlucode.h>

#include <CFBundle.h>
#include <Gestalt.h>
#include <Folders.h>

/* Prototypes functions */
OSStatus load_library(Str255 name, CFragConnectionID *id);
OSStatus load_framework(CFStringRef freame, CFBundleRef *bundle);
void create_error (LPCSTR text, LPCSTR errmsg);
void _init(void);

/* iODBC bundle prototypes functions */
typedef SQLRETURN SQL_API (*SQLAllocConnectPTR) (SQLHENV EnvironmentHandle,
      SQLHDBC * ConnectionHandle);
typedef SQLRETURN SQL_API (*SQLAllocEnvPTR) (SQLHENV * EnvironmentHandle);
typedef SQLRETURN SQL_API (*SQLAllocHandlePTR) (SQLSMALLINT HandleType,
      SQLHANDLE InputHandle, SQLHANDLE * OutputHandle);
typedef SQLRETURN SQL_API (*SQLAllocStmtPTR) (SQLHDBC ConnectionHandle,
      SQLHSTMT * StatementHandle);
typedef SQLRETURN SQL_API (*SQLBindColPTR) (SQLHSTMT StatementHandle,
      SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType,
      SQLPOINTER TargetValue, SQLINTEGER BufferLength, SQLINTEGER * StrLen_or_Ind);
typedef SQLRETURN SQL_API (*SQLBindParamPTR) (SQLHSTMT StatementHandle,
      SQLUSMALLINT ParameterNumber, SQLSMALLINT ValueType,
      SQLSMALLINT ParameterType, SQLUINTEGER LengthPrecision,
      SQLSMALLINT ParameterScale, SQLPOINTER ParameterValue, SQLINTEGER * StrLen_or_Ind);
typedef SQLRETURN SQL_API (*SQLBindParameterPTR) (SQLHSTMT hstmt,
      SQLUSMALLINT ipar, SQLSMALLINT fParamType,
      SQLSMALLINT fCType, SQLSMALLINT fSqlType,
      SQLUINTEGER cbColDef, SQLSMALLINT ibScale,
      SQLPOINTER rgbValue, SQLINTEGER cbValueMax, SQLINTEGER * pcbValue);
typedef SQLRETURN SQL_API (*SQLBrowseConnectPTR) (SQLHDBC hdbc,
      SQLCHAR * szConnStrIn, SQLSMALLINT cbConnStrIn,
      SQLCHAR * szConnStrOut, SQLSMALLINT cbConnStrOutMax, SQLSMALLINT * pcbConnStrOut);
typedef SQLRETURN SQL_API (*SQLBulkOperationsPTR) (SQLHSTMT StatementHandle,
      SQLSMALLINT Operation);
typedef SQLRETURN SQL_API (*SQLCancelPTR) (SQLHSTMT StatementHandle);
typedef SQLRETURN SQL_API (*SQLCloseCursorPTR) (SQLHSTMT StatementHandle);
typedef SQLRETURN SQL_API (*SQLColAttributePTR) (SQLHSTMT StatementHandle,
      SQLUSMALLINT ColumnNumber, SQLUSMALLINT FieldIdentifier,
      SQLPOINTER CharacterAttribute, SQLSMALLINT BufferLength,
      SQLSMALLINT * StringLength, SQLPOINTER NumericAttribute);
typedef SQLRETURN SQL_API (*SQLColAttributesPTR) (SQLHSTMT hstmt,
      SQLUSMALLINT icol, SQLUSMALLINT fDescType,
      SQLPOINTER rgbDesc, SQLSMALLINT cbDescMax, SQLSMALLINT * pcbDesc, SQLINTEGER * pfDesc);
typedef SQLRETURN SQL_API (*SQLColumnPrivilegesPTR) (SQLHSTMT hstmt,
      SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName,
      SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName,
      SQLCHAR * szTableName, SQLSMALLINT cbTableName,
      SQLCHAR * szColumnName, SQLSMALLINT cbColumnName);
typedef SQLRETURN SQL_API (*SQLColumnsPTR) (SQLHSTMT StatementHandle,
      SQLCHAR * CatalogName, SQLSMALLINT NameLength1,
      SQLCHAR * SchemaName, SQLSMALLINT NameLength2,
      SQLCHAR * TableName, SQLSMALLINT NameLength3, SQLCHAR * ColumnName, SQLSMALLINT NameLength4);
typedef SQLRETURN SQL_API (*SQLConnectPTR) (SQLHDBC ConnectionHandle,
      SQLCHAR * ServerName, SQLSMALLINT NameLength1,
      SQLCHAR * UserName, SQLSMALLINT NameLength2,
      SQLCHAR * Authentication, SQLSMALLINT NameLength3);
typedef SQLRETURN SQL_API (*SQLCopyDescPTR) (SQLHDESC SourceDescHandle,
      SQLHDESC TargetDescHandle);
typedef SQLRETURN SQL_API (*SQLDataSourcesPTR) (SQLHENV EnvironmentHandle,
      SQLUSMALLINT Direction, SQLCHAR * ServerName,
      SQLSMALLINT BufferLength1, SQLSMALLINT * NameLength1,
      SQLCHAR * Description, SQLSMALLINT BufferLength2, SQLSMALLINT * NameLength2);
typedef SQLRETURN SQL_API (*SQLDescribeColPTR) (SQLHSTMT StatementHandle,
      SQLUSMALLINT ColumnNumber, SQLCHAR * ColumnName,
      SQLSMALLINT BufferLength, SQLSMALLINT * NameLength,
      SQLSMALLINT * DataType, SQLUINTEGER * ColumnSize,
      SQLSMALLINT * DecimalDigits, SQLSMALLINT * Nullable);
typedef SQLRETURN SQL_API (*SQLDescribeParamPTR) (SQLHSTMT hstmt,
      SQLUSMALLINT ipar, SQLSMALLINT * pfSqlType,
      SQLUINTEGER * pcbParamDef, SQLSMALLINT * pibScale, SQLSMALLINT * pfNullable);
typedef SQLRETURN SQL_API (*SQLDisconnectPTR) (SQLHDBC ConnectionHandle);
typedef SQLRETURN SQL_API (*SQLDriverConnectPTR) (SQLHDBC hdbc,
      SQLHWND hwnd, SQLCHAR * szConnStrIn, SQLSMALLINT cbConnStrIn, SQLCHAR * szConnStrOut,
      SQLSMALLINT cbConnStrOutMax, SQLSMALLINT * pcbConnStrOut, SQLUSMALLINT fDriverCompletion);
typedef SQLRETURN SQL_API (*SQLDriversPTR) (SQLHENV henv,
      SQLUSMALLINT fDirection, SQLCHAR * szDriverDesc,
      SQLSMALLINT cbDriverDescMax, SQLSMALLINT * pcbDriverDesc,
      SQLCHAR * szDriverAttributes, SQLSMALLINT cbDrvrAttrMax, SQLSMALLINT * pcbDrvrAttr);
typedef SQLRETURN SQL_API (*SQLEndTranPTR) (SQLSMALLINT HandleType,
      SQLHANDLE Handle, SQLSMALLINT CompletionType);
typedef SQLRETURN SQL_API (*SQLErrorPTR) (SQLHENV EnvironmentHandle,
      SQLHDBC ConnectionHandle, SQLHSTMT StatementHandle,
      SQLCHAR * Sqlstate, SQLINTEGER * NativeError,
      SQLCHAR * MessageText, SQLSMALLINT BufferLength, SQLSMALLINT * TextLength);
typedef SQLRETURN SQL_API (*SQLExecDirectPTR) (SQLHSTMT StatementHandle,
      SQLCHAR * StatementText, SQLINTEGER TextLength);
typedef SQLRETURN SQL_API (*SQLExecutePTR) (SQLHSTMT StatementHandle);
typedef SQLRETURN SQL_API (*SQLExtendedFetchPTR) (SQLHSTMT hstmt,
      SQLUSMALLINT fFetchType,
      SQLINTEGER irow, SQLUINTEGER * pcrow, SQLUSMALLINT * rgfRowStatus);
typedef SQLRETURN SQL_API (*SQLFetchPTR) (SQLHSTMT StatementHandle);
typedef SQLRETURN SQL_API (*SQLFetchScrollPTR) (SQLHSTMT StatementHandle,
      SQLSMALLINT FetchOrientation, SQLINTEGER FetchOffset);
typedef SQLRETURN SQL_API (*SQLForeignKeysPTR) (SQLHSTMT hstmt,
      SQLCHAR * szPkCatalogName, SQLSMALLINT cbPkCatalogName,
      SQLCHAR * szPkSchemaName, SQLSMALLINT cbPkSchemaName,
      SQLCHAR * szPkTableName, SQLSMALLINT cbPkTableName,
      SQLCHAR * szFkCatalogName, SQLSMALLINT cbFkCatalogName,
      SQLCHAR * szFkSchemaName, SQLSMALLINT cbFkSchemaName,
      SQLCHAR * szFkTableName, SQLSMALLINT cbFkTableName);
typedef SQLRETURN SQL_API (*SQLFreeConnectPTR) (SQLHDBC ConnectionHandle);
typedef SQLRETURN SQL_API (*SQLFreeEnvPTR) (SQLHENV EnvironmentHandle);
typedef SQLRETURN SQL_API (*SQLFreeHandlePTR) (SQLSMALLINT HandleType, SQLHANDLE Handle);
typedef SQLRETURN SQL_API (*SQLFreeStmtPTR) (SQLHSTMT StatementHandle,
      SQLUSMALLINT Option);
typedef SQLRETURN SQL_API (*SQLGetConnectAttrPTR) (SQLHDBC ConnectionHandle,
      SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER * StringLength);
typedef SQLRETURN SQL_API (*SQLGetConnectOptionPTR) (SQLHDBC ConnectionHandle,
      SQLUSMALLINT Option, SQLPOINTER Value);
typedef SQLRETURN SQL_API (*SQLGetCursorNamePTR) (SQLHSTMT StatementHandle,
      SQLCHAR * CursorName, SQLSMALLINT BufferLength, SQLSMALLINT * NameLength);
typedef SQLRETURN SQL_API (*SQLGetDataPTR) (SQLHSTMT StatementHandle,
      SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType,
      SQLPOINTER TargetValue, SQLINTEGER BufferLength, SQLINTEGER * StrLen_or_Ind);
typedef SQLRETURN SQL_API (*SQLGetDescFieldPTR) (SQLHDESC DescriptorHandle,
      SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
      SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER * StringLength);
typedef SQLRETURN SQL_API (*SQLGetDescRecPTR) (SQLHDESC DescriptorHandle,
      SQLSMALLINT RecNumber, SQLCHAR * Name,
      SQLSMALLINT BufferLength, SQLSMALLINT * StringLength,
      SQLSMALLINT * Type, SQLSMALLINT * SubType,
      SQLINTEGER * Length, SQLSMALLINT * Precision, SQLSMALLINT * Scale, SQLSMALLINT * Nullable);
typedef SQLRETURN SQL_API (*SQLGetDiagFieldPTR) (SQLSMALLINT HandleType,
      SQLHANDLE Handle, SQLSMALLINT RecNumber,
      SQLSMALLINT DiagIdentifier, SQLPOINTER DiagInfo,
      SQLSMALLINT BufferLength, SQLSMALLINT * StringLength);
typedef SQLRETURN SQL_API (*SQLGetDiagRecPTR) (SQLSMALLINT HandleType,
      SQLHANDLE Handle, SQLSMALLINT RecNumber,
      SQLCHAR * Sqlstate, SQLINTEGER * NativeError,
      SQLCHAR * MessageText, SQLSMALLINT BufferLength, SQLSMALLINT * TextLength);
typedef SQLRETURN SQL_API (*SQLGetEnvAttrPTR) (SQLHENV EnvironmentHandle,
      SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER * StringLength);
typedef SQLRETURN SQL_API (*SQLGetFunctionsPTR) (SQLHDBC ConnectionHandle,
      SQLUSMALLINT FunctionId, SQLUSMALLINT * Supported);
typedef SQLRETURN SQL_API (*SQLGetInfoPTR) (SQLHDBC ConnectionHandle,
      SQLUSMALLINT InfoType, SQLPOINTER InfoValue,
      SQLSMALLINT BufferLength, SQLSMALLINT * StringLength);
typedef SQLRETURN SQL_API (*SQLGetStmtAttrPTR) (SQLHSTMT StatementHandle,
      SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER * StringLength);
typedef SQLRETURN SQL_API (*SQLGetStmtOptionPTR) (SQLHSTMT StatementHandle,
      SQLUSMALLINT Option, SQLPOINTER Value);
typedef SQLRETURN SQL_API (*SQLGetTypeInfoPTR) (SQLHSTMT StatementHandle,
      SQLSMALLINT DataType);
typedef SQLRETURN SQL_API (*SQLMoreResultsPTR) (SQLHSTMT hstmt);
typedef SQLRETURN SQL_API (*SQLNativeSqlPTR) (SQLHDBC hdbc,
      SQLCHAR * szSqlStrIn, SQLINTEGER cbSqlStrIn,
      SQLCHAR * szSqlStr, SQLINTEGER cbSqlStrMax, SQLINTEGER * pcbSqlStr);
typedef SQLRETURN SQL_API (*SQLNumParamsPTR) (SQLHSTMT hstmt, SQLSMALLINT * pcpar);
typedef SQLRETURN SQL_API (*SQLNumResultColsPTR) (SQLHSTMT StatementHandle,
      SQLSMALLINT * ColumnCount);
typedef SQLRETURN SQL_API (*SQLParamDataPTR) (SQLHSTMT StatementHandle,
      SQLPOINTER * Value);
typedef SQLRETURN SQL_API (*SQLParamOptionsPTR) (SQLHSTMT hstmt,
      SQLUINTEGER crow, SQLUINTEGER * pirow);
typedef SQLRETURN SQL_API (*SQLPreparePTR) (SQLHSTMT StatementHandle,
      SQLCHAR * StatementText, SQLINTEGER TextLength);
typedef SQLRETURN SQL_API (*SQLPrimaryKeysPTR) (SQLHSTMT hstmt,
      SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName,
      SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName,
      SQLCHAR * szTableName, SQLSMALLINT cbTableName);
typedef SQLRETURN SQL_API (*SQLProcedureColumnsPTR) (SQLHSTMT hstmt,
      SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName,
      SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName,
      SQLCHAR * szProcName, SQLSMALLINT cbProcName,
      SQLCHAR * szColumnName, SQLSMALLINT cbColumnName);
typedef SQLRETURN SQL_API (*SQLProceduresPTR) (SQLHSTMT hstmt,
      SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName,
      SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR * szProcName, SQLSMALLINT cbProcName);
typedef SQLRETURN SQL_API (*SQLPutDataPTR) (SQLHSTMT StatementHandle,
      SQLPOINTER Data, SQLINTEGER StrLen_or_Ind);
typedef SQLRETURN SQL_API (*SQLRowCountPTR) (SQLHSTMT StatementHandle,
      SQLINTEGER * RowCount);
typedef SQLRETURN SQL_API (*SQLSetConnectAttrPTR) (SQLHDBC ConnectionHandle,
      SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER StringLength);
typedef SQLRETURN SQL_API (*SQLSetConnectOptionPTR) (SQLHDBC ConnectionHandle,
      SQLUSMALLINT Option, SQLUINTEGER Value);
typedef SQLRETURN SQL_API (*SQLSetCursorNamePTR) (SQLHSTMT StatementHandle,
      SQLCHAR * CursorName, SQLSMALLINT NameLength);
typedef SQLRETURN SQL_API (*SQLSetDescFieldPTR) (SQLHDESC DescriptorHandle,
      SQLSMALLINT RecNumber,
      SQLSMALLINT FieldIdentifier, SQLPOINTER Value, SQLINTEGER BufferLength);
typedef SQLRETURN SQL_API (*SQLSetDescRecPTR) (SQLHDESC DescriptorHandle,
      SQLSMALLINT RecNumber, SQLSMALLINT Type,
      SQLSMALLINT SubType, SQLINTEGER Length,
      SQLSMALLINT Precision, SQLSMALLINT Scale,
      SQLPOINTER Data, SQLINTEGER * StringLength, SQLINTEGER * Indicator);
typedef SQLRETURN SQL_API (*SQLSetEnvAttrPTR) (SQLHENV EnvironmentHandle,
      SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER StringLength);
typedef SQLRETURN SQL_API (*SQLSetParamPTR) (SQLHSTMT StatementHandle,
      SQLUSMALLINT ParameterNumber, SQLSMALLINT ValueType,
      SQLSMALLINT ParameterType, SQLUINTEGER LengthPrecision,
      SQLSMALLINT ParameterScale, SQLPOINTER ParameterValue, SQLINTEGER * StrLen_or_Ind);
typedef SQLRETURN SQL_API (*SQLSetPosPTR) (SQLHSTMT hstmt,
      SQLUSMALLINT irow, SQLUSMALLINT fOption, SQLUSMALLINT fLock);
typedef SQLRETURN SQL_API (*SQLSetScrollOptionsPTR) (SQLHSTMT hstmt,
      SQLUSMALLINT fConcurrency, SQLINTEGER crowKeyset, SQLUSMALLINT crowRowset);
typedef SQLRETURN SQL_API (*SQLSetStmtAttrPTR) (SQLHSTMT StatementHandle,
      SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER StringLength);
typedef SQLRETURN SQL_API (*SQLSetStmtOptionPTR) (SQLHSTMT StatementHandle,
      SQLUSMALLINT Option, SQLUINTEGER Value);
typedef SQLRETURN SQL_API (*SQLSpecialColumnsPTR) (SQLHSTMT StatementHandle,
      SQLUSMALLINT IdentifierType, SQLCHAR * CatalogName,
      SQLSMALLINT NameLength1, SQLCHAR * SchemaName,
      SQLSMALLINT NameLength2, SQLCHAR * TableName,
      SQLSMALLINT NameLength3, SQLUSMALLINT Scope, SQLUSMALLINT Nullable);
typedef SQLRETURN SQL_API (*SQLStatisticsPTR) (SQLHSTMT StatementHandle,
      SQLCHAR * CatalogName, SQLSMALLINT NameLength1,
      SQLCHAR * SchemaName, SQLSMALLINT NameLength2,
      SQLCHAR * TableName,
      SQLSMALLINT NameLength3, SQLUSMALLINT Unique, SQLUSMALLINT Reserved);
typedef SQLRETURN SQL_API (*SQLTablePrivilegesPTR) (SQLHSTMT hstmt,
      SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName,
      SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName,
      SQLCHAR * szTableName, SQLSMALLINT cbTableName);
typedef SQLRETURN SQL_API (*SQLTablesPTR) (SQLHSTMT StatementHandle,
      SQLCHAR * CatalogName, SQLSMALLINT NameLength1,
      SQLCHAR * SchemaName, SQLSMALLINT NameLength2,
      SQLCHAR * TableName,
      SQLSMALLINT NameLength3, SQLCHAR * TableType, SQLSMALLINT NameLength4);
typedef SQLRETURN SQL_API (*SQLTransactPTR) (SQLHENV EnvironmentHandle,
      SQLHDBC ConnectionHandle, SQLUSMALLINT CompletionType);
/*Unicode functions  */
typedef SQLRETURN SQL_API (*SQLColAttributeWPTR)(SQLHSTMT hstmt,
      SQLUSMALLINT iCol, SQLUSMALLINT iField, SQLPOINTER pCharAttr,
      SQLSMALLINT cbCharAttrMax, SQLSMALLINT * pcbCharAttr,
      SQLPOINTER pNumAttr);
typedef SQLRETURN SQL_API (*SQLColAttributesWPTR)(SQLHSTMT hstmt,
      SQLUSMALLINT icol, SQLUSMALLINT fDescType, SQLPOINTER rgbDesc,
      SQLSMALLINT cbDescMax, SQLSMALLINT * pcbDesc,
      SQLINTEGER * pfDesc);
typedef SQLRETURN SQL_API (*SQLConnectWPTR)(SQLHDBC hdbc,
      SQLWCHAR * szDSN, SQLSMALLINT cbDSN, SQLWCHAR * szUID,
      SQLSMALLINT cbUID, SQLWCHAR * szAuthStr,
      SQLSMALLINT cbAuthStr);
typedef SQLRETURN SQL_API (*SQLDescribeColWPTR)(SQLHSTMT hstmt,
      SQLUSMALLINT icol, SQLWCHAR * szColName, SQLSMALLINT cbColNameMax,
      SQLSMALLINT * pcbColName, SQLSMALLINT * pfSqlType,
      SQLUINTEGER * pcbColDef, SQLSMALLINT * pibScale,
      SQLSMALLINT * pfNullable);
typedef SQLRETURN SQL_API (*SQLErrorWPTR)(SQLHENV henv,
      SQLHDBC hdbc, SQLHSTMT hstmt, SQLWCHAR * szSqlState,
      SQLINTEGER * pfNativeError, SQLWCHAR * szErrorMsg,
      SQLSMALLINT cbErrorMsgMax, SQLSMALLINT * pcbErrorMsg);
typedef SQLRETURN SQL_API (*SQLExecDirectWPTR)(SQLHSTMT hstmt,
      SQLWCHAR * szSqlStr, SQLINTEGER cbSqlStr);
typedef SQLRETURN SQL_API (*SQLGetConnectAttrWPTR)(SQLHDBC hdbc,
      SQLINTEGER fAttribute, SQLPOINTER rgbValue,
      SQLINTEGER cbValueMax, SQLINTEGER * pcbValue);
typedef SQLRETURN SQL_API (*SQLGetCursorNameWPTR)(SQLHSTMT hstmt,
      SQLWCHAR * szCursor, SQLSMALLINT cbCursorMax,
      SQLSMALLINT * pcbCursor);
typedef SQLRETURN SQL_API (*SQLSetDescFieldWPTR)(SQLHDESC DescriptorHandle,
      SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
      SQLPOINTER Value, SQLINTEGER BufferLength);
typedef SQLRETURN SQL_API (*SQLGetDescFieldWPTR)(SQLHDESC hdesc,
      SQLSMALLINT iRecord, SQLSMALLINT iField,
      SQLPOINTER rgbValue, SQLINTEGER cbValueMax,
      SQLINTEGER * pcbValue);
typedef SQLRETURN SQL_API (*SQLGetDescRecWPTR)(SQLHDESC hdesc,
      SQLSMALLINT iRecord, SQLWCHAR * szName, SQLSMALLINT cbNameMax,
      SQLSMALLINT * pcbName, SQLSMALLINT * pfType, SQLSMALLINT * pfSubType,
      SQLINTEGER * pLength, SQLSMALLINT * pPrecision, SQLSMALLINT * pScale,
      SQLSMALLINT * pNullable);
typedef SQLRETURN SQL_API (*SQLGetDiagFieldWPTR)(SQLSMALLINT fHandleType,
      SQLHANDLE handle, SQLSMALLINT iRecord, SQLSMALLINT fDiagField,
      SQLPOINTER rgbDiagInfo, SQLSMALLINT cbDiagInfoMax,
      SQLSMALLINT * pcbDiagInfo);
typedef SQLRETURN SQL_API (*SQLGetDiagRecWPTR)(SQLSMALLINT fHandleType,
      SQLHANDLE handle, SQLSMALLINT iRecord, SQLWCHAR * szSqlState,
      SQLINTEGER * pfNativeError, SQLWCHAR * szErrorMsg,
      SQLSMALLINT cbErrorMsgMax, SQLSMALLINT * pcbErrorMsg);
typedef SQLRETURN SQL_API (*SQLPrepareWPTR)(SQLHSTMT hstmt,
      SQLWCHAR * szSqlStr, SQLINTEGER cbSqlStr);
typedef SQLRETURN SQL_API (*SQLSetConnectAttrWPTR)(SQLHDBC hdbc,
      SQLINTEGER fAttribute, SQLPOINTER rgbValue, SQLINTEGER cbValue);
typedef SQLRETURN SQL_API (*SQLSetCursorNameWPTR)(SQLHSTMT hstmt,
      SQLWCHAR * szCursor, SQLSMALLINT cbCursor);
typedef SQLRETURN SQL_API (*SQLColumnsWPTR)(SQLHSTMT hstmt,
      SQLWCHAR * szCatalogName, SQLSMALLINT cbCatalogName,
      SQLWCHAR * szSchemaName, SQLSMALLINT cbSchemaName,
      SQLWCHAR * szTableName, SQLSMALLINT cbTableName,
      SQLWCHAR * szColumnName, SQLSMALLINT cbColumnName);
typedef SQLRETURN SQL_API (*SQLGetConnectOptionWPTR)(SQLHDBC hdbc,
      SQLUSMALLINT fOption, SQLPOINTER pvParam);
typedef SQLRETURN SQL_API (*SQLGetInfoWPTR)(SQLHDBC hdbc,
      SQLUSMALLINT fInfoType, SQLPOINTER rgbInfoValue,
      SQLSMALLINT cbInfoValueMax, SQLSMALLINT * pcbInfoValue);
typedef SQLRETURN SQL_API (*SQLGetTypeInfoWPTR)(SQLHSTMT StatementHandle,
      SQLSMALLINT DataTyoe);
typedef SQLRETURN SQL_API (*SQLSetConnectOptionWPTR)(SQLHDBC hdbc,
      SQLUSMALLINT fOption, SQLUINTEGER vParam);
typedef SQLRETURN SQL_API (*SQLSpecialColumnsWPTR)(SQLHSTMT hstmt,
      SQLUSMALLINT fColType, SQLWCHAR * szCatalogName,
      SQLSMALLINT cbCatalogName, SQLWCHAR * szSchemaName,
      SQLSMALLINT cbSchemaName, SQLWCHAR * szTableName,
      SQLSMALLINT cbTableName, SQLUSMALLINT fScope,
      SQLUSMALLINT fNullable);
typedef SQLRETURN SQL_API (*SQLStatisticsWPTR)(SQLHSTMT hstmt,
      SQLWCHAR * szCatalogName, SQLSMALLINT cbCatalogName,
      SQLWCHAR * szSchemaName, SQLSMALLINT cbSchemaName,
      SQLWCHAR * szTableName, SQLSMALLINT cbTableName,
      SQLUSMALLINT fUnique, SQLUSMALLINT fAccuracy);
typedef SQLRETURN SQL_API (*SQLTablesWPTR)(SQLHSTMT hstmt,
      SQLWCHAR * szCatalogName, SQLSMALLINT cbCatalogName,
      SQLWCHAR * szSchemaName, SQLSMALLINT cbSchemaName,
      SQLWCHAR * szTableName, SQLSMALLINT cbTableName,
      SQLWCHAR * szTableType, SQLSMALLINT cbTableType);
typedef SQLRETURN SQL_API (*SQLDataSourcesWPTR)(SQLHENV henv,
      SQLUSMALLINT fDirection, SQLWCHAR * szDSN,
      SQLSMALLINT cbDSNMax, SQLSMALLINT * pcbDSN,
      SQLWCHAR * szDescription, SQLSMALLINT cbDescriptionMax,
      SQLSMALLINT * pcbDescription);
typedef SQLRETURN SQL_API (*SQLDriverConnectWPTR)(SQLHDBC hdbc,
      SQLHWND hwnd, SQLWCHAR * szConnStrIn,
      SQLSMALLINT cbConnStrIn, SQLWCHAR * szConnStrOut,
      SQLSMALLINT cbConnStrOutMax, SQLSMALLINT * pcbConnStrOut,
      SQLUSMALLINT fDriverCompletion);
typedef SQLRETURN SQL_API (*SQLBrowseConnectWPTR)(SQLHDBC hdbc,
      SQLWCHAR * szConnStrIn, SQLSMALLINT cbConnStrIn,
      SQLWCHAR * szConnStrOut, SQLSMALLINT cbConnStrOutMax,
      SQLSMALLINT * pcbConnStrOut);
typedef SQLRETURN SQL_API (*SQLColumnPrivilegesWPTR)(SQLHSTMT hstmt,
      SQLWCHAR * szCatalogName, SQLSMALLINT cbCatalogName,
      SQLWCHAR * szSchemaName, SQLSMALLINT cbSchemaName,
      SQLWCHAR * szTableName, SQLSMALLINT cbTableName,
      SQLWCHAR * szColumnName, SQLSMALLINT cbColumnName);
typedef SQLRETURN SQL_API (*SQLGetStmtAttrWPTR)(SQLHSTMT hstmt,
      SQLINTEGER fAttribute, SQLPOINTER rgbValue,
      SQLINTEGER cbValueMax, SQLINTEGER * pcbValue);
typedef SQLRETURN SQL_API (*SQLSetStmtAttrWPTR)(SQLHSTMT hstmt,
      SQLINTEGER fAttribute, SQLPOINTER rgbValue,
      SQLINTEGER cbValueMax);
typedef SQLRETURN SQL_API (*SQLForeignKeysWPTR)(SQLHSTMT hstmt,
      SQLWCHAR * szPkCatalogName, SQLSMALLINT cbPkCatalogName,
      SQLWCHAR * szPkSchemaName, SQLSMALLINT cbPkSchemaName,
      SQLWCHAR * szPkTableName, SQLSMALLINT cbPkTableName,
      SQLWCHAR * szFkCatalogName, SQLSMALLINT cbFkCatalogName,
      SQLWCHAR * szFkSchemaName, SQLSMALLINT cbFkSchemaName,
      SQLWCHAR * szFkTableName, SQLSMALLINT cbFkTableName);
typedef SQLRETURN SQL_API (*SQLNativeSqlWPTR)(SQLHDBC hdbc,
      SQLWCHAR * szSqlStrIn, SQLINTEGER cbSqlStrIn,
      SQLWCHAR * szSqlStr, SQLINTEGER cbSqlStrMax,
      SQLINTEGER * pcbSqlStr);
typedef SQLRETURN SQL_API (*SQLPrimaryKeysWPTR)(SQLHSTMT hstmt,
      SQLWCHAR * szCatalogName, SQLSMALLINT cbCatalogName,
      SQLWCHAR * szSchemaName, SQLSMALLINT cbSchemaName,
      SQLWCHAR * szTableName, SQLSMALLINT cbTableName);
typedef SQLRETURN SQL_API (*SQLProcedureColumnsWPTR)(SQLHSTMT hstmt,
      SQLWCHAR * szCatalogName, SQLSMALLINT cbCatalogName,
      SQLWCHAR * szSchemaName, SQLSMALLINT cbSchemaName,
      SQLWCHAR * szProcName, SQLSMALLINT cbProcName,
      SQLWCHAR * szColumnName, SQLSMALLINT cbColumnName);
typedef SQLRETURN SQL_API (*SQLProceduresWPTR)(SQLHSTMT hstmt,
      SQLWCHAR * szCatalogName, SQLSMALLINT cbCatalogName,
      SQLWCHAR * szSchemaName, SQLSMALLINT cbSchemaName,
      SQLWCHAR * szProcName, SQLSMALLINT cbProcName);
typedef SQLRETURN SQL_API (*SQLTablePrivilegesWPTR)(SQLHSTMT hstmt,
      SQLWCHAR * szCatalogName, SQLSMALLINT cbCatalogName,
      SQLWCHAR * szSchemaName, SQLSMALLINT cbSchemaName,
      SQLWCHAR * szTableName, SQLSMALLINT cbTableName);
typedef SQLRETURN SQL_API (*SQLDriversWPTR)(SQLHENV henv,
      SQLUSMALLINT fDirection, SQLWCHAR * szDriverDesc,
      SQLSMALLINT cbDriverDescMax, SQLSMALLINT * pcbDriverDesc,
      SQLWCHAR * szDriverAttributes, SQLSMALLINT cbDrvrAttrMax,
      SQLSMALLINT * pcbDrvrAttr);
/* ANSI versions */
typedef SQLRETURN SQL_API (*SQLColAttributeAPTR)(SQLHSTMT hstmt,
      SQLSMALLINT iCol, SQLSMALLINT iField, SQLPOINTER pCharAttr,
      SQLSMALLINT cbCharAttrMax, SQLSMALLINT * pcbCharAttr,
      SQLPOINTER pNumAttr);
typedef SQLRETURN SQL_API (*SQLColAttributesAPTR)(SQLHSTMT hstmt,
      SQLUSMALLINT icol, SQLUSMALLINT fDescType,
      SQLPOINTER rgbDesc, SQLSMALLINT cbDescMax,
      SQLSMALLINT * pcbDesc, SQLINTEGER * pfDesc);
typedef SQLRETURN SQL_API (*SQLConnectAPTR)(SQLHDBC hdbc,
      SQLCHAR * szDSN, SQLSMALLINT cbDSN, SQLCHAR * szUID,
      SQLSMALLINT cbUID, SQLCHAR * szAuthStr, SQLSMALLINT cbAuthStr);
typedef SQLRETURN SQL_API (*SQLDescribeColAPTR)(SQLHSTMT hstmt,
      SQLUSMALLINT icol, SQLCHAR * szColName, SQLSMALLINT cbColNameMax,
      SQLSMALLINT * pcbColName, SQLSMALLINT * pfSqlType, SQLUINTEGER * pcbColDef,
      SQLSMALLINT * pibScale, SQLSMALLINT * pfNullable);
typedef SQLRETURN SQL_API (*SQLErrorAPTR)(SQLHENV henv,
      SQLHDBC hdbc, SQLHSTMT hstmt, SQLCHAR * szSqlState,
      SQLINTEGER * pfNativeError, SQLCHAR * szErrorMsg,
      SQLSMALLINT cbErrorMsgMax, SQLSMALLINT * pcbErrorMsg);
typedef SQLRETURN SQL_API (*SQLExecDirectAPTR)(SQLHSTMT hstmt,
      SQLCHAR * szSqlStr, SQLINTEGER cbSqlStr);
typedef SQLRETURN SQL_API (*SQLGetConnectAttrAPTR)(SQLHDBC hdbc,
      SQLINTEGER fAttribute, SQLPOINTER rgbValue, SQLINTEGER cbValueMax,
      SQLINTEGER * pcbValue);
typedef SQLRETURN SQL_API (*SQLGetCursorNameAPTR)(SQLHSTMT hstmt,
      SQLCHAR * szCursor, SQLSMALLINT cbCursorMax,
      SQLSMALLINT * pcbCursor);
typedef SQLRETURN SQL_API (*SQLSetDescFieldAPTR)(SQLHDESC DescriptorHandle,
      SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
      SQLPOINTER Value, SQLINTEGER BufferLength);
typedef SQLRETURN SQL_API (*SQLGetDescFieldAPTR)(SQLHDESC hdesc,
      SQLSMALLINT iRecord, SQLSMALLINT iField, SQLPOINTER rgbValue,
      SQLINTEGER cbValueMax, SQLINTEGER * pcbValue);
typedef SQLRETURN SQL_API (*SQLGetDescRecAPTR)(SQLHDESC hdesc,
      SQLSMALLINT iRecord, SQLCHAR * szName, SQLSMALLINT cbNameMax,
      SQLSMALLINT * pcbName, SQLSMALLINT * pfType, SQLSMALLINT * pfSubType,
      SQLINTEGER * pLength, SQLSMALLINT * pPrecision,
      SQLSMALLINT * pScale, SQLSMALLINT * pNullable);
typedef SQLRETURN SQL_API (*SQLGetDiagFieldAPTR)(SQLSMALLINT fHandleType,
      SQLHANDLE handle, SQLSMALLINT iRecord, SQLSMALLINT fDiagField,
      SQLPOINTER rgbDiagInfo, SQLSMALLINT cbDiagInfoMax,
      SQLSMALLINT * pcbDiagInfo);
typedef SQLRETURN SQL_API (*SQLGetDiagRecAPTR)(SQLSMALLINT fHandleType,
      SQLHANDLE handle, SQLSMALLINT iRecord, SQLCHAR * szSqlState,
      SQLINTEGER * pfNativeError, SQLCHAR * szErrorMsg,
      SQLSMALLINT cbErrorMsgMax, SQLSMALLINT * pcbErrorMsg);
typedef SQLRETURN SQL_API (*SQLGetStmtAttrAPTR)(SQLHSTMT hstmt,
      SQLINTEGER fAttribute, SQLPOINTER rgbValue,
      SQLINTEGER cbValueMax, SQLINTEGER * pcbValue);
typedef SQLRETURN SQL_API (*SQLGetTypeInfoAPTR)(SQLHSTMT StatementHandle,
      SQLSMALLINT DataTyoe);
typedef SQLRETURN SQL_API (*SQLPrepareAPTR)(SQLHSTMT hstmt,
      SQLCHAR * szSqlStr, SQLINTEGER cbSqlStr);
typedef SQLRETURN SQL_API (*SQLSetConnectAttrAPTR)(SQLHDBC hdbc,
      SQLINTEGER fAttribute, SQLPOINTER rgbValue, SQLINTEGER cbValue);
typedef SQLRETURN SQL_API (*SQLSetCursorNameAPTR)(SQLHSTMT hstmt,
      SQLCHAR * szCursor, SQLSMALLINT cbCursor);
typedef SQLRETURN SQL_API (*SQLColumnsAPTR)(SQLHSTMT hstmt,
      SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName,
      SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName,
      SQLCHAR * szTableName, SQLSMALLINT cbTableName,
      SQLCHAR * szColumnName, SQLSMALLINT cbColumnName);
typedef SQLRETURN SQL_API (*SQLGetConnectOptionAPTR)(SQLHDBC hdbc,
      SQLUSMALLINT fOption, SQLPOINTER pvParam);
typedef SQLRETURN SQL_API (*SQLGetInfoAPTR)(SQLHDBC hdbc,
      SQLUSMALLINT fInfoType, SQLPOINTER rgbInfoValue,
      SQLSMALLINT cbInfoValueMax, SQLSMALLINT * pcbInfoValue);
typedef SQLRETURN SQL_API (*SQLGetStmtOptionAPTR)(SQLHSTMT hstmt,
      SQLUSMALLINT fOption, SQLPOINTER pvParam);
typedef SQLRETURN SQL_API (*SQLSetConnectOptionAPTR)(SQLHDBC hdbc,
      SQLUSMALLINT fOption, SQLUINTEGER vParam);
typedef SQLRETURN SQL_API (*SQLSetStmtOptionAPTR)(SQLHSTMT hstmt,
      SQLUSMALLINT fOption, SQLUINTEGER vParam);
typedef SQLRETURN SQL_API (*SQLSpecialColumnsAPTR)(SQLHSTMT hstmt,
      SQLUSMALLINT fColType, SQLCHAR * szCatalogName,
      SQLSMALLINT cbCatalogName, SQLCHAR * szSchemaName,
      SQLSMALLINT cbSchemaName, SQLCHAR * szTableName,
      SQLSMALLINT cbTableName, SQLUSMALLINT fScope,
      SQLUSMALLINT fNullable);
typedef SQLRETURN SQL_API (*SQLStatisticsAPTR)(SQLHSTMT hstmt,
      SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName,
      SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName,
      SQLCHAR * szTableName, SQLSMALLINT cbTableName,
      SQLUSMALLINT fUnique, SQLUSMALLINT fAccuracy);
typedef SQLRETURN SQL_API (*SQLTablesAPTR)(SQLHSTMT hstmt,
      SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName,
      SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName,
      SQLCHAR * szTableName, SQLSMALLINT cbTableName,
      SQLCHAR * szTableType, SQLSMALLINT cbTableType);
typedef SQLRETURN SQL_API (*SQLDataSourcesAPTR)(SQLHENV henv,
      SQLUSMALLINT fDirection, SQLCHAR * szDSN,
      SQLSMALLINT cbDSNMax, SQLSMALLINT * pcbDSN,
      SQLCHAR * szDescription, SQLSMALLINT cbDescriptionMax,
      SQLSMALLINT * pcbDescription);
typedef SQLRETURN SQL_API (*SQLDriverConnectAPTR)(SQLHDBC hdbc,
      SQLHWND hwnd, SQLCHAR * szConnStrIn, SQLSMALLINT cbConnStrIn,
      SQLCHAR * szConnStrOut, SQLSMALLINT cbConnStrOutMax,
      SQLSMALLINT * pcbConnStrOut, SQLUSMALLINT fDriverCompletion);
typedef SQLRETURN SQL_API (*SQLBrowseConnectAPTR)(SQLHDBC hdbc,
      SQLCHAR * szConnStrIn, SQLSMALLINT cbConnStrIn,
      SQLCHAR * szConnStrOut, SQLSMALLINT cbConnStrOutMax,
      SQLSMALLINT * pcbConnStrOut);
typedef SQLRETURN SQL_API (*SQLColumnPrivilegesAPTR)(SQLHSTMT hstmt,
      SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName,
      SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName,
      SQLCHAR * szTableName, SQLSMALLINT cbTableName,
      SQLCHAR * szColumnName, SQLSMALLINT cbColumnName);
typedef SQLRETURN SQL_API (*SQLDescribeParamAPTR)(SQLHSTMT hstmt,
      SQLUSMALLINT ipar, SQLSMALLINT * pfSqlType,
      SQLUINTEGER * pcbParamDef, SQLSMALLINT * pibScale,
      SQLSMALLINT * pfNullable);
typedef SQLRETURN SQL_API (*SQLSetStmtAttrAPTR)(SQLHSTMT hstmt,
      SQLINTEGER fAttribute, SQLPOINTER rgbValue,
      SQLINTEGER cbValueMax);
typedef SQLRETURN SQL_API (*SQLForeignKeysAPTR)(SQLHSTMT hstmt,
      SQLCHAR * szPkCatalogName, SQLSMALLINT cbPkCatalogName,
      SQLCHAR * szPkSchemaName, SQLSMALLINT cbPkSchemaName,
      SQLCHAR * szPkTableName, SQLSMALLINT cbPkTableName,
      SQLCHAR * szFkCatalogName, SQLSMALLINT cbFkCatalogName,
      SQLCHAR * szFkSchemaName, SQLSMALLINT cbFkSchemaName,
      SQLCHAR * szFkTableName, SQLSMALLINT cbFkTableName);
typedef SQLRETURN SQL_API (*SQLNativeSqlAPTR)(SQLHDBC hdbc,
      SQLCHAR * szSqlStrIn, SQLINTEGER cbSqlStrIn,
      SQLCHAR * szSqlStr, SQLINTEGER cbSqlStrMax,
      SQLINTEGER * pcbSqlStr);
typedef SQLRETURN SQL_API (*SQLPrimaryKeysAPTR)(SQLHSTMT hstmt,
      SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName,
      SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName,
      SQLCHAR * szTableName, SQLSMALLINT cbTableName);
typedef SQLRETURN SQL_API (*SQLProcedureColumnsAPTR)(SQLHSTMT hstmt,
      SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName,
      SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName,
      SQLCHAR * szProcName, SQLSMALLINT cbProcName,
      SQLCHAR * szColumnName, SQLSMALLINT cbColumnName);
typedef SQLRETURN SQL_API (*SQLProceduresAPTR)(SQLHSTMT hstmt,
      SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName,
      SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName,
      SQLCHAR * szProcName, SQLSMALLINT cbProcName);
typedef SQLRETURN SQL_API (*SQLTablePrivilegesAPTR)(SQLHSTMT hstmt,
      SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName,
      SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName,
      SQLCHAR * szTableName, SQLSMALLINT cbTableName);
typedef SQLRETURN SQL_API (*SQLDriversAPTR)(SQLHENV henv,
      SQLUSMALLINT fDirection, SQLCHAR * szDriverDesc,
      SQLSMALLINT cbDriverDescMax, SQLSMALLINT * pcbDriverDesc,
      SQLCHAR * szDriverAttributes, SQLSMALLINT cbDrvrAttrMax,
      SQLSMALLINT * pcbDrvrAttr);
/* iODBCinst bundle prototypes functions */
typedef BOOL INSTAPI (*SQLConfigDataSourcePTR) (HWND hwndParent,
      WORD fRequest, LPCSTR lpszDriver, LPCSTR lpszAttributes);
typedef BOOL INSTAPI (*SQLConfigDriverPTR) (HWND hwndParent,
      WORD fRequest, LPCSTR lpszDriver,
      LPCSTR lpszArgs, LPSTR lpszMsg, WORD cbMsgMax, WORD FAR * pcbMsgOut);
typedef BOOL INSTAPI (*SQLCreateDataSourcePTR) (HWND hwndParent, LPCSTR lpszDSN);
typedef BOOL INSTAPI (*SQLGetConfigModePTR) (UWORD * pwConfigMode);
typedef BOOL INSTAPI (*SQLGetAvailableDriversPTR) (LPCSTR lpszInfFile,
      LPSTR lpszBuf, WORD cbBufMax, WORD FAR * pcbBufOut);
typedef BOOL INSTAPI (*SQLGetInstalledDriversPTR) (LPSTR lpszBuf,
      WORD cbBufMax, WORD FAR * pcbBufOut);
typedef int INSTAPI (*SQLGetPrivateProfileStringPTR) (LPCSTR lpszSection,
      LPCSTR lpszEntry, LPCSTR lpszDefault,
      LPSTR lpszRetBuffer, int cbRetBuffer, LPCSTR lpszFilename);
typedef BOOL INSTAPI (*SQLGetTranslatorPTR) (HWND hwnd,
      LPSTR lpszName, WORD cbNameMax,
      WORD FAR * pcbNameOut, LPSTR lpszPath,
      WORD cbPathMax, WORD FAR * pcbPathOut, DWORD FAR * pvOption);
typedef BOOL INSTAPI (*SQLInstallDriverExPTR) (LPCSTR lpszDriver,
      LPCSTR lpszPathIn, LPSTR lpszPathOut,
      WORD cbPathOutMax, WORD * pcbPathOut, WORD fRequest, LPDWORD lpdwUsageCount);
typedef BOOL INSTAPI (*SQLInstallDriverPTR) (LPCSTR lpszInfFile,
      LPCSTR lpszDriver, LPSTR lpszPath, WORD cbPathMax, WORD FAR * pcbPathOut);
typedef BOOL INSTAPI (*SQLInstallDriverManagerPTR) (LPSTR lpszPath,
      WORD cbPathMax, WORD FAR * pcbPathOut);
typedef RETCODE INSTAPI (*SQLInstallerErrorPTR) (WORD iError,
      DWORD * pfErrorCode,
      LPSTR lpszErrorMsg, WORD cbErrorMsgMax, WORD * pcbErrorMsg);
typedef BOOL INSTAPI (*SQLInstallODBCPTR) (HWND hwndParent,
      LPCSTR lpszInfFile, LPCSTR lpszSrcPath, LPCSTR lpszDrivers);
typedef BOOL INSTAPI (*SQLInstallTranslatorPTR) (LPCSTR lpszInfFile,
      LPCSTR lpszTranslator, LPCSTR lpszPathIn,
      LPSTR lpszPathOut, WORD cbPathOutMax,
      WORD FAR * pcbPathOut, WORD fRequest, LPDWORD lpdwUsageCount);
typedef BOOL INSTAPI (*SQLInstallTranslatorExPTR) (LPCSTR lpszTranslator,
      LPCSTR lpszPathIn, LPSTR lpszPathOut,
      WORD cbPathOutMax, WORD * pcbPathOut, WORD fRequest, LPDWORD lpdwUsageCount);
typedef BOOL INSTAPI (*SQLManageDataSourcesPTR) (HWND hwndParent);
typedef RETCODE INSTAPI (*SQLPostInstallerErrorPTR) (DWORD fErrorCode, LPSTR szErrorMsg);
typedef BOOL INSTAPI (*SQLReadFileDSNPTR) (LPCSTR lpszFileName,
      LPCSTR lpszAppName, LPCSTR lpszKeyName, LPSTR lpszString, WORD cbString, WORD * pcbString);
typedef BOOL INSTAPI (*SQLRemoveDSNFromIniPTR) (LPCSTR lpszDSN);
typedef BOOL INSTAPI (*SQLRemoveDefaultDataSourcePTR) (void);
typedef BOOL INSTAPI (*SQLRemoveDriverPTR) (LPCSTR lpszDriver,
      BOOL fRemoveDSN, LPDWORD lpdwUsageCount);
typedef BOOL INSTAPI (*SQLRemoveDriverManagerPTR) (LPDWORD lpdwUsageCount);
typedef BOOL INSTAPI (*SQLRemoveTranslatorPTR) (LPCSTR lpszTranslator,
      LPDWORD lpdwUsageCount);
typedef BOOL INSTAPI (*SQLSetConfigModePTR) (UWORD wConfigMode);
typedef BOOL INSTAPI (*SQLValidDSNPTR) (LPCSTR lpszDSN);
typedef BOOL INSTAPI (*SQLWriteDSNToIniPTR) (LPCSTR lpszDSN, LPCSTR lpszDriver);
typedef BOOL INSTAPI (*SQLWriteFileDSNPTR) (LPCSTR lpszFileName,
      LPCSTR lpszAppName, LPCSTR lpszKeyName, LPSTR lpszString);
typedef BOOL INSTAPI (*SQLWritePrivateProfileStringPTR) (LPCSTR lpszSection,
      LPCSTR lpszEntry, LPCSTR lpszString, LPCSTR lpszFilename);
typedef BOOL INSTAPI (*SQLGetKeywordValuePTR) (LPCSTR lpszSection,
      LPCSTR lpszEntry, LPSTR lpszBuffer, int cbBuffer, int *pcbBufOut);
typedef BOOL INSTAPI (*SQLSetKeywordValuePTR) (LPCSTR lpszSection,
      LPCSTR lpszEntry, LPSTR lpszString, int cbString);
typedef BOOL INSTAPI (*SQLWriteDSNPTR) (LPCSTR lpszDSN, LPCSTR lpszDriver);
typedef BOOL INSTAPI (*SQLRemoveDSNPTR) (LPCSTR lpszDSN);
/*Unicode functions  */
typedef BOOL INSTAPI (*SQLConfigDataSourceWPTR) (HWND hwndParent,
      WORD fRequest, LPCWSTR lpszDriver, LPCWSTR lpszAttributes);
typedef BOOL INSTAPI (*SQLConfigDriverWPTR) (HWND hwndParent,
      WORD fRequest, LPCWSTR lpszDriver,
      LPCWSTR lpszArgs, LPWSTR lpszMsg, WORD cbMsgMax, WORD FAR * pcbMsgOut);
typedef BOOL INSTAPI (*SQLCreateDataSourceWPTR) (HWND hwndParent, LPCWSTR lpszDSN);
typedef BOOL INSTAPI (*SQLGetAvailableDriversWPTR) (LPCWSTR lpszInfFile,
      LPWSTR lpszBuf, WORD cbBufMax, WORD FAR * pcbBufOut);
typedef BOOL INSTAPI (*SQLGetKeywordValueWPTR) (LPCWSTR lpszSection,
      LPCWSTR lpszEntry, LPWSTR lpszBuffer, int cbBuffer, int *pcbBufOut);
typedef BOOL INSTAPI (*SQLGetInstalledDriversWPTR) (LPWSTR lpszBuf,
      WORD cbBufMax, WORD FAR * pcbBufOut);
typedef int INSTAPI (*SQLGetPrivateProfileStringWPTR) (LPCWSTR lpszSection,
      LPCWSTR lpszEntry, LPCWSTR lpszDefault,
      LPWSTR lpszRetBuffer, int cbRetBuffer, LPCWSTR lpszFilename);
typedef BOOL INSTAPI (*SQLGetTranslatorWPTR) (HWND hwnd,
      LPWSTR lpszName, WORD cbNameMax,
      WORD FAR * pcbNameOut, LPWSTR lpszPath,
      WORD cbPathMax, WORD FAR * pcbPathOut, DWORD FAR * pvOption);
typedef BOOL INSTAPI (*SQLInstallDriverExWPTR) (LPCWSTR lpszDriver,
      LPCWSTR lpszPathIn, LPWSTR lpszPathOut,
      WORD cbPathOutMax, WORD * pcbPathOut, WORD fRequest, LPDWORD lpdwUsageCount);
typedef BOOL INSTAPI (*SQLInstallDriverWPTR) (LPCWSTR lpszInfFile,
      LPCWSTR lpszDriver, LPWSTR lpszPath, WORD cbPathMax, WORD FAR * pcbPathOut);
typedef RETCODE INSTAPI (*SQLInstallerErrorWPTR) (WORD iError,
      DWORD * pfErrorCode,
      LPWSTR lpszErrorMsg, WORD cbErrorMsgMax, WORD * pcbErrorMsg);
typedef BOOL INSTAPI (*SQLInstallDriverManagerWPTR) (LPWSTR lpszPath,
      WORD cbPathMax, WORD FAR * pcbPathOut);
typedef BOOL INSTAPI (*SQLInstallODBCWPTR) (HWND hwndParent,
      LPCWSTR lpszInfFile, LPCWSTR lpszSrcPath, LPCWSTR lpszDrivers);
typedef BOOL INSTAPI (*SQLInstallTranslatorWPTR) (LPCWSTR lpszInfFile,
      LPCWSTR lpszTranslator, LPCWSTR lpszPathIn,
      LPWSTR lpszPathOut, WORD cbPathOutMax,
      WORD FAR * pcbPathOut, WORD fRequest, LPDWORD lpdwUsageCount);
typedef BOOL INSTAPI (*SQLInstallTranslatorExWPTR) (LPCWSTR lpszTranslator,
      LPCWSTR lpszPathIn, LPWSTR lpszPathOut,
      WORD cbPathOutMax, WORD * pcbPathOut, WORD fRequest, LPDWORD lpdwUsageCount);
typedef RETCODE INSTAPI (*SQLPostInstallerErrorWPTR) (DWORD fErrorCode, LPWSTR szErrorMsg);
typedef BOOL INSTAPI (*SQLRemoveDriverWPTR) (LPCWSTR lpszDriver,
      BOOL fRemoveDSN, LPDWORD lpdwUsageCount);
typedef BOOL INSTAPI (*SQLReadFileDSNWPTR) (LPCWSTR lpszFileName,
      LPCWSTR lpszAppName, LPCWSTR lpszKeyName, LPWSTR lpszString, WORD cbString, WORD * pcbString);
typedef BOOL INSTAPI (*SQLRemoveDSNFromIniWPTR) (LPCWSTR lpszDSN);
typedef BOOL INSTAPI (*SQLRemoveTranslatorWPTR) (LPCWSTR lpszTranslator,
      LPDWORD lpdwUsageCount);
typedef BOOL INSTAPI (*SQLSetKeywordValueWPTR) (LPCWSTR lpszSection,
      LPCWSTR lpszEntry, LPWSTR lpszString, int cbString);
typedef BOOL INSTAPI (*SQLValidDSNWPTR) (LPCWSTR lpszDSN);
typedef BOOL INSTAPI (*SQLWriteDSNToIniWPTR) (LPCWSTR lpszDSN, LPCWSTR lpszDriver);
typedef BOOL INSTAPI (*SQLWriteFileDSNWPTR) (LPCWSTR lpszFileName,
      LPCWSTR lpszAppName, LPCWSTR lpszKeyName, LPWSTR lpszString);
typedef BOOL INSTAPI (*SQLWritePrivateProfileStringWPTR) (LPCWSTR lpszSection,
      LPCWSTR lpszEntry, LPCWSTR lpszString, LPCWSTR lpszFilename);
typedef BOOL INSTAPI (*SQLWriteDSNWPTR) (LPCWSTR lpszDSN, LPCWSTR lpszDriver);
typedef BOOL INSTAPI (*SQLRemoveDSNWPTR) (LPCWSTR lpszDSN);
