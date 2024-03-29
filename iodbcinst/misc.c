/*
 *  misc.c
 *
 *  $Id$
 *
 *  Miscellaneous functions
 *
 *  The iODBC driver manager.
 *
 *  Copyright (C) 1996-2024 OpenLink Software <iodbc@openlinksw.com>
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
#include <odbcinst.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "inifile.h"
#include "misc.h"

#ifdef _MAC
# include <getfpn.h>
#endif /* endif _MAC */

WORD wSystemDSN = USERDSN_ONLY;
WORD configMode = ODBC_BOTH_DSN;


#if !defined(WINDOWS) && !defined(WIN32) && !defined(OS2) && !defined(macintosh)
# include <pwd.h>
# define UNIX_PWD
#endif


/*
 * Algorithm for resolving an odbc.ini reference
 *
 * For UNIX :    1. Check for $ODBCINI variable, if exists return $ODBCINI.
 *               2. Check for $HOME/.odbc.ini or ~/.odbc.ini file,
 *                  if exists return it.
 *               3. Check for SYS_ODBC_INI build variable, if exists return
 *                  it. (ie : /etc/odbc.ini).
 *               4. No odbc.ini presence, return NULL.
 *
 * For WINDOWS, WIN32, OS2 :
 *               1. Check for the system odbc.ini file, if exists return it.
 *               2. No odbc.ini presence, return NULL.
 *
 * For VMS:      1. Check for $ODBCINI variable, if exists return $ODBCINI.
 *               2. Check for SYS$LOGIN:ODBC.INI file, if exists return it.
 *               3. No odbc.ini presence, return NULL.
 *
 * For Mac:      1. On powerPC, file is ODBC Preferences PPC
 *                  On 68k, file is ODBC Preferences
 *               2. Check for ...:System Folder:Preferences:ODBC Preferences
 *                  file, if exists return it.
 *               3. No odbc.ini presence, return NULL.
 *
 * For MacX:     1. Check for $ODBCINI variable, if exists return $ODBCINI.
 *               2. Check for $HOME/Library/ODBC/odbc.ini, 
 *                  if exists return it.
 *               3. Check for SYS_ODBC_INI build variable, if exists return
 *                  it. (ie : /etc/odbc.ini).
 *               4. Check for /Library/ODBC/odbc.ini
 *                  file, if exists return it.
 *               5. No odbc.ini presence, return NULL.
**/
char *
_iodbcadm_getinifile (char *buf, int size, int bIsInst, int doCreate)
{
#ifdef _MAC
  HParamBlockRec hp;
  long fldrDid;
  short fldrRef;
  OSErr result;
#endif /* endif _MAC */
  int i, j;
  char *ptr;

#ifdef _MAC
#  ifdef __POWERPC__
  j = STRLEN (bIsInst ? ":ODBC Installer Preferences PPC" :
      ":ODBC Preferences PPC") + 1;
#  else
  j = STRLEN (bIsInst ? ":ODBC Installer Preferences" : ":ODBC Preferences") +
      1;
#  endif /* endif __POWERPC__ */
#else
  j = STRLEN (bIsInst ? "/odbcinst.ini" : "/odbc.ini") + 1;
#endif /* endif _MAC */

  if (size < j)
    return NULL;

#if !defined(UNIX_PWD)
#  ifdef _MAC
  result =
      FindFolder (kOnSystemDisk, kPreferencesFolderType, kDontCreateFolder,
      &fldrRef, &fldrDid);
  if (result != noErr)
    return NULL;
  ptr = get_full_pathname (fldrDid, fldrRef);

  i = (ptr) ? STRLEN (ptr) : 0;
  if (i == 0 || i > size - j)
    {
      if (ptr)
	free (ptr);
      return NULL;
    }

#    ifdef __POWERPC__
  STRCPY (buf, ptr);
  STRCAT (buf,
      bIsInst ? ":ODBC Installer Preferences PPC" : ":ODBC Preferences PPC");
#    else
  STRCPY (buf, ptr);
  STRCAT (buf, bIsInst ? ":ODBC Installer Preferences" : ":ODBC Preferences");
#    endif /* endif __POWERPC__ */

  if (doCreate)
    {
      hp.fileParam.ioCompletion = NULL;
      hp.fileParam.ioVRefNum = fldrRef;
      hp.fileParam.ioDirID = fldrDid;
#    ifdef __POWERPC__
      hp.fileParam.ioNamePtr =
	  bIsInst ? "\pODBC Installer Preferences PPC" :
	  "\pODBC Preferences PPC";
#    else
      hp.fileParam.ioNamePtr =
	  bIsInst ? "\pODBC Installer Preferences" : "\pODBC Preferences";
#    endif
      PBHCreate (&hp, FALSE);
    }

  free (ptr);

  return buf;

#  else	/* else _MAC */

  /*
   *  On Windows, there is only one place to look
   */
  i = GetWindowsDirectory ((LPSTR) buf, size);

  if (i == 0 || i > size - j)
    return NULL;

  snprintf (buf + i, size - i, bIsInst ? "/odbcinst.ini" : "/odbc.ini");

  return buf;
#  endif /* endif _MAC */
#else
  if (wSystemDSN == USERDSN_ONLY)
    {
      /*
       *  1. Check $ODBCINI environment variable
       */
      if ((ptr = getenv (bIsInst ? "ODBCINSTINI" : "ODBCINI")) != NULL)
	{
	  STRNCPY (buf, ptr, size);

	  if (access (buf, R_OK) == 0)
	    return buf;
	  else if (doCreate)
	    {
	      int f = open ((char *) buf, O_CREAT,
		  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	      if (f != -1)
		{
		  close (f);
		  return buf;
		}
	    }
	}

#  ifdef VMS
      /*
       *  2a. VMS calls this HOME
       */
      STRNCPY (buf, bIsInst ? "SYS$LOGIN:ODBCINST.INI" : "SYS$LOGIN:ODBC.INI",
	  size);

      if (doCreate || access (buf, R_OK) == 0)
	return buf;
#  else	/* else VMS */

      if ((ptr = getenv ("HOME")) == NULL)
	{
	  ptr = (char *) getpwuid (getuid ());

	  if (ptr != NULL)
	    ptr = ((struct passwd *) ptr)->pw_dir;
	}

      if (ptr != NULL)
        {
#if defined(__APPLE__)
	  /*
	   * 2b. Try to check the ~/Library/ODBC/odbc.ini
	   */
	  snprintf (buf, size,
	      bIsInst ? "%s" ODBCINST_INI_APP : "%s" ODBC_INI_APP, ptr);

	  if (access (buf, R_OK) == 0)
	    return buf;
	  else if (doCreate)
	    {
	      int f = open ((char *) buf, O_CREAT,
		  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	      if (f != -1)
		{
		  close (f);
		  return buf;
		}
	    }
#   else /* else __APPLE__ */
      
          /*
           *  2b. Check either $HOME/.odbc.ini or ~/.odbc.ini
           */
	  snprintf (buf, size, bIsInst ? "%s/.odbcinst.ini" : "%s/.odbc.ini",
	      ptr);

	  if (doCreate || access (buf, R_OK) == 0)
	    return buf;
#   endif /* endif __APPLE__ */
#  endif /* endif VMS */
        }
    }

  /*
   *  3. Try SYS_ODBC_INI as the last resort
   */
  if (wSystemDSN == SYSTEMDSN_ONLY || bIsInst)
    {
      /*
       *  1. Check $SYSODBCINI environment variable
       */
      if ((ptr = getenv (bIsInst ? "SYSODBCINSTINI" : "SYSODBCINI")) != NULL)
	{
	  STRNCPY (buf, ptr, size);

	  if (access (buf, R_OK) == 0)
	    return buf;
	  else if (doCreate)
	    {
	      int f = open ((char *) buf, O_CREAT,
		  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	      if (f != -1)
		{
		  close (f);
		  return buf;
		}
	    }
	}

#if defined(__APPLE__)
      /*
       * Try to check the /Library/ODBC/odbc.ini
       */
      snprintf (buf, size, "%s", bIsInst ? ODBCINST_INI_APP : ODBC_INI_APP);

      if (access (buf, R_OK) == 0)
	return buf;
      else if (doCreate)
	{
	  int f = open ((char *) buf, O_CREAT,
	      S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	  if (f != -1)
	    {
	      close (f);
	      return buf;
	    }
	}
#   endif /* endif __APPLE__ */

      STRNCPY (buf, bIsInst ? SYS_ODBCINST_INI : SYS_ODBC_INI, size);
      return buf;
    }

  /*
   *  No ini file found or accessible
   */
  return NULL;
#endif /* UNIX_PWD */
}

const char *
_iodbcdm_check_for_string(const char *szList, const char *szString, int bContains)
{
  const char *currP = szList;

  while (*currP)
    {
      if (bContains)
	{
	  if (strstr (currP, szString))
	    return currP;
	}
      else
	{
	  if (!strcmp (currP, szString))
	    return currP;
	}
    }
  return NULL;
}


char *
_iodbcdm_remove_quotes(const char *szString)
{
  char *szWork, *szPtr;

  while (*szString == '\'' || *szString == '\"')
    szString += 1;

  if (!*szString)
    return NULL;
  szWork = strdup (szString);
  szPtr = strchr (szWork, '\'');
  if (szPtr)
    *szPtr = 0;
  szPtr = strchr (szWork, '\"');
  if (szPtr)
    *szPtr = 0;

  return szWork;
}

/*
 * Get FILEDSN file name
 *
 * If file name does not contain path component, the default directory for
 * saving and loading a .dsn file will be defined as follows:
 * 1. if FILEDSNPATH is set in environment: use environment
 * 2. if odbcinst.ini [odbc] FILEDSNPATH is set: use this
 * 3. else use DEFAULT_FILEDSNPATH
 *
 * ".dsn" extension is always appended to the resulting filename
 * (if not already exists).
 */
void
_iodbcdm_getdsnfile(const char *filedsn, char *buf, size_t buf_sz)
{
  char *p;
  char *default_path;

  if (strchr (filedsn, '/') != NULL)
    {
      /* has path component -- copy as is */
      _iodbcdm_strlcpy (buf, filedsn, buf_sz);
      goto done;
    }

  /* get default path */
  if ((default_path = getenv ("FILEDSNPATH")) != NULL)
    _iodbcdm_strlcpy (buf, default_path, buf_sz);
  else
    {
      SQLSetConfigMode (ODBC_BOTH_DSN);
      if (!SQLGetPrivateProfileString ("ODBC", "FileDSNPath", "",
				       buf, buf_sz, "odbcinst.ini"))
        _iodbcdm_strlcpy (buf, DEFAULT_FILEDSNPATH, buf_sz);
    }

  /* append filedsn file name */
  _iodbcdm_strlcat (buf, "/", buf_sz);
  _iodbcdm_strlcat (buf, filedsn, buf_sz);

done:
  /* append ".dsn" if extension is not ".dsn" */
  if ((p = strrchr (buf, '.')) == NULL ||
      strcasecmp (p, ".dsn") != 0)
    _iodbcdm_strlcat (buf, ".dsn", buf_sz);
}


/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 *
 * Taken from FreeBSD libc.
 */
size_t
_iodbcdm_strlcpy(char *dst, const char *src, size_t siz)
{
  char *d = dst;
  const char *s = src;
  size_t n = siz;

  /* Copy as many bytes as will fit */
  if (n != 0 && --n != 0)
    {
      do {
        if ((*d++ = *s++) == 0)
          break;
      } while (--n != 0);
    }

  /* Not enough room in dst, add NUL and traverse rest of src */
  if (n == 0)
    {
      if (siz != 0)
        *d = '\0';		/* NUL-terminate dst */
      while (*s++)
        ;
    }

   return(s - src - 1);		/* count does not include NUL */
}


/*
 * Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 * Returns strlen(src) + MIN(siz, strlen(initial dst)).
 * If retval >= siz, truncation occurred.
 *
 * Taken from FreeBSD libc.
 */
size_t
_iodbcdm_strlcat(char *dst, const char *src, size_t siz)
{
  char *d = dst;
  const char *s = src;
  size_t n = siz;
  size_t dlen;

  /* Find the end of dst and adjust bytes left but don't go past end */
  while (n-- != 0 && *d != '\0')
    d++;
  dlen = d - dst;
  n = siz - dlen;

  if (n == 0)
    return(dlen + strlen(s));
  while (*s != '\0')
    {
      if (n != 1)
        {
          *d++ = *s;
          n--;
        }
      s++;
    }
  *d = '\0';

  return(dlen + (s - src));	/* count does not include NUL */
}
