/*
 *  gui.h
 *
 *  $Id$
 *
 *  The iODBC driver manager.
 *  
 *  Copyright (C) 1999-2009 by OpenLink Software <iodbc@openlinksw.com>
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

#include <Carbon/Carbon.h>

#ifndef	_MACXGUI_H
#define	_MACXGUI_H

/* The column values for data browsers */
#define DBITEM_ID	'OPLs'
#define GSKEYWORD_ID	'keyw'
#define GSVALUE_ID	'valu'

#define	TABS_SIGNATURE	'tabs'
#define	CNTL_SIGNATURE	'CNTL'

#define GETCONTROLBYID(ctlID, ctlSIGN, ctl, wndREF, ctrlREF) { \
    ctlID.signature = ctlSIGN; \
    ctlID.id = ctl; \
    GetControlByID(wndREF, &ctlID, &ctrlREF); \
}

typedef struct TLOGIN
{
  ControlRef username, password;
  WindowRef mainwnd;
  char *user, *pwd;
  BOOL ok;
}
TLOGIN;

typedef struct TCONFIRM
{
  WindowRef mainwnd;
  BOOL yes_no;
}
TCONFIRM;

typedef struct TGENSETUP
{
  ControlRef dsn_entry, comment_entry, key_list, bupdate;
  ControlRef key_entry, value_entry;
  WindowRef mainwnd;
  char *connstr;
}
TGENSETUP;
#endif
