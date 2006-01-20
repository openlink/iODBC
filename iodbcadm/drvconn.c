/*
 *  drvconn.c
 *
 *  $Id$
 *
 *  The data_sources dialog for SQLDriverConnect
 *
 *  The iODBC driver manager.
 *
 *  Copyright (C) 1996-2006 by OpenLink Software <iodbc@openlinksw.com>
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


#include "gui.h"

#include <herr.h>
#include <unicode.h>
#include <dlproc.h>

#ifndef WIN32
#include <unistd.h>

typedef SQLRETURN SQL_API (*pDriverConnFunc) (HWND hwnd, LPSTR szInOutConnStr,
    DWORD cbInOutConnStr, int FAR * sqlStat, SQLUSMALLINT fDriverCompletion, UWORD *config);
typedef SQLRETURN SQL_API (*pDriverConnWFunc) (HWND hwnd, LPWSTR szInOutConnStr,
    DWORD cbInOutConnStr, int FAR * sqlStat, SQLUSMALLINT fDriverCompletion, UWORD *config);

#define CALL_DRVCONN_DIALBOXW(path, a) \
  { \
    char *_path_u8 = (a == 'A') ? NULL : dm_SQL_W2A ((wchar_t*)path, SQL_NTS); \
    if ((handle = DLL_OPEN((a == 'A') ? (char*)path : _path_u8)) != NULL) \
      { \
        if ((pDrvConnW = (pDriverConnWFunc)DLL_PROC(handle, "_iodbcdm_drvconn_dialboxw")) != NULL) \
          { \
            SQLSetConfigMode (*config); \
            if (pDrvConnW (hwnd, szInOutConnStr, cbInOutConnStr, sqlStat, fDriverCompletion, config) == SQL_SUCCESS) \
              { \
                MEM_FREE (_path_u8); \
                DLL_CLOSE(handle); \
                retcode = SQL_SUCCESS; \
                goto quit; \
              } \
            else \
              { \
                MEM_FREE (_path_u8); \
                DLL_CLOSE(handle); \
                retcode = SQL_NO_DATA_FOUND; \
                goto quit; \
              } \
          } \
        else \
          { \
            if ((pDrvConn = (pDriverConnFunc)DLL_PROC(handle, "_iodbcdm_drvconn_dialbox")) != NULL) \
              { \
                char *_szinoutconstr_u8 = malloc (cbInOutConnStr + 1); \
                wchar_t *_prvw; char *_prvu8; \
                for (_prvw = szInOutConnStr, _prvu8 = _szinoutconstr_u8 ; \
                  *_prvw != L'\0' ; _prvw += WCSLEN (_prvw) + 1, \
                  _prvu8 += STRLEN (_prvu8) + 1) \
                  dm_StrCopyOut2_W2A (_prvw, _prvu8, cbInOutConnStr, NULL); \
                *_prvu8 = '\0'; \
                SQLSetConfigMode (*config); \
                if (pDrvConn (hwnd, _szinoutconstr_u8, cbInOutConnStr, sqlStat, fDriverCompletion, config) == SQL_SUCCESS) \
                  { \
                    dm_StrCopyOut2_A2W (_szinoutconstr_u8, szInOutConnStr, cbInOutConnStr, NULL); \
                    MEM_FREE (_path_u8); \
                    MEM_FREE (_szinoutconstr_u8); \
                    DLL_CLOSE(handle); \
                    retcode = SQL_SUCCESS; \
                    goto quit; \
                  } \
                else \
                  { \
                    MEM_FREE (_path_u8); \
                    MEM_FREE (_szinoutconstr_u8); \
                    DLL_CLOSE(handle); \
                    retcode = SQL_NO_DATA_FOUND; \
                    goto quit; \
                  } \
              } \
          } \
        DLL_CLOSE(handle); \
      } \
    MEM_FREE (_path_u8); \
  }
#endif

SQLRETURN SQL_API
iodbcdm_drvconn_dialbox (
    HWND hwnd,
    LPSTR szInOutConnStr,
    DWORD cbInOutConnStr,
    int * sqlStat,
    SQLUSMALLINT fDriverCompletion,
    UWORD *config)
{
  RETCODE retcode = SQL_ERROR;
  wchar_t *_string_w = NULL;

  if (cbInOutConnStr > 0)
    {
      if ((_string_w = malloc (cbInOutConnStr * sizeof(wchar_t) + 1)) == NULL)
          goto done;
    }

  dm_StrCopyOut2_A2W (szInOutConnStr, _string_w,
    cbInOutConnStr * sizeof(wchar_t), NULL);

  retcode = iodbcdm_drvconn_dialboxw (hwnd, _string_w,
    cbInOutConnStr, sqlStat, fDriverCompletion, config);

  if (retcode == SQL_SUCCESS)
    {
      dm_StrCopyOut2_W2A (_string_w, szInOutConnStr, cbInOutConnStr - 1, NULL);
    }

done:
  MEM_FREE (_string_w);

  return retcode;
}


SQLRETURN SQL_API
iodbcdm_drvconn_dialboxw (
    HWND hwnd,
    LPWSTR szInOutConnStr,
    DWORD cbInOutConnStr,
    int * sqlStat,
    SQLUSMALLINT fDriverCompletion,
	 UWORD *config)
{
  RETCODE retcode = SQL_ERROR;
  TDSNCHOOSER choose_t;
  wchar_t *string = NULL, *prov, *prov1, *szDSN = NULL, *szDriver = NULL;
  wchar_t tokenstr[4096], drvbuf[4096];
  char *_szdriver_u8 = NULL;
  HDLL handle;
  pDriverConnFunc pDrvConn;
  pDriverConnWFunc pDrvConnW;
  int i;
#if defined (__APPLE__) && !(defined (NO_FRAMEWORKS) || defined (_LP64))
  CFStringRef libname = NULL;
  CFBundleRef bundle = NULL;
  CFURLRef liburl = NULL;
  char name[1024] = { '\0' };
#endif

  /* Check input parameters */
  if (!szInOutConnStr || cbInOutConnStr < 1)
    goto quit;

  /* Transform the string connection to list of key pairs */
  string = (wchar_t*) malloc((cbInOutConnStr + 1) * sizeof(wchar_t));
  if (string == NULL)
    {
      if (sqlStat)
#if (ODBCVER>=0x3000)
        *sqlStat = en_HY092;
#else
        *sqlStat = en_S1000;
#endif
      retcode = SQL_ERROR;
      goto quit;
    }

  /* Conversion to the list of key pairs */
  wcsncpy (string, szInOutConnStr, cbInOutConnStr);
  string[WCSLEN (string) + 1] = L'\0';
  for (i = WCSLEN (string) - 1 ; i >= 0 ; i--)
    if (string[i] == L';') string[i] = L'\0';

  /* Look for the DSN and DRIVER keyword */
  for (prov = string ; *prov != L'\0' ; prov += WCSLEN (prov) + 1)
    {
      if (!wcsncasecmp (prov, L"DSN=", WCSLEN (L"DSN=")))
        {
          szDSN = prov + WCSLEN (L"DSN=");
          continue;
        }
      if (!wcsncasecmp (prov, L"DRIVER=", WCSLEN (L"DRIVER=")))
        {
          szDriver = prov + WCSLEN (L"DRIVER=");
          continue;
        }
    }

  if (!szDSN && !szDriver)
    {
      /* Display the DSN chooser dialog box */
      create_dsnchooser (hwnd, &choose_t);

      /* Check output parameters */
      if (choose_t.dsn)
        {
          /* Change the config mode */
          switch (choose_t.type_dsn)
            {
              case USER_DSN:
                *config = ODBC_USER_DSN;
                break;
              case SYSTEM_DSN:
                *config = ODBC_SYSTEM_DSN;
                break;
            };

          /* Try to copy the DSN */
          if (cbInOutConnStr > WCSLEN (choose_t.dsn) + WCSLEN (L"DSN=") + 1)
            {
              WCSCPY (string, L"DSN=");
              WCSCAT (string, choose_t.dsn);
              string[WCSLEN (string) + 1] = L'\0';
              szDSN = string + WCSLEN (L"DSN=");
              retcode = SQL_SUCCESS;
            }
          else
            {
              if (sqlStat)
#if (ODBCVER>=0x3000)
                *sqlStat = en_HY092;
#else
                *sqlStat = en_S1000;
#endif
            }
        }
      else
        retcode = SQL_NO_DATA_FOUND;

      if (choose_t.dsn)
	free (choose_t.dsn);

      if (retcode != SQL_SUCCESS)
	goto quit;
    }

  /* Constitute the string connection */
  for (prov = szInOutConnStr, prov1 = string, i = 0 ; *prov1 != L'\0' ;
    prov1 += WCSLEN (prov) + 1, i += WCSLEN (prov) + 1, prov += WCSLEN (prov) + 1)
    WCSCPY (prov, prov1);
  *prov = L'\0';

  /* Check if the driver is provided */
  if (szDriver == NULL)
    {
      SQLSetConfigMode (ODBC_BOTH_DSN);
      SQLGetPrivateProfileStringW (L"ODBC Data Sources",
        szDSN && szDSN[0] != L'\0' ? szDSN : L"default",
        L"", tokenstr, sizeof (tokenstr)/sizeof(wchar_t), NULL);
      szDriver = tokenstr;
    }

  /* Call the iodbcdm_drvconn_dialbox */
  _szdriver_u8 = dm_SQL_W2A (szDriver, SQL_NTS);

  SQLSetConfigMode (ODBC_USER_DSN);
  if (!access (_szdriver_u8, X_OK))
    { CALL_DRVCONN_DIALBOXW (_szdriver_u8, 'A'); }
  if (SQLGetPrivateProfileStringW (szDriver, L"Driver", L"", drvbuf,
    sizeof (drvbuf) / sizeof(wchar_t), L"odbcinst.ini"))
    { CALL_DRVCONN_DIALBOXW (drvbuf, 'W'); }
  if (SQLGetPrivateProfileStringW (szDriver, L"Setup", L"", drvbuf,
    sizeof (drvbuf) / sizeof(wchar_t), L"odbcinst.ini"))
    { CALL_DRVCONN_DIALBOXW (drvbuf, 'W'); }
  if (SQLGetPrivateProfileStringW (L"Default", L"Driver", L"", drvbuf,
    sizeof (drvbuf) / sizeof(wchar_t), L"odbcinst.ini"))
    { CALL_DRVCONN_DIALBOXW (drvbuf, 'W'); }
  if (SQLGetPrivateProfileStringW (L"Default", L"Setup", L"", drvbuf,
    sizeof (drvbuf) / sizeof(wchar_t), L"odbcinst.ini"))
    { CALL_DRVCONN_DIALBOXW (drvbuf, 'W'); }

  SQLSetConfigMode (ODBC_SYSTEM_DSN);
  if (!access (_szdriver_u8, X_OK))
    { CALL_DRVCONN_DIALBOXW (_szdriver_u8, 'A'); }
  if (SQLGetPrivateProfileStringW (szDriver, L"Driver", L"", drvbuf,
    sizeof (drvbuf) / sizeof(wchar_t), L"odbcinst.ini"))
    { CALL_DRVCONN_DIALBOXW (drvbuf, 'W'); }
  if (SQLGetPrivateProfileStringW (szDriver, L"Setup", L"", drvbuf,
    sizeof (drvbuf) / sizeof(wchar_t), L"odbcinst.ini"))
    { CALL_DRVCONN_DIALBOXW (drvbuf, 'W'); }
  if (SQLGetPrivateProfileStringW (L"Default", L"Driver", L"", drvbuf,
    sizeof (drvbuf) / sizeof(wchar_t), L"odbcinst.ini"))
    { CALL_DRVCONN_DIALBOXW (drvbuf, 'W'); }
  if (SQLGetPrivateProfileStringW (L"Default", L"Setup", L"", drvbuf,
    sizeof (drvbuf) / sizeof(wchar_t), L"odbcinst.ini"))
    { CALL_DRVCONN_DIALBOXW (drvbuf, 'W'); }

  /* The last ressort, a proxy driver */
