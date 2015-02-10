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

#import "TestController.h"
#import "ExecController.h"
#import "NSAttributedStringAdditions.h"
#import "LinkTextFieldCell.h"


#define MIN_WIDTH       5
#define MAX_WIDTH       80
#define OpenTag         56000
#define CloseTag        56001
#define ExecSQLTag      56002
#define NextRowsetTag   56003

@implementation TestController
@synthesize fQuery = _fQuery;

#ifdef UNICODE
wchar_t *
NStoTEXT (NSString * str)
{
    wchar_t *prov;
    unsigned int len, i;
    
    if (str == nil)
        return NULL;
    
    len = [str length];
    prov = malloc (sizeof (wchar_t) * (len + 1));
    
    if (prov)
    {
        for (i = 0; i < len; i++)
            prov[i] = [str characterAtIndex:i];
        prov[i] = L'\0';
    }
    
    return prov;
}


CFStringRef
TEXTtoNS (wchar_t * str)
{
    CFMutableStringRef prov;
    CFIndex i;
    UniChar c;
    
    if (!str)
        return NULL;
    
    prov = CFStringCreateMutable (NULL, 0);
    
    if (prov)
    {
        for (i = 0; str[i] != L'\0'; i++)
        {
            c = (UniChar) str[i];
            CFStringAppendCharacters (prov, &c, 1);
        }
    }
    
    return prov;
}

#define TEXT(x)		(SQLWCHAR *) L##x
#define TEXTLEN(x)	wcslen ((wchar_t *) x)
#define TEXTCMP(x,y)	wcscmp((wchar_t *) x, (wchar_t *) y)
#define TEXTCPY(x,y)	wcscpy((wchar_t *) x, (wchar_t *) y)

#else

#define TEXT(x)		(SQLCHAR *) x
#define TEXTLEN(x)	strlen ((char *) x)
#define TEXTCMP(x,y)	strcmp((char *) x, (char *) y)
#define TEXTCPY(x,y)	strcpy((char *) x, (char *) y)

#define TEXTtoNS(x)	[NSString stringWithUTF8String: x]
#define NStoTEXT(x)	[ x UTF8String ]
#endif




void
_nativeerrorbox (SQLHENV _henv, SQLHDBC _hdbc, SQLHSTMT _hstmt)
{
    SQLTCHAR buf[250];
    SQLTCHAR sqlstate[15];
    
    /*
     * Get statement errors
     */
    if (SQLError (_henv, _hdbc, _hstmt, sqlstate, NULL,
                  buf, sizeof (buf), NULL) == SQL_SUCCESS)
        NSRunAlertPanel(@"Native ODBC Error",
                        [NSString stringWithFormat:@"%@ [%@]", TEXTtoNS(buf), TEXTtoNS(sqlstate)], NULL, NULL, NULL);
    
    /*
     * Get connection errors
     */
    if (SQLError (_henv, _hdbc, SQL_NULL_HSTMT, sqlstate,
                  NULL, buf, sizeof (buf), NULL) == SQL_SUCCESS)
        NSRunAlertPanel(@"Native ODBC Error",
                        [NSString stringWithFormat:@"%@ [%@]", TEXTtoNS(buf), TEXTtoNS(sqlstate)], NULL, NULL, NULL);
    
    /*
     * Get environmental errors
     */
    if (SQLError (_henv, SQL_NULL_HDBC, SQL_NULL_HSTMT,
                  sqlstate, NULL, buf, sizeof (buf), NULL) == SQL_SUCCESS)
        NSRunAlertPanel(@"Native ODBC Error",
                        [NSString stringWithFormat:@"%@ [%@]", TEXTtoNS(buf), TEXTtoNS(sqlstate)], NULL, NULL, NULL);
}



- (id)init
{
    [super init];
    mConnected = NO;
    mExistsResultset = NO;
    mNextResultset = NO;
    mBuffer = [NSMutableArray new];
    mRows = 0;
    self.fQuery = @"select * from orders";
    //self.fQuery = @"sparql select * {?s ?p ?o} limit 10";
    mMaxRows = 1000;
    return self;
}

- (void)dealloc
{
    [self disconnect];
    [mBuffer release];
    [super dealloc];
}


