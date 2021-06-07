/*
 *  info.c
 *
 *  $Id$
 *
 *  Information functions
 *
 *  The iODBC driver manager.
 *
 *  Copyright (C) 1995 Ke Jin <kejin@empress.com>
 *  Copyright (C) 1996-2021 OpenLink Software <iodbc@openlinksw.com>
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

#include <sql.h>
#include <sqlext.h>
#include <sqlucode.h>
#include <odbcinst.h>

#include "unicode.h"

#include "dlproc.h"

#include "herr.h"
#include "henv.h"
#include "hdbc.h"
#include "hstmt.h"

#include "itrace.h"

#include <stdio.h>
#include <ctype.h>

#ifdef WIN32
#define SECT1			"ODBC 32 bit Data Sources"
#define SECT2			"ODBC 32 bit Drivers"
#else
#define SECT1			"ODBC Data Sources"
#define SECT2			"ODBC Drivers"
#endif

#define MAX_ENTRIES		1024


static int
stricmp (const char *s1, const char *s2)
{
  int cmp;

  while (*s1)
    {
      if ((cmp = toupper (*s1) - toupper (*s2)) != 0)
	return cmp;
      s1++;
      s2++;
    }
  return (*s2) ? -1 : 0;
}


static int
SectSorter (const void *p1, const void *p2)
{
  const char **s1 = (const char **) p1;
  const char **s2 = (const char **) p2;

  return stricmp (*s1, *s2);
}


SQLRETURN SQL_API
SQLDataSources_Internal (
  SQLHENV		  henv,
  SQLUSMALLINT		  fDir,
  SQLPOINTER		  szDSN,
  SQLSMALLINT		  cbDSNMax,
  SQLSMALLINT 		* pcbDSN,
  SQLPOINTER		  szDesc,
  SQLSMALLINT		  cbDescMax,
  SQLSMALLINT 		* pcbDesc,
  SQLCHAR		  waMode)
{
  GENV (genv, henv);
  char buffer[4096], desc[1024], *ptr;
  int i, j, usernum = 0;
  static int cur_entry = -1;
  static int num_entries = 0;
  static void **sect = NULL;
  SQLUSMALLINT fDirOld = fDir;

  waMode = waMode; /*UNUSED*/

  /* check argument */
  if (cbDSNMax < 0 || cbDescMax < 0)
    {
      PUSHSQLERR (genv->herr, en_S1090);
      return SQL_ERROR;
    }

  if (fDir != SQL_FETCH_FIRST && fDir != SQL_FETCH_NEXT &&
      fDir != SQL_FETCH_FIRST_USER && fDir != SQL_FETCH_FIRST_SYSTEM)
    {
      PUSHSQLERR (genv->herr, en_S1103);
      return SQL_ERROR;
    }

  if (cur_entry < 0 || fDir == SQL_FETCH_FIRST ||
      fDir == SQL_FETCH_FIRST_USER || fDir == SQL_FETCH_FIRST_SYSTEM)
    {
      cur_entry = 0;
      num_entries = 0;

      /*
       *  Free old section list
       */
      if (sect)
	{
	  for (i = 0; i < MAX_ENTRIES; i++)
	    if (sect[i])
	      free (sect[i]);
	  free (sect);
	}
      if ((sect = (void **) calloc (MAX_ENTRIES, sizeof (void *))) == NULL)
	{
	  PUSHSQLERR (genv->herr, en_S1011);
	  return SQL_ERROR;
	}

      if (fDirOld == SQL_FETCH_FIRST)
        fDir = SQL_FETCH_FIRST_USER;

      do {
        SQLSetConfigMode (fDir == SQL_FETCH_FIRST_SYSTEM ? ODBC_SYSTEM_DSN : ODBC_USER_DSN);
        SQLGetPrivateProfileString (SECT1, NULL, "", buffer, sizeof(buffer) / sizeof(SQLTCHAR), "odbc.ini");

        /* For each datasources */
        for(ptr = buffer, i = 1 ; *ptr && i ; ptr += STRLEN(ptr) + 1)
          {
            /* Add this section to the datasources list */
            if (fDirOld == SQL_FETCH_FIRST && fDir == SQL_FETCH_FIRST_SYSTEM)
              {
                for(j = 0 ; j<usernum ; j++)
                  {
                    if(STREQ(sect[j<<1], ptr))
                      j = usernum;
                  }
                if(j == usernum + 1)
                  continue;
              }

            if ((num_entries << 1) >= MAX_ENTRIES)
              {
                i = 0;
                break;
              }			/* Skip the rest */

            /* Copy the datasource name */
            sect[num_entries<<1] = STRDUP (ptr);

            /* ... and its description */
            SQLSetConfigMode (fDir == SQL_FETCH_FIRST_SYSTEM ? ODBC_SYSTEM_DSN : ODBC_USER_DSN);
            SQLGetPrivateProfileString (SECT1, ptr, "", desc, sizeof(desc) / sizeof(SQLTCHAR), "odbc.ini");
            sect[(num_entries++<<1) + 1] = STRDUP (desc);
          }

        switch(fDir)
          {
            case SQL_FETCH_FIRST_USER:
              fDir = SQL_FETCH_FIRST_SYSTEM;
              usernum = num_entries;
              break;
            case SQL_FETCH_FIRST_SYSTEM:
              fDir = SQL_FETCH_FIRST;
              break;
          };
      } while (fDir!=SQL_FETCH_FIRST && fDirOld==SQL_FETCH_FIRST);

      fDir = fDirOld;

      /*
       *  Sort all entries so we can present a nice list
       */
      if (num_entries > 1)
	{
          qsort (sect, num_entries, sizeof (char **) + sizeof (char **),
            SectSorter);
	}
    }

  /*
   *  Try to get to the next item
   */
  if (cur_entry >= num_entries)
    {
      cur_entry = 0;		/* Next time, start all over again */
      return SQL_NO_DATA_FOUND;
    }

  /*
   *  Copy DSN information 
   */
  STRNCPY (szDSN, sect[cur_entry << 1], cbDSNMax);

  if (pcbDSN)
    *pcbDSN = STRLEN (szDSN);

  /*
   *  And find the description that goes with this entry
   */
  STRNCPY (szDesc, sect[(cur_entry << 1) + 1], cbDescMax);

  if (pcbDesc)
    *pcbDesc = STRLEN (szDesc);

  /*
   *  Next record
   */
  cur_entry++;

  return SQL_SUCCESS;
}


