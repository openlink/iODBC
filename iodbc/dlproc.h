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
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */
#ifndef	_DLPROC_H
#define	_DLPROC_H

#include	<dlf.h>

typedef RETCODE (FAR * HPROC) ();

#ifdef	DLDAPI_SVR4_DLFCN
#include	<dlfcn.h>
typedef void *HDLL;
#endif

#ifdef DLDAPI_HP_SHL
#include	<dl.h>
typedef shl_t HDLL;
#endif

#ifdef	DLDAPI_AIX_LOAD
typedef void *HDLL;
#endif

extern HPROC _iodbcdm_getproc ();
extern HDLL _iodbcdm_dllopen (char FAR * dll);
extern HPROC _iodbcdm_dllproc (HDLL hdll, char FAR * sym);
extern char FAR *_iodbcdm_dllerror ();
extern int _iodbcdm_dllclose (HDLL hdll);

#define	SQL_NULL_HDLL	((HDLL)NULL)
#define	SQL_NULL_HPROC	((HPROC)NULL)
#endif