- (void)awakeFromNib
{
    [RSTable setDataSource:self];
    [RSTable setDelegate:self];
    [self clearGrid];
#ifdef UNICODE
    [mWindow setTitle:[NSString stringWithFormat:@"iODBC Demo (Unicode) - Disconnected"]];
#else
    [mWindow setTitle:[NSString stringWithFormat:@"iODBC Demo (Ansi) - Disconnected"]];
#endif
}


- (void)disconnect
{
    if (hstmt != nil)
    {
#if (ODBCVER < 0x0300)
        SQLFreeStmt (hstmt, SQL_DROP);
#else
        SQLCloseCursor (hstmt);
        SQLFreeHandle (SQL_HANDLE_STMT, hstmt);
#endif
        hstmt = nil;
    }
    
    if (hdbc != nil)
    {
        if (mConnected)
            SQLDisconnect (hdbc);
        
#ifdef UNICODE
        [mWindow setTitle:[NSString stringWithFormat:@"iODBC Demo (Unicode) - Disconnected"]];
#else
        [mWindow setTitle:[NSString stringWithFormat:@"iODBC Demo (Ansi) - Disconnected"]];
#endif
        [self clearGrid];
        
        mConnected = NO;
        
#if (ODBCVER < 0x300)
        SQLFreeConnect (hdbc);
#else
        SQLFreeHandle (SQL_HANDLE_DBC, hdbc);
#endif
        hdbc = nil;
    }
    
    if (henv != nil)
    {
#if (ODBCVER < 0x300)
        SQLFreeEnv (henv);
#else
        SQLFreeHandle (SQL_HANDLE_ENV, henv);
#endif
        henv = nil;
    }
}


- (void)clearGrid
{
    NSArray *arr;
    unsigned i, count;
    NSTableColumn *aColumn;
    
    arr = [RSTable tableColumns];
    count = [arr count];
    for(i = 0; i < count; i++)
    {
        aColumn = [arr objectAtIndex:0];
        [RSTable removeTableColumn:aColumn];
    }
}


- (void)execSQL:(SQLTCHAR *)szSQL
{
    SQLRETURN sts;
    
    [self clearGrid];
    mSPARQL_executed = false;
    
    if (mExistsResultset == YES)
    {
#if (ODBCVER < 0x0300)
        SQLFreeStmt (hstmt, SQL_CLOSE);
#else
        SQLCloseCursor (hstmt);
#endif
    }
    
    mExistsResultset = NO;
    mNextResultset = NO;
    
    NSString *query = TEXTtoNS(szSQL);
    mSPARQL_executed = [[query.uppercaseString stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]] hasPrefix:@"SPARQL"];
    
    if (!TEXTCMP(szSQL, TEXT("tables")))
    {
        if (SQLTables (hstmt, NULL, 0, NULL, 0, NULL, 0, NULL, 0) != SQL_SUCCESS)
            goto error;
    }
    else if (!TEXTCMP(szSQL, TEXT("qualifiers")))
    {
        if (SQLTables (hstmt, TEXT("%"), SQL_NTS, TEXT(""), 0, TEXT(""), 0, TEXT(""), 0) != SQL_SUCCESS)
            goto error;
    }
    else if (!TEXTCMP(szSQL, TEXT("owners")))
    {
        if (SQLTables (hstmt, TEXT(""), SQL_NTS, TEXT("%"), 0, TEXT(""), 0, TEXT(""), 0) != SQL_SUCCESS)
            goto error;
    }
    else if (!TEXTCMP(szSQL, TEXT("types")))
    {
        if (SQLTables (hstmt, TEXT(""), SQL_NTS, TEXT(""), 0, TEXT(""), 0, TEXT("%"), 0) != SQL_SUCCESS)
            goto error;
    }
    else if (!TEXTCMP(szSQL, TEXT("datatypes")))
    {
        if (SQLGetTypeInfo (hstmt, 0) != SQL_SUCCESS)
            goto error;
    }
    else
    {
        if (SQLPrepare (hstmt, szSQL, SQL_NTS) != SQL_SUCCESS)
            goto error;
        
        if ((sts = SQLExecute (hstmt)) != SQL_SUCCESS)
        {
            _nativeerrorbox (henv, hdbc, hstmt);
            
            if (sts != SQL_SUCCESS_WITH_INFO)
                return;
        }
    }
    
    [self loadResult];
    return;
    
