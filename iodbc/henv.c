/*
 *  henv.c
 *
 *  $Id$
 *
 *  Environment object management functions
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

#include <iodbc.h>

#include <sql.h>
#include <sqlext.h>

#include <dlproc.h>

#include <herr.h>
#include <henv.h>

#include <itrace.h>



/*
 *  Use static initializer where possible
 */
#if defined (PTHREAD_MUTEX_INITIALIZER)
SPINLOCK_DECLARE (iodbcdm_global_lock) = PTHREAD_MUTEX_INITIALIZER;
#else
SPINLOCK_DECLARE (iodbcdm_global_lock);
#endif

static int _iodbcdm_initialized = 0;
static void Init_iODBC();
static void Done_iODBC();


SQLRETURN SQL_API
SQLAllocEnv (SQLHENV FAR * phenv)
{
  GENV_t FAR *genv;

  /* 
   *  One time initialization
   */
  if (!_iodbcdm_initialized)
      Init_iODBC();

  genv = (GENV_t *) MEM_ALLOC (sizeof (GENV_t));

  if (genv == NULL)
    {
      *phenv = SQL_NULL_HENV;

      return SQL_ERROR;
    }
  genv->rc = 0;

  /*
   *  Initialize this handle
   */
  genv->type = SQL_HANDLE_ENV;
  genv->henv = SQL_NULL_HENV;	/* driver's env list */
  genv->hdbc = SQL_NULL_HDBC;	/* driver's dbc list */
  genv->herr = SQL_NULL_HERR;	/* err list          */
#if (ODBCVER >= 0x300)
  genv->odbc_ver = SQL_OV_ODBC2;
#endif
  genv->err_rec = 0;

  *phenv = (SQLHENV) genv;

  return SQL_SUCCESS;
}


SQLRETURN SQL_API
SQLFreeEnv (SQLHENV henv)
{
  GENV (genv, henv);

  ODBC_LOCK();

  if (!IS_VALID_HENV (genv))
    {
      ODBC_UNLOCK();

      return SQL_INVALID_HANDLE;
    }
  CLEAR_ERRORS (genv);

  if (genv->hdbc != SQL_NULL_HDBC)
    {
      PUSHSQLERR (genv->herr, en_S1010);

      ODBC_UNLOCK();

      return SQL_ERROR;
    }

  /*
   *  Invalidate this handle
   */
  genv->type = 0;

  MEM_FREE (genv);

  ODBC_UNLOCK();

  return SQL_SUCCESS;
}


/*
 *  Initialize the system and let everyone wait until we have done so
 *  properly
 */
static void
Init_iODBC ()
{
#if !defined (PTHREAD_MUTEX_INITIALIZER) || defined (WINDOWS)
  SPINLOCK_INIT (iodbcdm_global_lock);
#endif

  SPINLOCK_LOCK (iodbcdm_global_lock);
  if (!_iodbcdm_initialized)
    {
      /*
       *  Other one time initializations can be performed here
       */

      /*
       *  OK, now flag we are not callable anymore and return
       */
      _iodbcdm_initialized = 1;
    }
  SPINLOCK_UNLOCK (iodbcdm_global_lock);

  return;
}


static void 
Done_iODBC()
{
    SPINLOCK_DONE (iodbcdm_global_lock);
}


/*
 *  DLL Entry points for Windows
 */
#if defined (WINDOWS)
STATIC int
DLLInit (HINSTANCE hModule)
{
  Init_iODBC ();

  return TRUE;
}


STATIC void
DLLExit (void)
{
  Done_iODBC ();
}


#pragma argused
BOOL WINAPI
DllMain (HINSTANCE hModule, DWORD fdReason, LPVOID lpvReserved)
{
  switch (fdReason)
    {
    case DLL_PROCESS_ATTACH:
      if (!DLLInit (hModule))
	return FALSE;
      break;
    case DLL_PROCESS_DETACH:
      DLLExit ();
    }
  return TRUE;
}
#endif
