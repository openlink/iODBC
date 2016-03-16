/*
 *  odbcimport.c
 *
 *  $Id: odbcimport.c,v 1.2 2009/08/30 10:48:35 source Exp $
 *
 *  ODBC settings import tool
 *
 *  Distributed as part of the iODBC driver manager.
 *  
 *  Copyright (C) 1996-2009 by OpenLink Software <iodbc@openlinksw.com>
 *  All Rights Reserved.
 *
 *  This software is released under the terms of either of the following
 *  licenses:
 *
 *      - GNU Library General Public License (see LICENSE.LGPL) 
 *      - The BSD License (see LICENSE.BSD).
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
 *
 */

#ifdef __APPLE__
#  include <iODBC/sql.h>
#  include <iODBC/sqlext.h>
#  include <iODBC/sqlucode.h>
#  include <iODBC/sqltypes.h>
#  include <iODBCinst/odbcinst.h>
#else
#  include <sql.h>
#  include <sqlext.h>
#  include <sqlucode.h>
#  include <sqltypes.h>
#  include <odbcinst.h>
#endif

#include <stdio.h>
#include <string.h>

#ifdef UNICODE
#include <wchar.h>
#define TXTLEN(x)      wcslen(x)
#define TXTCMP(x1,x2)  wcscmp(x1,x2)
typedef wchar_t TCHAR;
#else
#define TXTLEN(x)      strlen(x)
#define TXTCMP(x1,x2)  strcmp(x1,x2)
typedef char TCHAR;
#endif

#define ERR_ENTRY_EXISTS	-1000
#define ERR_NOENTRY		-1001

int odbc_import_section(TCHAR *src, TCHAR *dst, TCHAR *section, UWORD confMode, BOOL inst, BOOL force)
{
  TCHAR array[4096], value[1024], *ptr;
  UWORD oldconfMode;
  int length, err = 0;

  if(src==dst) return err;
  if(!section) return ERR_NOENTRY;

  /* Save the config mode */
  SQLGetConfigMode (&oldconfMode);

  /* Set the config mode for the operation */
  SQLSetConfigMode (confMode);

  /* Check if the entry already exists */
#ifdef UNICODE
  length = SQLGetPrivateProfileString (section, NULL, L"", array, sizeof(array)*sizeof(TCHAR),
    dst ? dst : (inst ? L"odbcinst.ini" : L"odbc.ini"));
#else
  length = SQLGetPrivateProfileString (section, NULL, "", array, sizeof(array)*sizeof(TCHAR),
    dst ? dst : (inst ? "odbcinst.ini" : "odbc.ini"));
#endif
  if(length && !force)
    {
      err = ERR_ENTRY_EXISTS;
      goto erro;
    }

  /* Get the keywords list of all the section */
#ifdef UNICODE
  length = SQLGetPrivateProfileString (section, NULL, L"", array, sizeof(array)*sizeof(TCHAR),
    src ? src : (inst ? L"odbcinst.ini" : L"odbc.ini"));
#else
  length = SQLGetPrivateProfileString (section, NULL, "", array, sizeof(array)*sizeof(TCHAR),
    src ? src : (inst ? "odbcinst.ini" : "odbc.ini"));
#endif
  if(length)
    {
      /* Get the values for each keywords */
      for(ptr = array ; *ptr ; ptr = ptr + TXTLEN(ptr) + 1)
        {
#ifdef UNICODE
          length = SQLGetPrivateProfileString (section, ptr, L"", value, sizeof(value)*sizeof(TCHAR),
            src ? src : (inst ? L"odbcinst.ini" : L"odbc.ini"));
          SQLWritePrivateProfileString (section, ptr, value,
            dst ? dst : (inst ? L"odbcinst.ini" : L"odbc.ini"));
#else
          length = SQLGetPrivateProfileString (section, ptr, "", value, sizeof(value)*sizeof(TCHAR),
            src ? src : (inst ? "odbcinst.ini" : "odbc.ini"));
          SQLWritePrivateProfileString (section, ptr, value,
            dst ? dst : (inst ? "odbcinst.ini" : "odbc.ini"));
#endif
        }
    }

erro:
  /* Restore the config mode */
  SQLSetConfigMode (oldconfMode);

  return err;
}

