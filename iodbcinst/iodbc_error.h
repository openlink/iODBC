/*
 *  iodbc_error.h
 *
 *  $Id$
 *
 *  Misc support functions
 *
 *  The iODBC driver manager.
 * 
 *  Copyright (C) 2001 by OpenLink Software <iodbc@openlinksw.com>
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
#ifndef _IODBC_ERROR_H
#define _IODBC_ERROR_H

/* Definition of the error code array */
#define ERROR_NUM 8

extern DWORD ierror[ERROR_NUM];
extern LPSTR errormsg[ERROR_NUM];
extern SWORD numerrors;

#define CLEAR_ERROR() \
	numerrors = -1;

#define PUSH_ERROR(error) \
	if(numerrors < ERROR_NUM) \
	{ \
		ierror[++numerrors] = (error); \
		errormsg[numerrors] = NULL; \
	}

#define POP_ERROR(error) \
	if(numerrors != -1) \
	{ \
		errormsg[numerrors] = NULL; \
		(error) = ierror[numerrors--]; \
	}

#ifdef IS_ERROR
#  undef IS_ERROR
#endif
#define IS_ERROR() \
	(numerrors != -1) ? 1 : 0

#endif