SQLRETURN SQL_API
SQLDataSources (
  SQLHENV		  henv,
  SQLUSMALLINT		  fDir,
  SQLCHAR 		* szDSN,
  SQLSMALLINT		  cbDSNMax,
  SQLSMALLINT 		* pcbDSN,
  SQLCHAR 		* szDesc,
  SQLSMALLINT		  cbDescMax,
  SQLSMALLINT 		* pcbDesc)
{
  ENTER_HENV (henv,
    trace_SQLDataSources (TRACE_ENTER,
    	henv,
	fDir,
	szDSN, cbDSNMax, pcbDSN,
	szDesc, cbDescMax, pcbDesc));

  retcode = SQLDataSources_Internal (
  	henv, 
	fDir, 
	szDSN, cbDSNMax, pcbDSN,
	szDesc, cbDescMax, pcbDesc, 
	'A');

  LEAVE_HENV (henv,
    trace_SQLDataSources (TRACE_LEAVE,
    	henv,
	fDir,
	szDSN, cbDSNMax, pcbDSN,
	szDesc, cbDescMax, pcbDesc));
}


#if ODBCVER >= 0x0300
SQLRETURN SQL_API
SQLDataSourcesA (
  SQLHENV		  henv,
  SQLUSMALLINT		  fDir,
  SQLCHAR 		* szDSN,
  SQLSMALLINT		  cbDSNMax,
  SQLSMALLINT 		* pcbDSN,
  SQLCHAR 		* szDesc,
  SQLSMALLINT		  cbDescMax,
  SQLSMALLINT		* pcbDesc)
{
  ENTER_HENV (henv,
    trace_SQLDataSources (TRACE_ENTER,
    	henv,
	fDir,
	szDSN, cbDSNMax, pcbDSN,
	szDesc, cbDescMax, pcbDesc));

  retcode = SQLDataSources_Internal(
  	henv, 
	fDir, 
	szDSN, cbDSNMax, pcbDSN,
	szDesc, cbDescMax, pcbDesc, 
	'A');

  LEAVE_HENV (henv,
    trace_SQLDataSources (TRACE_LEAVE,
    	henv,
	fDir,
	szDSN, cbDSNMax, pcbDSN,
	szDesc, cbDescMax, pcbDesc));
}


SQLRETURN SQL_API
SQLDataSourcesW (
  SQLHENV		  henv,
  SQLUSMALLINT		  fDir,
  SQLWCHAR 		* szDSN,
  SQLSMALLINT		  cbDSNMax,
  SQLSMALLINT 		* pcbDSN,
  SQLWCHAR 		* szDesc,
  SQLSMALLINT		  cbDescMax,
  SQLSMALLINT 		* pcbDesc)
{
  SQLCHAR *_DSN = NULL;
  SQLCHAR *_Desc = NULL;
  GENV (glenv, henv);
  DM_CONV *conv = &glenv->conv;

  ENTER_HENV (henv,
    trace_SQLDataSourcesW (TRACE_ENTER,
    	henv,
	fDir,
	szDSN, cbDSNMax, pcbDSN,
	szDesc, cbDescMax, pcbDesc));

  if (cbDSNMax > 0)
    {
      if ((_DSN = (SQLCHAR *) malloc (cbDSNMax * UTF8_MAX_CHAR_LEN + 1)) == NULL)
	{
	  PUSHSQLERR (genv->herr, en_S1001);
	  return SQL_ERROR;
	}
    }

  if (cbDescMax > 0)
    {
      if ((_Desc = (SQLCHAR *) malloc (cbDescMax * UTF8_MAX_CHAR_LEN + 1)) == NULL)
	{
	  PUSHSQLERR (genv->herr, en_S1001);
	  return SQL_ERROR;
	}
    }

  retcode = SQLDataSources_Internal (
  	henv, 
	fDir, 
	_DSN, cbDSNMax * UTF8_MAX_CHAR_LEN, pcbDSN, 
	_Desc, cbDescMax * UTF8_MAX_CHAR_LEN, pcbDesc, 
	'W');

  if (SQL_SUCCEEDED (retcode))
    {
      dm_StrCopyOut2_U8toW_d2m (conv, _DSN, szDSN,
	cbDSNMax * DM_WCHARSIZE(conv), pcbDSN, NULL);
      dm_StrCopyOut2_U8toW_d2m (conv, _Desc, szDesc,
	cbDescMax * DM_WCHARSIZE(conv), pcbDesc, NULL);
    }

  MEM_FREE (_DSN);
  MEM_FREE (_Desc);

  LEAVE_HENV (henv,
    trace_SQLDataSourcesW (TRACE_LEAVE,
    	henv,
	fDir,
	szDSN, cbDSNMax, pcbDSN,
	szDesc, cbDescMax, pcbDesc));
}
#endif


