#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>

#include "sql.h"
#include "sqlext.h"
#include "sqlucode.h"
#include "iodbcext.h"
#define TRUE 1
#define FALSE 0

/*
 *  Prototypes
 */
int ODBC_Connect (char *connStr);
int ODBC_Disconnect (void);
int ODBC_Errors (char *where, HSTMT stmt);
int ODBC_Test (void);

typedef unsigned short ucs2_t;
typedef unsigned int   ucs4_t;


#define MAXCOLS		32

# ifdef WIN32
#define OPL_A2W(a, w, cb)     \
	MultiByteToWideChar(CP_ACP, 0, a, -1, w, cb)
# else
#define OPL_A2W(XA, XW, SIZE)      mbstowcs(XW, XA, SIZE)
# endif /* WIN32 */

#ifdef WIDE_UCS2
#define NUMU2CHAR(X)	(sizeof (X) / sizeof (ucs2_t))
#else
#define NUMU2CHAR(X)	(sizeof (X) / sizeof (wchar_t))
#endif

#define EXEC(X) \
  if (!SQL_SUCCEEDED(X)) \
    { \
      ODBC_Errors (#X, stmt); \
      goto err; \
    }

/**  
#define EXEC(X) \
  sts=(X); \
  if (sts!= SQL_SUCCESS && sts!=SQL_SUCCESS_WITH_INFO) \
    { \
      ODBC_Errors (#X, stmt); \
      goto err; \
    }
**/

/*
 *  Global variables
 */
HENV henv = SQL_NULL_HANDLE;
HDBC hdbc = SQL_NULL_HANDLE;
HSTMT hstmt = SQL_NULL_HANDLE;

int connected = 0;


#define TMP_SIZE  8192
SQLWCHAR wbuf[TMP_SIZE];
ucs4_t u4buf[TMP_SIZE];
ucs2_t u2buf[TMP_SIZE];

#ifdef WIDE_UCS2
typedef ucs2_t  sqlwchar_t;
#define WCHAR_SIZE sizeof(ucs2_t)
ucs2_t outdsn[4096];			/* Store completed DSN for later use */
#else
typedef ucs4_t  sqlwchar_t;
#define WCHAR_SIZE sizeof(ucs4_t)
wchar_t outdsn[4096];			/* Store completed DSN for later use */
#endif


#ifdef WIDE_UCS2
static wchar_t*
strcpy_U2ToU4(ucs4_t *dest_u4, ucs2_t *src_u2)
{
  ucs4_t *ret = dest_u4;

  if (!dest_u4 || !src_u2)
    return (wchar_t*)dest_u4;

  while(*src_u2) {
      *dest_u4++ = *src_u2++;
  }
  *dest_u4 = 0;

  return (wchar_t*)ret;
}

static ucs2_t*
strcpy_U4ToU2(ucs2_t *dest_u2, wchar_t *src_u4)
{
  ucs2_t *ret = dest_u2;

  if (!dest_u2 || !src_u4)
    return dest_u2;

  while(*src_u4) {
      *dest_u2++ = *src_u4++;
  }
  *dest_u2 = 0;

  return ret;
}

static ucs2_t *
strcpy_A2U2 (ucs2_t * destStr, char *sourStr)
{
  size_t length;

  if (!sourStr || !destStr)
    return destStr;

  length = strlen (sourStr);
  if (length > 0) {
    length = (length>TMP_SIZE)?TMP_SIZE:length;
    OPL_A2W (sourStr, (wchar_t*)u4buf, length);
  }
  u4buf[length] = L'\0';

  strcpy_U4ToU2(destStr, (wchar_t*)u4buf);

  return destStr;
}

static int
ucs2len(ucs2_t *s)
{
  int len=0;
  while(*s++)
  {
    len++;
  }
}

#define _WCSCPY(X, Y)  strcpy_U2ToU4(X, Y)
#define _WCSLEN(X)     ucs2len(X)
#define _SQLWCHAR_CPY(X, Y)  strcpy_U4ToU2(X, Y)
#define _To_SQLWCHAR(X, Y)  strcpy_U4ToU2(X, Y)

#else

#define _WCSCPY(X, Y)  wcscpy(X, Y)
#define _WCSLEN(X)     wcslen(X)
#define _SQLWCHAR_CPY(X, Y)  wcscpy(X, Y)
#define _To_SQLWCHAR(X, Y)  (Y)

static SQLWCHAR *
strcpy_A2W (SQLWCHAR * destStr, char *sourStr)
{
  size_t length;

  if (!sourStr || !destStr)
    return destStr;

  length = strlen (sourStr);
  if (length > 0)
    OPL_A2W (sourStr, destStr, length);
  destStr[length] = L'\0';

  return destStr;
}
#endif


/*
 *  Connect to the datasource
 *
 *  The connect string can have the following parts and they refer to
 *  the values in the odbc.ini file
 *
 *	DSN=<data source name>		[mandatory]
 *	HOST=<server host name>		[optional - value of Host]
 *	SVT=<database server type>	[optional - value of ServerType]
 *	DATABASE=<database path>	[optional - value of Database]
 *	OPTIONS=<db specific opts>	[optional - value of Options]
 *	UID=<user name>			[optional - value of LastUser]
 *	PWD=<password>			[optional]
 *	READONLY=<N|Y>			[optional - value of ReadOnly]
 *	FBS=<fetch buffer size>		[optional - value of FetchBufferSize]
 *
 *   Examples:
 *
 *	HOST=star;SVT=SQLServer 2000;UID=demo;PWD=demo;DATABASE=pubs
 *
 *	DSN=pubs_sqlserver;PWD=demo
 */


int
ODBC_Connect (char *connStr)
{
  short buflen;
  SQLCHAR dataSource[1024];
  sqlwchar_t dsn[33];
  sqlwchar_t desc[255];
  sqlwchar_t driverInfo[255];
  sqlwchar_t wdataSource[1024];
  SQLSMALLINT len1, len2;
  int status;

#if (ODBCVER < 0x0300)
  if (SQLAllocEnv (&henv) != SQL_SUCCESS)
    return -1;

  if (SQLAllocConnect (henv, &hdbc) != SQL_SUCCESS)
    return -1;
#else
  if (SQLAllocHandle (SQL_HANDLE_ENV, NULL, &henv) != SQL_SUCCESS)
    return -1;

  SQLSetEnvAttr (henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER) SQL_OV_ODBC3,
      SQL_IS_UINTEGER);
      
#ifdef WIDE_UCS2
  SQLSetEnvAttr (henv, SQL_ATTR_APP_UNICODE_TYPE, (SQLPOINTER) SQL_DM_CP_UTF16,
      SQL_IS_UINTEGER);
#else      
  SQLSetEnvAttr (henv, SQL_ATTR_APP_UNICODE_TYPE, (SQLPOINTER) SQL_DM_CP_UCS4,
      SQL_IS_UINTEGER);
#endif      

  if (SQLAllocHandle (SQL_HANDLE_DBC, henv, &hdbc) != SQL_SUCCESS)
    return -1;
#endif


  /*
   *  Show the version number of the driver manager
   */
  status = SQLGetInfoW (hdbc, SQL_DM_VER,
      driverInfo, sizeof (driverInfo), &len1);
  if (status == SQL_SUCCESS)
    {
      printf ("Driver Manager: %S\n", _WCSCPY(u4buf, driverInfo));
    }


  /*
   *  Either use the connect string provided on the command line or
   *  ask for one. If an empty string or a ? is given, show a nice
   *  list of options
   */
  if (connStr && *connStr)
    strcpy ((char *) dataSource, connStr);
  else
    while (1)
      {
	/*
	 *  Ask for the connect string
	 */
	printf ("\nEnter ODBC connect string (? shows list): ");
	if (fgets ((char *) dataSource, sizeof (dataSource), stdin) == NULL)
	  return 1;

	/*
	 *  Remove trailing '\n'
	 */
	dataSource[strlen ((char *) dataSource) - 1] = '\0';

	/*
	 * Check if the user wants to quit
	 */
	if (!strcmp ((char *)dataSource, "quit") || !strcmp ((char *)dataSource, "exit"))
	  return -1;

	/*
	 *  If the user entered something other than a ?
	 *  break out of the while loop
	 */
	if (*dataSource && *dataSource != '?')
	  break;


	/*
	 *  Print headers
	 */
	fprintf (stderr, "\n%-32s | %-40s\n", "DSN", "Driver");
	fprintf (stderr,
	    "------------------------------------------------------------------------------\n");

	/*
	 *  Goto the first record
	 */
	if (SQLDataSourcesW (henv, SQL_FETCH_FIRST,
		(SQLWCHAR*)dsn, NUMU2CHAR(dsn), &len1,
		(SQLWCHAR*)desc, NUMU2CHAR(desc), &len2) != SQL_SUCCESS)
	  continue;

	/*
	 *  Show all records
	 */
	do
	  {
	    ucs4_t wdsn[256], wdesc[256];
	    fprintf (stderr, "%-32S | %-40S\n", 
	    	_WCSCPY(wdsn,dsn), 
	    	_WCSCPY(wdesc,desc));
	  }
	while (SQLDataSourcesW (henv, SQL_FETCH_NEXT,
		(SQLWCHAR*)dsn, NUMU2CHAR(dsn), &len1,
		(SQLWCHAR*)desc, NUMU2CHAR(desc), &len2) == SQL_SUCCESS);
      }

#if WIDE_UCS2
  strcpy_A2U2 (wdataSource, (char *) dataSource);
#else
  strcpy_A2W (wdataSource, (char *) dataSource);
#endif
  status = SQLDriverConnectW (hdbc, 0, (SQLWCHAR *) wdataSource, SQL_NTS,
      (SQLWCHAR *) outdsn, NUMU2CHAR (outdsn), &buflen, SQL_DRIVER_NOPROMPT);
/*      (SQLWCHAR *) outdsn, NUMU2CHAR (outdsn), &buflen, SQL_DRIVER_COMPLETE);*/
  if (status != SQL_SUCCESS && status != SQL_SUCCESS_WITH_INFO)
    ODBC_Errors ("SQLDriverConnectW", SQL_NULL_HANDLE);

  if (status != SQL_SUCCESS && status != SQL_SUCCESS_WITH_INFO)
    return -1;

  connected = 1;


  /*
   *  Print out the version number and the name of the connected driver
   */
  status = SQLGetInfoW (hdbc, SQL_DRIVER_VER,
      driverInfo, NUMU2CHAR (driverInfo), &len1);
  if (status == SQL_SUCCESS)
    {
      printf ("Driver: %S", _WCSCPY(u4buf, driverInfo));

      status = SQLGetInfoW (hdbc, SQL_DRIVER_NAME,
	  driverInfo, NUMU2CHAR (driverInfo), &len1);
      if (status == SQL_SUCCESS)
	{
	  printf (" (%S)", _WCSCPY(u4buf,driverInfo));
	}
      printf ("\n");
    }


  /*
   *  Show the list of supported functions in trace log
   */
#if (ODBCVER < 0x0300)
  {
     SQLUSMALLINT exists[100];

     SQLGetFunctions (hdbc, SQL_API_ALL_FUNCTIONS, exists);
  }
#else
  {
     SQLUSMALLINT exists[SQL_API_ODBC3_ALL_FUNCTIONS_SIZE];

     SQLGetFunctions (hdbc, SQL_API_ODBC3_ALL_FUNCTIONS, exists);
  }
#endif



  /*
   *  Allocate statement handle
   */
#if (ODBCVER < 0x0300)
  if (SQLAllocStmt (hdbc, &hstmt) != SQL_SUCCESS)
    return -1;
#else
  if (SQLAllocHandle (SQL_HANDLE_STMT, hdbc, &hstmt) != SQL_SUCCESS)
    return -1;
#endif

  return 0;
}