error:
    _nativeerrorbox (henv, hdbc, hstmt);
    return;
}


- (void)loadResult
{
    SQLTCHAR fetchBuffer[1024];
    size_t displayWidth;
    short numCols;
    unsigned colNum;
    SQLTCHAR colName[256];
    SQLSMALLINT colType;
    SQLULEN colPrecision;
    SQLLEN colIndicator;
    SQLSMALLINT colScale;
    SQLSMALLINT colNullable;
    unsigned long totalRows;
    NSTableColumn *aColumn;
    NSMutableArray *row;
    NSFont  *fnt = nil; //RSTable.font;
    
    if (!fnt)
        fnt = [NSFont systemFontOfSize:11.0];
    
    [self clearGrid];
    
    if (SQLNumResultCols (hstmt, &numCols) != SQL_SUCCESS)
    {
        _nativeerrorbox (henv, hdbc, hstmt);
        goto endCursor;
    }
    
    if (numCols == 0)
    {
        SQLLEN nrows = 0;
        SQLRowCount (hstmt, &nrows);
        
        NSRunAlertPanel(@"This operation completed successfully without returning data:",
                        [NSString stringWithFormat:@"%ld rows affected", (long)nrows],
                        NULL, NULL, NULL);
        goto endCursor;
    }
    
    mExistsResultset = YES;
    
    /*
     *  Get the names for the columns
     */
    for (colNum = 1; colNum <= numCols; colNum++)
    {
        /*
         *  Get the name and other type information
         */
        if (SQLDescribeCol (hstmt, colNum, (SQLTCHAR *) colName,
                            sizeof(colName), NULL, &colType, &colPrecision, &colScale,
                            &colNullable) != SQL_SUCCESS)
        {
            _nativeerrorbox (henv, hdbc, hstmt);
            goto endCursor;
        }
        
        /*
         *  Calculate the display width for the column
         */
        switch (colType)
        {
            case SQL_VARCHAR:
            case SQL_CHAR:
            case SQL_WVARCHAR:
            case SQL_WCHAR:
            case SQL_GUID:
                displayWidth = colPrecision;
                break;
                
            case SQL_BINARY:
                displayWidth = colPrecision * 2;
                break;
                
            case SQL_LONGVARCHAR:
            case SQL_WLONGVARCHAR:
            case SQL_LONGVARBINARY:
                displayWidth = 256;	/* show only first 256 */
                break;
                
            case SQL_BIT:
                displayWidth = 1;
                break;
                
            case SQL_TINYINT:
            case SQL_SMALLINT:
            case SQL_INTEGER:
            case SQL_BIGINT:
                displayWidth = colPrecision + 1;	/* sign */
                break;
                
            case SQL_DOUBLE:
            case SQL_DECIMAL:
            case SQL_NUMERIC:
            case SQL_FLOAT:
            case SQL_REAL:
                displayWidth = colPrecision + 2;	/* sign, comma */
                break;
                
#ifdef SQL_TYPE_DATE
            case SQL_TYPE_DATE:
#endif
            case SQL_DATE:
                displayWidth = 10;
                break;
                
#ifdef SQL_TYPE_TIME
            case SQL_TYPE_TIME:
#endif
            case SQL_TIME:
                displayWidth = 8;
                break;
                
#ifdef SQL_TYPE_TIMESTAMP
            case SQL_TYPE_TIMESTAMP:
#endif
            case SQL_TIMESTAMP:
                displayWidth = 19;
                if (colScale > 0)
                    displayWidth = displayWidth + colScale + 1;
                break;
                
            default:
                displayWidth = 0;	/* skip other data types */
                continue;
        }
        
        if (displayWidth < TEXTLEN(colName))
            displayWidth = TEXTLEN(colName);
        if (displayWidth > sizeof(fetchBuffer) - 1)
            displayWidth = sizeof(fetchBuffer) - 1;
        if (displayWidth > MAX_WIDTH)
            displayWidth = MAX_WIDTH;
        
        if (mSPARQL_executed && displayWidth <= 10)
            displayWidth = MAX_WIDTH;
        
        /* Add new column */
        aColumn = [NSTableColumn new];
        NSCell *cell = [[[LinkTextFieldCell alloc] init] autorelease];
        [aColumn setDataCell:cell];
        
        
        /* Calculate size of font */
        /**
         float fontWidth = [fnt boundingRectForFont].size.width;
         if (fontWidth == 0.0)
         fontWidth = [fnt maximumAdvancement].width;
         if (fontWidth == 0.0)
         fontWidth = 10.0; 	// Default
         **/
        float fontWidth = 10.0;
        
        /* Make sure we always have room for column label */
        int colNameLen = TEXTLEN(colName);
        colNameLen = colNameLen<MIN_WIDTH?MIN_WIDTH:colNameLen;
        displayWidth = displayWidth<MIN_WIDTH?MIN_WIDTH:displayWidth;
        
        float minWidth = fontWidth * (colNameLen<displayWidth?colNameLen:displayWidth);
        [aColumn setMinWidth:minWidth];
        
        /* Calculate maximum size of column */
        float strWidth = fontWidth * (displayWidth>colNameLen?displayWidth:colNameLen);
        [aColumn setMaxWidth:strWidth];
        
        /* Fill in column information  */
        [aColumn setIdentifier:[NSString stringWithFormat:@"%d", colNum-1]];
        [aColumn setEditable:NO];
        [aColumn setResizingMask:NSTableColumnAutoresizingMask|NSTableColumnUserResizingMask];
        
        [[aColumn headerCell] setStringValue:TEXTtoNS(colName)];
        
        /* Add to table */
        [RSTable addTableColumn:aColumn];
    }
    
    [mBuffer removeAllObjects];
    
    /*
     *  Print all the fields
     */
    totalRows = 0;
    while (totalRows < mMaxRows)
    {
        int sts = SQLFetch (hstmt);
        
        if (sts == SQL_NO_DATA_FOUND)
            break;
        
        if (sts != SQL_SUCCESS)
        {
            _nativeerrorbox (henv, hdbc, hstmt);
            break;
        }
        
        row = [NSMutableArray arrayWithCapacity:numCols];
        
        for (colNum = 1; colNum <= numCols; colNum++)
        {
            /*
             *  Fetch this column as character
             */
            sts = SQLGetData (hstmt, colNum, SQL_C_TCHAR, fetchBuffer,
                              sizeof(fetchBuffer), &colIndicator);
            if (sts != SQL_SUCCESS_WITH_INFO && sts != SQL_SUCCESS)
            {
                _nativeerrorbox (henv, hdbc, hstmt);
                goto endCursor;
            }
            
            /*
             *  Show NULL fields as ****
             */
            if (colIndicator == SQL_NULL_DATA)
                TEXTCPY(fetchBuffer, TEXT("<NULL>"));
            
            NSString *val = TEXTtoNS (fetchBuffer);
            if (val!=nil && ([val hasPrefix:@"http://"] || [val hasPrefix:@"https://"]))
                [row addObject:[NSAttributedString attributedStringWithBlueLink:val]];
            else
                [row addObject:val?val:@"??"];
        }
        [mBuffer addObject:row];
        totalRows++;
    }
    
    
    mRows = totalRows;
    [RSTable reloadData];
    if (mSPARQL_executed)
        [RSTable sizeToFit];
    return;
    
endCursor:
#if (ODBCVER < 0x0300)
    SQLFreeStmt (hstmt, SQL_CLOSE);
#else
    SQLCloseCursor (hstmt);
#endif
    return;
    
error:
    _nativeerrorbox (henv, hdbc, hstmt);
    return;
}

