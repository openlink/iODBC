/*
 *  gui.h
 *
 *  $Id$
 *
 *  The iODBC driver manager.
 *
 *  Copyright (C) 1996-2015 by OpenLink Software <iodbc@openlinksw.com>
 *  All Rights Reserved.
 *
 *  This software is released under the terms of either of the following
 *  licenses:
 *
 *      - GNU Library General Public License (see LICENSE.LGPL)
 *      - The BSD License (see LICENSE.BSD).
 *
 *  Note that the only valid version of the LGPL license as far as this
 *  project is concerned is the original GNU Library General Public License
 *  Version 2, dated June 1991.
 *
 *  While not mandated by the BSD license, any patches you make to the
 *  iODBC source code may be contributed back into the iODBC project
 *  at your discretion. Contributions will benefit the Open Source and
 *  Data Access community as a whole. Submissions may be made at:
 *
 *      http://www.iodbc.org
 *
 *
 *  GNU Library Generic Public License Version 2
 *  ============================================
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; only
 *  Version 2 of the License dated June 1991.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 *  The BSD License
 *  ===============
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *  3. Neither the name of OpenLink Software Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL OPENLINK OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iodbc.h>
#include <iodbcinst.h>
#include <Carbon/Carbon.h>

#ifndef	_MACXGUI_H
#define	_MACXGUI_H

#define USER_DSN        0
#define SYSTEM_DSN      1
#define FILE_DSN        2

/* The column values for data browsers */
#define DBITEM_ID	'OPLs'
#define DBNAME_ID	'name'
#define DBVERSION_ID	'vers'
#define DBFILE_ID	'file'
#define DBDATE_ID	'date'
#define DBSIZE_ID	'size'
#define CPTIMEOUT_ID	' to '
#define CPPROBE_ID	'sql '
#define DSNDRV_ID	'drv '
#define DSNDESC_ID	'desc'

#define	TABS_SIGNATURE	'tabs'
#define	CNTL_SIGNATURE	'CNTL'
#define PICT_SIGNATURE	'PICT'

#define GSKEYWORD_ID	'keyw'
#define GSVALUE_ID	'valu'


/* The global tab control */
#define GLOBAL_TAB	128
#define NUMBER_TAB	7
#define OKBUT_CNTL	149
#define CANCELBUT_CNTL	150

/* All user tab items */
#define	UDSN_TAB 	129
#define ULIST_CNTL	128
#define UADD_CNTL	130
#define UDEL_CNTL	131
#define UCFG_CNTL	132
#define UTST_CNTL	133

/* All system tab items */
#define SDSN_TAB	130
#define SLIST_CNTL	129
#define SADD_CNTL	134
#define SDEL_CNTL	135
#define SCFG_CNTL	136
#define STST_CNTL	137

/* All fileDsn tab items */
#define FDSN_TAB	155
#define FLIST_CNTL	156
#define FADD_CNTL	157
#define FDEL_CNTL	158
#define FCFG_CNTL	159
#define FTST_CNTL	160
#define FDIR_CNTL       161
#define FSETDIR_CNTL    162

/* All driver tab items */
#define DRIVER_TAB	131
#define DLIST_CNTL	138
#define DADD_CNTL	139
#define DDEL_CNTL	140
#define DCFG_CNTL	141

/* All connection pooling tab items */
#define POOL_TAB	132
#define	PLIST_CNTL	142
#define PPERF_CNTL	143
#define PRETR_CNTL	144

/* All tracing tab items */
#define	TRACE_TAB	133
#define TLOG_CNTL	145
#define TTRLIB_CNTL	146
#define TOPTI_CNTL	147
#define TSTART_CNTL	151
#define TLOGB_CNTL	152
#define TLIBB_CNTL	153
#define TWIDE_CNTL	154

/* All about tab items */
#define ABOUT_TAB	134
#define CLIST_CNTL	148

/* Control IDs for the connection pool dialog */
#define	CPFINISH_CNTL	1000
#define CPCANCEL_CNTL	1001
#define CPTIMEOUT_CNTL	1002
#define CPPROBE_CNTL	1003
#define CPGROUP_CNTL    1004
#define CPTITLE_CNTL    1005

/* Control IDs for the driver list dialog */
#define	DCFINISH_CNTL	1000
#define DCCANCEL_CNTL	1001
#define DCLIST_CNTL	2000


#define GETCONTROLBYID(ctlID, ctlSIGN, ctl, wndREF, ctrlREF) { \
    ctlID.signature = ctlSIGN; \
    ctlID.id = ctl; \
    GetControlByID(wndREF, &ctlID, &ctrlREF); \
}

extern char *szDSNColumnNames[];
extern char *szTabNames[];
extern char *szDSNButtons[];
extern char *szDriverColumnNames[];

typedef struct TFILEDSN
{
  ControlRef name_entry;
  WindowRef mainwnd;
  wchar_t *name;
}
TFILEDSN;

typedef struct TDSNCHOOSER
{
  ControlRef udsnlist, sdsnlist, fdsnlist;
  ControlRef uadd, uremove, utest, uconfigure;
  ControlRef sadd, sremove, stest, sconfigure;
  ControlRef fadd, fremove, ftest, fconfigure;
  ControlRef fdir;
  ControlRef dir_list, file_list, file_entry, dir_combo;
  WindowRef mainwnd;
  wchar_t *dsn;
  wchar_t *fdsn;
  char curr_dir[1024];
  int type_dsn;
}
TDSNCHOOSER;