/*
 *  Disconnect from the database
 */
int
ODBC_Disconnect (void)
{
#if (ODBCVER < 0x0300)
  if (hstmt)
    SQLFreeStmt (hstmt, SQL_DROP);

  if (connected)
    SQLDisconnect (hdbc);

  if (hdbc)
    SQLFreeConnect (hdbc);

  if (henv)
    SQLFreeEnv (henv);
#else
  if (hstmt)
    {
      SQLCloseCursor (hstmt);
      SQLFreeHandle (SQL_HANDLE_STMT, hstmt);
    }

  if (connected)
    SQLDisconnect (hdbc);

  if (hdbc)
    SQLFreeHandle (SQL_HANDLE_DBC, hdbc);

  if (henv)
    SQLFreeHandle (SQL_HANDLE_ENV, henv);
#endif

  return 0;
}


/*
 *  Perform a disconnect/reconnect using the DSN stored from the original
 *  SQLDriverConnect
 */
int 
ODBC_Reconnect (void)
{
  SQLRETURN status;
  ucs2_t buf[4096];
  SQLSMALLINT len;

  /*
   *  Free old statement handle
   */
#if (ODBCVER < 0x0300)
  SQLFreeStmt (hstmt, SQL_DROP);
#else
  SQLFreeHandle (SQL_HANDLE_STMT, hstmt);
#endif

  /*
   *  Disconnect
   */
  SQLDisconnect (hdbc);

  /*
   *  Reconnect
   */
  status = SQLDriverConnectW (hdbc, 0, (SQLWCHAR*)outdsn, SQL_NTS,
      (SQLWCHAR*)buf, sizeof (buf), &len, SQL_DRIVER_NOPROMPT);

  /*
   *  Allocate new statement handle
   */
  if (SQL_SUCCEEDED (status))
    {
#if (ODBCVER < 0x0300)
      status = SQLAllocStmt (hdbc, &hstmt);
#else
      status = SQLAllocHandle (SQL_HANDLE_STMT, hdbc, &hstmt);
#endif
    }

  /*
   *  Show why we where unsuccessful and return an error
   */
  if (!SQL_SUCCEEDED (status))
    {
      ODBC_Errors ("DriverConnect (reconnect)", SQL_NULL_HANDLE);
      return -1;
    }

  /*
   *  Success
   */
  return 0;
}


/*
 *  Show all the error information that is available
 */
int
ODBC_Errors (char *where, HSTMT stmt)
{
  ucs4_t u4tmp[15];
  sqlwchar_t buf[512];
  sqlwchar_t sqlstate[15];
  SQLINTEGER native_error = 0;
  int force_exit = 0;
  SQLRETURN sts;

  if (stmt == SQL_NULL_HANDLE)
    stmt = hstmt;

fprintf(stderr, "Error [%s]\n", where);
#if (ODBCVER < 0x0300)
  /*
   *  Get statement errors
   */
  while (stmt)
    {
      sts = SQLErrorW (henv, hdbc, stmt, sqlstate, &native_error,
	  buf, NUMU2CHAR (buf), NULL);
      if (!SQL_SUCCEEDED (sts))
	break;

      fprintf (stderr, "E:%s = %S (%ld) SQLSTATE=%S\n",
	  where, _WCSCPY(u4buf,buf), (long) native_error, _WCSCPY(u4tmp,sqlstate));

      /*
       *  If the driver could not be loaded, there is no point in
       *  continuing, after reading all the error messages
       */
      if (!wcscmp ((wchar_t*)_WCSCPY(u4buf,sqlstate), L"IM003"))
	force_exit = 1;
    }

  /*
   *  Get connection errors
   */
  while (hdbc)
    {
      sts = SQLErrorW (henv, hdbc, SQL_NULL_HSTMT, sqlstate, &native_error,
	  buf, NUMU2CHAR (buf), NULL);
      if (!SQL_SUCCEEDED (sts))
	break;

      fprintf (stderr, "E:%s = %S (%ld) SQLSTATE=%S\n",
	  where, _WCSCPY(u4buf,buf), (long) native_error, _WCSCPY(u4tmp,sqlstate));

      /*
       *  If the driver could not be loaded, there is no point in
       *  continuing, after reading all the error messages
       */
      if (!wcscmp ((wchar_t*)_WCSCPY(u4buf,sqlstate), L"IM003"))
	force_exit = 1;
    }

  /*
   *  Get environment errors
   */
  while (henv)
    {
      sts = SQLErrorW (henv, SQL_NULL_HDBC, SQL_NULL_HSTMT, sqlstate,
	  &native_error, buf, NUMU2CHAR (buf), NULL);
      if (!SQL_SUCCEEDED (sts))
	break;

      fprintf (stderr, "E:%s = %S (%ld) SQLSTATE=%S\n",
	  where, _WCSCPY(u4buf,buf), (long) native_error, _WCSCPY(u4tmp,sqlstate));

      /*
       *  If the driver could not be loaded, there is no point in
       *  continuing, after reading all the error messages
       */
      if (!wcscmp ((wchar_t*)_WCSCPY(u4buf,sqlstate), L"IM003"))
	force_exit = 1;
    }
#else /* ODBCVER */
  int i;

  /*
   *  Get statement errors
   */
  i = 0;
  while (stmt && i < 5)
    {
      sts = SQLGetDiagRecW (SQL_HANDLE_STMT, stmt, ++i,
	  (SQLWCHAR*)sqlstate, &native_error, (SQLWCHAR*)buf, NUMU2CHAR (buf), NULL);
      if (!SQL_SUCCEEDED (sts))
	break;

      fprintf (stderr, "%d: %s = %S (%ld) SQLSTATE=%S\n",
	  i, where, _WCSCPY(u4buf,buf), (long) native_error, _WCSCPY(u4tmp,sqlstate));

      /*
       *  If the driver could not be loaded, there is no point in
       *  continuing, after reading all the error messages
       */
      if (!wcscmp ((wchar_t*)_WCSCPY(u4buf,sqlstate), L"IM003"))
	force_exit = 1;
    }

  /*
   *  Get connection errors
   */
  i = 0;
  while (hdbc && i < 5)
    {
      sts = SQLGetDiagRecW (SQL_HANDLE_DBC, hdbc, ++i,
	  (SQLWCHAR*)sqlstate, &native_error, (SQLWCHAR*)buf, NUMU2CHAR (buf), NULL);
      if (!SQL_SUCCEEDED (sts))
	break;

      fprintf (stderr, "%d: %s = %S (%ld) SQLSTATE=%S\n",
	  i, where, _WCSCPY(u4buf,buf), (long) native_error, _WCSCPY(u4tmp,sqlstate));
      /*
       *  If the driver could not be loaded, there is no point in
       *  continuing, after reading all the error messages
       */
      if (!wcscmp ((wchar_t*)_WCSCPY(u4buf,sqlstate), L"IM003"))
	force_exit = 1;
    }

  /*
   *  Get environment errors
   */
  i = 0;
  while (henv && i < 5)
    {
      sts = SQLGetDiagRecW (SQL_HANDLE_ENV, henv, ++i,
	  (SQLWCHAR*)sqlstate, &native_error, (SQLWCHAR*)buf, NUMU2CHAR (buf), NULL);
      if (!SQL_SUCCEEDED (sts))
	break;

      fprintf (stderr, "%d: %s = %S (%ld) SQLSTATE=%S\n",
	  i, where, _WCSCPY(u4buf,buf), (long) native_error, _WCSCPY(u4tmp,sqlstate));
      /*
       *  If the driver could not be loaded, there is no point in
       *  continuing, after reading all the error messages
       */
      if (!wcscmp ((wchar_t*)_WCSCPY(u4buf,sqlstate), L"IM003"))
	force_exit = 1;
    }
#endif /* ODBCVER */

  /*
   *  Force an exit status
   */
  if (force_exit)
    exit (-1);

  return -1;
}


/*
 *  Test program to run on the connected database
 */
