/*
 *  dlproc.h
 *
 *  $Id$
 *
 *  Load driver and resolve driver's function entry point
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
#ifndef	_DLPROC_H
#define	_DLPROC_H

#include <dlf.h>

#if defined(_MAC)
typedef SQLRETURN (FAR * HPROC) (...);
#else
typedef SQLRETURN (FAR * HPROC) ();
#endif

#ifdef	DLDAPI_SVR4_DLFCN
#include <dlfcn.h>
#endif

#ifdef DLDAPI_HP_SHL
#include <dl.h>
typedef shl_t HDLL;
#endif

#if defined(_BE)		|| \
    defined(_MAC)		|| \
    defined(_MACX)		|| \
    defined(DLDAPI_AIX_LOAD)	|| \
    defined(DLDAPI_DYLD)	|| \
    defined(DLDAPI_SVR4_DLFCN)	|| \
    defined(VMS)
typedef void *HDLL;
#endif

extern HPROC _iodbcdm_getproc (HDBC hdbc, int idx);
extern HDLL _iodbcdm_dllopen (char FAR * dll);
extern HPROC _iodbcdm_dllproc (HDLL hdll, char FAR * sym);
extern char FAR *_iodbcdm_dllerror ();
extern int _iodbcdm_dllclose (HDLL hdll);

#define	SQL_NULL_HDLL	((HDLL)NULL)
#define	SQL_NULL_HPROC	((HPROC)NULL)
#endif
