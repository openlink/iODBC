/*
 *  isql.c
 *
 *  $Id$
 *
 *  Standard defines
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
#ifndef _INTRINSIC_SQL_H
#define _INTRINSIC_SQL_H

#ifdef WIN32
#define SQL_API			__stdcall
#else
#define FAR
#define EXPORT
#define CALLBACK
#define SQL_API			EXPORT CALLBACK
#endif

typedef unsigned char UCHAR;
typedef long int SDWORD;
typedef short int SWORD;
typedef unsigned long int UDWORD;
typedef unsigned short int UWORD;

typedef void FAR *PTR;
typedef void FAR *HENV;
typedef void FAR *HDBC;
typedef void FAR *HSTMT;

typedef signed short RETCODE;


#define ODBCVER			0x0200

#define SQL_MAX_MESSAGE_LENGTH		512
#define SQL_MAX_DSN_LENGTH		32

/* return code */
#define SQL_INVALID_HANDLE		(-2)
#define SQL_ERROR			(-1)
#define SQL_SUCCESS 			0
#define SQL_SUCCESS_WITH_INFO		1
#define SQL_NO_DATA_FOUND		100

/* standard SQL datatypes (agree with ANSI type numbering) */
#define SQL_CHAR			1
#define SQL_NUMERIC 			2
#define SQL_DECIMAL 			3
#define SQL_INTEGER 			4
#define SQL_SMALLINT			5
#define SQL_FLOAT			6
#define SQL_REAL			7
#define SQL_DOUBLE			8
#define SQL_VARCHAR 			12

#define SQL_TYPE_MIN			SQL_CHAR
#define SQL_TYPE_NULL			0
#define SQL_TYPE_MAX			SQL_VARCHAR

/* C to SQL datatype mapping */
#define SQL_C_CHAR			SQL_CHAR
#define SQL_C_LONG			SQL_INTEGER
#define SQL_C_SHORT   			SQL_SMALLINT
#define SQL_C_FLOAT   			SQL_REAL
#define SQL_C_DOUBLE			SQL_DOUBLE
#define SQL_C_DEFAULT 			99

#define SQL_NO_NULLS			0
#define SQL_NULLABLE			1
#define SQL_NULLABLE_UNKNOWN		2

/* some special length values */
#define SQL_NULL_DATA			(-1)
#define SQL_DATA_AT_EXEC		(-2)
#define SQL_NTS 			(-3)

/* SQLFreeStmt flag values */
#define SQL_CLOSE			0
#define SQL_DROP			1
#define SQL_UNBIND			2
#define SQL_RESET_PARAMS		3

/* SQLTransact flag values */
#define SQL_COMMIT			0
#define SQL_ROLLBACK			1

/* SQLColAttributes flag values */
#define SQL_COLUMN_COUNT            	0
#define SQL_COLUMN_LABEL		18
#define SQL_COLATT_OPT_MAX		SQL_COLUMN_LABEL
#define SQL_COLUMN_DRIVER_START	1000

#define SQL_COLATT_OPT_MIN		SQL_COLUMN_COUNT

/* Null handles */
#define SQL_NULL_HENV			0
#define SQL_NULL_HDBC			0
#define SQL_NULL_HSTMT			0
#endif
