/*
 *  iodbcadm.h
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

#ifdef _MACX
#  include <iODBCinst/iodbcinst.h>
#else
#  include <iodbcinst.h>
#endif

#ifndef	_IODBCADM_H
#define	_IODBCADM_H

SQLRETURN SQL_API _iodbcdm_loginbox (HWND hwnd, LPSTR szInOutConnStr,
    DWORD cbInOutConnStr, int FAR * sqlStat);
SQLRETURN SQL_API _iodbcdm_drvconn_dialbox (HWND hwnd, LPSTR szInOutConnStr,
    DWORD cbInOutConnStr, int FAR * sqlStat);
SQLRETURN SQL_API _iodbcdm_drvchoose_dialbox (HWND hwnd, LPSTR szInOutDrvStr,
    DWORD cbInOutDrvStr, int FAR * sqlStat);
SQLRETURN SQL_API _iodbcdm_trschoose_dialbox (HWND hwnd, LPSTR szInOutDrvStr,
    DWORD cbInOutDrvStr, int FAR * sqlStat);

void SQL_API _iodbcdm_errorbox (HWND hwnd, LPCSTR szDSN, LPCSTR szText);
void SQL_API _iodbcdm_messagebox (HWND hwnd, LPCSTR szDSN, LPCSTR szText);

SQLRETURN SQL_API _iodbcdm_admin_dialbox (HWND hwnd);

typedef SQLRETURN SQL_API (*pDrvConnFunc) (HWND hwnd, LPSTR szInOutConnStr,
    DWORD cbInOutConnStr, int FAR * sqlStat);

typedef SQLRETURN SQL_API (*pLoginFunc) (HWND hwnd, LPSTR szInOutConnStr,
    DWORD cbInOutConnStr, int FAR * sqlStat);

#endif