SQLRETURN SQL_API
SQLDrivers_Internal (
  SQLHENV		  henv,
  SQLUSMALLINT		  fDir,
  SQLPOINTER		  szDrvDesc,
  SQLSMALLINT		  cbDrvDescMax,
  SQLSMALLINT 		* pcbDrvDesc,
  SQLPOINTER		  szDrvAttr,
  SQLSMALLINT		  cbDrvAttrMax,
  SQLSMALLINT 		* pcbDrvAttr,
  SQLCHAR		  waMode)
{
  GENV (genv, henv);
  char buffer[4096], desc[1024], *ptr;
  int i, j, usernum = 0;
  static int cur_entry = -1;
  static int num_entries = 0;
  static void **sect = NULL;
  SQLUSMALLINT fDirOld = fDir;

  waMode = waMode; /*UNUSED*/

  /* check argument */
  if (cbDrvDescMax < 0 || cbDrvAttrMax < 0)
    {
      PUSHSQLERR (genv->herr, en_S1090);
      return SQL_ERROR;
    }

  if (fDir != SQL_FETCH_FIRST && fDir != SQL_FETCH_NEXT)
    {
      PUSHSQLERR (genv->herr, en_S1103);
      return SQL_ERROR;
    }

  if (cur_entry < 0 || fDir == SQL_FETCH_FIRST)
    {
      cur_entry = 0;
      num_entries = 0;

      /*
      *  Free old section list
      */
      if (sect)
	{
	  for (i = 0; i < MAX_ENTRIES; i++)
	    if (sect[i])
	      free (sect[i]);
	  free (sect);
	}
      if ((sect = (void **) calloc (MAX_ENTRIES, sizeof (void *))) == NULL)
	{
	  PUSHSQLERR (genv->herr, en_S1011);
	  return SQL_ERROR;
	}

      if (fDirOld == SQL_FETCH_FIRST)
        fDir = SQL_FETCH_FIRST_USER;

      do {
        SQLSetConfigMode (fDir == SQL_FETCH_FIRST_SYSTEM ? ODBC_SYSTEM_DSN : ODBC_USER_DSN);
        SQLGetPrivateProfileString (SECT2, NULL, "", buffer, sizeof(buffer) / sizeof(SQLTCHAR), "odbcinst.ini");

        /* For each datasources */
        for(ptr = buffer, i = 1 ; *ptr && i ; ptr += STRLEN(ptr) + 1)
          {
            /* Add this section to the datasources list */
            if (fDirOld == SQL_FETCH_FIRST && fDir == SQL_FETCH_FIRST_SYSTEM)
              {
                for(j = 0 ; j<usernum ; j++)
                  {
                    if(STREQ(sect[j<<1], ptr))
                      j = usernum;
                  }
                if(j == usernum + 1)
                  continue;
              }

            if ((num_entries << 1) >= MAX_ENTRIES)
              {
                i = 0;
                break;
              }			/* Skip the rest */

            /* ... and its description */
            SQLSetConfigMode (fDir == SQL_FETCH_FIRST_SYSTEM ? ODBC_SYSTEM_DSN : ODBC_USER_DSN);
            SQLGetPrivateProfileString (SECT2, ptr, "", desc, sizeof(desc) / sizeof(SQLTCHAR), "odbcinst.ini");

            /* Check if the driver is installed */
				if(!STRCASEEQ(desc, "Installed"))
				  continue;

            /* Copy the driver name */
            sect[num_entries<<1] = STRDUP (ptr);
            sect[(num_entries++<<1) + 1] = STRDUP (desc);
          }

        switch(fDir)
          {
            case SQL_FETCH_FIRST_USER:
              fDir = SQL_FETCH_FIRST_SYSTEM;
              usernum = num_entries;
              break;
            case SQL_FETCH_FIRST_SYSTEM:
              fDir = SQL_FETCH_FIRST;
              break;
          };
      } while (fDir!=SQL_FETCH_FIRST && fDirOld==SQL_FETCH_FIRST);

      fDir = fDirOld;

      /*
       *  Sort all entries so we can present a nice list
       */
      if (num_entries > 1)
	{
          qsort (sect, num_entries, sizeof (char **) + sizeof (char **),
            SectSorter);
	}
    }

  /*
   *  Try to get to the next item
   */
  if (cur_entry >= num_entries)
    {
      cur_entry = 0;		/* Next time, start all over again */
      return SQL_NO_DATA_FOUND;
    }

  /*
   *  Copy Driver information 
   */
  STRNCPY (szDrvDesc, sect[cur_entry << 1], cbDrvDescMax);

  if (pcbDrvDesc)
    *pcbDrvDesc = STRLEN (szDrvDesc);

  /*
   *  And find the description that goes with this entry
   */
  STRNCPY (szDrvAttr, sect[(cur_entry << 1) + 1], cbDrvAttrMax);

  if (pcbDrvAttr)
    *pcbDrvAttr = STRLEN (szDrvAttr);

  /*
   *  Next record
   */
  cur_entry++;

  return SQL_SUCCESS;
}


SQLRETURN SQL_API
SQLDrivers (
  SQLHENV		  henv,
  SQLUSMALLINT		  fDir,
  SQLCHAR 		* szDrvDesc,
  SQLSMALLINT		  cbDrvDescMax,
  SQLSMALLINT 		* pcbDrvDesc,
  SQLCHAR 		* szDrvAttr,
  SQLSMALLINT		  cbDrvAttrMax,
  SQLSMALLINT 		* pcbDrvAttr)
{
  ENTER_HENV (henv,
    trace_SQLDrivers (TRACE_ENTER,
  	henv, 
	fDir, 
	szDrvDesc, cbDrvDescMax, pcbDrvDesc, 
	szDrvAttr, cbDrvAttrMax, pcbDrvAttr));

  retcode = SQLDrivers_Internal(
  	henv, 
	fDir, 
	szDrvDesc, cbDrvDescMax, pcbDrvDesc, 
	szDrvAttr, cbDrvAttrMax, pcbDrvAttr, 
	'A');

  LEAVE_HENV (henv,
    trace_SQLDrivers (TRACE_LEAVE,
  	henv, 
	fDir, 
	szDrvDesc, cbDrvDescMax, pcbDrvDesc, 
	szDrvAttr, cbDrvAttrMax, pcbDrvAttr));
}


