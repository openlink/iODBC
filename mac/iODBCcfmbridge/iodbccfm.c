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

#include "iodbccfm.h"

/* Global variables for the CFM library */
CFragConnectionID iodbcId, iodbcinstId;
CFBundleRef iodbcRef, iodbcinstRef;
int isOnMacOSX;

/* Function which return true if running on Mac OS X */
static int IS_MAC_X(void)
{
  UInt32 response;
  return (Gestalt(gestaltSystemVersion, (SInt32*)&response) == noErr)
     && (response >= 0x01000);
}

/* Function which load and get a reference of a Mac OS Classic library */
OSStatus load_library(Str255 name, CFragConnectionID *id)
{
  OSStatus err = noErr;
  Ptr main_addr;
  Str255 err_msg;

  /* Check at least a pointer is provided */
  if(!id) return noErr;
  *id = NULL;

  /* Load the library */
  err = GetSharedLibrary (name, kPowerPCCFragArch, kLoadCFrag, id, &main_addr, err_msg);
  if(err != noErr) goto end;

end:
  /* Make some cleaning */
  if(err != noErr && *id != NULL)
  {
    CloseConnection(id);
	*id = NULL;
  }

  return err;
}

/* Function which load and get a reference of a Mac OS X framework */
OSStatus load_framework(CFStringRef frame, CFBundleRef *bundle)
{
  CFURLRef bundleURL=NULL;
  OSStatus err = noErr;
  
  /* Check at least a pointer is provided */
  if(!bundle) return noErr;
  *bundle = NULL;
  
  /* Get the bundle URL */
  bundleURL = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, frame, kCFURLPOSIXPathStyle, true);
  if(bundleURL == NULL)
  {
	err = coreFoundationUnknownErr;
	goto end;
  }

  /* Get the bundle reference */  
  *bundle = CFBundleCreate(kCFAllocatorSystemDefault, bundleURL);
  if(*bundle == NULL)
  {
	err = coreFoundationUnknownErr;
	goto end;
  }

  /* Load the bundle */
  if(!CFBundleLoadExecutable(*bundle))
  {
	err = coreFoundationUnknownErr;
	goto end;
  }

end:
  /* Make some cleaning */
  if(err != noErr && *bundle != NULL)
  {
    CFRelease(*bundle);
	*bundle = NULL;
  }
  
  if(bundleURL)
    CFRelease(bundleURL);

  return err;
}

/* Function which display a message error */
void create_error (LPCSTR text, LPCSTR errmsg)
{
  char msg[1024], msg1[1024];
  SInt16 out;

  memcpy (msg + 1, text, msg[0] = STRLEN(text));
  memcpy (msg1 + 1, errmsg, msg1[0] = STRLEN(errmsg));
  StandardAlert (kAlertStopAlert, (const unsigned char*)msg, (const unsigned char*)msg1, NULL, &out);
}

/* The initialization entry point of the library */
void _init(void)
{
  OSStatus err;

  /* Check if it is running on Mac OS X native mode */
  isOnMacOSX = IS_MAC_X();

  /* Load the bundle or CFM library */
  if(isOnMacOSX)
  {
    err = load_framework(CFSTR("/Library/Frameworks/iODBC.framework"), &iodbcRef);
    if(err != noErr)
    {
      err = load_framework(CFSTR("/System/Library/Frameworks/iODBC.framework"), &iodbcRef);
      if(err != noErr)
      {
        create_error("Framework loading problem",
          "The iODBC.framework component cannot be loaded. Please check if iODBC is"
          " well installed on your Mac OS X system.");
        return;
	  }
    }
    err = load_framework(CFSTR("/Library/Frameworks/iODBCinst.framework"), &iodbcinstRef);
    if(err != noErr)
    {
      err = load_framework(CFSTR("/System/Library/Frameworks/iODBCinst.framework"), &iodbcinstRef);
      if(err != noErr)
      {
        create_error("Framework loading problem",
          "The iODBCinst.framework component cannot be loaded. Please check if iODBC is"
          " well installed on your Mac OS X system.");
        return;
	  }
    }
  }
  else
  {
    err = load_library("\piodbc:ODBC$DriverMgr", &iodbcId);
    if(err != noErr)
    {
      create_error("Extension loading problem",
        "The ODBC Driver Manager PPC extension cannot be loaded. Please check if iODBC is"
        " well installed on your Mac OS system.");
      return;
    }
    err = load_library("\piodbc:ODBC$ConfigMgr", &iodbcinstId);
    if(err != noErr)
    {
      create_error("Extension loading problem",
        "The ODBC Configuration Manager PPC extension cannot be loaded. Please check if iODBC is"
        " well installed on your Mac OS system.");
      return;
    }
  }
}

