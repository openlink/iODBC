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

#include <iodbc.h>
#include "iodbcadm.h"

#if defined(__BEOS__)
#  include "be/gui.h"
#elif defined(_MAC)
#  include "mac/gui.h"
#elif defined(__GTK__)
#  include "gtk/gui.h"
#elif defined(_MACX)
#  include "macosx/gui.h"
#else
#  error GUI for this platform not supported ...
#endif

#ifndef	_GUI_H
#define _GUI_H

#ifdef _MACX
BOOL create_confirmadm (HWND hwnd, LPCSTR dsn, LPCSTR text);
#else
BOOL create_confirm (HWND hwnd, LPCSTR dsn, LPCSTR text);
#endif

void create_login (HWND hwnd, LPCSTR username, LPCSTR password, LPCSTR dsn, TLOGIN *log_t);
void create_dsnchooser (HWND hwnd, TDSNCHOOSER *choose_t);
void create_driverchooser (HWND hwnd, TDRIVERCHOOSER *choose_t);
void create_administrator (HWND hwnd);
void create_error (HWND hwnd, LPCSTR dsn, LPCSTR text, LPCSTR errmsg);
void create_message (HWND hwnd, LPCSTR dsn, LPCSTR text);
LPSTR create_driversetup (HWND hwnd, LPCSTR driver, LPCSTR attrs, BOOL add);
LPSTR create_filedsn (HWND hwnd);
LPSTR create_connectionpool (HWND hwnd, LPCSTR driver, LPCSTR oldtimeout);

#endif