#if ODBCVER >= 0x0300
SQLRETURN SQL_API
SQLDriversA (
  SQLHENV		  henv,
  SQLUSMALLINT		  fDir,
  SQLCHAR  		* szDrvDesc,
  SQLSMALLINT		  cbDrvDescMax,
  SQLSMALLINT 	 	* pcbDrvDesc,
  SQLCHAR  		* szDrvAttr,
  SQLSMALLINT		  cbDrvAttrMax,
  SQLSMALLINT 	 	* pcbDrvAttr)
{
  ENTER_HENV (henv,
    trace_SQLDrivers (TRACE_ENTER,
  	henv, 
	fDir, 
	szDrvDesc, cbDrvDescMax, pcbDrvDesc, 
	szDrvAttr, cbDrvAttrMax, pcbDrvAttr));

  retcode = SQLDrivers_Internal(
  	henv, 
	fDir, 
	szDrvDesc, cbDrvDescMax, pcbDrvDesc, 
	szDrvAttr, cbDrvAttrMax, pcbDrvAttr, 
	'A');

  LEAVE_HENV (henv,
    trace_SQLDrivers (TRACE_LEAVE,
  	henv, 
	fDir, 
	szDrvDesc, cbDrvDescMax, pcbDrvDesc, 
	szDrvAttr, cbDrvAttrMax, pcbDrvAttr));
}


SQLRETURN SQL_API
SQLDriversW (SQLHENV henv,
    SQLUSMALLINT 	  fDir,
    SQLWCHAR		* szDrvDesc,
    SQLSMALLINT		  cbDrvDescMax,
    SQLSMALLINT		* pcbDrvDesc,
    SQLWCHAR		* szDrvAttr,
    SQLSMALLINT		  cbDrvAttrMax,
    SQLSMALLINT		* pcbDrvAttr)
{
  SQLCHAR *_Driver = NULL;
  SQLCHAR *_Attrs = NULL;
  GENV (glenv, henv);
  DM_CONV *conv = &glenv->conv;

  ENTER_HENV (henv,
    trace_SQLDriversW (TRACE_ENTER,
  	henv, 
	fDir, 
	szDrvDesc, cbDrvDescMax, pcbDrvDesc, 
	szDrvAttr, cbDrvAttrMax, pcbDrvAttr));

  if (cbDrvDescMax > 0)
    {
      if ((_Driver = (SQLCHAR *) malloc (cbDrvDescMax * UTF8_MAX_CHAR_LEN + 1)) == NULL)
	{
	  PUSHSQLERR (genv->herr, en_S1001);
	  return SQL_ERROR;
	}
    }

  if (cbDrvAttrMax > 0)
    {
      if ((_Attrs = (SQLCHAR *) malloc (cbDrvAttrMax * UTF8_MAX_CHAR_LEN + 1)) == NULL)
	{
	  PUSHSQLERR (genv->herr, en_S1001);
	  return SQL_ERROR;
	}
    }

  retcode = SQLDrivers_Internal (
  	henv, 
	fDir, 
	_Driver, cbDrvDescMax * UTF8_MAX_CHAR_LEN, pcbDrvDesc, 
	_Attrs, cbDrvAttrMax * UTF8_MAX_CHAR_LEN, pcbDrvAttr, 
	'W');

  if (SQL_SUCCEEDED (retcode))
    {
      dm_StrCopyOut2_U8toW_d2m (conv, _Driver, szDrvDesc,
        cbDrvDescMax * DM_WCHARSIZE(conv), pcbDrvDesc, NULL);
      dm_StrCopyOut2_U8toW_d2m (conv, _Attrs, szDrvAttr,
        cbDrvAttrMax * DM_WCHARSIZE(conv), pcbDrvAttr, NULL);
    }

  MEM_FREE (_Driver);
  MEM_FREE (_Attrs);

  LEAVE_HENV (henv,
    trace_SQLDriversW (TRACE_LEAVE,
  	henv, 
	fDir, 
	szDrvDesc, cbDrvDescMax, pcbDrvDesc, 
	szDrvAttr, cbDrvAttrMax, pcbDrvAttr));
}
#endif