/* iODBC hook functions */
SQLRETURN SQL_API SQLAllocConnect (SQLHENV EnvironmentHandle,
      SQLHDBC * ConnectionHandle)
{
  SQLAllocConnectPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLAllocConnectPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLAllocConnect"));

    if(func) return func(EnvironmentHandle, ConnectionHandle);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLAllocConnect", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(EnvironmentHandle, ConnectionHandle);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLAllocEnv (SQLHENV * EnvironmentHandle)
{
  SQLAllocEnvPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLAllocEnvPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLAllocEnv"));

    if(func) return func(EnvironmentHandle);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLAllocEnv", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(EnvironmentHandle);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }
  
  return SQL_ERROR;
}

SQLRETURN SQL_API SQLAllocHandle (SQLSMALLINT HandleType,
      SQLHANDLE InputHandle, SQLHANDLE * OutputHandle)
{
  SQLAllocHandlePTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLAllocHandlePTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLAllocHandle"));

    if(func) return func(HandleType, InputHandle, OutputHandle);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLAllocHandle", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(HandleType, InputHandle, OutputHandle);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLAllocStmt (SQLHDBC ConnectionHandle,
      SQLHSTMT * StatementHandle)
{
  SQLAllocStmtPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLAllocStmtPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLAllocStmt"));

    if(func) return func(ConnectionHandle, StatementHandle);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLAllocStmt", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(ConnectionHandle, StatementHandle);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLBindCol (SQLHSTMT StatementHandle,
      SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType,
      SQLPOINTER TargetValue, SQLINTEGER BufferLength, SQLINTEGER * StrLen_or_Ind)
{
  SQLBindColPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLBindColPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLBindCol"));

    if(func) return func(StatementHandle, ColumnNumber, TargetType, TargetValue, BufferLength, StrLen_or_Ind);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLBindCol", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, ColumnNumber, TargetType, TargetValue, BufferLength, StrLen_or_Ind);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLBindParam (SQLHSTMT StatementHandle,
      SQLUSMALLINT ParameterNumber, SQLSMALLINT ValueType,
      SQLSMALLINT ParameterType, SQLUINTEGER LengthPrecision,
      SQLSMALLINT ParameterScale, SQLPOINTER ParameterValue, SQLINTEGER * StrLen_or_Ind)
{
  SQLBindParamPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLBindParamPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLBindParam"));

    if(func) return func(StatementHandle, ParameterNumber, ValueType, ParameterType, LengthPrecision,
	  ParameterScale, ParameterValue, StrLen_or_Ind);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLBindParam", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, ParameterNumber, ValueType, ParameterType, LengthPrecision,
	  ParameterScale, ParameterValue, StrLen_or_Ind);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLBindParameter (SQLHSTMT hstmt,
      SQLUSMALLINT ipar, SQLSMALLINT fParamType,
      SQLSMALLINT fCType, SQLSMALLINT fSqlType,
      SQLUINTEGER cbColDef, SQLSMALLINT ibScale,
      SQLPOINTER rgbValue, SQLINTEGER cbValueMax, SQLINTEGER * pcbValue)
{
  SQLBindParameterPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLBindParameterPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLBindParameter"));

    if(func) return func(hstmt, ipar, fParamType, fCType, fSqlType, cbColDef,
	  ibScale, rgbValue, cbValueMax, pcbValue);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLBindParameter", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hstmt, ipar, fParamType, fCType, fSqlType, cbColDef,
	  ibScale, rgbValue, cbValueMax, pcbValue);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLBrowseConnect (SQLHDBC hdbc,
      SQLCHAR * szConnStrIn, SQLSMALLINT cbConnStrIn,
      SQLCHAR * szConnStrOut, SQLSMALLINT cbConnStrOutMax, SQLSMALLINT * pcbConnStrOut)
{
  SQLBrowseConnectPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLBrowseConnectPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLBrowseConnect"));

    if(func) return func(hdbc, szConnStrIn, cbConnStrIn, szConnStrOut,
	  cbConnStrOutMax, pcbConnStrOut);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLBrowseConnect", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hdbc, szConnStrIn, cbConnStrIn, szConnStrOut,
	  cbConnStrOutMax, pcbConnStrOut);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLBrowseConnectW (SQLHDBC hdbc,
      SQLWCHAR * szConnStrIn, SQLSMALLINT cbConnStrIn,
      SQLWCHAR * szConnStrOut, SQLSMALLINT cbConnStrOutMax, SQLSMALLINT * pcbConnStrOut)
{
  SQLBrowseConnectWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLBrowseConnectWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLBrowseConnectW"));

    if(func) return func(hdbc, szConnStrIn, cbConnStrIn, szConnStrOut,
	  cbConnStrOutMax, pcbConnStrOut);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLBrowseConnectW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hdbc, szConnStrIn, cbConnStrIn, szConnStrOut,
	  cbConnStrOutMax, pcbConnStrOut);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLBrowseConnectA (SQLHDBC hdbc,
      SQLCHAR * szConnStrIn, SQLSMALLINT cbConnStrIn,
      SQLCHAR * szConnStrOut, SQLSMALLINT cbConnStrOutMax, SQLSMALLINT * pcbConnStrOut)
{
  SQLBrowseConnectAPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLBrowseConnectAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLBrowseConnectA"));

    if(func) return func(hdbc, szConnStrIn, cbConnStrIn, szConnStrOut,
	  cbConnStrOutMax, pcbConnStrOut);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLBrowseConnectA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hdbc, szConnStrIn, cbConnStrIn, szConnStrOut,
	  cbConnStrOutMax, pcbConnStrOut);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLBulkOperations (SQLHSTMT StatementHandle,
      SQLSMALLINT Operation)
{
  SQLBulkOperationsPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLBulkOperationsPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLBulkOperations"));

    if(func) return func(StatementHandle, Operation);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLBulkOperations", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, Operation);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}


SQLRETURN SQL_API SQLCancel (SQLHSTMT StatementHandle)
{
  SQLCancelPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLCancelPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLCancel"));

    if(func) return func(StatementHandle);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLCancel", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLCloseCursor (SQLHSTMT StatementHandle)
{
  SQLCloseCursorPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLCloseCursorPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLCloseCursor"));

    if(func) return func(StatementHandle);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLCloseCursor", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLColAttribute (SQLHSTMT StatementHandle,
      SQLUSMALLINT ColumnNumber, SQLUSMALLINT FieldIdentifier,
      SQLPOINTER CharacterAttribute, SQLSMALLINT BufferLength,
      SQLSMALLINT * StringLength, SQLPOINTER NumericAttribute)
{
  SQLColAttributePTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLColAttributePTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLColAttribute"));

    if(func) return func(StatementHandle, ColumnNumber, FieldIdentifier,
	  CharacterAttribute, BufferLength, StringLength, NumericAttribute);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLColAttribute", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, ColumnNumber, FieldIdentifier,
	  CharacterAttribute, BufferLength, StringLength, NumericAttribute);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLColAttributeW (SQLHSTMT StatementHandle,
      SQLUSMALLINT ColumnNumber, SQLUSMALLINT FieldIdentifier,
      SQLPOINTER CharacterAttribute, SQLSMALLINT BufferLength,
      SQLSMALLINT * StringLength, SQLPOINTER NumericAttribute)
{
  SQLColAttributeWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLColAttributeWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLColAttributeW"));

    if(func) return func(StatementHandle, ColumnNumber, FieldIdentifier,
	  CharacterAttribute, BufferLength, StringLength, NumericAttribute);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLColAttributeW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, ColumnNumber, FieldIdentifier,
	  CharacterAttribute, BufferLength, StringLength, NumericAttribute);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLColAttributeA (SQLHSTMT StatementHandle,
      SQLUSMALLINT ColumnNumber, SQLUSMALLINT FieldIdentifier,
      SQLPOINTER CharacterAttribute, SQLSMALLINT BufferLength,
      SQLSMALLINT * StringLength, SQLPOINTER NumericAttribute)
{
  SQLColAttributeAPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLColAttributeAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLColAttributeA"));

    if(func) return func(StatementHandle, ColumnNumber, FieldIdentifier,
	  CharacterAttribute, BufferLength, StringLength, NumericAttribute);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLColAttributeA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, ColumnNumber, FieldIdentifier,
	  CharacterAttribute, BufferLength, StringLength, NumericAttribute);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLColAttributes (SQLHSTMT hstmt,
      SQLUSMALLINT icol, SQLUSMALLINT fDescType,
      SQLPOINTER rgbDesc, SQLSMALLINT cbDescMax, SQLSMALLINT * pcbDesc, SQLINTEGER * pfDesc)
{
  SQLColAttributesPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLColAttributesPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLColAttributes"));

    if(func) return func(hstmt, icol, fDescType, rgbDesc, cbDescMax, pcbDesc, pfDesc);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLColAttributes", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hstmt, icol, fDescType, rgbDesc, cbDescMax, pcbDesc, pfDesc);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLColAttributesW (SQLHSTMT hstmt,
      SQLUSMALLINT icol, SQLUSMALLINT fDescType,
      SQLPOINTER rgbDesc, SQLSMALLINT cbDescMax, SQLSMALLINT * pcbDesc, SQLINTEGER * pfDesc)
{
  SQLColAttributesWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLColAttributesWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLColAttributesW"));

    if(func) return func(hstmt, icol, fDescType, rgbDesc, cbDescMax, pcbDesc, pfDesc);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLColAttributesW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hstmt, icol, fDescType, rgbDesc, cbDescMax, pcbDesc, pfDesc);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLColAttributesA (SQLHSTMT hstmt,
      SQLUSMALLINT icol, SQLUSMALLINT fDescType,
      SQLPOINTER rgbDesc, SQLSMALLINT cbDescMax, SQLSMALLINT * pcbDesc, SQLINTEGER * pfDesc)
{
  SQLColAttributesAPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLColAttributesAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLColAttributesA"));

    if(func) return func(hstmt, icol, fDescType, rgbDesc, cbDescMax, pcbDesc, pfDesc);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLColAttributesA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hstmt, icol, fDescType, rgbDesc, cbDescMax, pcbDesc, pfDesc);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLColumnPrivileges (SQLHSTMT hstmt,
      SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName,
      SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName,
      SQLCHAR * szTableName, SQLSMALLINT cbTableName,
      SQLCHAR * szColumnName, SQLSMALLINT cbColumnName)
{
  SQLColumnPrivilegesPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLColumnPrivilegesPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLColumnPrivileges"));

    if(func) return func(hstmt, szCatalogName, cbCatalogName, szSchemaName,
	  cbSchemaName, szTableName, cbTableName, szColumnName, cbColumnName);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLColumnPrivileges", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hstmt, szCatalogName, cbCatalogName, szSchemaName,
	  cbSchemaName, szTableName, cbTableName, szColumnName, cbColumnName);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLColumnPrivilegesW (SQLHSTMT hstmt,
      SQLWCHAR * szCatalogName, SQLSMALLINT cbCatalogName,
      SQLWCHAR * szSchemaName, SQLSMALLINT cbSchemaName,
      SQLWCHAR * szTableName, SQLSMALLINT cbTableName,
      SQLWCHAR * szColumnName, SQLSMALLINT cbColumnName)
{
  SQLColumnPrivilegesWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLColumnPrivilegesWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLColumnPrivilegesW"));

    if(func) return func(hstmt, szCatalogName, cbCatalogName, szSchemaName,
	  cbSchemaName, szTableName, cbTableName, szColumnName, cbColumnName);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLColumnPrivilegesW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hstmt, szCatalogName, cbCatalogName, szSchemaName,
	  cbSchemaName, szTableName, cbTableName, szColumnName, cbColumnName);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLColumnPrivilegesA (SQLHSTMT hstmt,
      SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName,
      SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName,
      SQLCHAR * szTableName, SQLSMALLINT cbTableName,
      SQLCHAR * szColumnName, SQLSMALLINT cbColumnName)
{
  SQLColumnPrivilegesAPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLColumnPrivilegesAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLColumnPrivilegesA"));

    if(func) return func(hstmt, szCatalogName, cbCatalogName, szSchemaName,
	  cbSchemaName, szTableName, cbTableName, szColumnName, cbColumnName);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLColumnPrivilegesA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hstmt, szCatalogName, cbCatalogName, szSchemaName,
	  cbSchemaName, szTableName, cbTableName, szColumnName, cbColumnName);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLColumns (SQLHSTMT StatementHandle,
      SQLCHAR * CatalogName, SQLSMALLINT NameLength1,
      SQLCHAR * SchemaName, SQLSMALLINT NameLength2,
      SQLCHAR * TableName, SQLSMALLINT NameLength3, SQLCHAR * ColumnName, SQLSMALLINT NameLength4)
{
  SQLColumnsPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLColumnsPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLColumns"));

    if(func) return func(StatementHandle, CatalogName, NameLength1, SchemaName,
	  NameLength2, TableName, NameLength3, ColumnName, NameLength4);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLColumns", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, CatalogName, NameLength1, SchemaName,
	  NameLength2, TableName, NameLength3, ColumnName, NameLength4);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLColumnsW (SQLHSTMT StatementHandle,
      SQLWCHAR* CatalogName, SQLSMALLINT NameLength1,
      SQLWCHAR* SchemaName, SQLSMALLINT NameLength2,
      SQLWCHAR* TableName, SQLSMALLINT NameLength3, SQLWCHAR* ColumnName, SQLSMALLINT NameLength4)
{
  SQLColumnsWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLColumnsWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLColumnsW"));

    if(func) return func(StatementHandle, CatalogName, NameLength1, SchemaName,
	  NameLength2, TableName, NameLength3, ColumnName, NameLength4);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLColumnsW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, CatalogName, NameLength1, SchemaName,
	  NameLength2, TableName, NameLength3, ColumnName, NameLength4);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLColumnsA (SQLHSTMT StatementHandle,
      SQLCHAR * CatalogName, SQLSMALLINT NameLength1,
      SQLCHAR * SchemaName, SQLSMALLINT NameLength2,
      SQLCHAR * TableName, SQLSMALLINT NameLength3, SQLCHAR * ColumnName, SQLSMALLINT NameLength4)
{
  SQLColumnsAPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLColumnsAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLColumnsA"));

    if(func) return func(StatementHandle, CatalogName, NameLength1, SchemaName,
	  NameLength2, TableName, NameLength3, ColumnName, NameLength4);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLColumnsA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, CatalogName, NameLength1, SchemaName,
	  NameLength2, TableName, NameLength3, ColumnName, NameLength4);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLConnect (SQLHDBC ConnectionHandle,
      SQLCHAR * ServerName, SQLSMALLINT NameLength1,
      SQLCHAR * UserName, SQLSMALLINT NameLength2,
      SQLCHAR * Authentication, SQLSMALLINT NameLength3)
{
  SQLConnectPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLConnectPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLConnect"));

    if(func) return func(ConnectionHandle, ServerName, NameLength1,
	  UserName, NameLength2, Authentication, NameLength3);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLConnect", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(ConnectionHandle, ServerName, NameLength1,
	  UserName, NameLength2, Authentication, NameLength3);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLConnectW (SQLHDBC ConnectionHandle,
      SQLWCHAR * ServerName, SQLSMALLINT NameLength1,
      SQLWCHAR * UserName, SQLSMALLINT NameLength2,
      SQLWCHAR * Authentication, SQLSMALLINT NameLength3)
{
  SQLConnectWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLConnectWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLConnectW"));

    if(func) return func(ConnectionHandle, ServerName, NameLength1,
	  UserName, NameLength2, Authentication, NameLength3);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLConnectW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(ConnectionHandle, ServerName, NameLength1,
	  UserName, NameLength2, Authentication, NameLength3);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLConnectA (SQLHDBC ConnectionHandle,
      SQLCHAR * ServerName, SQLSMALLINT NameLength1,
      SQLCHAR * UserName, SQLSMALLINT NameLength2,
      SQLCHAR * Authentication, SQLSMALLINT NameLength3)
{
  SQLConnectAPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLConnectAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLConnectA"));

    if(func) return func(ConnectionHandle, ServerName, NameLength1,
	  UserName, NameLength2, Authentication, NameLength3);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLConnectA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(ConnectionHandle, ServerName, NameLength1,
	  UserName, NameLength2, Authentication, NameLength3);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLCopyDesc (SQLHDESC SourceDescHandle,
      SQLHDESC TargetDescHandle)
{
  SQLCopyDescPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLCopyDescPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLCopyDesc"));

    if(func) return func(SourceDescHandle, TargetDescHandle);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLCopyDesc", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(SourceDescHandle, TargetDescHandle);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLDataSources (SQLHENV EnvironmentHandle,
      SQLUSMALLINT Direction, SQLCHAR * ServerName,
      SQLSMALLINT BufferLength1, SQLSMALLINT * NameLength1,
      SQLCHAR * Description, SQLSMALLINT BufferLength2, SQLSMALLINT * NameLength2)
{
  SQLDataSourcesPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLDataSourcesPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLDataSources"));

    if(func) return func(EnvironmentHandle, Direction, ServerName,
	  BufferLength1, NameLength1, Description, BufferLength2, NameLength2);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLDataSources", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(EnvironmentHandle, Direction, ServerName,
	  BufferLength1, NameLength1, Description, BufferLength2, NameLength2);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLDataSourcesW (SQLHENV EnvironmentHandle,
      SQLUSMALLINT Direction, SQLWCHAR * ServerName,
      SQLSMALLINT BufferLength1, SQLSMALLINT * NameLength1,
      SQLWCHAR * Description, SQLSMALLINT BufferLength2, SQLSMALLINT * NameLength2)
{
  SQLDataSourcesWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLDataSourcesWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLDataSourcesW"));

    if(func) return func(EnvironmentHandle, Direction, ServerName,
	  BufferLength1, NameLength1, Description, BufferLength2, NameLength2);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLDataSourcesW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(EnvironmentHandle, Direction, ServerName,
	  BufferLength1, NameLength1, Description, BufferLength2, NameLength2);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLDataSourcesA (SQLHENV EnvironmentHandle,
      SQLUSMALLINT Direction, SQLCHAR * ServerName,
      SQLSMALLINT BufferLength1, SQLSMALLINT * NameLength1,
      SQLCHAR * Description, SQLSMALLINT BufferLength2, SQLSMALLINT * NameLength2)
{
  SQLDataSourcesAPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLDataSourcesAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLDataSourcesA"));

    if(func) return func(EnvironmentHandle, Direction, ServerName,
	  BufferLength1, NameLength1, Description, BufferLength2, NameLength2);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLDataSourcesA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(EnvironmentHandle, Direction, ServerName,
	  BufferLength1, NameLength1, Description, BufferLength2, NameLength2);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLDescribeCol (SQLHSTMT StatementHandle,
      SQLUSMALLINT ColumnNumber, SQLCHAR * ColumnName,
      SQLSMALLINT BufferLength, SQLSMALLINT * NameLength,
      SQLSMALLINT * DataType, SQLUINTEGER * ColumnSize,
      SQLSMALLINT * DecimalDigits, SQLSMALLINT * Nullable)
{
  SQLDescribeColPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLDescribeColPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLDescribeCol"));

    if(func) return func(StatementHandle, ColumnNumber, ColumnName,
	  BufferLength, NameLength, DataType, ColumnSize, DecimalDigits, Nullable);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLDescribeCol", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, ColumnNumber, ColumnName,
	  BufferLength, NameLength, DataType, ColumnSize, DecimalDigits, Nullable);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLDescribeColW (SQLHSTMT StatementHandle,
      SQLUSMALLINT ColumnNumber, SQLWCHAR* ColumnName,
      SQLSMALLINT BufferLength, SQLSMALLINT * NameLength,
      SQLSMALLINT * DataType, SQLUINTEGER * ColumnSize,
      SQLSMALLINT * DecimalDigits, SQLSMALLINT * Nullable)
{
  SQLDescribeColWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLDescribeColWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLDescribeColW"));

    if(func) return func(StatementHandle, ColumnNumber, ColumnName,
	  BufferLength, NameLength, DataType, ColumnSize, DecimalDigits, Nullable);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLDescribeColW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, ColumnNumber, ColumnName,
	  BufferLength, NameLength, DataType, ColumnSize, DecimalDigits, Nullable);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLDescribeColA (SQLHSTMT StatementHandle,
      SQLUSMALLINT ColumnNumber, SQLCHAR* ColumnName,
      SQLSMALLINT BufferLength, SQLSMALLINT * NameLength,
      SQLSMALLINT * DataType, SQLUINTEGER * ColumnSize,
      SQLSMALLINT * DecimalDigits, SQLSMALLINT * Nullable)
{
  SQLDescribeColAPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLDescribeColAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLDescribeColA"));

    if(func) return func(StatementHandle, ColumnNumber, ColumnName,
	  BufferLength, NameLength, DataType, ColumnSize, DecimalDigits, Nullable);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLDescribeColA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, ColumnNumber, ColumnName,
	  BufferLength, NameLength, DataType, ColumnSize, DecimalDigits, Nullable);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLDescribeParam (SQLHSTMT hstmt,
      SQLUSMALLINT ipar, SQLSMALLINT * pfSqlType,
      SQLUINTEGER * pcbParamDef, SQLSMALLINT * pibScale, SQLSMALLINT * pfNullable)
{
  SQLDescribeParamPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLDescribeParamPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLDescribeParam"));

    if(func) return func(hstmt, ipar, pfSqlType, pcbParamDef, pibScale, pfNullable);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLDescribeParam", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hstmt, ipar, pfSqlType, pcbParamDef, pibScale, pfNullable);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLDisconnect (SQLHDBC ConnectionHandle)
{
  SQLDisconnectPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLDisconnectPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLDisconnect"));

    if(func) return func(ConnectionHandle);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLDisconnect", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(ConnectionHandle);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLDriverConnect (SQLHDBC hdbc,
      SQLHWND hwnd, SQLCHAR * szConnStrIn, SQLSMALLINT cbConnStrIn, SQLCHAR * szConnStrOut,
      SQLSMALLINT cbConnStrOutMax, SQLSMALLINT * pcbConnStrOut, SQLUSMALLINT fDriverCompletion)
{
  SQLDriverConnectPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLDriverConnectPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLDriverConnect"));

    if(func) return func(hdbc, hwnd, szConnStrIn, cbConnStrIn, szConnStrOut,
	  cbConnStrOutMax, pcbConnStrOut, fDriverCompletion);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLDriverConnect", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hdbc, hwnd, szConnStrIn, cbConnStrIn, szConnStrOut,
	  cbConnStrOutMax, pcbConnStrOut, fDriverCompletion);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLDriverConnectW (SQLHDBC hdbc,
      SQLHWND hwnd, SQLWCHAR * szConnStrIn, SQLSMALLINT cbConnStrIn, SQLWCHAR * szConnStrOut,
      SQLSMALLINT cbConnStrOutMax, SQLSMALLINT * pcbConnStrOut, SQLUSMALLINT fDriverCompletion)
{
  SQLDriverConnectWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLDriverConnectWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLDriverConnectW"));

    if(func) return func(hdbc, hwnd, szConnStrIn, cbConnStrIn, szConnStrOut,
	  cbConnStrOutMax, pcbConnStrOut, fDriverCompletion);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLDriverConnectW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hdbc, hwnd, szConnStrIn, cbConnStrIn, szConnStrOut,
	  cbConnStrOutMax, pcbConnStrOut, fDriverCompletion);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLDriverConnectA (SQLHDBC hdbc,
      SQLHWND hwnd, SQLCHAR * szConnStrIn, SQLSMALLINT cbConnStrIn, SQLCHAR * szConnStrOut,
      SQLSMALLINT cbConnStrOutMax, SQLSMALLINT * pcbConnStrOut, SQLUSMALLINT fDriverCompletion)
{
  SQLDriverConnectAPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLDriverConnectAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLDriverConnectA"));

    if(func) return func(hdbc, hwnd, szConnStrIn, cbConnStrIn, szConnStrOut,
	  cbConnStrOutMax, pcbConnStrOut, fDriverCompletion);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLDriverConnectA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hdbc, hwnd, szConnStrIn, cbConnStrIn, szConnStrOut,
	  cbConnStrOutMax, pcbConnStrOut, fDriverCompletion);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLDrivers (SQLHENV henv,
      SQLUSMALLINT fDirection, SQLCHAR * szDriverDesc,
      SQLSMALLINT cbDriverDescMax, SQLSMALLINT * pcbDriverDesc,
      SQLCHAR * szDriverAttributes, SQLSMALLINT cbDrvrAttrMax, SQLSMALLINT * pcbDrvrAttr)
{
  SQLDriversPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLDriversPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLDrivers"));

    if(func) return func(henv, fDirection, szDriverDesc, cbDriverDescMax,
	  pcbDriverDesc, szDriverAttributes, cbDrvrAttrMax, pcbDrvrAttr);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLDrivers", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(henv, fDirection, szDriverDesc, cbDriverDescMax,
	  pcbDriverDesc, szDriverAttributes, cbDrvrAttrMax, pcbDrvrAttr);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLDriversW (SQLHENV henv,
      SQLUSMALLINT fDirection, SQLWCHAR * szDriverDesc,
      SQLSMALLINT cbDriverDescMax, SQLSMALLINT * pcbDriverDesc,
      SQLWCHAR * szDriverAttributes, SQLSMALLINT cbDrvrAttrMax, SQLSMALLINT * pcbDrvrAttr)
{
  SQLDriversWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLDriversWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLDriversW"));

    if(func) return func(henv, fDirection, szDriverDesc, cbDriverDescMax,
	  pcbDriverDesc, szDriverAttributes, cbDrvrAttrMax, pcbDrvrAttr);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLDriversW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(henv, fDirection, szDriverDesc, cbDriverDescMax,
	  pcbDriverDesc, szDriverAttributes, cbDrvrAttrMax, pcbDrvrAttr);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLDriversA (SQLHENV henv,
      SQLUSMALLINT fDirection, SQLCHAR * szDriverDesc,
      SQLSMALLINT cbDriverDescMax, SQLSMALLINT * pcbDriverDesc,
      SQLCHAR * szDriverAttributes, SQLSMALLINT cbDrvrAttrMax, SQLSMALLINT * pcbDrvrAttr)
{
  SQLDriversAPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLDriversAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLDriversA"));

    if(func) return func(henv, fDirection, szDriverDesc, cbDriverDescMax,
	  pcbDriverDesc, szDriverAttributes, cbDrvrAttrMax, pcbDrvrAttr);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLDriversA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(henv, fDirection, szDriverDesc, cbDriverDescMax,
	  pcbDriverDesc, szDriverAttributes, cbDrvrAttrMax, pcbDrvrAttr);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLEndTran (SQLSMALLINT HandleType,
      SQLHANDLE Handle, SQLSMALLINT CompletionType)
{
  SQLEndTranPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLEndTranPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLEndTran"));

    if(func) return func(HandleType, Handle, CompletionType);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLEndTran", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(HandleType, Handle, CompletionType);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLError (SQLHENV EnvironmentHandle,
      SQLHDBC ConnectionHandle, SQLHSTMT StatementHandle,
      SQLCHAR * Sqlstate, SQLINTEGER * NativeError,
      SQLCHAR * MessageText, SQLSMALLINT BufferLength, SQLSMALLINT * TextLength)
{
  SQLErrorPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLErrorPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLError"));

    if(func) return func(EnvironmentHandle, ConnectionHandle, StatementHandle,
	  Sqlstate, NativeError, MessageText, BufferLength, TextLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLError", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(EnvironmentHandle, ConnectionHandle, StatementHandle,
	  Sqlstate, NativeError, MessageText, BufferLength, TextLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLErrorW (SQLHENV EnvironmentHandle,
      SQLHDBC ConnectionHandle, SQLHSTMT StatementHandle,
      SQLWCHAR * Sqlstate, SQLINTEGER * NativeError,
      SQLWCHAR * MessageText, SQLSMALLINT BufferLength, SQLSMALLINT * TextLength)
{
  SQLErrorWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLErrorWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLErrorW"));

    if(func) return func(EnvironmentHandle, ConnectionHandle, StatementHandle,
	  Sqlstate, NativeError, MessageText, BufferLength, TextLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLErrorW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(EnvironmentHandle, ConnectionHandle, StatementHandle,
	  Sqlstate, NativeError, MessageText, BufferLength, TextLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLErrorA (SQLHENV EnvironmentHandle,
      SQLHDBC ConnectionHandle, SQLHSTMT StatementHandle,
      SQLCHAR * Sqlstate, SQLINTEGER * NativeError,
      SQLCHAR * MessageText, SQLSMALLINT BufferLength, SQLSMALLINT * TextLength)
{
  SQLErrorAPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLErrorAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLErrorA"));

    if(func) return func(EnvironmentHandle, ConnectionHandle, StatementHandle,
	  Sqlstate, NativeError, MessageText, BufferLength, TextLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLErrorA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(EnvironmentHandle, ConnectionHandle, StatementHandle,
	  Sqlstate, NativeError, MessageText, BufferLength, TextLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLExecDirect (SQLHSTMT StatementHandle,
      SQLCHAR * StatementText, SQLINTEGER TextLength)
{
  SQLExecDirectPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLExecDirectPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLExecDirect"));

    if(func) return func(StatementHandle, StatementText, TextLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLExecDirect", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, StatementText, TextLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLExecDirectW (SQLHSTMT StatementHandle,
      SQLWCHAR * StatementText, SQLINTEGER TextLength)
{
  SQLExecDirectWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLExecDirectWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLExecDirectW"));

    if(func) return func(StatementHandle, StatementText, TextLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLExecDirectW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, StatementText, TextLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLExecDirectA (SQLHSTMT StatementHandle,
      SQLCHAR * StatementText, SQLINTEGER TextLength)
{
  SQLExecDirectAPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLExecDirectAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLExecDirectA"));

    if(func) return func(StatementHandle, StatementText, TextLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLExecDirectA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, StatementText, TextLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLExecute (SQLHSTMT StatementHandle)
{
  SQLExecutePTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLExecutePTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLExecute"));

    if(func) return func(StatementHandle);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLExecute", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLExtendedFetch (SQLHSTMT hstmt,
      SQLUSMALLINT fFetchType,
      SQLINTEGER irow, SQLUINTEGER * pcrow, SQLUSMALLINT * rgfRowStatus)
{
  SQLExtendedFetchPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLExtendedFetchPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLExtendedFetch"));

    if(func) return func(hstmt, fFetchType, irow, pcrow, rgfRowStatus);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLExtendedFetch", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hstmt, fFetchType, irow, pcrow, rgfRowStatus);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLFetch (SQLHSTMT StatementHandle)
{
  SQLFetchPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLFetchPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLFetch"));

    if(func) return func(StatementHandle);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLFetch", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLFetchScroll (SQLHSTMT StatementHandle,
      SQLSMALLINT FetchOrientation, SQLINTEGER FetchOffset)
{
  SQLFetchScrollPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLFetchScrollPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLFetchScroll"));

    if(func) return func(StatementHandle, FetchOrientation, FetchOffset);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLFetchScroll", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, FetchOrientation, FetchOffset);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLForeignKeys (SQLHSTMT hstmt,
      SQLCHAR * szPkCatalogName, SQLSMALLINT cbPkCatalogName,
      SQLCHAR * szPkSchemaName, SQLSMALLINT cbPkSchemaName,
      SQLCHAR * szPkTableName, SQLSMALLINT cbPkTableName,
      SQLCHAR * szFkCatalogName, SQLSMALLINT cbFkCatalogName,
      SQLCHAR * szFkSchemaName, SQLSMALLINT cbFkSchemaName,
      SQLCHAR * szFkTableName, SQLSMALLINT cbFkTableName)
{
  SQLForeignKeysPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLForeignKeysPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLForeignKeys"));

    if(func) return func(hstmt, szPkCatalogName, cbPkCatalogName,
      szPkSchemaName, cbPkSchemaName,
      szPkTableName, cbPkTableName,
      szFkCatalogName, cbFkCatalogName,
      szFkSchemaName, cbFkSchemaName,
      szFkTableName, cbFkTableName);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLForeignKeys", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hstmt, szPkCatalogName, cbPkCatalogName,
      szPkSchemaName, cbPkSchemaName,
      szPkTableName, cbPkTableName,
      szFkCatalogName, cbFkCatalogName,
      szFkSchemaName, cbFkSchemaName,
      szFkTableName, cbFkTableName);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLForeignKeysA (SQLHSTMT hstmt,
      SQLCHAR * szPkCatalogName, SQLSMALLINT cbPkCatalogName,
      SQLCHAR * szPkSchemaName, SQLSMALLINT cbPkSchemaName,
      SQLCHAR * szPkTableName, SQLSMALLINT cbPkTableName,
      SQLCHAR * szFkCatalogName, SQLSMALLINT cbFkCatalogName,
      SQLCHAR * szFkSchemaName, SQLSMALLINT cbFkSchemaName,
      SQLCHAR * szFkTableName, SQLSMALLINT cbFkTableName)
{
  SQLForeignKeysAPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLForeignKeysAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLForeignKeysA"));

    if(func) return func(hstmt, szPkCatalogName, cbPkCatalogName,
      szPkSchemaName, cbPkSchemaName,
      szPkTableName, cbPkTableName,
      szFkCatalogName, cbFkCatalogName,
      szFkSchemaName, cbFkSchemaName,
      szFkTableName, cbFkTableName);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLForeignKeysA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hstmt, szPkCatalogName, cbPkCatalogName,
      szPkSchemaName, cbPkSchemaName,
      szPkTableName, cbPkTableName,
      szFkCatalogName, cbFkCatalogName,
      szFkSchemaName, cbFkSchemaName,
      szFkTableName, cbFkTableName);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLForeignKeysW (SQLHSTMT hstmt,
      SQLWCHAR * szPkCatalogName, SQLSMALLINT cbPkCatalogName,
      SQLWCHAR * szPkSchemaName, SQLSMALLINT cbPkSchemaName,
      SQLWCHAR * szPkTableName, SQLSMALLINT cbPkTableName,
      SQLWCHAR * szFkCatalogName, SQLSMALLINT cbFkCatalogName,
      SQLWCHAR * szFkSchemaName, SQLSMALLINT cbFkSchemaName,
      SQLWCHAR * szFkTableName, SQLSMALLINT cbFkTableName)
{
  SQLForeignKeysWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLForeignKeysWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLForeignKeysW"));

    if(func) return func(hstmt, szPkCatalogName, cbPkCatalogName,
      szPkSchemaName, cbPkSchemaName,
      szPkTableName, cbPkTableName,
      szFkCatalogName, cbFkCatalogName,
      szFkSchemaName, cbFkSchemaName,
      szFkTableName, cbFkTableName);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLForeignKeysW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hstmt, szPkCatalogName, cbPkCatalogName,
      szPkSchemaName, cbPkSchemaName,
      szPkTableName, cbPkTableName,
      szFkCatalogName, cbFkCatalogName,
      szFkSchemaName, cbFkSchemaName,
      szFkTableName, cbFkTableName);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLFreeConnect (SQLHDBC ConnectionHandle)
{
  SQLFreeConnectPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLFreeConnectPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLFreeConnect"));

    if(func) return func(ConnectionHandle);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLFreeConnect", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(ConnectionHandle);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLFreeEnv (SQLHENV EnvironmentHandle)
{
  SQLFreeEnvPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLFreeEnvPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLFreeEnv"));

    if(func) return func(EnvironmentHandle);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLFreeEnv", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(EnvironmentHandle);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLFreeHandle (SQLSMALLINT HandleType, SQLHANDLE Handle)
{
  SQLFreeHandlePTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLFreeHandlePTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLFreeHandle"));

    if(func) return func(HandleType, Handle);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLFreeHandle", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(HandleType, Handle);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLFreeStmt (SQLHSTMT StatementHandle,
      SQLUSMALLINT Option)
{
  SQLFreeStmtPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLFreeStmtPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLFreeStmt"));

    if(func) return func(StatementHandle, Option);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLFreeStmt", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, Option);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLGetConnectAttr (SQLHDBC ConnectionHandle,
      SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER * StringLength)
{
  SQLGetConnectAttrPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetConnectAttrPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLGetConnectAttr"));

    if(func) return func(ConnectionHandle, Attribute, Value, BufferLength, StringLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLGetConnectAttr", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(ConnectionHandle, Attribute, Value, BufferLength, StringLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLGetConnectAttrW (SQLHDBC ConnectionHandle,
      SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER * StringLength)
{
  SQLGetConnectAttrWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetConnectAttrWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLGetConnectAttrW"));

    if(func) return func(ConnectionHandle, Attribute, Value, BufferLength, StringLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLGetConnectAttrW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(ConnectionHandle, Attribute, Value, BufferLength, StringLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLGetConnectAttrA (SQLHDBC ConnectionHandle,
      SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER * StringLength)
{
  SQLGetConnectAttrAPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetConnectAttrAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLGetConnectAttrA"));

    if(func) return func(ConnectionHandle, Attribute, Value, BufferLength, StringLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLGetConnectAttrA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(ConnectionHandle, Attribute, Value, BufferLength, StringLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLGetConnectOption (SQLHDBC ConnectionHandle,
      SQLUSMALLINT Option, SQLPOINTER Value)
{
  SQLGetConnectOptionPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetConnectOptionPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLGetConnectOption"));

    if(func) return func(ConnectionHandle, Option, Value);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLGetConnectOption", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(ConnectionHandle, Option, Value);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLGetConnectOptionW (SQLHDBC ConnectionHandle,
      SQLUSMALLINT Option, SQLPOINTER Value)
{
  SQLGetConnectOptionWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetConnectOptionWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLGetConnectOptionW"));

    if(func) return func(ConnectionHandle, Option, Value);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLGetConnectOptionW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(ConnectionHandle, Option, Value);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLGetConnectOptionA (SQLHDBC ConnectionHandle,
      SQLUSMALLINT Option, SQLPOINTER Value)
{
  SQLGetConnectOptionAPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetConnectOptionAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLGetConnectOptionA"));

    if(func) return func(ConnectionHandle, Option, Value);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLGetConnectOptionA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(ConnectionHandle, Option, Value);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLGetCursorName (SQLHSTMT StatementHandle,
      SQLCHAR * CursorName, SQLSMALLINT BufferLength, SQLSMALLINT * NameLength)
{
  SQLGetCursorNamePTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetCursorNamePTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLGetCursorName"));

    if(func) return func(StatementHandle, CursorName, BufferLength, NameLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLGetCursorName", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, CursorName, BufferLength, NameLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLGetCursorNameW (SQLHSTMT StatementHandle,
      SQLWCHAR * CursorName, SQLSMALLINT BufferLength, SQLSMALLINT * NameLength)
{
  SQLGetCursorNameWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetCursorNameWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLGetCursorNameW"));

    if(func) return func(StatementHandle, CursorName, BufferLength, NameLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLGetCursorNameW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, CursorName, BufferLength, NameLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLGetCursorNameA (SQLHSTMT StatementHandle,
      SQLCHAR * CursorName, SQLSMALLINT BufferLength, SQLSMALLINT * NameLength)
{
  SQLGetCursorNameAPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetCursorNameAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLGetCursorNameA"));

    if(func) return func(StatementHandle, CursorName, BufferLength, NameLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLGetCursorNameA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, CursorName, BufferLength, NameLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLGetData (SQLHSTMT StatementHandle,
      SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType,
      SQLPOINTER TargetValue, SQLINTEGER BufferLength, SQLINTEGER * StrLen_or_Ind)
{
  SQLGetDataPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetDataPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLGetData"));

    if(func) return func(StatementHandle, ColumnNumber, TargetType,
	  TargetValue, BufferLength, StrLen_or_Ind);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLGetData", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, ColumnNumber, TargetType,
	  TargetValue, BufferLength, StrLen_or_Ind);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLGetDescField (SQLHDESC DescriptorHandle,
      SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
      SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER * StringLength)
{
  SQLGetDescFieldPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetDescFieldPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLGetDescField"));

    if(func) return func(DescriptorHandle, RecNumber, FieldIdentifier,
	  Value, BufferLength, StringLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLGetDescField", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(DescriptorHandle, RecNumber, FieldIdentifier,
	  Value, BufferLength, StringLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLGetDescFieldW (SQLHDESC DescriptorHandle,
      SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
      SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER * StringLength)
{
  SQLGetDescFieldWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetDescFieldWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLGetDescFieldW"));

    if(func) return func(DescriptorHandle, RecNumber, FieldIdentifier,
	  Value, BufferLength, StringLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLGetDescFieldW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(DescriptorHandle, RecNumber, FieldIdentifier,
	  Value, BufferLength, StringLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLGetDescFieldA (SQLHDESC DescriptorHandle,
      SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
      SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER * StringLength)
{
  SQLGetDescFieldAPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetDescFieldAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLGetDescFieldA"));

    if(func) return func(DescriptorHandle, RecNumber, FieldIdentifier,
	  Value, BufferLength, StringLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLGetDescFieldA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(DescriptorHandle, RecNumber, FieldIdentifier,
	  Value, BufferLength, StringLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLGetDescRec (SQLHDESC DescriptorHandle,
      SQLSMALLINT RecNumber, SQLCHAR * Name,
      SQLSMALLINT BufferLength, SQLSMALLINT * StringLength,
      SQLSMALLINT * Type, SQLSMALLINT * SubType,
      SQLINTEGER * Length, SQLSMALLINT * Precision, SQLSMALLINT * Scale, SQLSMALLINT * Nullable)
{
  SQLGetDescRecPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetDescRecPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLGetDescRec"));

    if(func) return func(DescriptorHandle, RecNumber, Name, BufferLength,
	  StringLength, Type, SubType, Length, Precision, Scale, Nullable);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLGetDescRec", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(DescriptorHandle, RecNumber, Name, BufferLength,
	  StringLength, Type, SubType, Length, Precision, Scale, Nullable);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLGetDescRecW (SQLHDESC DescriptorHandle,
      SQLSMALLINT RecNumber, SQLWCHAR * Name,
      SQLSMALLINT BufferLength, SQLSMALLINT * StringLength,
      SQLSMALLINT * Type, SQLSMALLINT * SubType,
      SQLINTEGER * Length, SQLSMALLINT * Precision, SQLSMALLINT * Scale, SQLSMALLINT * Nullable)
{
  SQLGetDescRecWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetDescRecWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLGetDescRecW"));

    if(func) return func(DescriptorHandle, RecNumber, Name, BufferLength,
	  StringLength, Type, SubType, Length, Precision, Scale, Nullable);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLGetDescRecW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(DescriptorHandle, RecNumber, Name, BufferLength,
	  StringLength, Type, SubType, Length, Precision, Scale, Nullable);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLGetDescRecA (SQLHDESC DescriptorHandle,
      SQLSMALLINT RecNumber, SQLCHAR * Name,
      SQLSMALLINT BufferLength, SQLSMALLINT * StringLength,
      SQLSMALLINT * Type, SQLSMALLINT * SubType,
      SQLINTEGER * Length, SQLSMALLINT * Precision, SQLSMALLINT * Scale, SQLSMALLINT * Nullable)
{
  SQLGetDescRecPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetDescRecAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLGetDescRecA"));

    if(func) return func(DescriptorHandle, RecNumber, Name, BufferLength,
	  StringLength, Type, SubType, Length, Precision, Scale, Nullable);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLGetDescRecA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(DescriptorHandle, RecNumber, Name, BufferLength,
	  StringLength, Type, SubType, Length, Precision, Scale, Nullable);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLGetDiagField (SQLSMALLINT HandleType,
      SQLHANDLE Handle, SQLSMALLINT RecNumber,
      SQLSMALLINT DiagIdentifier, SQLPOINTER DiagInfo,
      SQLSMALLINT BufferLength, SQLSMALLINT * StringLength)
{
  SQLGetDiagFieldPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetDiagFieldPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLGetDiagField"));

    if(func) return func(HandleType, Handle, RecNumber, DiagIdentifier,
	  DiagInfo, BufferLength, StringLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLGetDiagField", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(HandleType, Handle, RecNumber, DiagIdentifier,
	  DiagInfo, BufferLength, StringLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLGetDiagFieldW (SQLSMALLINT HandleType,
      SQLHANDLE Handle, SQLSMALLINT RecNumber,
      SQLSMALLINT DiagIdentifier, SQLPOINTER DiagInfo,
      SQLSMALLINT BufferLength, SQLSMALLINT * StringLength)
{
  SQLGetDiagFieldWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetDiagFieldWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLGetDiagFieldW"));

    if(func) return func(HandleType, Handle, RecNumber, DiagIdentifier,
	  DiagInfo, BufferLength, StringLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLGetDiagFieldW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(HandleType, Handle, RecNumber, DiagIdentifier,
	  DiagInfo, BufferLength, StringLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLGetDiagFieldA (SQLSMALLINT HandleType,
      SQLHANDLE Handle, SQLSMALLINT RecNumber,
      SQLSMALLINT DiagIdentifier, SQLPOINTER DiagInfo,
      SQLSMALLINT BufferLength, SQLSMALLINT * StringLength)
{
  SQLGetDiagFieldAPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetDiagFieldAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLGetDiagFieldA"));

    if(func) return func(HandleType, Handle, RecNumber, DiagIdentifier,
	  DiagInfo, BufferLength, StringLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLGetDiagFieldA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(HandleType, Handle, RecNumber, DiagIdentifier,
	  DiagInfo, BufferLength, StringLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLGetDiagRec (SQLSMALLINT HandleType,
      SQLHANDLE Handle, SQLSMALLINT RecNumber,
      SQLCHAR * Sqlstate, SQLINTEGER * NativeError,
      SQLCHAR * MessageText, SQLSMALLINT BufferLength, SQLSMALLINT * TextLength)
{
  SQLGetDiagRecPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetDiagRecPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLGetDiagRec"));

    if(func) return func(HandleType, Handle, RecNumber, Sqlstate, NativeError,
	  MessageText, BufferLength, TextLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLGetDiagRec", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(HandleType, Handle, RecNumber, Sqlstate, NativeError,
	  MessageText, BufferLength, TextLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLGetDiagRecW (SQLSMALLINT HandleType,
      SQLHANDLE Handle, SQLSMALLINT RecNumber,
      SQLWCHAR * Sqlstate, SQLINTEGER * NativeError,
      SQLWCHAR * MessageText, SQLSMALLINT BufferLength, SQLSMALLINT * TextLength)
{
  SQLGetDiagRecWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetDiagRecWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLGetDiagRecW"));

    if(func) return func(HandleType, Handle, RecNumber, Sqlstate, NativeError,
	  MessageText, BufferLength, TextLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLGetDiagRecW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(HandleType, Handle, RecNumber, Sqlstate, NativeError,
	  MessageText, BufferLength, TextLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLGetDiagRecA (SQLSMALLINT HandleType,
      SQLHANDLE Handle, SQLSMALLINT RecNumber,
      SQLCHAR * Sqlstate, SQLINTEGER * NativeError,
      SQLCHAR * MessageText, SQLSMALLINT BufferLength, SQLSMALLINT * TextLength)
{
  SQLGetDiagRecAPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetDiagRecAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLGetDiagRecA"));

    if(func) return func(HandleType, Handle, RecNumber, Sqlstate, NativeError,
	  MessageText, BufferLength, TextLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLGetDiagRecA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(HandleType, Handle, RecNumber, Sqlstate, NativeError,
	  MessageText, BufferLength, TextLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLGetEnvAttr (SQLHENV EnvironmentHandle,
      SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER * StringLength)
{
  SQLGetEnvAttrPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetEnvAttrPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLGetEnvAttr"));

    if(func) return func(EnvironmentHandle, Attribute, Value,
	  BufferLength, StringLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLGetEnvAttr", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(EnvironmentHandle, Attribute, Value,
	  BufferLength, StringLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLGetFunctions (SQLHDBC ConnectionHandle,
      SQLUSMALLINT FunctionId, SQLUSMALLINT * Supported)
{
  SQLGetFunctionsPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetFunctionsPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLGetFunctions"));

    if(func) return func(ConnectionHandle, FunctionId, Supported);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLGetFunctions", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(ConnectionHandle, FunctionId, Supported);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLGetInfo (SQLHDBC ConnectionHandle,
      SQLUSMALLINT InfoType, SQLPOINTER InfoValue,
      SQLSMALLINT BufferLength, SQLSMALLINT * StringLength)
{
  SQLGetInfoPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetInfoPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLGetInfo"));

    if(func) return func(ConnectionHandle, InfoType, InfoValue,
	  BufferLength, StringLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLGetInfo", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(ConnectionHandle, InfoType, InfoValue,
	  BufferLength, StringLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLGetInfoW (SQLHDBC ConnectionHandle,
      SQLUSMALLINT InfoType, SQLPOINTER InfoValue,
      SQLSMALLINT BufferLength, SQLSMALLINT * StringLength)
{
  SQLGetInfoWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetInfoWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLGetInfoW"));

    if(func) return func(ConnectionHandle, InfoType, InfoValue,
	  BufferLength, StringLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLGetInfoW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(ConnectionHandle, InfoType, InfoValue,
	  BufferLength, StringLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLGetInfoA (SQLHDBC ConnectionHandle,
      SQLUSMALLINT InfoType, SQLPOINTER InfoValue,
      SQLSMALLINT BufferLength, SQLSMALLINT * StringLength)
{
  SQLGetInfoAPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetInfoAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLGetInfoA"));

    if(func) return func(ConnectionHandle, InfoType, InfoValue,
	  BufferLength, StringLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLGetInfoA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(ConnectionHandle, InfoType, InfoValue,
	  BufferLength, StringLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLGetStmtAttr (SQLHSTMT StatementHandle,
      SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER * StringLength)
{
  SQLGetStmtAttrPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetStmtAttrPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLGetStmtAttr"));

    if(func) return func(StatementHandle, Attribute, Value, BufferLength, StringLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLGetStmtAttr", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, Attribute, Value, BufferLength, StringLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLGetStmtAttrW (SQLHSTMT StatementHandle,
      SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER * StringLength)
{
  SQLGetStmtAttrWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetStmtAttrWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLGetStmtAttrW"));

    if(func) return func(StatementHandle, Attribute, Value, BufferLength, StringLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLGetStmtAttrW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, Attribute, Value, BufferLength, StringLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLGetStmtAttrA (SQLHSTMT StatementHandle,
      SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER * StringLength)
{
  SQLGetStmtAttrAPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetStmtAttrAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLGetStmtAttrA"));

    if(func) return func(StatementHandle, Attribute, Value, BufferLength, StringLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLGetStmtAttrA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, Attribute, Value, BufferLength, StringLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLGetStmtOption (SQLHSTMT StatementHandle,
      SQLUSMALLINT Option, SQLPOINTER Value)
{
  SQLGetStmtOptionPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetStmtOptionPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLGetStmtOption"));

    if(func) return func(StatementHandle, Option, Value);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLGetStmtOption", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, Option, Value);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLGetTypeInfo (SQLHSTMT StatementHandle,
      SQLSMALLINT DataType)
{
  SQLGetTypeInfoPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetTypeInfoPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLGetTypeInfo"));

    if(func) return func(StatementHandle, DataType);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLGetTypeInfo", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, DataType);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLGetTypeInfoW (SQLHSTMT StatementHandle,
      SQLSMALLINT DataType)
{
  SQLGetTypeInfoWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetTypeInfoWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLGetTypeInfoW"));

    if(func) return func(StatementHandle, DataType);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLGetTypeInfoW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, DataType);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLGetTypeInfoA (SQLHSTMT StatementHandle,
      SQLSMALLINT DataType)
{
  SQLGetTypeInfoAPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetTypeInfoAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLGetTypeInfoA"));

    if(func) return func(StatementHandle, DataType);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLGetTypeInfoA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, DataType);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLMoreResults (SQLHSTMT hstmt)
{
  SQLMoreResultsPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLMoreResultsPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLMoreResults"));

    if(func) return func(hstmt);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLMoreResults", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hstmt);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLNativeSql (SQLHDBC hdbc,
      SQLCHAR * szSqlStrIn, SQLINTEGER cbSqlStrIn,
      SQLCHAR * szSqlStr, SQLINTEGER cbSqlStrMax, SQLINTEGER * pcbSqlStr)
{
  SQLNativeSqlPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLNativeSqlPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLNativeSql"));

    if(func) return func(hdbc, szSqlStrIn, cbSqlStrIn,
	  szSqlStr, cbSqlStrMax, pcbSqlStr);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLNativeSql", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hdbc, szSqlStrIn, cbSqlStrIn,
	  szSqlStr, cbSqlStrMax, pcbSqlStr);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLNativeSqlW (SQLHDBC hdbc,
      SQLWCHAR * szSqlStrIn, SQLINTEGER cbSqlStrIn,
      SQLWCHAR * szSqlStr, SQLINTEGER cbSqlStrMax, SQLINTEGER * pcbSqlStr)
{
  SQLNativeSqlWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLNativeSqlWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLNativeSqlW"));

    if(func) return func(hdbc, szSqlStrIn, cbSqlStrIn,
	  szSqlStr, cbSqlStrMax, pcbSqlStr);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLNativeSqlW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hdbc, szSqlStrIn, cbSqlStrIn,
	  szSqlStr, cbSqlStrMax, pcbSqlStr);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLNativeSqlA (SQLHDBC hdbc,
      SQLCHAR * szSqlStrIn, SQLINTEGER cbSqlStrIn,
      SQLCHAR * szSqlStr, SQLINTEGER cbSqlStrMax, SQLINTEGER * pcbSqlStr)
{
  SQLNativeSqlAPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLNativeSqlAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLNativeSqlA"));

    if(func) return func(hdbc, szSqlStrIn, cbSqlStrIn,
	  szSqlStr, cbSqlStrMax, pcbSqlStr);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLNativeSqlA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hdbc, szSqlStrIn, cbSqlStrIn,
	  szSqlStr, cbSqlStrMax, pcbSqlStr);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLNumParams (SQLHSTMT hstmt, SQLSMALLINT * pcpar)
{
  SQLNumParamsPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLNumParamsPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLNumParams"));

    if(func) return func(hstmt, pcpar);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLNumParams", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hstmt, pcpar);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLNumResultCols (SQLHSTMT StatementHandle,
      SQLSMALLINT * ColumnCount)
{
  SQLNumResultColsPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLNumResultColsPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLNumResultCols"));

    if(func) return func(StatementHandle, ColumnCount);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLNumResultCols", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, ColumnCount);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLParamData (SQLHSTMT StatementHandle,
      SQLPOINTER * Value)
{
  SQLParamDataPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLParamDataPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLParamData"));

    if(func) return func(StatementHandle, Value);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLParamData", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, Value);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLParamOptions (SQLHSTMT hstmt,
      SQLUINTEGER crow, SQLUINTEGER * pirow)
{
  SQLParamOptionsPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLParamOptionsPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLParamOptions"));

    if(func) return func(hstmt, crow, pirow);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLParamOptions", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hstmt, crow, pirow);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLPrepare (SQLHSTMT StatementHandle,
      SQLCHAR * StatementText, SQLINTEGER TextLength)
{
  SQLPreparePTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLPreparePTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLPrepare"));

    if(func) return func(StatementHandle, StatementText, TextLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLPrepare", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, StatementText, TextLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLPrepareW (SQLHSTMT StatementHandle,
      SQLWCHAR * StatementText, SQLINTEGER TextLength)
{
  SQLPrepareWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLPrepareWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLPrepareW"));

    if(func) return func(StatementHandle, StatementText, TextLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLPrepareW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, StatementText, TextLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLPrepareA (SQLHSTMT StatementHandle,
      SQLCHAR * StatementText, SQLINTEGER TextLength)
{
  SQLPrepareAPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLPrepareAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLPrepareA"));

    if(func) return func(StatementHandle, StatementText, TextLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLPrepareA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, StatementText, TextLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLPrimaryKeys (SQLHSTMT hstmt,
      SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName,
      SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName,
      SQLCHAR * szTableName, SQLSMALLINT cbTableName)
{
  SQLPrimaryKeysPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLPrimaryKeysPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLPrimaryKeys"));

    if(func) return func(hstmt, szCatalogName, cbCatalogName,
	  szSchemaName, cbSchemaName, szTableName, cbTableName);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLPrimaryKeys", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hstmt, szCatalogName, cbCatalogName,
	  szSchemaName, cbSchemaName, szTableName, cbTableName);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLPrimaryKeysW (SQLHSTMT hstmt,
      SQLWCHAR * szCatalogName, SQLSMALLINT cbCatalogName,
      SQLWCHAR * szSchemaName, SQLSMALLINT cbSchemaName,
      SQLWCHAR * szTableName, SQLSMALLINT cbTableName)
{
  SQLPrimaryKeysWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLPrimaryKeysWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLPrimaryKeysW"));

    if(func) return func(hstmt, szCatalogName, cbCatalogName,
	  szSchemaName, cbSchemaName, szTableName, cbTableName);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLPrimaryKeysW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hstmt, szCatalogName, cbCatalogName,
	  szSchemaName, cbSchemaName, szTableName, cbTableName);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLPrimaryKeysA (SQLHSTMT hstmt,
      SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName,
      SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName,
      SQLCHAR * szTableName, SQLSMALLINT cbTableName)
{
  SQLPrimaryKeysAPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLPrimaryKeysAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLPrimaryKeysA"));

    if(func) return func(hstmt, szCatalogName, cbCatalogName,
	  szSchemaName, cbSchemaName, szTableName, cbTableName);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLPrimaryKeysA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hstmt, szCatalogName, cbCatalogName,
	  szSchemaName, cbSchemaName, szTableName, cbTableName);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLProcedureColumns (SQLHSTMT hstmt,
      SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName,
      SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName,
      SQLCHAR * szProcName, SQLSMALLINT cbProcName,
      SQLCHAR * szColumnName, SQLSMALLINT cbColumnName)
{
  SQLProcedureColumnsPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLProcedureColumnsPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLProcedureColumns"));

    if(func) return func(hstmt, szCatalogName, cbCatalogName,
	  szSchemaName, cbSchemaName, szProcName, cbProcName,
	  szColumnName, cbColumnName);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLProcedureColumns", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hstmt, szCatalogName, cbCatalogName,
	  szSchemaName, cbSchemaName, szProcName, cbProcName,
	  szColumnName, cbColumnName);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLProcedureColumnsW (SQLHSTMT hstmt,
      SQLWCHAR * szCatalogName, SQLSMALLINT cbCatalogName,
      SQLWCHAR * szSchemaName, SQLSMALLINT cbSchemaName,
      SQLWCHAR * szProcName, SQLSMALLINT cbProcName,
      SQLWCHAR * szColumnName, SQLSMALLINT cbColumnName)
{
  SQLProcedureColumnsWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLProcedureColumnsWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLProcedureColumnsW"));

    if(func) return func(hstmt, szCatalogName, cbCatalogName,
	  szSchemaName, cbSchemaName, szProcName, cbProcName,
	  szColumnName, cbColumnName);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLProcedureColumnsW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hstmt, szCatalogName, cbCatalogName,
	  szSchemaName, cbSchemaName, szProcName, cbProcName,
	  szColumnName, cbColumnName);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLProcedureColumnsA (SQLHSTMT hstmt,
      SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName,
      SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName,
      SQLCHAR * szProcName, SQLSMALLINT cbProcName,
      SQLCHAR * szColumnName, SQLSMALLINT cbColumnName)
{
  SQLProcedureColumnsAPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLProcedureColumnsAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLProcedureColumnsA"));

    if(func) return func(hstmt, szCatalogName, cbCatalogName,
	  szSchemaName, cbSchemaName, szProcName, cbProcName,
	  szColumnName, cbColumnName);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLProcedureColumnsA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hstmt, szCatalogName, cbCatalogName,
	  szSchemaName, cbSchemaName, szProcName, cbProcName,
	  szColumnName, cbColumnName);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLProcedures (SQLHSTMT hstmt,
      SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName,
      SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR * szProcName, SQLSMALLINT cbProcName)
{
  SQLProceduresPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLProceduresPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLProcedures"));

    if(func) return func(hstmt, szCatalogName, cbCatalogName,
	  szSchemaName, cbSchemaName, szProcName, cbProcName);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLProcedures", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hstmt, szCatalogName, cbCatalogName,
	  szSchemaName, cbSchemaName, szProcName, cbProcName);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLProceduresW (SQLHSTMT hstmt,
      SQLWCHAR * szCatalogName, SQLSMALLINT cbCatalogName,
      SQLWCHAR * szSchemaName, SQLSMALLINT cbSchemaName, SQLWCHAR * szProcName, SQLSMALLINT cbProcName)
{
  SQLProceduresWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLProceduresWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLProceduresW"));

    if(func) return func(hstmt, szCatalogName, cbCatalogName,
	  szSchemaName, cbSchemaName, szProcName, cbProcName);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLProceduresW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hstmt, szCatalogName, cbCatalogName,
	  szSchemaName, cbSchemaName, szProcName, cbProcName);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLProceduresA (SQLHSTMT hstmt,
      SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName,
      SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR * szProcName, SQLSMALLINT cbProcName)
{
  SQLProceduresAPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLProceduresAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLProceduresA"));

    if(func) return func(hstmt, szCatalogName, cbCatalogName,
	  szSchemaName, cbSchemaName, szProcName, cbProcName);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLProceduresA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hstmt, szCatalogName, cbCatalogName,
	  szSchemaName, cbSchemaName, szProcName, cbProcName);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLPutData (SQLHSTMT StatementHandle,
      SQLPOINTER Data, SQLINTEGER StrLen_or_Ind)
{
  SQLPutDataPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLPutDataPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLPutData"));

    if(func) return func(StatementHandle, Data, StrLen_or_Ind);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLPutData", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, Data, StrLen_or_Ind);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLRowCount (SQLHSTMT StatementHandle,
      SQLINTEGER * RowCount)
{
  SQLRowCountPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLRowCountPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLRowCount"));

    if(func) return func(StatementHandle, RowCount);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLRowCount", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, RowCount);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLSetConnectAttr (SQLHDBC ConnectionHandle,
      SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER StringLength)
{
  SQLSetConnectAttrPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLSetConnectAttrPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLSetConnectAttr"));

    if(func) return func(ConnectionHandle, Attribute, Value, StringLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLSetConnectAttr", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(ConnectionHandle, Attribute, Value, StringLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLSetConnectAttrW (SQLHDBC ConnectionHandle,
      SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER StringLength)
{
  SQLSetConnectAttrWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLSetConnectAttrWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLSetConnectAttrW"));

    if(func) return func(ConnectionHandle, Attribute, Value, StringLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLSetConnectAttrW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(ConnectionHandle, Attribute, Value, StringLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLSetConnectAttrA (SQLHDBC ConnectionHandle,
      SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER StringLength)
{
  SQLSetConnectAttrAPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLSetConnectAttrAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLSetConnectAttrA"));

    if(func) return func(ConnectionHandle, Attribute, Value, StringLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLSetConnectAttrA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(ConnectionHandle, Attribute, Value, StringLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLSetConnectOption (SQLHDBC ConnectionHandle,
      SQLUSMALLINT Option, SQLUINTEGER Value)
{
  SQLSetConnectOptionPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLSetConnectOptionPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLSetConnectOption"));

    if(func) return func(ConnectionHandle, Option, Value);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLSetConnectOption", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(ConnectionHandle, Option, Value);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLSetConnectOptionW (SQLHDBC ConnectionHandle,
      SQLUSMALLINT Option, SQLUINTEGER Value)
{
  SQLSetConnectOptionWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLSetConnectOptionWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLSetConnectOptionW"));

    if(func) return func(ConnectionHandle, Option, Value);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLSetConnectOptionW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(ConnectionHandle, Option, Value);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLSetConnectOptionA (SQLHDBC ConnectionHandle,
      SQLUSMALLINT Option, SQLUINTEGER Value)
{
  SQLSetConnectOptionAPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLSetConnectOptionAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLSetConnectOptionA"));

    if(func) return func(ConnectionHandle, Option, Value);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLSetConnectOptionA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(ConnectionHandle, Option, Value);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLSetCursorName (SQLHSTMT StatementHandle,
      SQLCHAR * CursorName, SQLSMALLINT NameLength)
{
  SQLSetCursorNamePTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLSetCursorNamePTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLSetCursorName"));

    if(func) return func(StatementHandle, CursorName, NameLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLSetCursorName", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, CursorName, NameLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLSetCursorNameW (SQLHSTMT StatementHandle,
      SQLWCHAR * CursorName, SQLSMALLINT NameLength)
{
  SQLSetCursorNameWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLSetCursorNameWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLSetCursorNameW"));

    if(func) return func(StatementHandle, CursorName, NameLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLSetCursorNameW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, CursorName, NameLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLSetCursorNameA (SQLHSTMT StatementHandle,
      SQLCHAR * CursorName, SQLSMALLINT NameLength)
{
  SQLSetCursorNameAPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLSetCursorNameAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLSetCursorNameA"));

    if(func) return func(StatementHandle, CursorName, NameLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLSetCursorNameA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, CursorName, NameLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLSetDescField (SQLHDESC DescriptorHandle,
      SQLSMALLINT RecNumber,
      SQLSMALLINT FieldIdentifier, SQLPOINTER Value, SQLINTEGER BufferLength)
{
  SQLSetDescFieldPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLSetDescFieldPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLSetDescField"));

    if(func) return func(DescriptorHandle, RecNumber, FieldIdentifier,
	  Value, BufferLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLSetDescField", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(DescriptorHandle, RecNumber, FieldIdentifier,
	  Value, BufferLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLSetDescFieldW (SQLHDESC DescriptorHandle,
      SQLSMALLINT RecNumber,
      SQLSMALLINT FieldIdentifier, SQLPOINTER Value, SQLINTEGER BufferLength)
{
  SQLSetDescFieldWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLSetDescFieldWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLSetDescFieldW"));

    if(func) return func(DescriptorHandle, RecNumber, FieldIdentifier,
	  Value, BufferLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLSetDescFieldW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(DescriptorHandle, RecNumber, FieldIdentifier,
	  Value, BufferLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLSetDescFieldA (SQLHDESC DescriptorHandle,
      SQLSMALLINT RecNumber,
      SQLSMALLINT FieldIdentifier, SQLPOINTER Value, SQLINTEGER BufferLength)
{
  SQLSetDescFieldAPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLSetDescFieldAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLSetDescFieldA"));

    if(func) return func(DescriptorHandle, RecNumber, FieldIdentifier,
	  Value, BufferLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLSetDescFieldA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(DescriptorHandle, RecNumber, FieldIdentifier,
	  Value, BufferLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLSetDescRec (SQLHDESC DescriptorHandle,
      SQLSMALLINT RecNumber, SQLSMALLINT Type,
      SQLSMALLINT SubType, SQLINTEGER Length,
      SQLSMALLINT Precision, SQLSMALLINT Scale,
      SQLPOINTER Data, SQLINTEGER * StringLength, SQLINTEGER * Indicator)
{
  SQLSetDescRecPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLSetDescRecPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLSetDescRec"));

    if(func) return func(DescriptorHandle, RecNumber, Type, SubType, Length,
	  Precision, Scale, Data, StringLength, Indicator);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLSetDescRec", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(DescriptorHandle, RecNumber, Type, SubType, Length,
	  Precision, Scale, Data, StringLength, Indicator);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLSetEnvAttr (SQLHENV EnvironmentHandle,
      SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER StringLength)
{
  SQLSetEnvAttrPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLSetEnvAttrPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLSetEnvAttr"));

    if(func) return func(EnvironmentHandle, Attribute, Value, StringLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLSetEnvAttr", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(EnvironmentHandle, Attribute, Value, StringLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLSetParam (SQLHSTMT StatementHandle,
      SQLUSMALLINT ParameterNumber, SQLSMALLINT ValueType,
      SQLSMALLINT ParameterType, SQLUINTEGER LengthPrecision,
      SQLSMALLINT ParameterScale, SQLPOINTER ParameterValue, SQLINTEGER * StrLen_or_Ind)
{
  SQLSetParamPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLSetParamPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLSetParam"));

    if(func) return func(StatementHandle, ParameterNumber, ValueType,
	  ParameterType, LengthPrecision, ParameterScale, ParameterValue,
	  StrLen_or_Ind);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLSetParam", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, ParameterNumber, ValueType,
	  ParameterType, LengthPrecision, ParameterScale, ParameterValue,
	  StrLen_or_Ind);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLSetPos (SQLHSTMT hstmt,
      SQLUSMALLINT irow, SQLUSMALLINT fOption, SQLUSMALLINT fLock)
{
  SQLSetPosPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLSetPosPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLSetPos"));

    if(func) return func(hstmt, irow, fOption, fLock);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLSetPos", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hstmt, irow, fOption, fLock);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLSetScrollOptions (SQLHSTMT hstmt,
      SQLUSMALLINT fConcurrency, SQLINTEGER crowKeyset, SQLUSMALLINT crowRowset)
{
  SQLSetScrollOptionsPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLSetScrollOptionsPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLSetScrollOptions"));

    if(func) return func(hstmt, fConcurrency, crowKeyset, crowRowset);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLSetScrollOptions", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hstmt, fConcurrency, crowKeyset, crowRowset);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLSetStmtAttr (SQLHSTMT StatementHandle,
      SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER StringLength)
{
  SQLSetStmtAttrPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLSetStmtAttrPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLSetStmtAttr"));

    if(func) return func(StatementHandle, Attribute, Value, StringLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLSetStmtAttr", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, Attribute, Value, StringLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLSetStmtAttrW (SQLHSTMT StatementHandle,
      SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER StringLength)
{
  SQLSetStmtAttrWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLSetStmtAttrWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLSetStmtAttrW"));

    if(func) return func(StatementHandle, Attribute, Value, StringLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLSetStmtAttrW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, Attribute, Value, StringLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLSetStmtAttrA (SQLHSTMT StatementHandle,
      SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER StringLength)
{
  SQLSetStmtAttrAPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLSetStmtAttrAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLSetStmtAttrA"));

    if(func) return func(StatementHandle, Attribute, Value, StringLength);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLSetStmtAttrA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, Attribute, Value, StringLength);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLSetStmtOption (SQLHSTMT StatementHandle,
      SQLUSMALLINT Option, SQLUINTEGER Value)
{
  SQLSetStmtOptionPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLSetStmtOptionPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLSetStmtOption"));

    if(func) return func(StatementHandle, Option, Value);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLSetStmtOption", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, Option, Value);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLSpecialColumns (SQLHSTMT StatementHandle,
      SQLUSMALLINT IdentifierType, SQLCHAR * CatalogName,
      SQLSMALLINT NameLength1, SQLCHAR * SchemaName,
      SQLSMALLINT NameLength2, SQLCHAR * TableName,
      SQLSMALLINT NameLength3, SQLUSMALLINT Scope, SQLUSMALLINT Nullable)
{
  SQLSpecialColumnsPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLSpecialColumnsPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLSpecialColumns"));

    if(func) return func(StatementHandle, IdentifierType, CatalogName,
	  NameLength1, SchemaName, NameLength2, TableName,
	  NameLength3, Scope, Nullable);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLSpecialColumns", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, IdentifierType, CatalogName,
	  NameLength1, SchemaName, NameLength2, TableName,
	  NameLength3, Scope, Nullable);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLSpecialColumnsW (SQLHSTMT StatementHandle,
      SQLUSMALLINT IdentifierType, SQLWCHAR * CatalogName,
      SQLSMALLINT NameLength1, SQLWCHAR * SchemaName,
      SQLSMALLINT NameLength2, SQLWCHAR * TableName,
      SQLSMALLINT NameLength3, SQLUSMALLINT Scope, SQLUSMALLINT Nullable)
{
  SQLSpecialColumnsWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLSpecialColumnsWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLSpecialColumnsW"));

    if(func) return func(StatementHandle, IdentifierType, CatalogName,
	  NameLength1, SchemaName, NameLength2, TableName,
	  NameLength3, Scope, Nullable);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLSpecialColumnsW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, IdentifierType, CatalogName,
	  NameLength1, SchemaName, NameLength2, TableName,
	  NameLength3, Scope, Nullable);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLSpecialColumnsA (SQLHSTMT StatementHandle,
      SQLUSMALLINT IdentifierType, SQLCHAR * CatalogName,
      SQLSMALLINT NameLength1, SQLCHAR * SchemaName,
      SQLSMALLINT NameLength2, SQLCHAR * TableName,
      SQLSMALLINT NameLength3, SQLUSMALLINT Scope, SQLUSMALLINT Nullable)
{
  SQLSpecialColumnsAPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLSpecialColumnsAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLSpecialColumnsA"));

    if(func) return func(StatementHandle, IdentifierType, CatalogName,
	  NameLength1, SchemaName, NameLength2, TableName,
	  NameLength3, Scope, Nullable);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLSpecialColumnsA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, IdentifierType, CatalogName,
	  NameLength1, SchemaName, NameLength2, TableName,
	  NameLength3, Scope, Nullable);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLStatistics (SQLHSTMT StatementHandle,
      SQLCHAR * CatalogName, SQLSMALLINT NameLength1,
      SQLCHAR * SchemaName, SQLSMALLINT NameLength2,
      SQLCHAR * TableName,
      SQLSMALLINT NameLength3, SQLUSMALLINT Unique, SQLUSMALLINT Reserved)
{
  SQLStatisticsPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLStatisticsPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLStatistics"));

    if(func) return func(StatementHandle, CatalogName, NameLength1, SchemaName,
	  NameLength2, TableName, NameLength3, Unique, Reserved);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLStatistics", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, CatalogName, NameLength1, SchemaName,
	  NameLength2, TableName, NameLength3, Unique, Reserved);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLStatisticsW (SQLHSTMT StatementHandle,
      SQLWCHAR * CatalogName, SQLSMALLINT NameLength1,
      SQLWCHAR * SchemaName, SQLSMALLINT NameLength2,
      SQLWCHAR * TableName,
      SQLSMALLINT NameLength3, SQLUSMALLINT Unique, SQLUSMALLINT Reserved)
{
  SQLStatisticsWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLStatisticsWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLStatisticsW"));

    if(func) return func(StatementHandle, CatalogName, NameLength1, SchemaName,
	  NameLength2, TableName, NameLength3, Unique, Reserved);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLStatisticsW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, CatalogName, NameLength1, SchemaName,
	  NameLength2, TableName, NameLength3, Unique, Reserved);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLStatisticsA (SQLHSTMT StatementHandle,
      SQLCHAR * CatalogName, SQLSMALLINT NameLength1,
      SQLCHAR * SchemaName, SQLSMALLINT NameLength2,
      SQLCHAR * TableName,
      SQLSMALLINT NameLength3, SQLUSMALLINT Unique, SQLUSMALLINT Reserved)
{
  SQLStatisticsAPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLStatisticsAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLStatisticsA"));

    if(func) return func(StatementHandle, CatalogName, NameLength1, SchemaName,
	  NameLength2, TableName, NameLength3, Unique, Reserved);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLStatisticsA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, CatalogName, NameLength1, SchemaName,
	  NameLength2, TableName, NameLength3, Unique, Reserved);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLTablePrivileges (SQLHSTMT hstmt,
      SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName,
      SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName,
      SQLCHAR * szTableName, SQLSMALLINT cbTableName)
{
  SQLTablePrivilegesPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLTablePrivilegesPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLTablePrivileges"));

    if(func) return func(hstmt, szCatalogName, cbCatalogName,
	  szSchemaName, cbSchemaName, szTableName, cbTableName);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLTablePrivileges", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hstmt, szCatalogName, cbCatalogName,
	  szSchemaName, cbSchemaName, szTableName, cbTableName);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLTablePrivilegesW (SQLHSTMT hstmt,
      SQLWCHAR * szCatalogName, SQLSMALLINT cbCatalogName,
      SQLWCHAR * szSchemaName, SQLSMALLINT cbSchemaName,
      SQLWCHAR * szTableName, SQLSMALLINT cbTableName)
{
  SQLTablePrivilegesWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLTablePrivilegesWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLTablePrivilegesW"));

    if(func) return func(hstmt, szCatalogName, cbCatalogName,
	  szSchemaName, cbSchemaName, szTableName, cbTableName);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLTablePrivilegesW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hstmt, szCatalogName, cbCatalogName,
	  szSchemaName, cbSchemaName, szTableName, cbTableName);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLTablePrivilegesA (SQLHSTMT hstmt,
      SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName,
      SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName,
      SQLCHAR * szTableName, SQLSMALLINT cbTableName)
{
  SQLTablePrivilegesAPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLTablePrivilegesAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLTablePrivilegesA"));

    if(func) return func(hstmt, szCatalogName, cbCatalogName,
	  szSchemaName, cbSchemaName, szTableName, cbTableName);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLTablePrivilegesA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hstmt, szCatalogName, cbCatalogName,
	  szSchemaName, cbSchemaName, szTableName, cbTableName);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLTables (SQLHSTMT StatementHandle,
      SQLCHAR * CatalogName, SQLSMALLINT NameLength1,
      SQLCHAR * SchemaName, SQLSMALLINT NameLength2,
      SQLCHAR * TableName,
      SQLSMALLINT NameLength3, SQLCHAR * TableType, SQLSMALLINT NameLength4)
{
  SQLTablesPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLTablesPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLTables"));

    if(func) return func(StatementHandle, CatalogName, NameLength1,
	  SchemaName, NameLength2, TableName, NameLength3, TableType, NameLength4);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLTables", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, CatalogName, NameLength1,
	  SchemaName, NameLength2, TableName, NameLength3, TableType, NameLength4);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLTablesW (SQLHSTMT StatementHandle,
      SQLWCHAR * CatalogName, SQLSMALLINT NameLength1,
      SQLWCHAR * SchemaName, SQLSMALLINT NameLength2,
      SQLWCHAR * TableName,
      SQLSMALLINT NameLength3, SQLWCHAR * TableType, SQLSMALLINT NameLength4)
{
  SQLTablesWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLTablesWPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLTablesW"));

    if(func) return func(StatementHandle, CatalogName, NameLength1,
	  SchemaName, NameLength2, TableName, NameLength3, TableType, NameLength4);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLTablesW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, CatalogName, NameLength1,
	  SchemaName, NameLength2, TableName, NameLength3, TableType, NameLength4);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLTablesA (SQLHSTMT StatementHandle,
      SQLCHAR * CatalogName, SQLSMALLINT NameLength1,
      SQLCHAR * SchemaName, SQLSMALLINT NameLength2,
      SQLCHAR * TableName,
      SQLSMALLINT NameLength3, SQLCHAR * TableType, SQLSMALLINT NameLength4)
{
  SQLTablesAPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLTablesAPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLTablesA"));

    if(func) return func(StatementHandle, CatalogName, NameLength1,
	  SchemaName, NameLength2, TableName, NameLength3, TableType, NameLength4);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLTablesA", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(StatementHandle, CatalogName, NameLength1,
	  SchemaName, NameLength2, TableName, NameLength3, TableType, NameLength4);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

SQLRETURN SQL_API SQLTransact (SQLHENV EnvironmentHandle,
      SQLHDBC ConnectionHandle, SQLUSMALLINT CompletionType)
{
  SQLTransactPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLTransactPTR)CFBundleGetFunctionPointerForName(iodbcRef,
      CFSTR("SQLTransact"));

    if(func) return func(EnvironmentHandle, ConnectionHandle, CompletionType);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcId, "\pSQLTransact", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(EnvironmentHandle, ConnectionHandle, CompletionType);
    create_error("Function loading problem",
      "The ODBC Driver Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

/* iODBCinst bundle prototypes functions */
BOOL INSTAPI SQLConfigDataSource (HWND hwndParent,
      WORD fRequest, LPCSTR lpszDriver, LPCSTR lpszAttributes)
{
  SQLConfigDataSourcePTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLConfigDataSourcePTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLConfigDataSource"));

    if(func) return func(hwndParent, fRequest, lpszDriver, lpszAttributes);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLConfigDataSource", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hwndParent, fRequest, lpszDriver, lpszAttributes);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLConfigDataSourceW (HWND hwndParent,
      WORD fRequest, LPCWSTR lpszDriver, LPCWSTR lpszAttributes)
{
  SQLConfigDataSourceWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLConfigDataSourceWPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLConfigDataSourceW"));

    if(func) return func(hwndParent, fRequest, lpszDriver, lpszAttributes);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLConfigDataSourceW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hwndParent, fRequest, lpszDriver, lpszAttributes);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLConfigDriver (HWND hwndParent,
      WORD fRequest, LPCSTR lpszDriver,
      LPCSTR lpszArgs, LPSTR lpszMsg, WORD cbMsgMax, WORD FAR * pcbMsgOut)
{
  SQLConfigDriverPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLConfigDriverPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLConfigDriver"));

    if(func) return func(hwndParent, fRequest, lpszDriver, lpszArgs,
	  lpszMsg, cbMsgMax, pcbMsgOut);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLConfigDriver", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hwndParent, fRequest, lpszDriver, lpszArgs,
	  lpszMsg, cbMsgMax, pcbMsgOut);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLConfigDriverW (HWND hwndParent,
      WORD fRequest, LPCWSTR lpszDriver,
      LPCWSTR lpszArgs, LPWSTR lpszMsg, WORD cbMsgMax, WORD FAR * pcbMsgOut)
{
  SQLConfigDriverWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLConfigDriverWPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLConfigDriverW"));

    if(func) return func(hwndParent, fRequest, lpszDriver, lpszArgs,
	  lpszMsg, cbMsgMax, pcbMsgOut);
    create_error("Function loading problem",
      "The iODBC.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLConfigDriverW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hwndParent, fRequest, lpszDriver, lpszArgs,
	  lpszMsg, cbMsgMax, pcbMsgOut);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLCreateDataSource (HWND hwndParent, LPCSTR lpszDSN)
{
  SQLCreateDataSourcePTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLCreateDataSourcePTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLCreateDataSource"));

    if(func) return func(hwndParent, lpszDSN);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLCreateDataSource", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hwndParent, lpszDSN);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLCreateDataSourceW (HWND hwndParent, LPCWSTR lpszDSN)
{
  SQLCreateDataSourceWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLCreateDataSourceWPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLCreateDataSourceW"));

    if(func) return func(hwndParent, lpszDSN);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLCreateDataSourceW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hwndParent, lpszDSN);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLGetConfigMode (UWORD * pwConfigMode)
{
  SQLGetConfigModePTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetConfigModePTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLGetConfigMode"));

    if(func) return func(pwConfigMode);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLGetConfigMode", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(pwConfigMode);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLGetAvailableDrivers (LPCSTR lpszInfFile,
      LPSTR lpszBuf, WORD cbBufMax, WORD FAR * pcbBufOut)
{
  SQLGetAvailableDriversPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetAvailableDriversPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLGetAvailableDrivers"));

    if(func) return func(lpszInfFile, lpszBuf, cbBufMax, pcbBufOut);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLGetAvailableDrivers", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszInfFile, lpszBuf, cbBufMax, pcbBufOut);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLGetAvailableDriversW (LPCWSTR lpszInfFile,
      LPWSTR lpszBuf, WORD cbBufMax, WORD FAR * pcbBufOut)
{
  SQLGetAvailableDriversWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetAvailableDriversWPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLGetAvailableDriversW"));

    if(func) return func(lpszInfFile, lpszBuf, cbBufMax, pcbBufOut);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLGetAvailableDriversW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszInfFile, lpszBuf, cbBufMax, pcbBufOut);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLGetInstalledDrivers (LPSTR lpszBuf,
      WORD cbBufMax, WORD FAR * pcbBufOut)
{
  SQLGetInstalledDriversPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetInstalledDriversPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLGetInstalledDrivers"));

    if(func) return func(lpszBuf, cbBufMax, pcbBufOut);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLGetInstalledDrivers", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszBuf, cbBufMax, pcbBufOut);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLGetInstalledDriversW (LPWSTR lpszBuf,
      WORD cbBufMax, WORD FAR * pcbBufOut)
{
  SQLGetInstalledDriversWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetInstalledDriversWPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLGetInstalledDriversW"));

    if(func) return func(lpszBuf, cbBufMax, pcbBufOut);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLGetInstalledDriversW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszBuf, cbBufMax, pcbBufOut);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

int INSTAPI SQLGetPrivateProfileString (LPCSTR lpszSection,
      LPCSTR lpszEntry, LPCSTR lpszDefault,
      LPSTR lpszRetBuffer, int cbRetBuffer, LPCSTR lpszFilename)
{
  SQLGetPrivateProfileStringPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetPrivateProfileStringPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLGetPrivateProfileString"));

    if(func) return func(lpszSection, lpszEntry, lpszDefault,
	  lpszRetBuffer, cbRetBuffer, lpszFilename);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLGetPrivateProfileString", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszSection, lpszEntry, lpszDefault,
	  lpszRetBuffer, cbRetBuffer, lpszFilename);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

int INSTAPI SQLGetPrivateProfileStringW (LPCWSTR lpszSection,
      LPCWSTR lpszEntry, LPCWSTR lpszDefault,
      LPWSTR lpszRetBuffer, int cbRetBuffer, LPCWSTR lpszFilename)
{
  SQLGetPrivateProfileStringWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetPrivateProfileStringWPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLGetPrivateProfileStringW"));

    if(func) return func(lpszSection, lpszEntry, lpszDefault,
	  lpszRetBuffer, cbRetBuffer, lpszFilename);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLGetPrivateProfileStringW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszSection, lpszEntry, lpszDefault,
	  lpszRetBuffer, cbRetBuffer, lpszFilename);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLGetTranslator (HWND hwnd,
      LPSTR lpszName, WORD cbNameMax,
      WORD FAR * pcbNameOut, LPSTR lpszPath,
      WORD cbPathMax, WORD FAR * pcbPathOut, DWORD FAR * pvOption)
{
  SQLGetTranslatorPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetTranslatorPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLGetTranslator"));

    if(func) return func(hwnd, lpszName, cbNameMax, pcbNameOut,
	  lpszPath, cbPathMax, pcbPathOut, pvOption);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLGetTranslator", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hwnd, lpszName, cbNameMax, pcbNameOut,
	  lpszPath, cbPathMax, pcbPathOut, pvOption);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLGetTranslatorW (HWND hwnd,
      LPWSTR lpszName, WORD cbNameMax,
      WORD FAR * pcbNameOut, LPWSTR lpszPath,
      WORD cbPathMax, WORD FAR * pcbPathOut, DWORD FAR * pvOption)
{
  SQLGetTranslatorWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetTranslatorWPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLGetTranslatorW"));

    if(func) return func(hwnd, lpszName, cbNameMax, pcbNameOut,
	  lpszPath, cbPathMax, pcbPathOut, pvOption);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLGetTranslatorW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hwnd, lpszName, cbNameMax, pcbNameOut,
	  lpszPath, cbPathMax, pcbPathOut, pvOption);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLInstallDriverEx (LPCSTR lpszDriver,
      LPCSTR lpszPathIn, LPSTR lpszPathOut,
      WORD cbPathOutMax, WORD * pcbPathOut, WORD fRequest, LPDWORD lpdwUsageCount)
{
  SQLInstallDriverExPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLInstallDriverExPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLInstallDriverEx"));

    if(func) return func(lpszDriver, lpszPathIn, lpszPathOut,
	  cbPathOutMax, pcbPathOut, fRequest, lpdwUsageCount);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLInstallDriverEx", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszDriver, lpszPathIn, lpszPathOut,
	  cbPathOutMax, pcbPathOut, fRequest, lpdwUsageCount);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLInstallDriverExW (LPCWSTR lpszDriver,
      LPCWSTR lpszPathIn, LPWSTR lpszPathOut,
      WORD cbPathOutMax, WORD * pcbPathOut, WORD fRequest, LPDWORD lpdwUsageCount)
{
  SQLInstallDriverExWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLInstallDriverExWPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLInstallDriverExW"));

    if(func) return func(lpszDriver, lpszPathIn, lpszPathOut,
	  cbPathOutMax, pcbPathOut, fRequest, lpdwUsageCount);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLInstallDriverExW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszDriver, lpszPathIn, lpszPathOut,
	  cbPathOutMax, pcbPathOut, fRequest, lpdwUsageCount);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLInstallDriver (LPCSTR lpszInfFile,
      LPCSTR lpszDriver, LPSTR lpszPath, WORD cbPathMax, WORD FAR * pcbPathOut)
{
  SQLInstallDriverPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLInstallDriverPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLInstallDriver"));

    if(func) return func(lpszInfFile, lpszDriver, lpszPath, cbPathMax, pcbPathOut);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLInstallDriver", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszInfFile, lpszDriver, lpszPath, cbPathMax, pcbPathOut);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLInstallDriverW (LPCWSTR lpszInfFile,
      LPCWSTR lpszDriver, LPWSTR lpszPath, WORD cbPathMax, WORD FAR * pcbPathOut)
{
  SQLInstallDriverWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLInstallDriverWPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLInstallDriverW"));

    if(func) return func(lpszInfFile, lpszDriver, lpszPath, cbPathMax, pcbPathOut);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLInstallDriverW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszInfFile, lpszDriver, lpszPath, cbPathMax, pcbPathOut);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLInstallDriverManager (LPSTR lpszPath,
      WORD cbPathMax, WORD FAR * pcbPathOut)
{
  SQLInstallDriverManagerPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLInstallDriverManagerPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLInstallDriverManager"));

    if(func) return func(lpszPath, cbPathMax, pcbPathOut);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLInstallDriverManager", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszPath, cbPathMax, pcbPathOut);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLInstallDriverManagerW (LPWSTR lpszPath,
      WORD cbPathMax, WORD FAR * pcbPathOut)
{
  SQLInstallDriverManagerWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLInstallDriverManagerWPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLInstallDriverManagerW"));

    if(func) return func(lpszPath, cbPathMax, pcbPathOut);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLInstallDriverManagerW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszPath, cbPathMax, pcbPathOut);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

RETCODE INSTAPI SQLInstallerError (WORD iError,
      DWORD * pfErrorCode,
      LPSTR lpszErrorMsg, WORD cbErrorMsgMax, WORD * pcbErrorMsg)
{
  SQLInstallerErrorPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLInstallerErrorPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLInstallerError"));

    if(func) return func(iError, pfErrorCode, lpszErrorMsg, cbErrorMsgMax, pcbErrorMsg);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLInstallerError", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(iError, pfErrorCode, lpszErrorMsg, cbErrorMsgMax, pcbErrorMsg);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

RETCODE INSTAPI SQLInstallerErrorW (WORD iError,
      DWORD * pfErrorCode,
      LPWSTR lpszErrorMsg, WORD cbErrorMsgMax, WORD * pcbErrorMsg)
{
  SQLInstallerErrorWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLInstallerErrorWPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLInstallerErrorW"));

    if(func) return func(iError, pfErrorCode, lpszErrorMsg, cbErrorMsgMax, pcbErrorMsg);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLInstallerErrorW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(iError, pfErrorCode, lpszErrorMsg, cbErrorMsgMax, pcbErrorMsg);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLInstallODBC (HWND hwndParent,
      LPCSTR lpszInfFile, LPCSTR lpszSrcPath, LPCSTR lpszDrivers)
{
  SQLInstallODBCPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLInstallODBCPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLInstallODBC"));

    if(func) return func(hwndParent, lpszInfFile, lpszSrcPath, lpszDrivers);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLInstallODBC", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hwndParent, lpszInfFile, lpszSrcPath, lpszDrivers);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLInstallODBCW (HWND hwndParent,
      LPCWSTR lpszInfFile, LPCWSTR lpszSrcPath, LPCWSTR lpszDrivers)
{
  SQLInstallODBCWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLInstallODBCWPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLInstallODBCW"));

    if(func) return func(hwndParent, lpszInfFile, lpszSrcPath, lpszDrivers);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLInstallODBCW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hwndParent, lpszInfFile, lpszSrcPath, lpszDrivers);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLInstallTranslator (LPCSTR lpszInfFile,
      LPCSTR lpszTranslator, LPCSTR lpszPathIn,
      LPSTR lpszPathOut, WORD cbPathOutMax,
      WORD FAR * pcbPathOut, WORD fRequest, LPDWORD lpdwUsageCount)
{
  SQLInstallTranslatorPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLInstallTranslatorPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLInstallTranslator"));

    if(func) return func(lpszInfFile, lpszTranslator, lpszPathIn,
	  lpszPathOut, cbPathOutMax, pcbPathOut, fRequest, lpdwUsageCount);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLInstallTranslator", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszInfFile, lpszTranslator, lpszPathIn,
	  lpszPathOut, cbPathOutMax, pcbPathOut, fRequest, lpdwUsageCount);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLInstallTranslatorW (LPCWSTR lpszInfFile,
      LPCWSTR lpszTranslator, LPCWSTR lpszPathIn,
      LPWSTR lpszPathOut, WORD cbPathOutMax,
      WORD FAR * pcbPathOut, WORD fRequest, LPDWORD lpdwUsageCount)
{
  SQLInstallTranslatorWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLInstallTranslatorWPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLInstallTranslatorW"));

    if(func) return func(lpszInfFile, lpszTranslator, lpszPathIn,
	  lpszPathOut, cbPathOutMax, pcbPathOut, fRequest, lpdwUsageCount);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLInstallTranslatorW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszInfFile, lpszTranslator, lpszPathIn,
	  lpszPathOut, cbPathOutMax, pcbPathOut, fRequest, lpdwUsageCount);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLInstallTranslatorEx (LPCSTR lpszTranslator,
      LPCSTR lpszPathIn, LPSTR lpszPathOut,
      WORD cbPathOutMax, WORD * pcbPathOut, WORD fRequest, LPDWORD lpdwUsageCount)
{
  SQLInstallTranslatorExPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLInstallTranslatorExPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLInstallTranslatorEx"));

    if(func) return func(lpszTranslator, lpszPathIn, lpszPathOut,
	  cbPathOutMax, pcbPathOut, fRequest, lpdwUsageCount);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLInstallTranslatorEx", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszTranslator, lpszPathIn, lpszPathOut,
	  cbPathOutMax, pcbPathOut, fRequest, lpdwUsageCount);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLInstallTranslatorExW (LPCWSTR lpszTranslator,
      LPCWSTR lpszPathIn, LPWSTR lpszPathOut,
      WORD cbPathOutMax, WORD * pcbPathOut, WORD fRequest, LPDWORD lpdwUsageCount)
{
  SQLInstallTranslatorExWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLInstallTranslatorExWPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLInstallTranslatorExW"));

    if(func) return func(lpszTranslator, lpszPathIn, lpszPathOut,
	  cbPathOutMax, pcbPathOut, fRequest, lpdwUsageCount);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLInstallTranslatorExW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszTranslator, lpszPathIn, lpszPathOut,
	  cbPathOutMax, pcbPathOut, fRequest, lpdwUsageCount);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLManageDataSources (HWND hwndParent)
{
  SQLManageDataSourcesPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLManageDataSourcesPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLManageDataSources"));

    if(func) return func(hwndParent);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLManageDataSources", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(hwndParent);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

RETCODE INSTAPI SQLPostInstallerError (DWORD fErrorCode, LPSTR szErrorMsg)
{
  SQLPostInstallerErrorPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLPostInstallerErrorPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLPostInstallerError"));

    if(func) return func(fErrorCode, szErrorMsg);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLPostInstallerError", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(fErrorCode, szErrorMsg);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

RETCODE INSTAPI SQLPostInstallerErrorW (DWORD fErrorCode, LPWSTR szErrorMsg)
{
  SQLPostInstallerErrorWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLPostInstallerErrorWPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLPostInstallerErrorW"));

    if(func) return func(fErrorCode, szErrorMsg);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLPostInstallerErrorW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(fErrorCode, szErrorMsg);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLReadFileDSN (LPCSTR lpszFileName,
      LPCSTR lpszAppName, LPCSTR lpszKeyName, LPSTR lpszString, WORD cbString, WORD * pcbString)
{
  SQLReadFileDSNPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLReadFileDSNPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLReadFileDSN"));

    if(func) return func(lpszFileName, lpszAppName, lpszKeyName,
	  lpszString, cbString, pcbString);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLReadFileDSN", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszFileName, lpszAppName, lpszKeyName,
	  lpszString, cbString, pcbString);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLReadFileDSNW (LPCWSTR lpszFileName,
      LPCWSTR lpszAppName, LPCWSTR lpszKeyName, LPWSTR lpszString, WORD cbString, WORD * pcbString)
{
  SQLReadFileDSNWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLReadFileDSNWPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLReadFileDSNW"));

    if(func) return func(lpszFileName, lpszAppName, lpszKeyName,
	  lpszString, cbString, pcbString);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLReadFileDSNW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszFileName, lpszAppName, lpszKeyName,
	  lpszString, cbString, pcbString);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLRemoveDSNFromIni (LPCSTR lpszDSN)
{
  SQLRemoveDSNFromIniPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLRemoveDSNFromIniPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLRemoveDSNFromIni"));

    if(func) return func(lpszDSN);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLRemoveDSNFromIni", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszDSN);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLRemoveDSNFromIniW (LPCWSTR lpszDSN)
{
  SQLRemoveDSNFromIniWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLRemoveDSNFromIniWPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLRemoveDSNFromIniW"));

    if(func) return func(lpszDSN);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLRemoveDSNFromIniW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszDSN);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLRemoveDefaultDataSource (void)
{
  SQLRemoveDefaultDataSourcePTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLRemoveDefaultDataSourcePTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLRemoveDefaultDataSource"));

    if(func) return func();
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLRemoveDefaultDataSource", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func();
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLRemoveDriver (LPCSTR lpszDriver,
      BOOL fRemoveDSN, LPDWORD lpdwUsageCount)
{
  SQLRemoveDriverPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLRemoveDriverPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLRemoveDriver"));

    if(func) return func(lpszDriver, fRemoveDSN, lpdwUsageCount);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLRemoveDriver", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszDriver, fRemoveDSN, lpdwUsageCount);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLRemoveDriverW (LPCWSTR lpszDriver,
      BOOL fRemoveDSN, LPDWORD lpdwUsageCount)
{
  SQLRemoveDriverWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLRemoveDriverWPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLRemoveDriverW"));

    if(func) return func(lpszDriver, fRemoveDSN, lpdwUsageCount);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLRemoveDriverW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszDriver, fRemoveDSN, lpdwUsageCount);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLRemoveDriverManager (LPDWORD lpdwUsageCount)
{
  SQLRemoveDriverManagerPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLRemoveDriverManagerPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLRemoveDriverManager"));

    if(func) return func(lpdwUsageCount);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLRemoveDriverManager", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpdwUsageCount);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLRemoveTranslator (LPCSTR lpszTranslator,
      LPDWORD lpdwUsageCount)
{
  SQLRemoveTranslatorPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLRemoveTranslatorPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLRemoveTranslator"));

    if(func) return func(lpszTranslator, lpdwUsageCount);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLRemoveTranslator", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszTranslator, lpdwUsageCount);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLRemoveTranslatorW (LPCWSTR lpszTranslator,
      LPDWORD lpdwUsageCount)
{
  SQLRemoveTranslatorWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLRemoveTranslatorWPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLRemoveTranslatorW"));

    if(func) return func(lpszTranslator, lpdwUsageCount);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLRemoveTranslatorW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszTranslator, lpdwUsageCount);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLSetConfigMode (UWORD wConfigMode)
{
  SQLSetConfigModePTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLSetConfigModePTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLSetConfigMode"));

    if(func) return func(wConfigMode);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLSetConfigMode", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(wConfigMode);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLValidDSN (LPCSTR lpszDSN)
{
  SQLValidDSNPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLValidDSNPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLValidDSN"));

    if(func) return func(lpszDSN);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLValidDSN", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszDSN);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLValidDSNW (LPCWSTR lpszDSN)
{
  SQLValidDSNWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLValidDSNWPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLValidDSNW"));

    if(func) return func(lpszDSN);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLValidDSNW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszDSN);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLWriteDSNToIni (LPCSTR lpszDSN, LPCSTR lpszDriver)
{
  SQLWriteDSNToIniPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLWriteDSNToIniPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLWriteDSNToIni"));

    if(func) return func(lpszDSN, lpszDriver);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLWriteDSNToIni", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszDSN, lpszDriver);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLWriteDSNToIniW (LPCWSTR lpszDSN, LPCWSTR lpszDriver)
{
  SQLWriteDSNToIniWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLWriteDSNToIniWPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLWriteDSNToIniW"));

    if(func) return func(lpszDSN, lpszDriver);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLWriteDSNToIniW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszDSN, lpszDriver);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLWriteFileDSN (LPCSTR lpszFileName,
      LPCSTR lpszAppName, LPCSTR lpszKeyName, LPSTR lpszString)
{
  SQLWriteFileDSNPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLWriteFileDSNPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLWriteFileDSN"));

    if(func) return func(lpszFileName, lpszAppName, lpszKeyName, lpszString);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLWriteFileDSN", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszFileName, lpszAppName, lpszKeyName, lpszString);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLWriteFileDSNW (LPCWSTR lpszFileName,
      LPCWSTR lpszAppName, LPCWSTR lpszKeyName, LPWSTR lpszString)
{
  SQLWriteFileDSNWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLWriteFileDSNWPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLWriteFileDSNW"));

    if(func) return func(lpszFileName, lpszAppName, lpszKeyName, lpszString);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLWriteFileDSNW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszFileName, lpszAppName, lpszKeyName, lpszString);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLWritePrivateProfileString (LPCSTR lpszSection,
      LPCSTR lpszEntry, LPCSTR lpszString, LPCSTR lpszFilename)
{
  SQLWritePrivateProfileStringPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLWritePrivateProfileStringPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLWritePrivateProfileString"));

    if(func) return func(lpszSection, lpszEntry, lpszString, lpszFilename);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLWritePrivateProfileString", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszSection, lpszEntry, lpszString, lpszFilename);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLWritePrivateProfileStringW (LPCWSTR lpszSection,
      LPCWSTR lpszEntry, LPCWSTR lpszString, LPCWSTR lpszFilename)
{
  SQLWritePrivateProfileStringWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLWritePrivateProfileStringWPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLWritePrivateProfileStringW"));

    if(func) return func(lpszSection, lpszEntry, lpszString, lpszFilename);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLWritePrivateProfileStringW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszSection, lpszEntry, lpszString, lpszFilename);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLGetKeywordValue (LPCSTR lpszSection,
      LPCSTR lpszEntry, LPSTR lpszBuffer, int cbBuffer, int *pcbBufOut)
{
  SQLGetKeywordValuePTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetKeywordValuePTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLGetKeywordValue"));

    if(func) return func(lpszSection, lpszEntry, lpszBuffer, cbBuffer, pcbBufOut);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLGetKeywordValue", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszSection, lpszEntry, lpszBuffer, cbBuffer, pcbBufOut);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLGetKeywordValueW (LPCWSTR lpszSection,
      LPCWSTR lpszEntry, LPWSTR lpszBuffer, int cbBuffer, int *pcbBufOut)
{
  SQLGetKeywordValueWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLGetKeywordValueWPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLGetKeywordValueW"));

    if(func) return func(lpszSection, lpszEntry, lpszBuffer, cbBuffer, pcbBufOut);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLGetKeywordValueW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszSection, lpszEntry, lpszBuffer, cbBuffer, pcbBufOut);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLSetKeywordValue (LPCSTR lpszSection,
      LPCSTR lpszEntry, LPSTR lpszString, int cbString)
{
  SQLSetKeywordValuePTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLSetKeywordValuePTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLSetKeywordValue"));

    if(func) return func(lpszSection, lpszEntry, lpszString, cbString);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLSetKeywordValue", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszSection, lpszEntry, lpszString, cbString);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLSetKeywordValueW (LPCWSTR lpszSection,
      LPCWSTR lpszEntry, LPWSTR lpszString, int cbString)
{
  SQLSetKeywordValueWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLSetKeywordValueWPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLSetKeywordValueW"));

    if(func) return func(lpszSection, lpszEntry, lpszString, cbString);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLSetKeywordValueW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszSection, lpszEntry, lpszString, cbString);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLWriteDSN (LPCSTR lpszDSN, LPCSTR lpszDriver)
{
  SQLWriteDSNPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLWriteDSNPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLWriteDSN"));

    if(func) return func(lpszDSN, lpszDriver);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLWriteDSN", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszDSN, lpszDriver);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLWriteDSNW (LPCWSTR lpszDSN, LPCWSTR lpszDriver)
{
  SQLWriteDSNWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLWriteDSNWPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLWriteDSNW"));

    if(func) return func(lpszDSN, lpszDriver);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLWriteDSNW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszDSN, lpszDriver);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLRemoveDSN (LPCSTR lpszDSN)
{
  SQLRemoveDSNPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLRemoveDSNPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLRemoveDSN"));

    if(func) return func(lpszDSN);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLRemoveDSN", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszDSN);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}

BOOL INSTAPI SQLRemoveDSNW (LPCWSTR lpszDSN)
{
  SQLRemoveDSNWPTR func;
  
  if(isOnMacOSX)
  {
    func = (SQLRemoveDSNWPTR)CFBundleGetFunctionPointerForName(iodbcinstRef,
      CFSTR("SQLRemoveDSNW"));

    if(func) return func(lpszDSN);
    create_error("Function loading problem",
      "The iODBCinst.framework function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS X system.");
  }
  else
  {
    CFragSymbolClass symbol_type;
    OSStatus err;

    err = FindSymbol (iodbcinstId, "\pSQLRemoveDSNW", (Ptr*)(&func), &symbol_type);
    if(err == noErr) return func(lpszDSN);
    create_error("Function loading problem",
      "The ODBC Configuration Manager PPC function cannot be loaded. Please check if iODBC is"
      " well installed on your Mac OS system.");
  }

  return SQL_ERROR;
}
