/*
 *  hdbc.h
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
#ifndef	_HDBC_H
#define	_HDBC_H

#if (ODBCVER >= 0x0300)
#include <hdesc.h>
#endif

typedef struct _drvopt
  {
    SQLUSMALLINT Option;
    SQLUINTEGER  Param;

    struct _drvopt *next;
  } 
DRVOPT;

typedef struct DBC
  {
    int type;			/* must be 1st field */
    HERR herr;
    SQLRETURN rc;

    struct DBC FAR * next;

    HENV genv;			/* back point to global env object */

    HDBC dhdbc;			/* driver's private dbc */
    HENV henv;			/* back point to instant env object */
    HSTMT hstmt;		/* list of statement object handle(s) */
#if (ODBCVER >= 0x300)
    HDESC hdesc;    		/* list of connection descriptors */
#endif    

    int state;

    /* options */
    UDWORD access_mode;
    UDWORD autocommit;

    UDWORD login_timeout;
    UDWORD odbc_cursors;
    UDWORD packet_size;
    UDWORD quiet_mode;
    UDWORD txn_isolation;
    SWORD cb_commit;
    SWORD cb_rollback;

    char FAR * current_qualifier;

    int trace;				/* trace flag */
    char FAR * tfile;
    void FAR * tstm;			/* trace stream */

    SWORD dbc_cip;			/* Call in Progess flag */

    DRVOPT *drvopt;			/* Driver specific connect options */
  }
DBC_t;

#define IS_VALID_HDBC(x) \
	((x) != SQL_NULL_HDBC && ((DBC_t FAR *)(x))->type == SQL_HANDLE_DBC)

#define ENTER_HDBC(pdbc) \
        ODBC_LOCK();\
    	if (!IS_VALID_HDBC (pdbc)) \
	  { \
	    ODBC_UNLOCK (); \
	    return SQL_INVALID_HANDLE; \
	  } \
	else if (pdbc->dbc_cip) \
          { \
	    PUSHSQLERR (pdbc->herr, en_S1010); \
	    ODBC_UNLOCK(); \
	    return SQL_ERROR; \
	  } \
	pdbc->dbc_cip = 1; \
	CLEAR_ERRORS (pdbc); \
	ODBC_UNLOCK();


#define LEAVE_HDBC(pdbc, err) \
	pdbc->dbc_cip = 0; \
	return (err);

/* 
 * Note:
 *  - ODBC applications can see address of driver manager's 
 *    connection object, i.e connection handle -- a void pointer, 
 *    but not detail of it. ODBC applications can neither see 
 *    detail driver's connection object nor its address.
 *
 *  - ODBC driver manager knows its own connection objects and
 *    exposes their address to an ODBC application. Driver manager
 *    also knows address of driver's connection objects and keeps
 *    it via dhdbc field in driver manager's connection object.
 * 
 *  - ODBC driver exposes address of its own connection object to
 *    driver manager without detail.
 *
 *  - Applications can get driver's connection object handle by
 *    SQLGetInfo() with fInfoType equals to SQL_DRIVER_HDBC.
 */

enum
  {
    en_dbc_allocated,
    en_dbc_needdata,
    en_dbc_connected,
    en_dbc_hstmt
  };


/*
 *  Internal prototypes 
 */
SQLRETURN SQL_API _iodbcdm_SetConnectOption (
    SQLHDBC hdbc,
    SQLUSMALLINT fOption, 
    SQLUINTEGER vParam);
SQLRETURN SQL_API _iodbcdm_GetConnectOption (
    SQLHDBC hdbc,
    SQLUSMALLINT fOption, 
    SQLPOINTER pvParam);
#endif