#if defined (__APPLE__) && !(defined (NO_FRAMEWORKS) || defined (_LP64))
  bundle = CFBundleGetBundleWithIdentifier (CFSTR ("org.iodbc.core"));
  if (!bundle)
    bundle = CFBundleGetBundleWithIdentifier (CFSTR ("org.iodbc.inst"));
  if (bundle)
    {
      /* Search for the drvproxy library */
      liburl =
	  CFBundleCopyResourceURL (bundle, CFSTR ("iODBCdrvproxy.bundle"),
	  NULL, NULL);
      if (liburl && (libname =
       CFURLCopyFileSystemPath (liburl, kCFURLPOSIXPathStyle)))
	{
          CFStringGetCString (libname, name, sizeof (name),
            kCFStringEncodingASCII);
          STRCAT (name, "/Contents/MacOS/iODBCdrvproxy");
          CALL_DRVCONN_DIALBOXW (name, 'A');
	}
    }
#else
  CALL_DRVCONN_DIALBOXW ("libdrvproxy.so", 'A');
#endif /* __APPLE__ */

  if (sqlStat)
    *sqlStat = en_IM003;

quit:
#if defined (__APPLE__) && !(defined (NO_FRAMEWORKS) || defined (_LP64))
  if (liburl) CFRelease (liburl);
  if (libname) CFRelease (libname);