typedef struct TDRIVERCHOOSER
{
  ControlRef driverlist, b_add, b_remove, b_configure;
  WindowRef mainwnd;
  wchar_t *driver;
}
TDRIVERCHOOSER;

typedef struct TFDRIVERCHOOSER
{
  ControlRef driverlist, dsn_entry;
  ControlRef back_button, continue_button;
  ControlRef tab_panel, mess_entry;
  WindowRef mainwnd;
  int tab_number;
  char *curr_dir;
  char *attrs;
  char *dsn;
  BOOL verify_conn;
  wchar_t *driver;
  BOOL ok;
}
TFDRIVERCHOOSER;

typedef struct TCONNECTIONPOOLING
{
  ControlRef driverlist, perfmon_rb, retwait_entry, timeout_entry, probe_entry;
  WindowRef mainwnd;
  BOOL changed;
  wchar_t *timeout;
}
TCONNECTIONPOOLING;

typedef struct TTRANSLATORCHOOSER
{
  ControlRef translatorlist, b_finish, b_cancel;
  WindowRef mainwnd;
  wchar_t *translator;
}
TTRANSLATORCHOOSER;

typedef struct TCOMPONENT
{
  ControlRef componentlist;
}
TCOMPONENT;

typedef struct TTRACING
{
  ControlRef logfile_entry, tracelib_entry, b_start_stop, trace_rb;
  ControlRef trace_wide;
  BOOL changed;
}
TTRACING;

typedef struct TCONFIRM
{
  WindowRef mainwnd;
  BOOL yes_no;
}
TCONFIRM;

typedef struct TDRIVERSETUP
{
  ControlRef name_entry, driver_entry, setup_entry, key_list, bupdate;
  ControlRef key_entry, value_entry, filesel, sysuser_rb;
  WindowRef mainwnd;
  LPWSTR connstr;
}
TDRIVERSETUP;

typedef struct TDRIVERREMOVE
{
  ControlRef user_cb, system_cb, dsn_rb;
  WindowRef mainwnd;
  BOOL deletedsn;
  int dsns;
}
TDRIVERREMOVE;

typedef struct TGENSETUP
{
  ControlRef dsn_entry, key_list, bupdate;
  ControlRef key_entry, value_entry;
  ControlRef verify_conn_cb;
  WindowRef mainwnd;
  char *connstr;
  BOOL verify_conn;
}
TGENSETUP;


typedef struct TKEYVAL
{
  ControlRef key_list, bupdate;
  ControlRef key_entry, value_entry;
  ControlRef verify_conn_cb;
  WindowRef mainwnd;
  char *connstr;
  BOOL verify_conn;
}
TKEYVAL;


extern char* convert_CFString_to_char(const CFStringRef str);

pascal OSStatus userdsn_add_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData);
pascal OSStatus userdsn_remove_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData);
pascal OSStatus userdsn_configure_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData);
pascal OSStatus userdsn_test_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData);
pascal OSStatus systemdsn_add_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData);
pascal OSStatus systemdsn_remove_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData);
pascal OSStatus systemdsn_configure_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData);
pascal OSStatus systemdsn_test_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData);
pascal OSStatus filedsn_add_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData);
pascal OSStatus filedsn_remove_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData);
pascal OSStatus filedsn_configure_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData);
pascal OSStatus filedsn_test_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData);
pascal OSStatus filedsn_setdir_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData);
pascal OSStatus filedsn_select_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData);


pascal OSStatus translators_getset_item (ControlRef browser,
    DataBrowserItemID itemID, DataBrowserPropertyID property,
    DataBrowserItemDataRef itemData, Boolean changeValue);
pascal OSStatus driver_configure_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData);
pascal OSStatus driverchooser_ok_clicked (EventHandlerCallRef inHandlerRef,
    EventRef inEvent, void *inUserData);
pascal OSStatus translatorchooser_ok_clicked (EventHandlerCallRef
    inHandlerRef, EventRef inEvent, void *inUserData);

void translators_notification_item (ControlRef browser,
    DataBrowserItemID itemID, DataBrowserItemNotification message);
void DisplayTabControlNumber (int index, ControlRef tabControl, WindowRef wnd,
    int *tabs);
void adddrivers_to_list (ControlRef widget, WindowRef dlg, BOOL addNotification);
void addtranslators_to_list (ControlRef widget, WindowRef dlg);
void adddsns_to_list (ControlRef widget, BOOL systemDSN, WindowRef dlg);
void addconnectionpool_to_list (ControlRef widget, WindowRef dlg);
void addfdsns_to_list (TDSNCHOOSER *dsnchoose_t, char *path, Boolean b_reset);

LPSTR create_keyval (WindowRef wnd, LPCSTR attrs, BOOL *verify_conn);
LPSTR create_gensetup (HWND hwnd, LPCSTR dsn, LPCSTR attrs, BOOL add, BOOL *verify_conn);
void create_fdriverchooser (HWND hwnd, TFDRIVERCHOOSER * choose_t);

#endif