int odbc_import_whole(TCHAR *src, TCHAR *dst, UWORD confMode, BOOL inst, BOOL force)
{
  TCHAR array[4096], *ptr;
  UWORD oldconfMode;
  int length, err = 0;

  /* Save the config mode */
  SQLGetConfigMode (&oldconfMode);

  /* Set the config mode for the operation */
  SQLSetConfigMode (confMode);

  /* Get the keywords list of the whole file */
#ifdef UNICODE
  length = SQLGetPrivateProfileString (NULL, NULL, L"", array, sizeof(array)*sizeof(TCHAR),
    src ? src : (inst ? L"odbcinst.ini" : L"odbc.ini"));
#else
  length = SQLGetPrivateProfileString (NULL, NULL, "", array, sizeof(array)*sizeof(TCHAR),
    src ? src : (inst ? "odbcinst.ini" : "odbc.ini"));
#endif
  if(length)
    {
      /* Import each section */
      for(ptr = array ; *ptr && !err ; ptr = ptr + TXTLEN(ptr) + 1)
        {
          err = odbc_import_section(src, dst, ptr, confMode, inst, force);
          if(err==ERR_ENTRY_EXISTS) err = 0;
        }
    }

erro:
  /* Restore the config mode */
  SQLSetConfigMode (oldconfMode);

  return err;
}

void showusage(void)
{
  fprintf (stderr, "\nUsage:\n  odbcimport -input <file> -output <file> -section <name> -system -force\n");
  fprintf (stderr, "\n\t-input <file>\tSpecify the file that the DSNs will be imported from\n");
  fprintf (stderr, "\t-output <file>\tSpecify the file where the DSNs have to be wrote to\n");
  fprintf (stderr, "\t-section <name>\tOnly import the specified section\n");
  fprintf (stderr, "\t-force\t\tIf the DSNs to import already exists, overwrite it\n");
  fprintf (stderr, "\t-system\t\tProcess the system DSNs set\n");
  exit (0);
}

int main(int argc, TCHAR** argv)
{
  TCHAR *in = NULL, *out = NULL, *section = NULL;
  BOOL force=FALSE, system=FALSE;
  int i, err = 0;

  if(argc == 1)
    showusage();

  for(i=1 ; i<argc ; i++)
    {
      if(!TXTCMP("-help", argv[i]))
        showusage();
      if(!TXTCMP("-force", argv[i]))
        {
          force = TRUE;
          continue;
        }
      if(!TXTCMP("-system", argv[i]))
        {
          system = TRUE;
          continue;
        }
      if(!TXTCMP("-section", argv[i]))
        {
          section = argv[i+1]; i++;
          continue;
        }
      if(!TXTCMP("-input", argv[i]))
        {
          in = argv[i+1]; i++;
          continue;
        }
      if(!TXTCMP("-output", argv[i]))
        {
          out = argv[i+1]; i++;
          continue;
        }
    }

  if(!section)
    err = odbc_import_whole(in, out, system ? ODBC_SYSTEM_DSN : ODBC_USER_DSN, system, force);
  else
    err = odbc_import_section(in, out, section, system ? ODBC_SYSTEM_DSN : ODBC_USER_DSN, system, force);

  switch(err)
    {
      case ERR_ENTRY_EXISTS:
        fprintf (stderr, "At least one entry already exists ... specify -force to overwrite them.\n");
        exit(-1);
      case ERR_NOENTRY:
        fprintf (stderr, "The entry specified does not exist\n");
        exit(-1);
    };

  return 0;
}