int
ODBC_Test ()
{
  sqlwchar_t request[4096];
  sqlwchar_t fetchBuffer[1024];
  char buf[4096];
  size_t displayWidths[MAXCOLS];
  size_t displayWidth;
  short numCols;
  short colNum;
  sqlwchar_t colName[50];
  SQLSMALLINT colType;
  SQLULEN colPrecision;
  SQLLEN colIndicator;
  SQLSMALLINT colScale;
  SQLSMALLINT colNullable;
  unsigned long totalRows;
  unsigned long totalSets;
  int i;
  SQLRETURN sts;
#ifdef WIDE_UCS2
  ucs2_t u2empty[2];
  ucs2_t u2percent[2];

  strcpy_U4ToU2(u2empty, L"");
  strcpy_U4ToU2(u2percent, L"%");
#endif

  while (1)
    {
      /*
       *  Ask the user for a dynamic SQL statement
       */
#ifdef VIRTUOSO
      printf("tests: insert1, insert2, proc1, proc2, fetch1, fetch2, blob, getblob, setpos");
#else
      printf("tests: insert1, insert2, proc1, proc2, fetch1, fetch2, \n blob, getblob, setpos, bulkop, insert_date1, insert_date2");
#endif
      printf ("\nSQL>");
      if (fgets (buf, sizeof (buf), stdin) == NULL)
	break;

      buf[strlen(buf) - 1] = 0;

#ifdef WIDE_UCS2
      strcpy_A2U2 (request, buf);
#else
      strcpy_A2W (request, buf);
#endif

      if (request[0] == 0)
	continue;

      /*
       *  If the user just types tables, give him a list
       */
      if (!strcmp (buf, "insert1"))
	{
	  if (Test_Insert1()!=0)
	    printf("Error in Test_Insert1\n");
          continue;
	}
      else if (!strcmp (buf, "insert2"))
	{
	  if (Test_Insert2()!=0)
	    printf("Error in Test_Insert2\n");
          continue;
	}
      else if (!strcmp (buf, "proc1"))
	{
	  if (Test_Proc1()!=0)
	    printf("Error in Test_Proc1\n");
          continue;
	}
      else if (!strcmp (buf, "proc2"))
	{
	  if (Test_Proc2()!=0)
	    printf("Error in Test_Proc2\n");
	  continue;
	}
      else if (!strcmp (buf, "fetch1"))
	{
	  if (ODBC_TestF1()!=0)
	    printf("Error in TestF1\n");
	  continue;
	}
      else if (!strcmp (buf, "fetch2"))
	{
	  if (ODBC_TestF2()!=0)
	    printf("Error in TestF2\n");
	  continue;
	}
      else if (!strcmp (buf, "blob"))
	{
	  if (Test_Blob1()!=0)
	    printf("Error in Test_Blob1\n");
	  continue;
	}
      else if (!strcmp (buf, "getblob"))
	{
	  if (Test_GetBlob1()!=0)
	    printf("Error in Test_GetBlob1\n");
	  continue;
	}
      else if (!strcmp (buf, "setpos"))
	{
	  if (Test_SetPos()!=0)
	    printf("Error in Test_SetPos\n");
	  continue;
	}
#ifndef VIRTUOSO
      else if (!strcmp (buf, "bulkop"))
	{
	  if (Test_BulkOp()!=0)
	    printf("Error in Test_BulkOp\n");
	  continue;
	}
      else if (!strcmp (buf, "insert_date1"))
	{
	  if (Test_Insert_Date1()!=0)
	    printf("Error in Test_Insert_Date1\n");
	  continue;
	}
      else if (!strcmp (buf, "insert_date2"))
	{
	  if (Test_Insert_Date2()!=0)
	    printf("Error in Test_Insert_Date2\n");
	  continue;
	}
#endif
      
      else if (!strcmp (buf, "tables"))
	{
	  if (SQLTablesW (hstmt, NULL, 0, NULL, 0, NULL, 0,
		  NULL, 0) != SQL_SUCCESS)
	    {
	      ODBC_Errors ("SQLTables(tables)", SQL_NULL_HANDLE);
	      continue;
	    }
	}
      /*
       *  If the user just types qualifiers, give him a list
       */
      else if (!strcmp (buf, "qualifiers"))
	{
#ifdef WIDE_UCS2
	  if (SQLTablesW (hstmt, 
	          (SQLWCHAR*)u2percent, SQL_NTS, 
	          (SQLWCHAR*)u2empty, 0,
		  (SQLWCHAR*)u2empty, 0, 
		  (SQLWCHAR*)u2empty, 0) != SQL_SUCCESS)
#else
	  if (SQLTablesW (hstmt, 
	          (SQLWCHAR*)L"%", SQL_NTS, 
	          (SQLWCHAR*)L"", 0,
		  (SQLWCHAR*)L"", 0, 
		  (SQLWCHAR*)L"", 0) != SQL_SUCCESS)
#endif
	    {
	      ODBC_Errors ("SQLTables(qualifiers)", SQL_NULL_HANDLE);
	      continue;
	    }
	}
      /*
       *  If the user just types owners, give him a list
       */
      else if (!strcmp (buf, "owners"))
	{
#ifdef WIDE_UCS2
	  if (SQLTables (hstmt, 
	          (SQLWCHAR*)u2empty, 0, 
	          (SQLWCHAR*)u2percent, SQL_NTS,
		  (SQLWCHAR*)u2empty, 0, 
		  (SQLWCHAR*)u2empty, 0) != SQL_SUCCESS)
#else
	  if (SQLTables (hstmt, 
	          (SQLWCHAR*)L"", 0, 
	          (SQLWCHAR*)L"%", SQL_NTS,
		  (SQLWCHAR*)L"", 0, 
		  (SQLWCHAR*)L"", 0) != SQL_SUCCESS)
#endif
	    {
	      ODBC_Errors ("SQLTables(owners)", SQL_NULL_HANDLE);
	      continue;
	    }
	}
      /*
       *  If the user just types "types", give him a list
       */
      else if (!strcmp (buf, "types"))
	{
#ifdef WIDE_UCS2
	  if (SQLTables (hstmt, 
	          (SQLWCHAR*)u2empty, 0, 
	          (SQLWCHAR*)u2empty, 0,
		  (SQLWCHAR*)u2empty, 0, 
		  (SQLWCHAR*)u2percent, SQL_NTS) != SQL_SUCCESS)
#else
	  if (SQLTables (hstmt, 
	          (SQLWCHAR*)L"", 0, 
	          (SQLWCHAR*)L"", 0,
		  (SQLWCHAR*)L"", 0, 
		  (SQLWCHAR*)L"%", SQL_NTS) != SQL_SUCCESS)
#endif
	    {
	      ODBC_Errors ("SQLTables(types)", SQL_NULL_HANDLE);
	      continue;
	    }
	}
      /*
       *  If the user just types "datatypes", give him a list
       */
      else if (!strcmp (buf, "datatypes"))
	{
	  if (SQLGetTypeInfo (hstmt, 0) != SQL_SUCCESS)
	    {
	      ODBC_Errors ("SQLGetTypeInfo", SQL_NULL_HANDLE);
	      continue;
	    }
	}
      else if (!strcmp (buf, "reconnect"))
	{
  	  if (ODBC_Reconnect())
	    return -1;

	  continue;
	}
#if defined (unix)
      else if (!strcmp (buf, "environment"))
	{
	  extern char **environ;
	  int i;

	  for (i = 0; environ[i]; i++)
	    fprintf (stderr, "%03d: [%s]\n", i, environ[i]);

	  continue;
	}
#endif
      else if (!strcmp (buf, "quit")
	  || !strcmp (buf, "exit"))
	break;			/* If you want to quit, just say so */
      else
	{
	  /*
	   *  Prepare & Execute the statement
	   */
	  if (SQLPrepareW (hstmt, (SQLWCHAR *) request,
		  SQL_NTS) != SQL_SUCCESS)
	    {
	      ODBC_Errors ("SQLPrepare", hstmt);
	      continue;
	    }
	  if ((sts = SQLExecute (hstmt)) != SQL_SUCCESS)
	    {
	      ODBC_Errors ("SQLExec", hstmt);

	      if (sts != SQL_SUCCESS_WITH_INFO)
		continue;
	    }
	}

      /*
       *  Loop through all the result sets
       */
      totalSets = 1;
      do
	{
	  /*
	   *  Get the number of result columns for this cursor.
	   *  If it is 0, then the statement was probably a select
	   */
	  if (SQLNumResultCols (hstmt, &numCols) != SQL_SUCCESS)
	    {
	      ODBC_Errors ("SQLNumResultCols", SQL_NULL_HANDLE);
	      goto endCursor;
	    }
	  if (numCols == 0)
	    {
	      SQLLEN nrows = 0;

	      SQLRowCount (hstmt, &nrows);
	      printf ("Statement executed. %ld rows affected.\n",
		  nrows > 0 ? (long) nrows : 0L);
	      goto endCursor;
	    }

	  if (numCols > MAXCOLS)
	    {
	      numCols = MAXCOLS;
	      fprintf (stderr,
		  "NOTE: Resultset truncated to %d columns.\n", MAXCOLS);
	    }

	  /*
	   *  Get the names for the columns
	   */
	  putchar ('\n');
	  for (colNum = 1; colNum <= numCols; colNum++)
	    {
	      /*
	       *  Get the name and other type information
	       */
	      if (SQLDescribeColW (hstmt, colNum,
		      (SQLWCHAR *) colName, NUMU2CHAR (colName), NULL,
		      &colType, &colPrecision, &colScale,
		      &colNullable) != SQL_SUCCESS)
		{
		  ODBC_Errors ("SQLDescribeCol", SQL_NULL_HANDLE);
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
		  displayWidth = 30;	/* show only first 30 */
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
		  displayWidths[colNum - 1] = 0;	/* skip other data types */
		  continue;
		}

	      if (displayWidth < _WCSLEN (colName))
		displayWidth = _WCSLEN (colName);
	      if (displayWidth > NUMU2CHAR (fetchBuffer) - 1)
		displayWidth = NUMU2CHAR (fetchBuffer) - 1;

	      displayWidths[colNum - 1] = displayWidth;

	      /*
	       *  Print header field
	       */
	      printf ("%-*.*S", displayWidth, displayWidth, _WCSCPY(u4buf,colName));
	      if (colNum < numCols)
		putchar ('|');
	    }
	  putchar ('\n');

	  /*
	   *  Print second line
	   */
	  for (colNum = 1; colNum <= numCols; colNum++)
	    {
	      for (i = 0; i < displayWidths[colNum - 1]; i++)
		putchar ('-');
	      if (colNum < numCols)
		putchar ('+');
	    }
	  putchar ('\n');

	  /*
	   *  Print all the fields
	   */
	  totalRows = 0;
	  while (1)
	    {
#if (ODBCVER < 0x0300)
	      int sts = SQLFetch (hstmt);
#else
	      int sts = SQLFetchScroll (hstmt, SQL_FETCH_NEXT, 1);
#endif

	      if (sts == SQL_NO_DATA_FOUND)
		break;

	      if (sts != SQL_SUCCESS)
		{
		  ODBC_Errors ("Fetch", SQL_NULL_HANDLE);
		  break;
		}
	      for (colNum = 1; colNum <= numCols; colNum++)
		{
		  /*
		   *  Fetch this column as character
		   */
		  sts = SQLGetData (hstmt, colNum, SQL_C_WCHAR, fetchBuffer,
		      NUMU2CHAR (fetchBuffer), &colIndicator);
		  if (sts != SQL_SUCCESS_WITH_INFO && sts != SQL_SUCCESS)
		    {
		      ODBC_Errors ("SQLGetData", SQL_NULL_HANDLE);
		      goto endCursor;
		    }

		  /*
		   *  Show NULL fields as ****
		   */
		  if (colIndicator == SQL_NULL_DATA)
		    {
		      for (i = 0; i < displayWidths[colNum - 1]; i++)
			fetchBuffer[i] = L'*';
		      fetchBuffer[i] = L'\0';
		    }

		  printf ("%-*.*S", displayWidths[colNum - 1],
		      displayWidths[colNum - 1], _WCSCPY(u4buf,fetchBuffer));
		  if (colNum < numCols)
		    putchar ('|');
		}
	      putchar ('\n');
	      totalRows++;
	    }

	  printf ("\n result set %lu returned %lu rows.\n\n",
	      totalSets, totalRows);
	  totalSets++;
	}
      while ((sts = SQLMoreResults (hstmt)) == SQL_SUCCESS);

      if (sts == SQL_ERROR)
	ODBC_Errors ("SQLMoreResults", SQL_NULL_HANDLE);

    endCursor:
#if (ODBCVER < 0x0300)
      SQLFreeStmt (hstmt, SQL_CLOSE);
#else
      SQLCloseCursor (hstmt);
#endif
    }

  return 0;
}

int
ExecSelect(wchar_t *query)
{
  SQLRETURN sts;
  HSTMT stmt = SQL_NULL_HANDLE;
  int ret = -1;

  EXEC(SQLAllocHandle (SQL_HANDLE_STMT, hdbc, &stmt));
  EXEC(SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf, query), SQL_NTS));

  ODBC_Results(stmt);
  ret = 0;

