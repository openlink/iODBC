/*
 *  ithread.h
 *
 *  $Id$
 *
 *  Macros for locking & multihreading
 *
 *  The iODBC driver manager.
 *
 *  (C)Copyright 2000 OpenLink Software.
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
#ifndef _ITHREAD_H
#define _ITHREAD_H


/*
 *  Threading under windows
 */
#if defined (WIN32) && !defined (NO_THREADING)

# define IODBC_THREADING

# define THREAD_IDENT			((unsigned long) GetCurrentThreadId())

# define MUTEX_DECLARE(M)		HANDLE M
# define MUTEX_INIT(M)			M = CreateMutex (NULL, FALSE, NULL)
# define MUTEX_DONE(M)			CloseHandle (M)
# define MUTEX_LOCK(M)			WaitForSingleObject (M, INFINITE)
# define MUTEX_UNLOCK(M)		ReleaseMutex (M)

# define SPINLOCK_DECLARE(M)		CRITICAL_SECTION M
# define SPINLOCK_INIT(M)		InitializeCriticalSection (&M)
# define SPINLOCK_DONE(M)		DeleteCriticalSection (&M)
# define SPINLOCK_LOCK(M)		EnterCriticalSection (&M)
# define SPINLOCK_UNLOCK(M)		LeaveCriticalSection (&M)


/*
 *  Threading with pthreads
 */
#elif defined (WITH_PTHREADS)

#ifndef _REENTRANT
# error Add -D_REENTRANT to your compiler flags
#endif

#include <pthread.h>

# define IODBC_THREADING

# ifndef OLD_PTHREADS
#  define THREAD_IDENT			((unsigned long) (pthread_self ()))
# else
#  define THREAD_IDENT			0UL
# endif

# define MUTEX_DECLARE(M)		pthread_mutex_t M
# define MUTEX_INIT(M)			pthread_mutex_init (&M, NULL)
# define MUTEX_DONE(M)			pthread_mutex_destroy (&M);
# define MUTEX_LOCK(M)			pthread_mutex_lock(&M)
# define MUTEX_UNLOCK(M)		pthread_mutex_unlock(&M)

# define SPINLOCK_DECLARE(M)		MUTEX_DECLARE(M)
# define SPINLOCK_INIT(M)		MUTEX_INIT(M)
# define SPINLOCK_DONE(M)		MUTEX_DONE(M)
# define SPINLOCK_LOCK(M)		MUTEX_LOCK(M)
# define SPINLOCK_UNLOCK(M)		MUTEX_UNLOCK(M)


/*
 *  No threading
 */
#else
	
# undef IODBC_THREADING

# undef THREAD_IDENT

# define MUTEX_DECLARE(M)		int M
# define MUTEX_INIT(M)			M = 1
# define MUTEX_DONE(M)			M = 1
# define MUTEX_LOCK(M)			M = 1
# define MUTEX_UNLOCK(M)		M = 1

# define SPINLOCK_DECLARE(M)		MUTEX_DECLARE (M)
# define SPINLOCK_INIT(M)		MUTEX_INIT (M)
# define SPINLOCK_DONE(M)		MUTEX_DONE (M)
# define SPINLOCK_LOCK(M)		MUTEX_LOCK (M)
# define SPINLOCK_UNLOCK(M)		MUTEX_UNLOCK (M)

#endif

#endif /* _ITHREAD_H */