#endif

  MEM_FREE (string);
  MEM_FREE (_szdriver_u8);

  return retcode;
}


SQLRETURN SQL_API
_iodbcdm_drvchoose_dialbox (
    HWND hwnd,
    LPSTR szInOutConnStr,
    DWORD cbInOutConnStr,
    int * sqlStat)
{
  RETCODE retcode = SQL_ERROR;
  wchar_t *_string_w = NULL;
  WORD len;

  if (cbInOutConnStr > 0)
    {
      if ((_string_w = malloc (cbInOutConnStr * sizeof(wchar_t) + 1)) == NULL)
          goto done;
    }

  retcode = _iodbcdm_drvchoose_dialboxw (hwnd, _string_w,
    cbInOutConnStr * sizeof(wchar_t), sqlStat);

  if (retcode == SQL_SUCCESS)
    {
      dm_StrCopyOut2_W2A (_string_w, szInOutConnStr, cbInOutConnStr - 1, &len);
    }

done:
  MEM_FREE (_string_w);

  return retcode;
}


SQLRETURN SQL_API
_iodbcdm_drvchoose_dialboxw (HWND hwnd,
    LPWSTR szInOutConnStr,
    DWORD cbInOutConnStr,
    int FAR * sqlStat)
{
  RETCODE retcode = SQL_ERROR;
  TDRIVERCHOOSER choose_t;

  /* Check input parameters */
  if (!hwnd || !szInOutConnStr || cbInOutConnStr < 1)
    goto quit;

  create_driverchooser (hwnd, &choose_t);

  /* Check output parameters */
  if (choose_t.driver)
    {
      if (cbInOutConnStr > WCSLEN (choose_t.driver) + WCSLEN (L"DRIVER="))
	{
          WCSCPY (szInOutConnStr, L"DRIVER=");
          WCSCAT (szInOutConnStr, choose_t.driver);
	  retcode = SQL_SUCCESS;
	}
      else
	{
	  if (sqlStat)
#if (ODBCVER>=0x3000)
	    *sqlStat = en_HY092;
#else
	    *sqlStat = en_S1000;
#endif
	  retcode = SQL_ERROR;
	}
    }
  else
    retcode = SQL_NO_DATA;

  if (choose_t.driver)
    free (choose_t.driver);

quit:
  return retcode;
}