err:
  if (stmt!=SQL_NULL_HANDLE)
    SQLFreeHandle (SQL_HANDLE_STMT, stmt);
  return ret;
}


#define ARRAY_SIZE 3
#define F1_SIZE  30

int
Test_Insert1()
{
  sqlwchar_t f1Arr[ARRAY_SIZE][F1_SIZE];
  SQLINTEGER idArr[ARRAY_SIZE];
  SQLFLOAT f2Arr[ARRAY_SIZE];
  char f3Arr[ARRAY_SIZE][F1_SIZE];
  SQLLEN idIndArr[ARRAY_SIZE], f1IndArr[ARRAY_SIZE], f2IndArr[ARRAY_SIZE], f3IndArr[ARRAY_SIZE];
  SQLUSMALLINT ParamsProcessed, ParamStatusArray[ARRAY_SIZE];
  int i;
  SQLRETURN sts;
  SQLRETURN status;
  HSTMT stmt = SQL_NULL_HANDLE;
  int ret = -1;


  for(i=0; i < ARRAY_SIZE; i++) {
    idIndArr[i]=0;
    f1IndArr[i]=SQL_NTS;
    f2IndArr[i]=0;
    f3IndArr[i]=SQL_NTS;
  }

  idArr[0]=1;
  idArr[1]=2;
  idArr[2]=3;

  _SQLWCHAR_CPY(f1Arr[0],L"test1");
  _SQLWCHAR_CPY(f1Arr[1],L"test2");
  _SQLWCHAR_CPY(f1Arr[2],L"test3");

  f2Arr[0] = 1234.12;
  f2Arr[1] = 2345.12;
  f2Arr[2] = 3456.12;

  strcpy(f3Arr[0],"atest1");
  strcpy(f3Arr[1],"atest2");
  strcpy(f3Arr[2],"atest3");


  EXEC(SQLAllocHandle (SQL_HANDLE_STMT, hdbc, &stmt));

  SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"drop table test_ins"), SQL_NTS);
  printf("create table test_ins\n");

  EXEC(SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"create table test_ins(id integer, f1 nvarchar(30), f2 float, f3 varchar(30))"), SQL_NTS));

  EXEC(SQLFreeStmt (stmt, SQL_RESET_PARAMS));

printf("use bind_by_column\n");
  EXEC(SQLSetStmtAttr(stmt, SQL_ATTR_PARAM_BIND_TYPE, SQL_BIND_BY_COLUMN, 0));

  EXEC(SQLSetStmtAttr(stmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)ARRAY_SIZE, 0));


printf("bind params\n");
  EXEC(SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 
  		0, 0, idArr, 0, idIndArr));
  EXEC(SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WCHAR, 
  		F1_SIZE - 1, 0, f1Arr, F1_SIZE*WCHAR_SIZE, f1IndArr));
  EXEC(SQLBindParameter(stmt, 3, SQL_PARAM_INPUT, SQL_C_DOUBLE, SQL_FLOAT, 
  		0, 0, f2Arr, 0, f2IndArr));
  EXEC(SQLBindParameter(stmt, 4, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 
  		F1_SIZE - 1, 0, f3Arr, F1_SIZE, f3IndArr));
printf("insert 3rows\n");

  EXEC(SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"insert into test_ins values(?,?,?,?)"), SQL_NTS));

  SQLFreeStmt (stmt, SQL_RESET_PARAMS);
  SQLFreeStmt (stmt, SQL_UNBIND);

printf("select * from test_ins\n");
  ExecSelect(L"select * from test_ins");
  ret = 0;

err:
  if (stmt!=SQL_NULL_HANDLE)
    SQLFreeHandle (SQL_HANDLE_STMT, stmt);
  return ret;
}




typedef struct _ParStruct {
  SQLINTEGER id;
  SQLLEN     idInd;
  sqlwchar_t     f1[F1_SIZE];
  SQLLEN     f1Ind;
  SQLFLOAT   f2;
  SQLLEN     f2Ind;
  char       f3[F1_SIZE];
  SQLLEN     f3Ind;
} ParStruct;


int
Test_Insert2()
{
  ParStruct params[ARRAY_SIZE];
  SQLUINTEGER ParamsProcessed;
  SQLUSMALLINT ParamStatusArray[ARRAY_SIZE];

  int i;
  SQLRETURN sts;
  SQLRETURN status;
  HSTMT stmt = SQL_NULL_HANDLE;
  int ret = -1;


  params[0].id=1;
  params[0].idInd=0;
  _SQLWCHAR_CPY(params[0].f1, L"test01");
  params[0].f1Ind=SQL_NTS;
  params[0].f2=1234.12;
  params[0].f2Ind=0;
  strcpy(params[0].f3, "atest01");
  params[0].f3Ind=SQL_NTS;

  params[1].id=2;
  params[1].idInd=0;
  _SQLWCHAR_CPY(params[1].f1, L"test02");
  params[1].f1Ind=SQL_NTS;
  params[1].f2=2345.12;
  params[1].f2Ind=0;
  strcpy(params[1].f3, "atest02");
  params[1].f3Ind=SQL_NTS;

  params[2].id=3;
  params[2].idInd=0;
  _SQLWCHAR_CPY(params[2].f1, L"test03");
  params[2].f1Ind=SQL_NTS;
  params[2].f2=3456.12;
  params[2].f2Ind=0;
  strcpy(params[2].f3, "atest03");
  params[2].f3Ind=SQL_NTS;


  EXEC(SQLAllocHandle (SQL_HANDLE_STMT, hdbc, &stmt)); 

  SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"drop table test_ins"), SQL_NTS);
printf("create table test_ins\n");

  EXEC(SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"create table test_ins(id integer, f1 nvarchar(30), f2 float, f3 varchar(30))"), SQL_NTS));

  EXEC(SQLFreeStmt (stmt, SQL_RESET_PARAMS));

printf("use bind_by_row\n");
  EXEC(SQLSetStmtAttr(stmt, SQL_ATTR_PARAM_BIND_TYPE, (SQLPOINTER)sizeof(ParStruct), 0));

  EXEC(SQLSetStmtAttr(stmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)ARRAY_SIZE, 0));

printf("bind params\n");
  EXEC(SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 
  		0, 0, &params[0].id, 0, &params[0].idInd));
  EXEC(SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WCHAR, 
  		F1_SIZE - 1, 0, params[0].f1, F1_SIZE*WCHAR_SIZE, &params[0].f1Ind));
  EXEC(SQLBindParameter(stmt, 3, SQL_PARAM_INPUT, SQL_C_DOUBLE, SQL_FLOAT, 
  		0, 0, &params[0].f2, 0, &params[0].f2Ind));
  EXEC(SQLBindParameter(stmt, 4, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 
  		F1_SIZE - 1, 0, params[0].f3, F1_SIZE, &params[0].f3Ind));

printf("insert 3rows\n");
  EXEC(SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"insert into test_ins values(?,?,?,?)"), SQL_NTS));

  SQLFreeStmt (stmt, SQL_RESET_PARAMS);
  SQLFreeStmt (stmt, SQL_UNBIND);

printf("select * from test_ins\n");
  ExecSelect(L"select * from test_ins");
  ret = 0;

err:
  if (stmt!=SQL_NULL_HANDLE)
    SQLFreeHandle (SQL_HANDLE_STMT, stmt);
  return ret;
}




int
Test_Proc1()
{
  SQLINTEGER id;
  sqlwchar_t f1[F1_SIZE];
  sqlwchar_t f2[F1_SIZE];
  SQLLEN idInd, f1Ind, f2Ind;
  int i;
  SQLRETURN sts;
  SQLRETURN status;
#ifdef VIRTUOSO
  wchar_t *create_cmd = L"create procedure test_params "
	"(in p1 integer, in p2 nvarchar(10) , out p3  nvarchar(10) )\n"
	"{  p3 := cast(p1 as varchar) || p2; } \n";
#else
  wchar_t *create_cmd = L"create procedure test_params "
	"@p1  int, \n"
	"@p2  nvarchar(10) , \n"
	"@p3  nvarchar(10) output \n"
	"as \n"
	"begin \n"
	"select @p3 = convert(varchar(4),@p1)+@p2 \n"
	"end \n";
#endif
  HSTMT stmt = SQL_NULL_HANDLE;
  int ret = -1;

  idInd=0;
  f1Ind=SQL_NTS;
  f2Ind=SQL_NTS;

  id=1;

  _SQLWCHAR_CPY(f1, L"test1");
  _SQLWCHAR_CPY(f2, L"t");


  EXEC(SQLAllocHandle (SQL_HANDLE_STMT, hdbc, &stmt));

printf("drop procedure test_params\n");
  SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"drop procedure test_params"), SQL_NTS);
printf("create procedure test_params\n");

  EXEC(SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,create_cmd), SQL_NTS));

  EXEC(SQLFreeStmt (stmt, SQL_RESET_PARAMS));

printf("use bind_by_column\n");
  EXEC(SQLSetStmtAttr(stmt, SQL_ATTR_PARAM_BIND_TYPE, SQL_BIND_BY_COLUMN, 0));

printf("bind params\n");
  EXEC(SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 
  		0, 0, &id, 0, &idInd));
  EXEC(SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WCHAR, 
  		F1_SIZE - 1, 0, f1, F1_SIZE*WCHAR_SIZE, &f1Ind));
  EXEC(SQLBindParameter(stmt, 3, SQL_PARAM_INPUT_OUTPUT, SQL_C_WCHAR, SQL_WCHAR, 
  		F1_SIZE - 1, 0, f2, F1_SIZE*WCHAR_SIZE, &f2Ind));

printf("call with 3rows in params\n");

  EXEC(SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"{call test_params(?,?,?)}"), SQL_NTS));
printf("Received values\n");
  sts = SQLMoreResults(stmt);

  printf("Return [%S] Ind=%ld\n\n",_WCSCPY(u4buf,f2),f2Ind);
  SQLFreeStmt (stmt, SQL_RESET_PARAMS);
  ret = 0;

err:
  if (stmt!=SQL_NULL_HANDLE)
    SQLFreeHandle (SQL_HANDLE_STMT, stmt);
  return ret;
}




typedef struct _CParStruct {
  SQLINTEGER id;
  SQLLEN     idInd;
  sqlwchar_t   f1[F1_SIZE];
  SQLLEN   f1Ind;
  sqlwchar_t   f2[F1_SIZE];
  SQLLEN   f2Ind;
} CParStruct;


