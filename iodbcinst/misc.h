/*
 *  misc.h
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
#ifndef _MISC_H
#define _MISC_H

char *_iodbcadm_getinifile (char *buf, int size, int bIsInst, int doCreate);
const char *_iodbcdm_check_for_string (const char *szList,
    const char *szString, int bContains);
char *_iodbcdm_remove_quotes (const char *szString);

extern WORD wSystemDSN;
extern WORD configMode;

#endif