SQLRETURN SQL_API
SQLGetInfo_Internal (
    SQLHDBC		  hdbc,
    SQLUSMALLINT	  fInfoType,
    SQLPOINTER		  rgbInfoValue,
    SQLSMALLINT		  cbInfoValueMax,
    SQLSMALLINT		* pcbInfoValue,
    SQLCHAR		  waMode)
{
  CONN (pdbc, hdbc);
  ENVR (penv, pdbc->henv);
  STMT (pstmt, NULL);
  STMT (tpstmt, NULL);
  HPROC hproc = SQL_NULL_HPROC;
  SQLRETURN retcode = SQL_SUCCESS;
  void * _InfoValue = NULL;
  void * infoValueOut = rgbInfoValue;
  SQLPOINTER ptr = 0;
  SQLSMALLINT _cbInfoValueMax = cbInfoValueMax;
  CONV_DIRECT conv_direct = CD_NONE;
  DM_CONV *conv = &pdbc->conv;

  int size = 0, len = 0, ret = 0;
  wchar_t buf[20] = {'\0'};

  if (cbInfoValueMax < 0)
    {
      PUSHSQLERR (pdbc->herr, en_S1090);
      return SQL_ERROR;
    }

#if (ODBCVER < 0x0300)
  if (				/* fInfoType < SQL_INFO_FIRST || */
      (fInfoType > SQL_INFO_LAST
	  && fInfoType < SQL_INFO_DRIVER_START))
    {
      PUSHSQLERR (pdbc->herr, en_S1096);
      return SQL_ERROR;
    }
#endif
  if (fInfoType == SQL_ODBC_VER 
#if (ODBCVER >= 0x0300)
  	|| fInfoType == SQL_DM_VER
#endif
	)
    {
#if (ODBCVER >= 0x0300)
      if (fInfoType == SQL_DM_VER)
	sprintf ((char*)buf, "%02d.%02d.%04d.%04d", 
	  	SQL_SPEC_MAJOR, SQL_SPEC_MINOR, IODBC_BUILD / 10000, IODBC_BUILD % 10000);
      else
#endif
	sprintf ((char*)buf, "%02d.%02d.0000", SQL_SPEC_MAJOR, SQL_SPEC_MINOR);
      if(waMode == 'W')
        {
          void *prov = DM_U8toW(conv, (SQLCHAR *)buf, SQL_NTS);
          if(prov)
            {
              DM_WCSNCPY(conv, buf, prov, sizeof(buf)/DM_WCHARSIZE(conv));
              free(prov);
            }
          else
            DM_SetWCharAt(conv, buf, 0, 0);
        }


      if (rgbInfoValue != NULL  && cbInfoValueMax > 0)
	{
	  len = (waMode != 'W' ? STRLEN (buf) : DM_WCSLEN(conv, buf));

	  if (len > cbInfoValueMax - 1)
	    {
	      len = cbInfoValueMax - 1;
	      PUSHSQLERR (pdbc->herr, en_01004);

	      retcode = SQL_SUCCESS_WITH_INFO;
	    }

	  if (waMode != 'W')
	    {
	      STRNCPY (rgbInfoValue, buf, len);
	      ((char *) rgbInfoValue)[len] = '\0';
	    }
	  else
	    {
	      DM_WCSNCPY (conv, rgbInfoValue, buf, len);
	      DM_SetWCharAt(conv, rgbInfoValue, len, 0);
	    }
	}

      if (pcbInfoValue != NULL)
	{
	  *pcbInfoValue = (SWORD) len;
	}

      return retcode;
    }

  if (pdbc->state == en_dbc_allocated || pdbc->state == en_dbc_needdata)
    {
      PUSHSQLERR (pdbc->herr, en_08003);

      return SQL_ERROR;
    }

  switch (fInfoType)
    {
    case SQL_DRIVER_HDBC:
      ptr = (SQLPOINTER) (pdbc->dhdbc);
      size = sizeof (ptr);
      break;

    case SQL_DRIVER_HENV:
      penv = (ENV_t *) (pdbc->henv);
      ptr = (SQLPOINTER) (penv->dhenv);
      size = sizeof (ptr);
      break;

    case SQL_DRIVER_HLIB:
      penv = (ENV_t *) (pdbc->henv);
      ptr = (SQLPOINTER) (penv->hdll);
      size = sizeof (ptr);
      break;

    case SQL_DRIVER_HSTMT:
      if (rgbInfoValue != NULL)
	{
	  pstmt = *((STMT_t **) rgbInfoValue);
	}

      for (tpstmt = (STMT_t *) (pdbc->hstmt);
	  tpstmt != NULL;
	  tpstmt = tpstmt->next)
	{
	  if (tpstmt == pstmt)
	    {
	      break;
	    }
	}

      if (tpstmt == NULL)
	{
	  PUSHSQLERR (pdbc->herr, en_S1009);

	  return SQL_ERROR;
	}

      ptr = (SQLPOINTER) (pstmt->dhstmt);
      size = sizeof (ptr);
      break;

    case SQL_DRIVER_NAME:
    case SQL_DRIVER_ODBC_VER:
    case SQL_DRIVER_VER:
    case SQL_ODBC_INTERFACE_CONFORMANCE:
      break;

    default:
      /* NOTE : this was before the switch, just move here to let some information going through */
      if (pdbc->state == en_dbc_allocated || pdbc->state == en_dbc_needdata)
	{
	  PUSHSQLERR (pdbc->herr, en_08003);
	  return SQL_ERROR;
	}
    }

  if (size)
    {
      if (rgbInfoValue != NULL)
	{
	  rgbInfoValue = ptr;
	}

      if (pcbInfoValue != NULL)
	{
	  *(pcbInfoValue) = (SWORD) size;
	}

      return SQL_SUCCESS;
    }

#if (ODBCVER >= 0x0300)
  /*
   *  This was a temp value in ODBC 2
   */
  if (((ENV_t *) pdbc->henv)->dodbc_ver == SQL_OV_ODBC2 && 
	  fInfoType == SQL_OJ_CAPABILITIES)
      fInfoType = 65003;
#endif /* ODBCVER >= 0x0300 */

  if (penv->unicode_driver && waMode != 'W')
    conv_direct = CD_A2W;
  else if (!penv->unicode_driver && waMode == 'W')
    conv_direct = CD_W2A;
  else if (waMode == 'W' && conv && conv->dm_cp!=conv->drv_cp)
    conv_direct = CD_W2W;

  if (conv_direct != CD_NONE)
    {
      switch(fInfoType)
        {
        case SQL_ACCESSIBLE_PROCEDURES:
        case SQL_ACCESSIBLE_TABLES:
        case SQL_CATALOG_NAME:
        case SQL_CATALOG_NAME_SEPARATOR:
        case SQL_CATALOG_TERM:
        case SQL_COLLATION_SEQ:
        case SQL_COLUMN_ALIAS:
        case SQL_DATA_SOURCE_NAME:
        case SQL_DATA_SOURCE_READ_ONLY:
        case SQL_DATABASE_NAME:
        case SQL_DBMS_NAME:
        case SQL_DBMS_VER:
        case SQL_DESCRIBE_PARAMETER:
        case SQL_DRIVER_NAME:
        case SQL_DRIVER_ODBC_VER:
        case SQL_DRIVER_VER:
        case SQL_ODBC_VER:
        case SQL_EXPRESSIONS_IN_ORDERBY:
        case SQL_IDENTIFIER_QUOTE_CHAR:
        case SQL_INTEGRITY:
        case SQL_KEYWORDS:
        case SQL_LIKE_ESCAPE_CLAUSE:
        case SQL_MAX_ROW_SIZE_INCLUDES_LONG:
        case SQL_MULT_RESULT_SETS:
        case SQL_MULTIPLE_ACTIVE_TXN:
        case SQL_NEED_LONG_DATA_LEN:
        case SQL_ORDER_BY_COLUMNS_IN_SELECT:
        case SQL_PROCEDURE_TERM:
        case SQL_PROCEDURES:
        case SQL_ROW_UPDATES:
        case SQL_SCHEMA_TERM:
        case SQL_SEARCH_PATTERN_ESCAPE:
        case SQL_SERVER_NAME:
        case SQL_SPECIAL_CHARACTERS:
        case SQL_TABLE_TERM:
        case SQL_USER_NAME:
        case SQL_XOPEN_CLI_YEAR:
        case SQL_OUTER_JOINS:
          if (conv_direct == CD_A2W || conv_direct == CD_W2W)
            {
              /* ansi<=unicode  OR  unicode<=unicode*/
              if (conv_direct == CD_W2W)
                _cbInfoValueMax /= DM_WCHARSIZE(conv);

              if ((_InfoValue = malloc((_cbInfoValueMax + 1) * DRV_WCHARSIZE_ALLOC(conv))) == NULL)
	        {
                  PUSHSQLERR (pdbc->herr, en_HY001);
                  return SQL_ERROR;
                }
              _cbInfoValueMax *=  DRV_WCHARSIZE_ALLOC(conv);
            }
          else if (conv_direct == CD_W2A)
            {
              /* unicode<=ansi*/
              if ((_InfoValue = malloc(_cbInfoValueMax * MB_CUR_MAX + 1)) == NULL)
	        {
                  PUSHSQLERR (pdbc->herr, en_HY001);
                  return SQL_ERROR;
                }
              _cbInfoValueMax /=  DM_WCHARSIZE(conv);
            }

          infoValueOut = _InfoValue;
          break;
        }
    }

  CALL_UDRIVER(hdbc, pdbc, retcode, hproc, penv->unicode_driver,
    en_GetInfo, (pdbc->dhdbc, fInfoType, infoValueOut, _cbInfoValueMax,
    pcbInfoValue));

  if (hproc == SQL_NULL_HPROC)
    {
      PUSHSQLERR (pdbc->herr, en_IM001);
      return SQL_ERROR;
    }

  if (retcode == SQL_ERROR  && fInfoType == SQL_DRIVER_ODBC_VER)
    {
      if (waMode != 'W')
        {
          STRCPY (buf, "01.00");

          if (rgbInfoValue != NULL && cbInfoValueMax > 0)
	    {
	      len = STRLEN (buf);

	      if (len > cbInfoValueMax - 1)
	        {
	          len = cbInfoValueMax - 1;
                  ret = -1;
	        }
              else
                {
                  ret = 0;
                }

	      STRNCPY (rgbInfoValue, buf, len);
	      ((char *) rgbInfoValue)[len] = '\0';
	    }

          if (pcbInfoValue != NULL)
            *pcbInfoValue = (SWORD) len;
        }
      else
        {
          int count;
          ret = dm_StrCopyOut2_A2W_d2m (conv, (SQLCHAR *) "01.00",
		rgbInfoValue, cbInfoValueMax, NULL, &count);
          if (pcbInfoValue)
            *pcbInfoValue = (SQLSMALLINT)count;
        }

       if (ret == -1)
         {
           PUSHSQLERR (pdbc->herr, en_01004);
           retcode = SQL_SUCCESS_WITH_INFO;
         }
       else
         {
           retcode = SQL_SUCCESS;
         }

    }
  else if (rgbInfoValue && conv_direct != CD_NONE && SQL_SUCCEEDED (retcode))
    {
      int count;
      switch(fInfoType)
        {
        case SQL_ACCESSIBLE_PROCEDURES:
        case SQL_ACCESSIBLE_TABLES:
        case SQL_CATALOG_NAME:
        case SQL_CATALOG_NAME_SEPARATOR:
        case SQL_CATALOG_TERM:
        case SQL_COLLATION_SEQ:
        case SQL_COLUMN_ALIAS:
        case SQL_DATA_SOURCE_NAME:
        case SQL_DATA_SOURCE_READ_ONLY:
        case SQL_DATABASE_NAME:
        case SQL_DBMS_NAME:
        case SQL_DBMS_VER:
        case SQL_DESCRIBE_PARAMETER:
        case SQL_DRIVER_NAME:
        case SQL_DRIVER_ODBC_VER:
        case SQL_DRIVER_VER:
        case SQL_ODBC_VER:
        case SQL_EXPRESSIONS_IN_ORDERBY:
        case SQL_IDENTIFIER_QUOTE_CHAR:
        case SQL_INTEGRITY:
        case SQL_KEYWORDS:
        case SQL_LIKE_ESCAPE_CLAUSE:
        case SQL_MAX_ROW_SIZE_INCLUDES_LONG:
        case SQL_MULT_RESULT_SETS:
        case SQL_MULTIPLE_ACTIVE_TXN:
        case SQL_NEED_LONG_DATA_LEN:
        case SQL_ORDER_BY_COLUMNS_IN_SELECT:
        case SQL_PROCEDURE_TERM:
        case SQL_PROCEDURES:
        case SQL_ROW_UPDATES:
        case SQL_SCHEMA_TERM:
        case SQL_SEARCH_PATTERN_ESCAPE:
        case SQL_SERVER_NAME:
        case SQL_SPECIAL_CHARACTERS:
        case SQL_TABLE_TERM:
        case SQL_USER_NAME:
        case SQL_XOPEN_CLI_YEAR:
        case SQL_OUTER_JOINS:
          if (conv_direct == CD_A2W)
            {
            /* ansi<=unicode*/
              ret = dm_StrCopyOut2_W2A_d2m (conv, infoValueOut,
		(SQLCHAR *) rgbInfoValue, cbInfoValueMax, NULL, &count);
              if (pcbInfoValue)
                *pcbInfoValue = (SQLSMALLINT)count;
            }
          else if (conv_direct == CD_W2A)
            {
            /* unicode<=ansi*/
              ret = dm_StrCopyOut2_A2W_d2m (conv, (SQLCHAR *) infoValueOut,
		rgbInfoValue, cbInfoValueMax, NULL, &count);
              if (pcbInfoValue)
                *pcbInfoValue = (SQLSMALLINT)count;
            }
          else if (conv_direct == CD_W2W)
            {
              /* unicode<=unicode*/
              ret = dm_StrCopyOut2_W2W_d2m (conv, infoValueOut,
		rgbInfoValue, cbInfoValueMax, NULL, &count);
              if (pcbInfoValue)
                *pcbInfoValue = (SQLSMALLINT)count;
            }

          if (ret == -1)
            {
              PUSHSQLERR (pdbc->herr, en_01004);
              retcode = SQL_SUCCESS_WITH_INFO;
            }
          break;
        }
    }
  MEM_FREE(_InfoValue);
  
  return retcode;
}