int
Test_Proc2()
{
  CParStruct cparams;
  int i;
  SQLRETURN sts;
  SQLRETURN status;
#ifdef VIRTUOSO
  wchar_t *create_cmd = L"create procedure test_params "
	"(in p1 integer, in p2 nvarchar(10) , out p3  nvarchar(10) )\n"
	"{  p3 := cast(p1 as varchar) || p2; } \n";
#else
  wchar_t *create_cmd = L"create procedure test_params "
	"@p1  int, \n"
	"@p2  nvarchar(10) , \n"
	"@p3  nvarchar(10) output \n"
	"as \n"
	"begin \n"
	"select @p3 = convert(varchar(4),@p1)+@p2 \n"
	"end \n";
#endif
  HSTMT stmt = SQL_NULL_HANDLE;
  int ret = -1;


  cparams.id=1;
  cparams.idInd=0;
  _SQLWCHAR_CPY(cparams.f1, L"test01");
  cparams.f1Ind=SQL_NTS;
  _SQLWCHAR_CPY(cparams.f2, L"t");
  cparams.f2Ind=0;


  EXEC(SQLAllocHandle (SQL_HANDLE_STMT, hdbc, &stmt));

printf("drop procedure test_params\n");
  SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"drop procedure test_params"), SQL_NTS);
printf("create procedure test_params\n");

  EXEC(SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,create_cmd), SQL_NTS));

  EXEC(SQLFreeStmt (stmt, SQL_RESET_PARAMS));

printf("use bind_by_row\n");
  EXEC(SQLSetStmtAttr(stmt, SQL_ATTR_PARAM_BIND_TYPE, (SQLPOINTER)sizeof(CParStruct), 0));


printf("bind params\n");
  EXEC(SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 
  		0, 0, &cparams.id, 0, &cparams.idInd));
  EXEC(SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WCHAR, 
  		F1_SIZE - 1, 0, cparams.f1, F1_SIZE*WCHAR_SIZE, &cparams.f1Ind));
  EXEC(SQLBindParameter(stmt, 3, SQL_PARAM_INPUT_OUTPUT, SQL_C_WCHAR, SQL_WCHAR, 
  		F1_SIZE - 1, 0, cparams.f2, F1_SIZE*WCHAR_SIZE, &cparams.f2Ind));

printf("call with 3rows in params\n");

  EXEC(SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"{call test_params(?,?,?)}"), SQL_NTS));
  sts = SQLMoreResults(stmt);

printf("Received values\n");
  printf("Return [%S] Ind=%ld\n\n",_WCSCPY(u4buf,cparams.f2),cparams.f2Ind);

  SQLFreeStmt (stmt, SQL_RESET_PARAMS);

  ret = 0;

err:
  if (stmt!=SQL_NULL_HANDLE)
    SQLFreeHandle (SQL_HANDLE_STMT, stmt);
  return ret;
}




int
ODBC_TestF1()
{
  SQLINTEGER idArr[ARRAY_SIZE];
  sqlwchar_t f1Arr[ARRAY_SIZE][F1_SIZE];
  SQLFLOAT f2Arr[ARRAY_SIZE];
  char f3Arr[ARRAY_SIZE][F1_SIZE];
  SQLLEN idIndArr[ARRAY_SIZE], f1IndArr[ARRAY_SIZE], f2IndArr[ARRAY_SIZE], f3IndArr[ARRAY_SIZE];
  int i,j;
  SQLRETURN sts;
  HSTMT stmt = SQL_NULL_HANDLE;
  int ret = -1;

  EXEC(SQLAllocHandle (SQL_HANDLE_STMT, hdbc, &stmt));

printf("use bind_by_col\n");
  EXEC(SQLSetStmtAttr(stmt, SQL_ATTR_ROW_BIND_TYPE, SQL_BIND_BY_COLUMN, 0));
  EXEC(SQLSetStmtAttr(stmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)ARRAY_SIZE, 0));

  EXEC(SQLBindCol(stmt, 1, SQL_C_SLONG, idArr, 0, idIndArr));
  EXEC(SQLBindCol(stmt, 2, SQL_C_WCHAR, f1Arr, F1_SIZE*WCHAR_SIZE, f1IndArr));
  EXEC(SQLBindCol(stmt, 3, SQL_C_DOUBLE, f2Arr, 0, f2IndArr));
  EXEC(SQLBindCol(stmt, 4, SQL_C_CHAR, f3Arr, F1_SIZE, f3IndArr));

printf("select * from test_ins\n");

  EXEC(SQLExecDirect (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"select * from test_ins"), SQL_NTS));

  sts = SQLFetch(stmt);
  while(sts == SQL_SUCCESS)
  {
    printf("-----------\n");
    for(i=0; i < ARRAY_SIZE; i++) {
      printf("|%d| |%S| |%f| |%s| \n", idArr[i], _WCSCPY(u4buf,f1Arr[i]), f2Arr[i], f3Arr[i]);
    }

    sts = SQLFetch(stmt);
  }

  SQLFreeStmt (stmt, SQL_UNBIND);
  SQLFreeStmt (stmt, SQL_CLOSE);
  ret = 0;

err:
  if (stmt!=SQL_NULL_HANDLE)
    SQLFreeHandle (SQL_HANDLE_STMT, stmt);
  return ret;
}


int
ODBC_TestF2()
{
  ParStruct params[ARRAY_SIZE];
  int i;
  SQLRETURN sts;
  SQLRETURN status;
  HSTMT stmt = SQL_NULL_HANDLE;
  int ret = -1;


  EXEC(SQLAllocHandle (SQL_HANDLE_STMT, hdbc, &stmt));

printf("use bind_by_row\n");
  EXEC(SQLSetStmtAttr(stmt, SQL_ATTR_ROW_BIND_TYPE, (SQLPOINTER)sizeof(ParStruct), 0));
  EXEC(SQLSetStmtAttr(stmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)ARRAY_SIZE, 0));

  EXEC(SQLBindCol(stmt, 1, SQL_C_SLONG, &params[0].id, 0, &params[0].idInd));
  EXEC(SQLBindCol(stmt, 2, SQL_C_WCHAR, params[0].f1, F1_SIZE*WCHAR_SIZE, &params[0].f1Ind));
  EXEC(SQLBindCol(stmt, 3, SQL_C_DOUBLE, &params[0].f2, 0, &params[0].f2Ind));
  EXEC(SQLBindCol(stmt, 4, SQL_C_CHAR, params[0].f3, F1_SIZE, &params[0].f3Ind));

printf("select * from test_ins\n");

  EXEC(SQLExecDirect (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"select * from test_ins"), SQL_NTS));

  sts = SQLFetch(stmt);
  while(sts == SQL_SUCCESS)
  {
    printf("-----------\n");
    for(i=0; i < ARRAY_SIZE; i++) {
      printf("|%d| |%S| |%f| |%s|\n", params[i].id, _WCSCPY(u4buf,params[i].f1), params[i].f2, params[i].f3);
    }

    sts = SQLFetch(stmt);
  }

  SQLFreeStmt (stmt, SQL_UNBIND);
  SQLFreeStmt (stmt, SQL_CLOSE);

  ret = 0;

err:
  if (stmt!=SQL_NULL_HANDLE)
    SQLFreeHandle (SQL_HANDLE_STMT, stmt);
  return ret;
}



int
Test_SetPos()
{
  SQLINTEGER idArr[ARRAY_SIZE];
  sqlwchar_t f1Arr[ARRAY_SIZE][F1_SIZE];
  SQLINTEGER f2Arr[ARRAY_SIZE];
  SQLLEN idIndArr[ARRAY_SIZE], f1IndArr[ARRAY_SIZE], f2IndArr[ARRAY_SIZE];
  int x,i,j;
  SQLRETURN sts = SQL_SUCCESS;
  HSTMT stmt = SQL_NULL_HANDLE;
  int ret = -1;
  SQLUSMALLINT   RowStatusArray[ARRAY_SIZE];

  EXEC(SQLAllocHandle (SQL_HANDLE_STMT, hdbc, &stmt));

  SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"drop table test_pos"), SQL_NTS);
  printf("create table test_pos\n");

  EXEC(SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"create table test_pos(id integer not null, f1 nvarchar(50), f2 integer)"), SQL_NTS));
#ifdef VIRTUOSO
  EXEC(SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"alter table test_pos modify primary key(id)"), SQL_NTS));
#else
  EXEC(SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"alter table test_pos add constraint pk_test_pos primary key(id)"), SQL_NTS));
#endif

  EXEC(SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"insert into test_pos values(1, 'test1', 101)"), SQL_NTS));
  EXEC(SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"insert into test_pos values(2, 'test2', 102)"), SQL_NTS));
  EXEC(SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"insert into test_pos values(3, 'test3', 103)"), SQL_NTS));
  EXEC(SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"insert into test_pos values(4, 'test4', 104)"), SQL_NTS));
  EXEC(SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"insert into test_pos values(5, 'test5', 105)"), SQL_NTS));
  EXEC(SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"insert into test_pos values(6, 'test6', 106)"), SQL_NTS));

  SQLFreeStmt (stmt, SQL_CLOSE);

printf("use bind_by_col\n");
  EXEC(SQLSetStmtAttr(stmt, SQL_ATTR_ROW_BIND_TYPE, SQL_BIND_BY_COLUMN, 0));
  EXEC(SQLSetStmtAttr(stmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)ARRAY_SIZE, 0));

//  EXEC(SQLSetStmtAttr(stmt, SQL_ATTR_CURSOR_TYPE, (SQLPOINTER)SQL_CURSOR_STATIC, 0));
  EXEC(SQLSetStmtAttr(stmt, SQL_ATTR_CURSOR_TYPE, (SQLPOINTER)SQL_CURSOR_DYNAMIC, 0));
  EXEC(SQLSetStmtAttr(stmt, SQL_ATTR_CONCURRENCY, (SQLPOINTER)SQL_CONCUR_VALUES, 0));
  EXEC(SQLSetStmtAttr(stmt, SQL_ATTR_ROW_STATUS_PTR, RowStatusArray, 0));


  EXEC(SQLBindCol(stmt, 1, SQL_C_SLONG, idArr, 0, idIndArr));
  EXEC(SQLBindCol(stmt, 2, SQL_C_WCHAR, f1Arr, F1_SIZE*WCHAR_SIZE, f1IndArr));
  EXEC(SQLBindCol(stmt, 3, SQL_C_SLONG, f2Arr, 0, f2IndArr));

