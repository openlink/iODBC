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
#include <iodbc.h>
#include <iodbcinst.h>
#include <gtk/gtk.h>

#ifndef	_GTKGUI_H
#define _GTKGUI_H

typedef struct TLOGIN {
  GtkWidget *username,*password,*mainwnd;
  char *user, *pwd;
} TLOGIN;

typedef struct TOPLSETUP
{
	GtkWidget *dsn_entry, *comment_entry, *host_entry, *db_entry;
	GtkWidget *server_entry, *user_entry, *bufsize_entry;
	GtkWidget *readonly_chk, *nologbox_chk, *type_cb, *protocol_cb;
	GtkWidget *mainwnd;
	LPSTR connstr;
} TOPLSETUP;

typedef struct TVIRTSETUP
{
	GtkWidget *dsn_entry, *comment_entry, *host_entry, *user_entry;
	GtkWidget *database_cb,*mainwnd;
	LPSTR connstr;
} TVIRTSETUP;

typedef struct TGENSETUP
{
	GtkWidget *dsn_entry, *comment_entry, *key_list, *bupdate;
	GtkWidget *key_entry, *value_entry;
	GtkWidget *mainwnd;
	LPSTR connstr;
} TGENSETUP;

typedef struct TCONFIRM {
  GtkWidget *mainwnd;
  BOOL yes_no;
} TCONFIRM;

#endif