SQLRETURN SQL_API
SQLGetInfo (SQLHDBC hdbc,
  SQLUSMALLINT		  fInfoType,
  SQLPOINTER		  rgbInfoValue,
  SQLSMALLINT		  cbInfoValueMax,
  SQLSMALLINT 		* pcbInfoValue)
{
  ENTER_HDBC (hdbc, 0,
    trace_SQLGetInfo (TRACE_ENTER,
    	hdbc, 
	fInfoType, 
	rgbInfoValue, cbInfoValueMax, pcbInfoValue));

  retcode = SQLGetInfo_Internal(
  	hdbc, 
	fInfoType, 
	rgbInfoValue, cbInfoValueMax, pcbInfoValue, 
	'A');

  LEAVE_HDBC (hdbc, 0,
    trace_SQLGetInfo (TRACE_LEAVE,
    	hdbc, 
	fInfoType, 
	rgbInfoValue, cbInfoValueMax, pcbInfoValue));
}


#if ODBCVER >= 0x0300
SQLRETURN SQL_API
SQLGetInfoA (SQLHDBC hdbc,
  SQLUSMALLINT		  fInfoType,
  SQLPOINTER		  rgbInfoValue,
  SQLSMALLINT		  cbInfoValueMax,
  SQLSMALLINT 		* pcbInfoValue)
{
  ENTER_HDBC (hdbc, 0,
    trace_SQLGetInfo (TRACE_ENTER,
    	hdbc, 
	fInfoType, 
	rgbInfoValue, cbInfoValueMax, pcbInfoValue));

  retcode = SQLGetInfo_Internal(
  	hdbc, 
	fInfoType, 
	rgbInfoValue, cbInfoValueMax, pcbInfoValue, 
	'A');

  LEAVE_HDBC (hdbc, 0,
    trace_SQLGetInfo (TRACE_LEAVE,
    	hdbc, 
	fInfoType, 
	rgbInfoValue, cbInfoValueMax, pcbInfoValue));
}