- (IBAction)aCloseConnection:(id)sender
{
    [self disconnect];
}


- (IBAction)aExecSQL:(id)sender
{
    ExecController *dlg = [[ExecController alloc] init];
    dlg.fSQL = _fQuery;
    dlg.MaxRows = mMaxRows;
    
    NSInteger rc = [NSApp runModalForWindow:dlg.window];
    self.fQuery = dlg.fSQL;
    mMaxRows = dlg.MaxRows;

    [dlg.window orderOut:mWindow];
    [dlg release];
    
    if (rc)
        [self execSQL:NStoTEXT(self.fQuery)];
}


- (IBAction)aFetchNextRowset:(id)sender
{
    if (mExistsResultset)
    {
        if (SQLMoreResults (hstmt) == SQL_SUCCESS)
        {
            [self loadResult];
        }
    }
}


- (IBAction)aOpenConnection:(id)sender
{
    SQLTCHAR szDSN[1024];
    SQLTCHAR dataSource[1024];
    SQLSMALLINT dsLen;
    SQLRETURN status;
    
#if (ODBCVER < 0x300)
    if (SQLAllocEnv (&henv) != SQL_SUCCESS)
#else
        if (SQLAllocHandle (SQL_HANDLE_ENV, NULL, &henv) != SQL_SUCCESS)
#endif
        {
            _nativeerrorbox (henv, SQL_NULL_HDBC, SQL_NULL_HSTMT);
            return;
        }
    
#if (ODBCVER < 0x300)
    if (SQLAllocConnect (henv, &hdbc) != SQL_SUCCESS)
#else
        SQLSetEnvAttr (henv, SQL_ATTR_ODBC_VERSION,
                       (SQLPOINTER) SQL_OV_ODBC3, SQL_IS_UINTEGER);
    if (SQLAllocHandle (SQL_HANDLE_DBC, henv, &hdbc) != SQL_SUCCESS)
#endif
    {
        _nativeerrorbox (henv, hdbc, SQL_NULL_HSTMT);
        [self disconnect];
        return;
    }
    
    status = SQLDriverConnect (hdbc, [mWindow windowRef], TEXT(""), SQL_NTS,
                               dataSource, sizeof (dataSource), &dsLen, SQL_DRIVER_COMPLETE);
    if (status != SQL_SUCCESS)
    {
        _nativeerrorbox (henv, hdbc, SQL_NULL_HSTMT);
        if (status != SQL_SUCCESS_WITH_INFO)
        {
            [self disconnect];
            return;
        }
    }
    
    mConnected = YES;
    SQLGetInfo (hdbc, SQL_DATA_SOURCE_NAME, szDSN, sizeof (szDSN), NULL);
    
#ifdef UNICODE
    [mWindow setTitle:[NSString stringWithFormat:@"iODBC Demo (Unicode) - Connected to [%@]", TEXTtoNS(szDSN)]];
#else
    [mWindow setTitle:[NSString stringWithFormat:@"iODBC Demo (Ansi) - Connected to [%@]", TEXTtoNS(szDSN)]];
#endif
    
#if (ODBCVER < 0x0300)
    if (SQLAllocStmt (hdbc, &hstmt) != SQL_SUCCESS)
#else
        if (SQLAllocHandle (SQL_HANDLE_STMT, hdbc, &hstmt) != SQL_SUCCESS)
#endif
        {
            _nativeerrorbox (henv, hdbc, hstmt);
            [self disconnect];
            return;
        }
    
    [RSTable reloadData];
}



