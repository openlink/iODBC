/*
 *  isqltypes.h
 *
 *  $Id$
 *
 *  iODBC typedefs
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
#ifndef _ISQLTYPES_H
#define _ISQLTYPES_H

/* 
 *  Set default specification to  ODBC 2.5 
 */
#ifndef ODBCVER
#define ODBCVER	0x0250
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  Environment specific definitions
 */
#ifndef NEAR
#define NEAR
#endif

#ifndef FAR
#define FAR
#endif

#ifndef EXPORT
#define EXPORT
#endif

#ifdef WIN32
#define SQL_API	__stdcall
#else
#define SQL_API
#endif


/* 
 *  SQL portable types for C 
 */
typedef unsigned char           UCHAR;
typedef signed char             SCHAR;
typedef unsigned short          USHORT;
typedef signed short            SSHORT;
typedef unsigned long 		UDWORD;
typedef long 			SDWORD;
typedef unsigned short 		UWORD;
typedef short 			SWORD;
typedef unsigned long           ULONG;
typedef signed long             SLONG;
typedef float                   SFLOAT;
typedef double                  SDOUBLE;
typedef double            	LDOUBLE; 


/*
 *  API declaration data types
 */
typedef unsigned char		SQLCHAR;
typedef signed char		SQLSCHAR;
typedef short           	SQLSMALLINT;
typedef unsigned short		SQLUSMALLINT;
typedef long                 	SQLINTEGER;
typedef unsigned long		SQLUINTEGER;


/*
 *  Generic pointer types
 */
typedef void FAR *              PTR;
typedef void FAR *              SQLPOINTER;
#if defined(WIN32)
typedef void FAR *		SQLHANDLE;
#else
typedef SQLINTEGER		SQLHANDLE;
#endif


/*
 *  Handles
 */
typedef void FAR *		HENV;
typedef void FAR *		HDBC;
typedef void FAR *		HSTMT;

typedef SQLHANDLE		SQLHENV;
typedef SQLHANDLE		SQLHDBC;
typedef SQLHANDLE		SQLHSTMT;


/*
 *  Window Handle
 */
#if defined(WIN32) || defined(OS2)
typedef HWND			SQLHWND;
#else
typedef SQLPOINTER		HWND;
typedef SQLPOINTER		SQLHWND;
#endif


/*
 *  Return type for functions
 */
typedef SQLSMALLINT           	RETCODE;
typedef SQLSMALLINT            	SQLRETURN;


/*
 *  SQL portable types for C - DATA, TIME, TIMESTAMP, and BOOKMARK
 */
typedef unsigned long int BOOKMARK;


typedef struct tagDATE_STRUCT
  {
    SQLSMALLINT year;
    SQLUSMALLINT month;
    SQLUSMALLINT day;
  }
DATE_STRUCT;


typedef struct tagTIME_STRUCT
  {
    SQLUSMALLINT hour;
    SQLUSMALLINT minute;
    SQLUSMALLINT second;
  }
TIME_STRUCT;


typedef struct tagTIMESTAMP_STRUCT
  {
    SQLSMALLINT year;
    SQLUSMALLINT month;
    SQLUSMALLINT day;
    SQLUSMALLINT hour;
    SQLUSMALLINT minute;
    SQLUSMALLINT second;
    SQLUINTEGER fraction;
  }
TIMESTAMP_STRUCT;


#ifdef __cplusplus
}
#endif

#endif /* _ISQLTYPES_H */
