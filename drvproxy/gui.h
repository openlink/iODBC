/*
 *  gui.h
 *
 *  $Id$
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

#include <config.h>
#include <iodbcinst.h>

#if defined(__BEOS__)
#include "be/gui.h"
#elif defined(macintosh)
#include "mac/gui.h"
#elif defined(__GTK__)
#include "gtk/gui.h"
#elif defined(__QT__)
#include "qt/gui.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef	_GUI_H
#define _GUI_H

BOOL create_confirm (HWND hwnd, LPCSTR dsn, LPCSTR text);
LPSTR create_oplsetup (HWND hwnd, LPCSTR dsn, LPCSTR attrs, BOOL add);
LPSTR create_virtsetup (HWND hwnd, LPCSTR dsn, LPCSTR attrs, BOOL add);
LPSTR create_gensetup (HWND hwnd, LPCSTR dsn, LPCSTR attrs, BOOL add);
void create_login (HWND hwnd, LPCSTR username, LPCSTR password, LPCSTR dsn, TLOGIN *log_t);

#ifdef __cplusplus
}
#endif

#endif