// Show/hide menu items
- (BOOL)validateMenuItem:(NSMenuItem *)item
{
    int tag = [item tag];
    if (tag == OpenTag)
    {
        return (hdbc == nil ? YES : NO);
    }
    else if (tag == CloseTag)
    {
        return (hdbc == nil ? NO : YES);
    }
    else if (tag == ExecSQLTag)
    {
        return (hstmt != nil ? YES : NO);
    }
    else if (tag == NextRowsetTag)
    {
        return mNextResultset;
    }
    else
        return YES;
}


- (id)tableView:(NSTableView *)aTable objectValueForTableColumn:(NSTableColumn *)aColumn
            row:(int)rowIndex
{
    NSMutableArray *row;
    unsigned id;
    
    if (rowIndex < 0)
        return nil;
    
    id = [[aColumn identifier] intValue];
    row = [mBuffer objectAtIndex:rowIndex];
    return [row objectAtIndex:id];
}



- (int)numberOfRowsInTableView:(NSTableView *)aTable
{
    return mRows;
}


/**
 - (void)tableView:(NSTableView *)tableView willDisplayCell:(id)cell forTableColumn:(NSTableColumn *)tableColumn 
     row:(NSInteger)row
 {
   if ([cell isKindOfClass:[LinkTextFieldCell class]]) 
   {
      LinkTextFieldCell *linkCell = (LinkTextFieldCell *)cell;
       // Setup the work to be done when a link is clicked
      linkCell.clickHandler = ^(NSString *url, id sender) {
      };
    }
 }
 **/


@end