printf("select * from test_pos\n");

  EXEC(SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"select * from test_pos"), SQL_NTS));

  for(x=1; x<=9; x++)
  {
    sts = SQL_SUCCESS;

    switch(x) {
      case 1:
      case 2:
        printf("FETCH_NEXT [%d] rows\n", ARRAY_SIZE);
        sts = SQLFetchScroll(stmt, SQL_FETCH_NEXT, 0);
        break;

      case 3:
        printf("FETCH_FIRST [%d] rows\n", ARRAY_SIZE);
        sts = SQLFetchScroll(stmt, SQL_FETCH_FIRST, 0);
        break;

      case 4:
        printf("UPDATE 1row\n");
        _SQLWCHAR_CPY(f1Arr[0], L"xxxx99");
        f2Arr[0] = 9999;
        sts = SQLSetPos(stmt, 1, SQL_UPDATE, SQL_LOCK_NO_CHANGE);
        break;

      case 5:
        printf("FETCH_FIRST [%d] rows\n", ARRAY_SIZE);
        sts = SQLFetchScroll(stmt, SQL_FETCH_FIRST, 0);
        break;

      case 6:
        printf("DELETE 1 row\n");
        sts = SQLSetPos(stmt, 1, SQL_DELETE, SQL_LOCK_NO_CHANGE);
        break;

      case 7:
        printf("FETCH_FIRST [%d] rows\n", ARRAY_SIZE);
        sts = SQLFetchScroll(stmt, SQL_FETCH_FIRST, 0);
        break;

      case 8:
        printf("ADD 1row\n");
        idArr[0] = 1;
        _SQLWCHAR_CPY(f1Arr[0], L"zzzz55");
        f2Arr[0] = 5555;
        sts = SQLSetPos(stmt, 1, SQL_ADD, SQL_LOCK_NO_CHANGE);
        break;

      case 9:
        printf("FETCH_FIRST [%d] rows\n", ARRAY_SIZE);
        sts = SQLFetchScroll(stmt, SQL_FETCH_FIRST, 0);
        break;

    }

    if (sts!= SQL_SUCCESS && sts!= SQL_SUCCESS_WITH_INFO) {
      printf("sts=%d\n",sts);
      ODBC_Errors ("SQL", stmt);
      goto err;
    }

    if (x==1 || x==2 || x==3 || x==5 || x==7 || x==9)
    {
      if (sts == SQL_SUCCESS)
      {
        printf("-----------\n");
        for(i=0; i < ARRAY_SIZE; i++) {
          printf("row[%d]   id=|%d| f1=|%S| f2=|%d|\n", i+1, 
      		idArr[i], 
      		_WCSCPY(u4buf,f1Arr[i]), 
      		f2Arr[i]);
        }
        printf("-----------\n\n");
      }
    }
  }

  SQLFreeStmt (stmt, SQL_UNBIND);
  SQLFreeStmt (stmt, SQL_CLOSE);
  ret = 0;

err:
  if (stmt!=SQL_NULL_HANDLE)
    SQLFreeHandle (SQL_HANDLE_STMT, stmt);
  return ret;
}


int
Test_BulkOp()
{
  BOOKMARK bm;
  SQLINTEGER id;
  sqlwchar_t f1[F1_SIZE];
  SQLINTEGER f2;
  SQLLEN bmInd, idInd, f1Ind, f2Ind;
  int x,i,j;
  SQLRETURN sts = SQL_SUCCESS;
  HSTMT stmt = SQL_NULL_HANDLE;
  int ret = -1;
  SQLUSMALLINT   RowStatus;

  EXEC(SQLAllocHandle (SQL_HANDLE_STMT, hdbc, &stmt));

  SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"drop table test_pos"), SQL_NTS);
  printf("create table test_pos\n");

  EXEC(SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"create table test_pos(id integer not null, f1 nvarchar(50), f2 integer)"), SQL_NTS));
#ifdef VIRTUOSO
  EXEC(SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"alter table test_pos modify primary key(id)"), SQL_NTS));
#else
  EXEC(SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"alter table test_pos add constraint pk_test_pos primary key(id)"), SQL_NTS));
#endif

  EXEC(SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"insert into test_pos values(2, 'test2', 101)"), SQL_NTS));
  EXEC(SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"insert into test_pos values(3, 'test3', 102)"), SQL_NTS));
  EXEC(SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"insert into test_pos values(4, 'test4', 103)"), SQL_NTS));


  SQLFreeStmt (stmt, SQL_CLOSE);

printf("use bind_by_col\n");
  EXEC(SQLSetStmtAttr(stmt, SQL_ATTR_ROW_BIND_TYPE, SQL_BIND_BY_COLUMN, 0));
  EXEC(SQLSetStmtAttr(stmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)1, 0));

  EXEC(SQLSetStmtAttr(stmt, SQL_ATTR_CURSOR_TYPE, (SQLPOINTER)SQL_CURSOR_KEYSET_DRIVEN, 0));
  EXEC(SQLSetStmtAttr(stmt, SQL_ATTR_CONCURRENCY, (SQLPOINTER)SQL_CONCUR_VALUES, 0));
  EXEC(SQLSetStmtAttr(stmt, SQL_ATTR_ROW_STATUS_PTR, &RowStatus, 0));
  EXEC(SQLSetStmtAttr(stmt, SQL_ATTR_USE_BOOKMARKS, (SQLPOINTER)SQL_UB_ON, 0));


  EXEC(SQLBindCol(stmt, 0, SQL_C_BOOKMARK, &bm, 0, &bmInd));
  EXEC(SQLBindCol(stmt, 1, SQL_C_SLONG, &id, 0, &idInd));
  EXEC(SQLBindCol(stmt, 2, SQL_C_WCHAR, &f1, F1_SIZE*WCHAR_SIZE, &f1Ind));
  EXEC(SQLBindCol(stmt, 3, SQL_C_SLONG, &f2, 0, &f2Ind));

printf("select * from test_pos\n");

  EXEC(SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"select * from test_pos"), SQL_NTS));

  for(x=0; x<=6; x++)
  {
    sts = SQL_SUCCESS;

    switch(x) {
      case 1:
        printf("FETCH_FIRST \n");
        sts = SQLFetchScroll(stmt, SQL_FETCH_FIRST, 0);
        break;

      case 2:
        printf("FETCH_NEXT \n");
        sts = SQLFetchScroll(stmt, SQL_FETCH_NEXT, 0);
        break;

      case 3:
        printf("FETCH_NEXT \n");
        sts = SQLFetchScroll(stmt, SQL_FETCH_NEXT, 0);
        break;

      case 4:
        printf("FETCH_FIRST \n");
        sts = SQLFetchScroll(stmt, SQL_FETCH_FIRST, 0);
        break;

      case 5:
        printf("ADD 1row\n");
        bm = 0;
        id = 1;
        _SQLWCHAR_CPY(f1, L"zzzz55");
        f2 = 5555;
        sts = SQLBulkOperations(stmt, SQL_ADD);
        break;

      case 6:
        printf("FETCH_FIRST \n");
        sts = SQLFetchScroll(stmt, SQL_FETCH_FIRST, 0);
        break;
    }
    
    if (sts!= SQL_SUCCESS && sts!= SQL_SUCCESS_WITH_INFO) {
      printf("sts=%d\n",sts);
      ODBC_Errors ("SQL", stmt);
      goto err;
    }

    if (x==1 || x==2 || x==3 || x==4 || x==6)
    {
      if (sts == SQL_SUCCESS)
      {
        printf("-----------\n");
//        printf("[%d]  bm=[%ld]  id=|%d| f1=|%S| f2=|%d|\n", RowStatus,
        printf("bm=[%ld]  id=|%d| f1=|%S| f2=|%d|\n",
                bm, id, _WCSCPY(u4buf,f1), f2);
        printf("-----------\n\n");
      }
    }
  }

  SQLFreeStmt (stmt, SQL_UNBIND);
  SQLFreeStmt (stmt, SQL_CLOSE);
  ret = 0;

err:
  if (stmt!=SQL_NULL_HANDLE)
    SQLFreeHandle (SQL_HANDLE_STMT, stmt);
  return ret;
}





//#define MAX_LOB_LEN 32 
//#define CHUNK_SIZE  8   
//#define MAX_LOB_LEN 524288 
#define MAX_LOB_LEN 131072 
#define CHUNK_SIZE  65536 

int Test_GetBlob1();


int
Test_Blob1()
{
  char *bData;
  SQLINTEGER id;
  SQLLEN idInd;
  SQLLEN f1Ind;
  SQLLEN  fDAE = SQL_DATA_AT_EXEC;
//  SDWORD  nParamId = 2;
  SQLLEN  nParamId = 2;
  long    lCursize;
  sqlwchar_t *wbuf = NULL;
  int i;
  SQLRETURN rc,sts;
  HSTMT stmt = SQL_NULL_HANDLE;
  int ret = -1;

  EXEC(SQLAllocHandle (SQL_HANDLE_STMT, hdbc, &stmt));

  SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"drop table test_blob"), SQL_NTS);
printf("create table test_blob\n");

#ifdef VIRTUOSO
  EXEC(SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"create table test_blob(id integer, f1 long nvarchar, f2 long varchar)"), SQL_NTS));
#else
  EXEC(SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"create table test_blob(id integer, f1 ntext, f2 text)"), SQL_NTS));
#endif
  EXEC(SQLFreeStmt (stmt, SQL_RESET_PARAMS));

  EXEC(SQLPrepareW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"insert into test_blob (id, f1) values(?,?)"), SQL_NTS));

  EXEC(SQLSetStmtAttr(stmt, SQL_ATTR_PARAM_BIND_TYPE, SQL_BIND_BY_COLUMN, 0));


printf("bind params\n");
  id = 1;
  idInd = 0;
  EXEC(SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 
  		0, 0, &id, 0, &idInd));
  EXEC(SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WLONGVARCHAR, 
  		0, 0, &nParamId, 0, &fDAE));

  if (!(wbuf = (sqlwchar_t *)malloc(CHUNK_SIZE)))
  {
     printf("\aMemory allocation error !\n");
     goto rowInsertErr;
  }

  rc = SQLExecute (stmt);
printf("bind params exec rc=%d\n", rc);
  if (rc == SQL_NEED_DATA)
  {
    PTR     rgbValue;
    SQLLEN  cbChunkSize;
    SQLLEN  cbAvailable;
    BOOL    bLastChunk;

    // Insert the LOB in several chunks
    rc = SQLParamData (stmt, &rgbValue);
    if (rc != SQL_NEED_DATA || ((SQLLEN *) rgbValue) != &nParamId)
    {
      printf("SQLParamData = %d\n", rc);
      goto rowInsertErr;
    }

    cbChunkSize = CHUNK_SIZE;
    lCursize = cbAvailable = MAX_LOB_LEN * WCHAR_SIZE;
    bLastChunk = FALSE;

    printf("Attempting piecewise insert of value of length %ld\n", lCursize);

    while (cbAvailable > 0)
    {
      if (cbAvailable <= CHUNK_SIZE)
      {
        cbChunkSize = cbAvailable;
        bLastChunk = TRUE;
      }

      // Create a chunk...
      // Fill character = small letter d with caron
      // End marker = capital letter E with dot above
      long cwChars = cbChunkSize / WCHAR_SIZE;
      for(i=0; i<cwChars; i++)
        wbuf[i] =  L'\x010F'; //L'\x0411'; //
      if (bLastChunk)
         wbuf[cwChars - 1] = L'\x0116'; //L'\x0436'; //

      // Insert the chunk
      rc = SQLPutData (stmt, (SQLPOINTER) wbuf, cbChunkSize);
      if (rc != SQL_SUCCESS)
      {
        ODBC_Errors ("SQLPutData", stmt);
        goto rowInsertErr;
      }

      printf("\tInserted %s chunk of length %d\n", bLastChunk?"LAST":"", cbChunkSize);
      cbAvailable -= cbChunkSize;
    }

    //  all data for this DATA-AT-EXEC parameter  has been sent.
    if ((rc = SQLParamData (stmt, &rgbValue)) != SQL_SUCCESS)
    {
      ODBC_Errors ("SQLParamData", stmt);
      goto rowInsertErr;
    }

    printf("SUCCESS: Inserted value of total length %ld\n", lCursize); 
    rc = SQL_SUCCESS;
  }