SQLRETURN SQL_API
_iodbcdm_admin_dialbox (HWND hwnd)
{
  RETCODE retcode = SQL_ERROR;

  /* Check input parameters */
  if (!hwnd)
    goto quit;

  create_administrator (hwnd);
  retcode = SQL_SUCCESS;

quit:
  return retcode;
}


SQLRETURN SQL_API
_iodbcdm_trschoose_dialbox (
    HWND hwnd,
    LPSTR szInOutConnStr,
    DWORD cbInOutConnStr,
    int FAR * sqlStat)
{
  RETCODE retcode = SQL_ERROR;
  wchar_t *_string_w = NULL;
  WORD len;

  if (cbInOutConnStr > 0)
    {
      if ((_string_w = malloc (cbInOutConnStr * sizeof(wchar_t) + 1)) == NULL)
          goto done;
    }

  retcode = _iodbcdm_trschoose_dialboxw (hwnd, _string_w,
    cbInOutConnStr * sizeof(wchar_t), sqlStat);

  if (retcode == SQL_SUCCESS)
    {
      dm_StrCopyOut2_W2A (_string_w, szInOutConnStr, cbInOutConnStr - 1, &len);
    }

done:
  MEM_FREE (_string_w);

  return retcode;
}


SQLRETURN SQL_API
_iodbcdm_trschoose_dialboxw (
    HWND hwnd,
    LPWSTR szInOutConnStr,
    DWORD cbInOutConnStr,
    int * sqlStat)
{
  RETCODE retcode = SQL_ERROR;
  TTRANSLATORCHOOSER choose_t;

  /* Check input parameters */
  if (!hwnd || !szInOutConnStr || cbInOutConnStr < 1)
    goto quit;

  create_translatorchooser (hwnd, &choose_t);

  /* Check output parameters */
  if (choose_t.translator)
    {
      if (cbInOutConnStr >
          WCSLEN (choose_t.translator) + WCSLEN (L"TranslationName="))
	{
          WCSCPY (szInOutConnStr, L"TranslationName");
          WCSCAT (szInOutConnStr, choose_t.translator);
	  retcode = SQL_SUCCESS;
	}
      else
	{
	  if (sqlStat)
#if (ODBCVER>=0x3000)
	    *sqlStat = en_HY092;
#else
	    *sqlStat = en_S1000;
#endif
	  retcode = SQL_ERROR;
	}
    }
  else
    retcode = SQL_NO_DATA;

  if (choose_t.translator)
    free (choose_t.translator);

quit:
  return retcode;
}