SQLRETURN SQL_API
SQLGetInfoW (
  SQLHDBC		  hdbc,
  SQLUSMALLINT		  fInfoType,
  SQLPOINTER		  rgbInfoValue,
  SQLSMALLINT		  cbInfoValueMax,
  SQLSMALLINT 		* pcbInfoValue)
{
  ENTER_HDBC (hdbc, 0,
    trace_SQLGetInfoW (TRACE_ENTER,
    	hdbc, 
	fInfoType, 
	rgbInfoValue, cbInfoValueMax, pcbInfoValue));

  retcode = SQLGetInfo_Internal (
  	hdbc, 
	fInfoType, 
	rgbInfoValue, cbInfoValueMax, pcbInfoValue, 
	'W');

  LEAVE_HDBC (hdbc, 0,
    trace_SQLGetInfoW (TRACE_LEAVE,
    	hdbc, 
	fInfoType, 
	rgbInfoValue, cbInfoValueMax, pcbInfoValue));
}
#endif


static int FunctionNumbers[] =
{
    0
#define FUNCDEF(A,B,C)	,A
#include "henv.ci"
#undef FUNCDEF
};



static SQLRETURN 
SQLGetFunctions_Internal (
  SQLHDBC		  hdbc,
  SQLUSMALLINT		  fFunc,
  SQLUSMALLINT		* pfExists)
{
  CONN (pdbc, hdbc);
  HPROC hproc;
  SQLRETURN retcode;
  int i;
  UWORD functions2[100];
#if (ODBCVER >= 0x0300)
  UWORD functions3[SQL_API_ODBC3_ALL_FUNCTIONS_SIZE];
#endif

  if (pdbc->state == en_dbc_allocated
      || pdbc->state == en_dbc_needdata)
    {
      PUSHSQLERR (pdbc->herr, en_S1010);

      return SQL_ERROR;
    }

  if (pfExists == NULL)
    {
      return SQL_SUCCESS;
    }

  /*
   *  These functions are supported by the iODBC driver manager
   */
  if (fFunc == SQL_API_SQLDATASOURCES
      || fFunc == SQL_API_SQLDRIVERS
#if (ODBCVER >= 0x0300)
      || fFunc == SQL_API_SQLGETENVATTR
      || fFunc == SQL_API_SQLSETENVATTR
#endif
      )
    {
      *pfExists = (UWORD) 1;
      return SQL_SUCCESS;
    }

  /*
   *  Check if function number is within ODBC version context
   */
#if (ODBCVER < 0x0300)
  if (fFunc > SQL_EXT_API_LAST)
    {
      PUSHSQLERR (pdbc->herr, en_S1095);

      return SQL_ERROR;
    }
#endif

  /*
   *  In a ODBC 2.x driver context, the ODBC 3.x API calls are 
   *  mapped by the driver manager.
   */
#if (ODBCVER >= 0x0300)
  if (((ENV_t *) pdbc->henv)->dodbc_ver == SQL_OV_ODBC2)
    {
      switch (fFunc)
	{
	case SQL_API_ALL_FUNCTIONS:
	case SQL_API_ODBC3_ALL_FUNCTIONS:
	  break;

	  /* Mapped ODBC3 app -> ODBC2 driver functions */
	case SQL_API_SQLALLOCHANDLE:
	case SQL_API_SQLFREEHANDLE:
	case SQL_API_SQLSETCONNECTATTR:
	case SQL_API_SQLGETCONNECTATTR:
	case SQL_API_SQLGETSTMTATTR:
	case SQL_API_SQLSETSTMTATTR:
	case SQL_API_SQLCOLATTRIBUTE:
	case SQL_API_SQLENDTRAN:
	case SQL_API_SQLBULKOPERATIONS:
	case SQL_API_SQLFETCHSCROLL:
	case SQL_API_SQLGETDIAGREC:
	case SQL_API_SQLGETDIAGFIELD:
	  *pfExists = SQL_TRUE;
	  return SQL_SUCCESS;

	case SQL_API_SQLBINDPARAM:
	  fFunc = SQL_API_SQLBINDPARAMETER;
	  break;

	default:
	  if (fFunc > SQL_API_SQLBINDPARAMETER)
	    {
	      *pfExists = SQL_FALSE;

	      return SQL_SUCCESS;
	    }
	  break;
	}
    }
#endif


  /*
   *  If the driver exports a SQLGetFunctions call, use it
   */
  hproc = _iodbcdm_getproc (pdbc, en_GetFunctions);

  if (hproc != SQL_NULL_HPROC)
    {
      CALL_DRIVER (hdbc, pdbc, retcode, hproc, (pdbc->dhdbc, fFunc, pfExists));

      return retcode;
    }

  /*
   *  Map deprecated functions
   */
  if (fFunc == SQL_API_SQLSETPARAM)
    {
      fFunc = SQL_API_SQLBINDPARAMETER;
    }

  /*
   *  Initialize intermediate result arrays
   */
  memset (functions2, '\0', sizeof (functions2));
#if (ODBCVER > 0x0300)
  memset (functions3, '\0', sizeof (functions3));
#endif

  /*
   *  Build result array by scanning for all API calls
   */
  for (i = 1; i < __LAST_API_FUNCTION__; i++)
    {
      int j = FunctionNumbers[i];

      hproc = _iodbcdm_getproc (pdbc, i);

      if (hproc != SQL_NULL_HPROC)
	{
	  if (j < 100)
	    functions2[j] = 1;
#if (ODBCVER >= 0x0300)
	  functions3[j >> 4] |= (1 << (j & 0x000F));
#endif
	}
    }

  /*
   *  Finally return the information
   */
  if (fFunc == SQL_API_ALL_FUNCTIONS)
    {
      memcpy (pfExists, &functions2, sizeof (functions2));
    }
#if (ODBCVER < 0x0300)
  else
    {
      *pfExists = functions2[fFunc];
    }
#else
  else if (fFunc == SQL_API_ODBC3_ALL_FUNCTIONS)
    {
      memcpy (pfExists, &functions3, sizeof (functions3));
    }
  else
    {
      *pfExists = SQL_FUNC_EXISTS (functions3, fFunc);
    }
#endif

  return SQL_SUCCESS;
}


SQLRETURN SQL_API
SQLGetFunctions (
  SQLHDBC		  hdbc,
  SQLUSMALLINT		  fFunc,
  SQLUSMALLINT	 	* pfExists)
{
  ENTER_HDBC (hdbc, 0,
    trace_SQLGetFunctions (TRACE_ENTER,
    	hdbc,
	fFunc,
	pfExists));

  retcode = SQLGetFunctions_Internal (
    	hdbc,
	fFunc,
	pfExists);

  LEAVE_HDBC (hdbc, 0,
    trace_SQLGetFunctions (TRACE_LEAVE,
    	hdbc,
	fFunc,
	pfExists));
}
