/*
 *  misc.c
 *
 *  $Id$
 *
 *  Miscellaneous functions
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
#include <iodbcinst.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "inifile.h"
#include "misc.h"

#ifdef _MAC
# include <getfpn.h>
#endif /* endif _MAC */

WORD wSystemDSN = USERDSN_ONLY;
WORD configMode = ODBC_BOTH_DSN;

static int
upper_strneq (char *s1, char *s2, int n)
{
  int i;
  char c1, c2;

  for (i = 1; i < n; i++)
    {
      c1 = s1[i];
      c2 = s2[i];

      if (c1 >= 'a' && c1 <= 'z')
	{
	  c1 += ('A' - 'a');
	}
      else if (c1 == '\n')
	{
	  c1 = '\0';
	}

      if (c2 >= 'a' && c2 <= 'z')
	{
	  c2 += ('A' - 'a');
	}
      else if (c2 == '\n')
	{
	  c2 = '\0';
	}

      if ((c1 - c2) || !c1 || !c2)
	{
	  break;
	}
    }

  return (int) !(c1 - c2);
}


static char *			/* return new position in input str */
readtoken (
    char *istr,			/* old position in input buf */
    char *obuf)			/* token string ( if "\0", then finished ) */
{
  char *start = obuf;

  /* Skip leading white space */
  while (*istr == ' ' || *istr == '\t')
    istr++;

  for (; *istr && *istr != '\n'; istr++)
    {
      char c, nx;

      c = *(istr);
      nx = *(istr + 1);

      if (c == ';')
	{
	  for (; *istr && *istr != '\n'; istr++);
	  break;
	}
      *obuf = c;
      obuf++;

      if (nx == ';' || nx == '=' || c == '=')
	{
	  istr++;
	  break;
	}
    }
  *obuf = '\0';

  /* Trim end of token */
  for (; obuf > start && (*(obuf - 1) == ' ' || *(obuf - 1) == '\t');)
    *--obuf = '\0';

  return istr;
}


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
 *               2. Check for $HOME/.odbc.ini or ~/.odbc.ini file, if exists
 *                  return it.
 *               3. Check for $HOME/Library/Preferences/ODBC.preference or
 *                  ~/.odbc.ini file, if exists return it.
 *               4. Check for SYS_ODBC_INI build variable, if exists return
 *                  it. (ie : /etc/odbc.ini).
 *               5. Check for /System/Library/Preferences/ODBC.preference
 *                  file, if exists return it.
 *               6. No odbc.ini presence, return NULL.
 */
char *
_iodbcadm_getinifile (char *buf, int size, int bIsInst, int doCreate)
{
#ifdef _MAC
  OSErr result;
  long fldrDid;
  short fldrRef;
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

	  if (doCreate || access (buf, R_OK) == 0)
	    return buf;
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
      /*
       *  2b. Check either $HOME/.odbc.ini or ~/.odbc.ini
       */
      if ((ptr = getenv ("HOME")) == NULL)
	{
	  ptr = (char *) getpwuid (getuid ());

	  if (ptr != NULL)
	    ptr = ((struct passwd *) ptr)->pw_dir;
	}

      if (ptr != NULL)
	{
	  snprintf (buf, size, bIsInst ? "%s/.odbcinst.ini" : "%s/.odbc.ini",
	      ptr);

	  if (doCreate || access (buf, R_OK) == 0)
	    return buf;

#   ifdef _MACX
	  /*
	   * Try to check the ~/Library/Preferences/ODBC.preference
	   */
	  snprintf (buf, size,
	      bIsInst ? "%s" ODBCINST_INI_APP : "%s" ODBC_INI_APP, ptr);

	  if (doCreate || access (buf, R_OK) == 0)
	    return buf;
#   endif /* endif _MACX */
	}

#  endif /* endif VMS */
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

	  if (doCreate || access (buf, R_OK) == 0)
	    return buf;
	}

      STRNCPY (buf, bIsInst ? SYS_ODBCINST_INI : SYS_ODBC_INI, size);

      if (doCreate || access (buf, R_OK) == 0)
	return buf;

#   ifdef _MACX
      /*
       * Try to check the /System/Library/Preferences/ODBC.preference
       */
      snprintf (buf, size, "/System%s",
	  bIsInst ? ODBCINST_INI_APP : ODBC_INI_APP);

      if (access (buf, R_OK) == 0)
	return buf;
#   endif /* endif _MACX */
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
  const char *currP = szList, *ret;

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