rowInsertErr:
  if (wbuf!=NULL)
  {
    free(wbuf);
    wbuf = NULL;
  }

  if (rc != SQL_SUCCESS) 
  {
    printf("\aERROR: LOB insertion failed\n\n");
    // Issue cancel in case statement failed whilst needing data
    SQLCancel (stmt);
    goto err;
  }

  SQLFreeStmt (stmt, SQL_RESET_PARAMS);
  SQLFreeStmt (stmt, SQL_CLOSE);

  ret = Test_GetBlob1();

err:
  if (stmt!=SQL_NULL_HANDLE)
    SQLFreeHandle (SQL_HANDLE_STMT, stmt);
  return ret;
}



int
Test_GetBlob1()
{
  char *bData;
  SQLINTEGER id;
  SQLLEN idInd;
  SQLLEN f1Ind;
  long    lCursize;
  sqlwchar_t *wbuf = NULL;
  int i;
  SQLRETURN rc,sts;
  HSTMT stmt = SQL_NULL_HANDLE;
  int ret = -1;
  
  EXEC(SQLAllocHandle (SQL_HANDLE_STMT, hdbc, &stmt));

  //================= Fetch Data =======================

  EXEC(SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"select id,f1 from test_blob"), SQL_NTS));

  EXEC(SQLBindCol (stmt, 1, SQL_C_SLONG, &id, sizeof (long), &idInd));

  {
    long    j;
    long    cbChunkSize;
    SQLRETURN   rc;
    SQLLEN  cbTotalRetrieved;
    SQLLEN  cbTotalAvailable;
    SQLLEN  cbRetrieved;
    SQLLEN  cbAvailable;
    BOOL    bLastChunk;

    cbChunkSize = CHUNK_SIZE;
    lCursize = MAX_LOB_LEN * WCHAR_SIZE;

    // Allocate buffer to hold chunks...
    // Allow space for null terminator if fetching character data. 
    if (!(wbuf = (sqlwchar_t *)malloc(cbChunkSize+WCHAR_SIZE)))
    {
      printf("\aMemory allocation error !\n");
      goto rowFetchErr;
    }

    if ((rc = SQLFetch (stmt)) != SQL_SUCCESS)
    {
      ODBC_Errors ("SQLFetch", stmt);
      goto rowFetchErr;
    }

    printf("Attempting piecewise fetch of value of total length %ld bytes\n", lCursize);
    printf("using a chunk size of %ld bytes\n", cbChunkSize);

    cbTotalAvailable = SQL_NO_TOTAL;
    cbTotalRetrieved = 0;
    bLastChunk = FALSE;

    do
    {

      cbAvailable = 0;
      printf("Getting chunk...\n");

      memset (wbuf, 0, cbChunkSize+WCHAR_SIZE);
      rc = SQLGetData (stmt, 2, SQL_C_WCHAR, wbuf, 
                   cbChunkSize+WCHAR_SIZE, &cbAvailable);
      printf("cbAvailable = %d rc=%d\n",cbAvailable,rc);  

      if (rc == SQL_SUCCESS)  
      {
        printf("SQL_SUCCESS\n"); 
      }  
      else if (rc == SQL_SUCCESS_WITH_INFO)
      {
	// If more than one call to SQLGetData is required
	// to retrieve the data, the driver should return
	// SQL_SUCCESS_WITH_INFO. A subsequent call to SQLError
	// should return SQLSTATE 01004 (Data truncated).

        printf("SQL_SUCCESS_WITH_INFO\n"); 
      }
      else if (rc == SQL_NO_DATA)  
      {
        printf("SQL_NO_DATA\n");
        break; 
      }  
      else
      {
        ODBC_Errors ("SQLGetData", stmt);
        goto rowFetchErr;
      }

      if (cbAvailable == SQL_NO_TOTAL)
      {
        printf("\nERROR: Driver cannot determine total length of "
                 "available data.\n");
        goto rowFetchErr;
      }
      else if (cbTotalAvailable == SQL_NO_TOTAL)
      {
        cbTotalAvailable = cbAvailable;
        if (cbTotalAvailable != lCursize)
        {
          printf("\nERROR: Requested value of total length %ld\n",lCursize);
          printf("Available length = %ld\n", cbTotalAvailable);
//          break;
        }
      }

      cbRetrieved = cbChunkSize <= cbAvailable ? cbChunkSize : cbAvailable;

      printf("\tRequested chunk of length %ld ", cbChunkSize);
      printf(" got %ld \n", cbRetrieved);
      printf("\tBytes remaining prior to this call: %ld \n", cbAvailable);

      if (cbTotalRetrieved + cbRetrieved == lCursize)
        bLastChunk = TRUE;

      // Check the integrity of the data retrieved
//--      if (CType == SQL_C_WCHAR)
      {
	long cwChars = cbRetrieved / WCHAR_SIZE;
        for (j = 0; j < cwChars; j++)
        {
          if (bLastChunk && j == cwChars - 1)
          {
            if (wbuf[j] != L'\x0116') //L'\x0436')   // 
            {
              printf("ERROR: Buffer EOT marker not found\n");
              rc = SQL_ERROR;
	      goto rowFetchErr;
            }
          }
          else if (wbuf[j] != L'\x010F') //L'\x0411')   // 
          {
            printf("ERROR: Buffer compare failed at offset %ld ", j);
            printf(" - expected x010F, got %x \n",(unsigned short) wbuf[j]);
            rc = SQL_ERROR;
	    goto rowFetchErr;
          }
	}
      }

      cbTotalRetrieved += cbRetrieved;
    }
    while (rc == SQL_SUCCESS_WITH_INFO || rc == SQL_SUCCESS);
    /* rc == SQL_SUCCESS => all of the data has been retrieved */

    if (cbTotalRetrieved == lCursize)
    {
      printf("\n\nSUCCESS: Requested value of total length %ld ", lCursize);
      printf(", got %ld \n", cbTotalRetrieved);
    }
    else
    {
      printf("\n\nERROR: Requested value of total length %ld ", lCursize);
      printf(", got %ld \n", cbTotalRetrieved);
      rc = SQL_ERROR;
      goto rowFetchErr;
    }

    rc = SQL_SUCCESS;

rowFetchErr:
    if (wbuf!=NULL)
    {
      free(wbuf);
      wbuf = NULL;
    }

    if (rc!=SQL_SUCCESS)
    {
      printf("\aERROR: LOB retrieval failed\n\n");
    }
  } // while (nRow < MAX_SIZES...

  EXEC(SQLFreeStmt (stmt, SQL_CLOSE));

//=============================================================================
  ret = 0;

err:
  if (stmt!=SQL_NULL_HANDLE)
    SQLFreeHandle (SQL_HANDLE_STMT, stmt);
  return ret;
}



//??FIXME
int
Test_Insert_Date1()
{
  SQLINTEGER idArr[ARRAY_SIZE];
  sqlwchar_t f1Arr[ARRAY_SIZE][F1_SIZE];
  sqlwchar_t f2Arr[ARRAY_SIZE][F1_SIZE];
  sqlwchar_t f3Arr[ARRAY_SIZE][F1_SIZE];
  SQLLEN idIndArr[ARRAY_SIZE], f1IndArr[ARRAY_SIZE], f2IndArr[ARRAY_SIZE], f3IndArr[ARRAY_SIZE];
  SQLUSMALLINT ParamsProcessed, ParamStatusArray[ARRAY_SIZE];
  int i;
  SQLRETURN sts;
  SQLRETURN status;
  HSTMT stmt = SQL_NULL_HANDLE;
  int ret = -1;


  for(i=0; i < ARRAY_SIZE; i++) {
    idIndArr[i]=0;
    f1IndArr[i]=SQL_NTS;
    f2IndArr[i]=SQL_NTS;
    f3IndArr[i]=SQL_NTS;
  }

  idArr[0]=1;
  idArr[1]=2;
  idArr[2]=3;

  _SQLWCHAR_CPY(f1Arr[0],L"{d '2012-08-01'}");
  _SQLWCHAR_CPY(f1Arr[1],L"{d '2012-08-02'}");
  _SQLWCHAR_CPY(f1Arr[2],L"{d '2012-08-03'}");

  _SQLWCHAR_CPY(f2Arr[0],L"{t '20:41:01'}");
  _SQLWCHAR_CPY(f2Arr[1],L"{t '20:41:02'}");
  _SQLWCHAR_CPY(f2Arr[2],L"{t '20:41:03'}");

  _SQLWCHAR_CPY(f3Arr[0],L"{ts '2012-08-01 20:41:01'}");
  _SQLWCHAR_CPY(f3Arr[1],L"{ts '2012-08-02 20:41:02'}");
  _SQLWCHAR_CPY(f3Arr[2],L"{ts '2012-08-03 20:41:03'}");


  EXEC(SQLAllocHandle (SQL_HANDLE_STMT, hdbc, &stmt));

  SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"drop table test_ins_date"), SQL_NTS);
  printf("create table test_ins\n");

//  EXEC(SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"create table test_ins_date(id integer, f1 date, f2 time, f3 datetime)"), SQL_NTS));
  EXEC(SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"create table test_ins_date(id integer, f1 date, f2 datetime, f3 datetime)"), SQL_NTS));
  EXEC(SQLFreeStmt (stmt, SQL_RESET_PARAMS));

printf("use bind_by_column\n");
  EXEC(SQLSetStmtAttr(stmt, SQL_ATTR_PARAM_BIND_TYPE, SQL_BIND_BY_COLUMN, 0));

  EXEC(SQLSetStmtAttr(stmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)ARRAY_SIZE, 0));


printf("bind params\n");
  EXEC(SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 
  		0, 0, idArr, 0, idIndArr));
  EXEC(SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_DATE, 
  		F1_SIZE - 1, 0, f1Arr, F1_SIZE*WCHAR_SIZE, f1IndArr));
  EXEC(SQLBindParameter(stmt, 3, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_TIME, 
  		F1_SIZE - 1, 0, f2Arr, F1_SIZE*WCHAR_SIZE, f2IndArr));
  EXEC(SQLBindParameter(stmt, 4, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_TIMESTAMP, 
  		F1_SIZE - 1, 0, f3Arr, F1_SIZE*WCHAR_SIZE, f3IndArr));

printf("insert 3rows\n");

  EXEC(SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"insert into test_ins_date values(?,?,?,?)"), SQL_NTS));

  SQLFreeStmt (stmt, SQL_RESET_PARAMS);
  SQLFreeStmt (stmt, SQL_UNBIND);

