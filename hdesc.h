/*
 *  hdesc.h
 *
 *  $Id$
 *
 *  Descriptor object
 *
 *  The iODBC driver manager.
 *  
 *  Copyright (C) 1999 by OpenLink Software <iodbc@openlinksw.com>
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

#ifndef __DESC_H
#define __DESC_H

#define APP_ROW_DESC	0
#define APP_PARAM_DESC	1
#define IMP_ROW_DESC	2
#define IMP_PARAM_DESC	3

typedef struct DESC_s {

  int type;
  HERR herr;   		/* list of descriptor errors */
  SQLRETURN rc;
  
  struct DESC_s *next;

  SQLHDBC hdbc;	 	/* connection associated with the descriptor */
  SQLHDESC dhdesc; 	/* the driver's desc handle */
  HSTMT hstmt;   	/* if not null - the descriptor is implicit to that statement */
  
} DESC_t;

#ifndef HDESC
#define HDESC SQLHDESC
#endif
#define IS_VALID_HDESC(x) \
	((x) != SQL_NULL_HSTMT && \
	 ((DESC_t FAR *)(x))->type == SQL_HANDLE_DESC && \
	 ((DESC_t FAR *)(x))->hdbc != SQL_NULL_HDBC)

#endif /* __DESC_H */