printf("select * from test_ins_date\n");
  ExecSelect(L"select * from test_ins_date");
  ret = 0;

err:
  if (stmt!=SQL_NULL_HANDLE)
    SQLFreeHandle (SQL_HANDLE_STMT, stmt);
  return ret;
}


typedef struct _ParDateStruct {
  SQLINTEGER id;
  SQLLEN idInd;
  sqlwchar_t     f1[F1_SIZE];
  SQLLEN f1Ind;
  sqlwchar_t     f2[F1_SIZE];
  SQLLEN f2Ind;
  sqlwchar_t     f3[F1_SIZE];
  SQLLEN f3Ind;
} ParDateStruct;


//??FIXME
int
Test_Insert_Date2()
{
  ParDateStruct params[ARRAY_SIZE];
  SQLUINTEGER ParamsProcessed;
  SQLUSMALLINT ParamStatusArray[ARRAY_SIZE];

  int i;
  SQLRETURN sts;
  SQLRETURN status;
  HSTMT stmt = SQL_NULL_HANDLE;
  int ret = -1;


  params[0].id=1;
  params[0].idInd=0;
  _SQLWCHAR_CPY(params[0].f1, L"{d '2012-08-01'}");
  params[0].f1Ind=SQL_NTS;
  _SQLWCHAR_CPY(params[0].f2, L"{t '20:41:01'}");
  params[0].f2Ind=SQL_NTS;
  _SQLWCHAR_CPY(params[0].f3, L"{ts '2012-08-01 20:41:01'}");
  params[0].f3Ind=SQL_NTS;

  params[1].id=2;
  params[1].idInd=0;
  _SQLWCHAR_CPY(params[1].f1, L"{d '2012-08-02'}");
  params[1].f1Ind=SQL_NTS;
  _SQLWCHAR_CPY(params[1].f2, L"{t '20:41:02'}");
  params[1].f2Ind=SQL_NTS;
  _SQLWCHAR_CPY(params[1].f3, L"{ts '2012-08-02 20:41:02'}");
  params[1].f3Ind=SQL_NTS;

  params[2].id=3;
  params[2].idInd=0;
  _SQLWCHAR_CPY(params[2].f1, L"{d '2012-08-03'}");
  params[2].f1Ind=SQL_NTS;
  _SQLWCHAR_CPY(params[2].f2, L"{t '20:41:03'}");
  params[2].f2Ind=SQL_NTS;
  _SQLWCHAR_CPY(params[2].f3, L"{ts '2012-08-03 20:41:03'}");
  params[2].f3Ind=SQL_NTS;



  EXEC(SQLAllocHandle (SQL_HANDLE_STMT, hdbc, &stmt)); 

  SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"drop table test_ins_date"), SQL_NTS);
printf("create table test_ins\n");

//  EXEC(SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"create table test_ins_date(id integer, f1 date, f2 time, f3 datetime)"), SQL_NTS));
  EXEC(SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"create table test_ins_date(id integer, f1 date, f2 datetime, f3 datetime)"), SQL_NTS));

  EXEC(SQLFreeStmt (stmt, SQL_RESET_PARAMS));

printf("use bind_by_row\n");
  EXEC(SQLSetStmtAttr(stmt, SQL_ATTR_PARAM_BIND_TYPE, (SQLPOINTER)sizeof(ParDateStruct), 0));

  EXEC(SQLSetStmtAttr(stmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)ARRAY_SIZE, 0));

printf("bind params\n");
  EXEC(SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 
  		0, 0, &params[0].id, 0, &params[0].idInd));
  EXEC(SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_DATE, 
  		F1_SIZE - 1, 0, params[0].f1, F1_SIZE*WCHAR_SIZE, &params[0].f1Ind));
  EXEC(SQLBindParameter(stmt, 3, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_TIME, 
  		F1_SIZE - 1, 0, params[0].f2, F1_SIZE*WCHAR_SIZE, &params[0].f2Ind));
  EXEC(SQLBindParameter(stmt, 4, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_TIMESTAMP, 
  		F1_SIZE - 1, 0, params[0].f3, F1_SIZE*WCHAR_SIZE, &params[0].f3Ind));

printf("insert 3rows\n");
  EXEC(SQLExecDirectW (stmt, (SQLWCHAR*)_To_SQLWCHAR(u2buf,L"insert into test_ins_date values(?,?,?,?)"), SQL_NTS));

  SQLFreeStmt (stmt, SQL_RESET_PARAMS);
  SQLFreeStmt (stmt, SQL_UNBIND);

printf("select * from test_ins_date\n");
  ExecSelect(L"select * from test_ins_date");
  ret = 0;

err:
  if (stmt!=SQL_NULL_HANDLE)
    SQLFreeHandle (SQL_HANDLE_STMT, stmt);
  return ret;
}







int
ODBC_Results (HSTMT stmt)
{
  sqlwchar_t fetchBuffer[1024];
  size_t displayWidths[MAXCOLS];
  size_t displayWidth;
  short numCols;
  short colNum;
  sqlwchar_t colName[50];
  SQLSMALLINT colType;
  SQLULEN colPrecision;
  SQLLEN colIndicator;
  SQLSMALLINT colScale;
  SQLSMALLINT colNullable;
  unsigned long totalRows;
  unsigned long totalSets;
  int i;
  SQLRETURN sts;

      totalSets = 1;
      do
	{
	  /*
	   *  Get the number of result columns for this cursor.
	   *  If it is 0, then the statement was probably a select
	   */
	  if (SQLNumResultCols (stmt, &numCols) != SQL_SUCCESS)
	    {
	      ODBC_Errors ("SQLNumResultCols", stmt);
	      goto endCursor;
	    }
	  if (numCols == 0)
	    {
	      SQLLEN nrows = 0;

	      SQLRowCount (stmt, &nrows);
	      printf ("Statement executed. %ld rows affected.\n",
		  nrows > 0 ? (long) nrows : 0L);
	      goto endCursor;
	    }

	  if (numCols > MAXCOLS)
	    {
	      numCols = MAXCOLS;
	      fprintf (stderr,
		  "NOTE: Resultset truncated to %d columns.\n", MAXCOLS);
	    }

	  /*
	   *  Get the names for the columns
	   */
	  putchar ('\n');
	  for (colNum = 1; colNum <= numCols; colNum++)
	    {
	      /*
	       *  Get the name and other type information
	       */
	      if (SQLDescribeColW (stmt, colNum,
		      (SQLWCHAR *) colName, NUMU2CHAR (colName), NULL,
		      &colType, &colPrecision, &colScale,
		      &colNullable) != SQL_SUCCESS)
		{
		  ODBC_Errors ("SQLDescribeCol", stmt);
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
		  displayWidth = 30;	/* show only first 30 */
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
		  displayWidths[colNum - 1] = 0;	/* skip other data types */
		  continue;
		}

	      if (displayWidth < _WCSLEN (colName))
		displayWidth = _WCSLEN (colName);
	      if (displayWidth > NUMU2CHAR (fetchBuffer) - 1)
		displayWidth = NUMU2CHAR (fetchBuffer) - 1;

	      displayWidths[colNum - 1] = displayWidth;

	      /*
	       *  Print header field
	       */
#ifdef UNICODE
	      printf ("%-*.*S", displayWidth, displayWidth, _WCSCPY(u4buf,colName));
#else
	      printf ("%-*.*s", displayWidth, displayWidth, colName);
#endif
	      if (colNum < numCols)
		putchar ('|');
	    }
	  putchar ('\n');

	  /*
	   *  Print second line
	   */
	  for (colNum = 1; colNum <= numCols; colNum++)
	    {
	      for (i = 0; i < displayWidths[colNum - 1]; i++)
		putchar ('-');
	      if (colNum < numCols)
		putchar ('+');
	    }
	  putchar ('\n');

	  /*
	   *  Print all the fields
	   */
	  totalRows = 0;
	  while (1)
	    {
#if (ODBCVER < 0x0300)
	      int sts = SQLFetch (stmt);
#else
	      int sts = SQLFetchScroll (stmt, SQL_FETCH_NEXT, 1);
#endif

	      if (sts == SQL_NO_DATA_FOUND)
		break;

	      if (sts != SQL_SUCCESS)
		{
		  ODBC_Errors ("Fetch", stmt);
		  break;
		}
	      for (colNum = 1; colNum <= numCols; colNum++)
		{
		  /*
		   *  Fetch this column as character
		   */
		  sts = SQLGetData (stmt, colNum, SQL_C_WCHAR, fetchBuffer,
		      NUMU2CHAR (fetchBuffer), &colIndicator);
		  if (sts != SQL_SUCCESS_WITH_INFO && sts != SQL_SUCCESS)
		    {
		      ODBC_Errors ("SQLGetData", stmt);
		      goto endCursor;
		    }

		  /*
		   *  Show NULL fields as ****
		   */
		  if (colIndicator == SQL_NULL_DATA)
		    {
		      for (i = 0; i < displayWidths[colNum - 1]; i++)
			fetchBuffer[i] = L'*';
		      fetchBuffer[i] = L'\0';
		    }

		  printf ("%-*.*S", displayWidths[colNum - 1],
		      displayWidths[colNum - 1], _WCSCPY(u4buf,fetchBuffer));
		  if (colNum < numCols)
		    putchar ('|');
		}
	      putchar ('\n');
	      totalRows++;
	    }

	  printf ("\n result set %lu returned %lu rows.\n\n",
	      totalSets, totalRows);
	  totalSets++;
	}
      while ((sts = SQLMoreResults (stmt)) == SQL_SUCCESS);

      if (sts == SQL_ERROR)
	ODBC_Errors ("SQLMoreResults", stmt);

    endCursor:

  return 0;
}




int
main (int argc, char **argv)
{
  /*
   *  Set locale based on environment variables
   */
  setlocale (LC_ALL, "");

  /*
   *  Show welcome message
   */
#ifdef UNICODE
  printf ("iODBC Unicode Demonstration program\n");
#else
  printf ("iODBC Demonstration program\n");
#endif
  printf ("This program shows an interactive SQL processor\n");

  /*
   *  Show a usage string when the user asks for this
   */
  if (argc > 2 || (argc == 2 && argv[1][0] == '-'))
    {
      fprintf (stderr,
	  "\nUsage:\n  iodbctest [\"DSN=xxxx;UID=xxxx;PWD=xxxx\"]\n");
      exit (0);
    }

  /*
   *  If we can connect to this datasource, run the test program
   */
  if (ODBC_Connect (argv[1]) != 0)
    {
      ODBC_Errors ("ODBC_Connect", SQL_NULL_HANDLE);
    }
  else if (ODBC_Test () != 0)
    {
      ODBC_Errors ("ODBC_Test", SQL_NULL_HANDLE);
    }

  /*
   *  End the connection
   */
  ODBC_Disconnect ();

  printf ("\nHave a nice day.");

  return 0;
}
