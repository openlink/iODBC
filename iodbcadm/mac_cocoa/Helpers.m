/*
 *  Helpers.m
 *
 *  $Id$
 *
 *  The iODBC driver manager.
 *
 *  Copyright (C) 1996-2014 by OpenLink Software <iodbc@openlinksw.com>
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

#import "Helpers.h"
#include "iodbc.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <pwd.h>

#include "dlf.h"


static BOOL _CheckDriverLoginDlg (char *drv);
static void filedsn_configure (char *drv, char *dsn, char *in_attrs, BOOL b_add, BOOL verify_conn);
static BOOL test_driver_connect (char *connstr);

//??--extern void __create_message (void* hwnd, const void *dsn, const void *text, char waMode, int alertType);


/** helpers ***/
void addComponents_to_list(NSArrayController* list)
{
    NSArray *components = @[@"org.iodbc.core", @"org.iodbc.inst",  @"org.iodbc.adm",  @"org.iodbc.drvproxy",  @"org.iodbc.trans"];
    NSBundle *bundle, *bundle0 = nil;
    
    [list removeObjects:[list arrangedObjects]];
    
    for(int i=0; i < components.count; i++)
    {
        BOOL isBundle = FALSE;
        NSString *a_name;
        NSString *a_ver;
        NSString *a_ident;
        NSString *a_date = @"-";
        NSString *a_size = @"-";
        NSString *a_comp_name = [components objectAtIndex:i];
        bundle = [NSBundle bundleWithIdentifier:a_comp_name];
        
        if (i == 0)
            bundle0 = bundle;
        
        if (bundle)
        {
            NSURL *liburl;
            CFStringRef cflibname;
            NSDictionary *bundledict = [bundle infoDictionary];
            a_name = [bundledict valueForKey:@"CFBundleName"];
            a_ver = [bundledict valueForKey:@"CFBundleVersion"];
            a_ident = bundle.bundleIdentifier;

            NSString *type_pkg = [bundledict valueForKey:@"CFBundlePackageType"];
            if (type_pkg && [type_pkg isEqualToString:@"BNDL"])
                isBundle = TRUE;
            
            NSString *exec_name = [bundledict valueForKey:@"CFBundleExecutable"];
            
            if (isBundle && bundle0)
            {
                NSString *mstr = [NSString stringWithFormat:@"%@.bundle/Contents/MacOS", exec_name];
                liburl = [bundle0 URLForResource:exec_name withExtension:nil subdirectory:mstr];
            }
            else
                liburl = [bundle URLForResource:exec_name withExtension:nil subdirectory:nil];
            
            if (liburl && (cflibname = CFURLCopyFileSystemPath ((CFURLRef)liburl, kCFURLPOSIXPathStyle)))
            {
                char *libname = conv_NSString_to_char((NSString*)cflibname);
                if (libname!=NULL)
                {
                    struct stat _stat;
                    /* Get some information about the component */
                    if (!stat (libname, &_stat))
                    {
                        struct tm drivertime;
                        char buf[100];
                        
                        localtime_r (&_stat.st_mtime, &drivertime);
                        strftime (buf, sizeof(buf), "%c", &drivertime);
                        a_date = conv_char_to_NSString(buf);
                        a_size = [NSString stringWithFormat:@"%d Kb", (int) (_stat.st_size / 1024)];
                    }
                    free(libname);
                }
                CFRelease(cflibname);
            }

            [list addObject:[NSMutableDictionary dictionaryWithObjectsAndKeys:a_name, @"name",
                             a_ver, @"ver", a_ident, @"file", a_date, @"date", a_size, @"size", nil]];
        }
    }
}

void addPools_to_list(NSArrayController* list)
{
    wchar_t drvdesc[1024], drvattrs[1024];
    SQLSMALLINT len, len1;
    SQLRETURN ret;
    HENV henv;
    
    [list removeObjects:[list arrangedObjects]];
    
    /* Create a HENV to get the list of data sources then */
    ret = SQLAllocHandle (SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
    {
        _iodbcdm_nativeerrorbox ((void*)1L, henv, SQL_NULL_HANDLE, SQL_NULL_HANDLE);
        goto end;
    }
    
    /* Set the version ODBC API to use */
    SQLSetEnvAttr (henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER) SQL_OV_ODBC3,
                   SQL_IS_UINTEGER);
    
    /* Get the list of drivers */
    ret = SQLDriversW (henv, SQL_FETCH_FIRST, drvdesc, sizeof (drvdesc)/sizeof(wchar_t),
                       &len, drvattrs, sizeof (drvattrs)/sizeof(wchar_t), &len1);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO && ret != SQL_NO_DATA)
    {
        _iodbcdm_nativeerrorbox ((void*)1L, henv, SQL_NULL_HANDLE, SQL_NULL_HANDLE);
        goto error;
    }
    
    while (ret != SQL_NO_DATA)
    {
        NSString *a_name = conv_wchar_to_NSString(drvdesc);
        NSString *a_timeout = NULL;
        NSString *a_query = NULL;
        
        /* Get the driver library name */
        SQLSetConfigMode (ODBC_BOTH_DSN);

        SQLGetPrivateProfileStringW (drvdesc, L"CPTimeout", L"", drvattrs,
                                     sizeof (drvattrs) / sizeof(wchar_t), L"odbcinst.ini");
        
        if (drvattrs[0] == L'\0')
            SQLGetPrivateProfileStringW (L"Default", L"CPTimeout", L"", drvattrs,
                                         sizeof (drvattrs) / sizeof(wchar_t), L"odbcinst.ini");
        a_timeout = conv_wchar_to_NSString(drvattrs);
        
        SQLGetPrivateProfileStringW (drvdesc, L"CPProbe", L"", drvattrs,
                                     sizeof (drvattrs) / sizeof(wchar_t), L"odbcinst.ini");
        if (drvattrs[0] == L'\0')
            SQLGetPrivateProfileStringW (L"Default", L"CPProbe", L"", drvattrs,
                                         sizeof (drvattrs) / sizeof(wchar_t), L"odbcinst.ini");

        a_query = conv_wchar_to_NSString(drvattrs);
        
        [list addObject:[NSMutableDictionary dictionaryWithObjectsAndKeys:a_name, @"drv",
                         a_timeout, @"timeout", a_query, @"query", nil]];
        
        /* Process next one */
        ret = SQLDriversW (henv, SQL_FETCH_NEXT, drvdesc,
                           sizeof (drvdesc)/sizeof(wchar_t), &len, drvattrs,
                           sizeof (drvattrs)/sizeof(wchar_t), &len1);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO && ret != SQL_NO_DATA)
        {
            _iodbcdm_nativeerrorbox ((void*)1L, henv, SQL_NULL_HANDLE,
                                     SQL_NULL_HANDLE);
            goto error;
        }
    }
    
error:
    /* Clean all that */
    SQLFreeHandle (SQL_HANDLE_ENV, henv);
    
end:
    return;
}



void addDrivers_to_list(NSArrayController* list)
{
    wchar_t drvdesc[1024], drvattrs[1024], driver[1024];
    void *handle;
    struct stat _stat;
    SQLSMALLINT len, len1;
    SQLRETURN ret;
    HENV henv, drv_henv;
    HDBC drv_hdbc;
    pSQLGetInfoFunc funcHdl;
    pSQLAllocHandle allocHdl;
    pSQLAllocEnv allocEnvHdl = NULL;
    pSQLAllocConnect allocConnectHdl = NULL;
    pSQLFreeHandle freeHdl;
    pSQLFreeEnv freeEnvHdl;
    pSQLFreeConnect freeConnectHdl;
    char *_drv_u8 = NULL;
    
    
    [list removeObjects:[list arrangedObjects]];
    
    /* Create a HENV to get the list of data sources then */
    ret = SQLAllocHandle (SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
    {
        _iodbcdm_nativeerrorbox ((void*)1L, henv, SQL_NULL_HANDLE, SQL_NULL_HANDLE);
        goto end;
    }
    
    /* Set the version ODBC API to use */
    SQLSetEnvAttr (henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER) SQL_OV_ODBC3,
                   SQL_IS_UINTEGER);
    
    /* Get the list of drivers */
    ret = SQLDriversW (henv, SQL_FETCH_FIRST, drvdesc, sizeof (drvdesc)/sizeof(wchar_t),
                 &len, drvattrs, sizeof (drvattrs)/sizeof(wchar_t), &len1);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO && ret != SQL_NO_DATA)
    {
        _iodbcdm_nativeerrorbox ((void*)1L, henv, SQL_NULL_HANDLE, SQL_NULL_HANDLE);
        goto error;
    }
    
    while (ret != SQL_NO_DATA)
    {
        NSString *a_name = conv_wchar_to_NSString(drvdesc);
        NSString *a_drv = NULL;
        NSString *a_ver = NULL;
        NSString *a_size = NULL;
        NSString *a_date = NULL;
        
        /* Get the driver library name */
        SQLSetConfigMode (ODBC_BOTH_DSN);
        SQLGetPrivateProfileStringW (drvdesc, L"Driver", L"", driver,
                                     sizeof (driver)/sizeof(wchar_t), L"odbcinst.ini");
        if (driver[0] == L'\0')
            SQLGetPrivateProfileStringW (L"Default", L"Driver", L"", driver,
                                         sizeof (driver)/sizeof(wchar_t), L"odbcinst.ini");
        if (driver[0] == L'\0')
            goto skip;

        a_drv = conv_wchar_to_NSString(driver);
        
        /* Alloc a connection handle */
        drv_hdbc = NULL;
        drv_henv = NULL;
        
        _drv_u8 = (char *) conv_NSString_to_char(a_drv);
        if (_drv_u8 == NULL)
            goto skip;
        
        if ((handle = DLL_OPEN(_drv_u8)) != NULL)
        {
            if ((allocHdl = (pSQLAllocHandle)DLL_PROC(handle, "SQLAllocHandle")) != NULL)
            {
                ret = allocHdl(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &drv_henv);
                if (ret == SQL_ERROR) goto nodriverver;
                ret = allocHdl(SQL_HANDLE_DBC, drv_henv, &drv_hdbc);
                if (ret == SQL_ERROR) goto nodriverver;
            }
            else
            {
                if ((allocEnvHdl = (pSQLAllocEnv)DLL_PROC(handle, "SQLAllocEnv")) != NULL)
                {
                    ret = allocEnvHdl(&drv_henv);
                    if (ret == SQL_ERROR) goto nodriverver;
                }
                else goto nodriverver;
                
                if ((allocConnectHdl = (pSQLAllocConnect)DLL_PROC(handle, "SQLAllocConnect")) != NULL)
                {
                    ret = allocConnectHdl(drv_henv, &drv_hdbc);
                    if (ret == SQL_ERROR) goto nodriverver;
                }
                else goto nodriverver;
            }
            
            /*
             *  Use SQLGetInfoA for Unicode drivers
             *  and SQLGetInfo  for ANSI drivers
             */
            funcHdl = (pSQLGetInfoFunc)DLL_PROC(handle, "SQLGetInfoA");
            if (!funcHdl)
                funcHdl = (pSQLGetInfoFunc)DLL_PROC(handle, "SQLGetInfo");
            
            if (funcHdl)
            {
                /* Retrieve some informations */
                ret = funcHdl (drv_hdbc, SQL_DRIVER_VER, drvattrs, sizeof(drvattrs), &len);
                if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
                {
                    char *p = (char*)drvattrs;
                    
                    /* Find the description if one provided */
                    for (; *p ; p++)
                    {
                        if (*p == ' ')
                        {
                            *p++ = '\0';
                            break;
                        }
                    }
                    
                    /*
                     * Store Version
                     */
                    a_ver = conv_char_to_NSString((char*)drvattrs);
                }
                else goto nodriverver;
            }
            else if ((funcHdl = (pSQLGetInfoFunc)DLL_PROC(handle, "SQLGetInfoW")) != NULL)
            {
                /* Retrieve some informations */
                ret = funcHdl (drv_hdbc, SQL_DRIVER_VER, drvattrs, sizeof(drvattrs), &len);
                if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
                {
                    wchar_t *p = drvattrs;
                    
                    /* Find the description if one provided */
                    for (; *p ; p++)
                    {
                        if (*p == L' ')
                        {
                            *p++ = L'\0';
                            break;
                        }
                    }
                    
                    /*
                     * Store Version
                     */
                    a_ver = conv_wchar_to_NSString(drvattrs);
                }
                else goto nodriverver;
                
            }
            else goto nodriverver;
        }
        else
        {
        nodriverver:
            a_ver = @"##.##";
        }
        
        if(drv_hdbc || drv_henv)
        {
            if(allocConnectHdl &&
               (freeConnectHdl = (pSQLFreeConnect)DLL_PROC(handle, "SQLFreeConnect")) != NULL)
            { freeConnectHdl(drv_hdbc); drv_hdbc = NULL; }
            
            if(allocEnvHdl &&
               (freeEnvHdl = (pSQLFreeEnv)DLL_PROC(handle, "SQLFreeEnv")) != NULL)
            { freeEnvHdl(drv_henv); drv_henv = NULL; }
        }
        
        if ((drv_hdbc || drv_henv) &&
            (freeHdl = (pSQLFreeHandle)DLL_PROC(handle, "SQLFreeHandle")) != NULL)
        {
            if(drv_hdbc) freeHdl(SQL_HANDLE_DBC, drv_hdbc);
            if(drv_henv) freeHdl(SQL_HANDLE_ENV, drv_henv);
        }
        
        DLL_CLOSE(handle);
        
        /* Get the size and date of the driver */
        if (!stat (_drv_u8, &_stat))
        {
            struct tm drivertime;
            char buf[100];
            
            a_size = [NSString stringWithFormat:@"%d Kb", (int) (_stat.st_size / 1024)];
            
            localtime_r (&_stat.st_mtime, &drivertime);
            strftime (buf, sizeof (buf), "%c", &drivertime);
            a_date = conv_char_to_NSString(buf);
        }
        else
        {
            a_size = @"-";
            a_date = @"-";
        }
        
        [list addObject:[NSMutableDictionary dictionaryWithObjectsAndKeys:a_name, @"name",
                         a_drv, @"file", a_ver, @"ver",
                         a_size, @"size", a_date, @"date", nil]];
        /* Process next one */
    skip:
        MEM_FREE (_drv_u8);
        _drv_u8 = NULL;
        
        ret = SQLDriversW (henv, SQL_FETCH_NEXT, drvdesc,
                           sizeof (drvdesc)/sizeof(wchar_t), &len, drvattrs,
                           sizeof (drvattrs)/sizeof(wchar_t), &len1);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO && ret != SQL_NO_DATA)
        {
            _iodbcdm_nativeerrorbox ((void*)1L, henv, SQL_NULL_HANDLE,
                                     SQL_NULL_HANDLE);
            goto error;
        }
    }
    
error:
    /* Clean all that */
    SQLFreeHandle (SQL_HANDLE_ENV, henv);
    
end:
    return;
}


void addDSNs_to_list(BOOL systemDSN, NSArrayController* list)
{
    wchar_t dsnname[1024], dsndesc[1024];
    SQLSMALLINT len;
    SQLRETURN ret;
    HENV henv;
    
    [list removeObjects:[list arrangedObjects]];
    
    /* Create a HENV to get the list of data sources then */
    ret = SQLAllocHandle (SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
    {
        _iodbcdm_nativeerrorbox ((void*)1L, henv, SQL_NULL_HANDLE, SQL_NULL_HANDLE);
        goto end;
    }
    
    /* Set the version ODBC API to use */
    SQLSetEnvAttr (henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER) SQL_OV_ODBC3,
                   SQL_IS_UINTEGER);
    
    /* Get the list of datasources */
    ret = SQLDataSourcesW (henv,
                           systemDSN ? SQL_FETCH_FIRST_SYSTEM : SQL_FETCH_FIRST_USER,
                           dsnname, sizeof (dsnname)/sizeof(wchar_t), &len,
                           dsndesc, sizeof (dsndesc)/sizeof(wchar_t), NULL);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO && ret != SQL_NO_DATA)
    {
        _iodbcdm_nativeerrorbox ((void*)1L, henv, SQL_NULL_HANDLE, SQL_NULL_HANDLE);
        goto error;
    }
    
    while (ret != SQL_NO_DATA)
    {
        NSString *name = conv_wchar_to_NSString(dsnname);
        if (dsndesc[0] == L'\0')
        {
            SQLSetConfigMode (ODBC_BOTH_DSN);
            SQLGetPrivateProfileStringW (L"Default", L"Driver", L"", dsndesc,
                                         sizeof (dsndesc)/sizeof(wchar_t), L"odbc.ini");
        }
        NSString *drv = dsndesc[0]!=L'\0'? conv_wchar_to_NSString(dsndesc): @"-";
        
        /* Get the description */
        SQLSetConfigMode (systemDSN ? ODBC_SYSTEM_DSN : ODBC_USER_DSN);
        SQLGetPrivateProfileStringW (dsnname, L"Description", L"", dsndesc,
                                     sizeof (dsndesc)/sizeof(wchar_t), L"odbc.ini");
        
        NSString *desc = dsndesc[0]!=L'\0'? conv_wchar_to_NSString(dsndesc): @"-";
        
        [list addObject:[NSMutableDictionary dictionaryWithObjectsAndKeys:name, @"name",
                         desc, @"desc", drv, @"drv", nil]];
        
        /* Process next one */
        ret = SQLDataSourcesW (henv, SQL_FETCH_NEXT, dsnname,
                               sizeof (dsnname)/sizeof(wchar_t), &len, dsndesc,
                               sizeof (dsndesc)/sizeof(wchar_t), NULL);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO
            && ret != SQL_NO_DATA)
        {
            _iodbcdm_nativeerrorbox ((void*)1L, henv, SQL_NULL_HANDLE, SQL_NULL_HANDLE);
            goto error;
        }
    }
    
error:
    /* Clean all that */
    SQLFreeHandle (SQL_HANDLE_ENV, henv);
end:
    return;
}

#define MAX_ROWS 1024

void addFDSNs_to_list(char* path, BOOL b_reset, NSArrayController* list)
{
    int nrows;
    DIR *dir;
    char *path_buf;
    struct dirent *dir_entry;
    struct stat fstat;
    int b_added;
    
    [list removeObjects:[list arrangedObjects]];
    nrows = 0;

    if ((dir = opendir (path)))
    {
        while ((dir_entry = readdir (dir)) && nrows < MAX_ROWS)
        {
            asprintf (&path_buf, "%s/%s", path, dir_entry->d_name);
            b_added = 0;
            
            if (stat ((const char*) path_buf, &fstat) >= 0 && S_ISDIR (fstat.st_mode))
            {
                if (dir_entry->d_name && dir_entry->d_name[0] != '.')
                {
                    NSString *name = conv_char_to_NSString(dir_entry->d_name);
                    NSImage *icon = [NSImage imageNamed:NSImageNameFolder];
                    [list addObject:[NSMutableDictionary dictionaryWithObjectsAndKeys:name, @"name",
                                     icon, @"icon", [NSNumber numberWithBool:TRUE], @"isdir", nil]];
                    nrows++;
                    b_added = 1;
                }
            }
            else if (stat ((const char*) path_buf, &fstat) >= 0 && !S_ISDIR (fstat.st_mode)
                     && strstr (dir_entry->d_name, ".dsn"))
            {
                NSString *name = conv_char_to_NSString(dir_entry->d_name);
                NSImage *icon = [[NSWorkspace sharedWorkspace]
                                 iconForFileType:NSFileTypeForHFSTypeCode(kGenericDocumentIcon)];
                [list addObject:[NSMutableDictionary dictionaryWithObjectsAndKeys:name, @"name",
                                 icon, @"icon", [NSNumber numberWithBool:FALSE], @"isdir", nil]];
                nrows++;
                b_added = 1;
            }
            
            if (path_buf)
                free (path_buf);
        }
        
        /* Close the directory entry */
        closedir (dir);
    }
    else
        create_error (NULL, NULL, "Error during accessing directory information",
                      strerror (errno));
    
//??    if (b_reset)
//??        SetDataBrowserScrollPosition(widget, 0, 0);
}

void fill_dir_menu(char* path, NSPopUpButton* list)
{
    char *curr_dir, *prov, *dir;
    
    if (!path || !(prov = strdup (path)))
        return;
    
    if (prov[strlen(prov) - 1] == '/' && strlen(prov) > 1)
        prov[strlen(prov) - 1] = 0;
    
    [list removeAllItems];
    
    /* Add the root directory */
    [list addItemWithTitle:@"/"];
    
    if (strlen(prov) > 1)
        for (curr_dir = prov, dir = NULL; curr_dir;
             curr_dir = strchr (curr_dir + 1, '/'))
        {
            if (strchr (curr_dir + 1, '/'))
            {
                dir = strchr (curr_dir + 1, '/');
                *dir = 0;
            }

            [list addItemWithTitle:conv_char_to_NSString(prov)];
            
            if (dir)
                *dir = '/';
        }
    free(prov);
    [list selectItemAtIndex:list.numberOfItems-1];
}


BOOL remove_dsn(BOOL systemDSN, NSString *_dsn, NSString *_driver)
{
    wchar_t *szDSN = conv_NSString_to_wchar(_dsn);
    wchar_t *szDriver = conv_NSString_to_wchar(_driver);
    wchar_t dsn_remove[1024] = { L'\0' };
    BOOL ret = FALSE;
    
    
    if (szDSN && szDriver){
        if (create_confirmw ((void*)1L, szDSN, L"Are you sure you want to remove this DSN ?"))
        {
            /* Call the right function */
            WCSCPY(dsn_remove, L"DSN="); WCSCAT(dsn_remove, szDSN);
            dsn_remove[WCSLEN(dsn_remove)+1] = L'\0';
            if (!SQLConfigDataSourceW ((void*)1L, systemDSN?ODBC_REMOVE_SYS_DSN:ODBC_REMOVE_DSN, szDriver, dsn_remove))
                _iodbcdm_errorboxw ((void*)1L, szDSN, L"An error occurred when trying to remove the DSN ");
            ret = TRUE;
        }
    }
    if (szDSN) free(szDSN);
    if (szDSN) free(szDriver);
    return ret;
}


BOOL configure_dsn(BOOL systemDSN, NSString *dsn, NSString *driver)
{
    wchar_t *szDSN = conv_NSString_to_wchar(dsn);
    wchar_t *szDriver = conv_NSString_to_wchar(driver);
    wchar_t connstr[4096] = { L'\0' }, tokenstr[4096] = { L'\0' };
    int size = sizeof (connstr)/sizeof(wchar_t);
    wchar_t *curr, *cour;
    DWORD error;
    BOOL ret = FALSE;
    
    if (szDSN && szDriver){
        /* Call the right function */
        WCSCPY(connstr, L"DSN="); WCSCAT(connstr, szDSN);
        connstr[WCSLEN(connstr)+1] = L'\0';
        size -= (WCSLEN (connstr) + 1);
        
        SQLSetConfigMode (systemDSN? ODBC_SYSTEM_DSN: ODBC_USER_DSN);
        if (!SQLGetPrivateProfileStringW (szDSN, NULL, L"",
                                          tokenstr, sizeof (tokenstr)/sizeof(wchar_t), NULL))
        {
            _iodbcdm_errorboxw ((void*)1L, szDSN,
                                L"An error occurred when trying to configure the DSN ");
            goto done;
        }
        
        for (curr = tokenstr, cour = connstr + WCSLEN (connstr) + 1;
             *curr != L'\0' ; curr += (WCSLEN (curr) + 1), cour += (WCSLEN (cour) + 1))
        {
            WCSCPY (cour, curr);
            cour[WCSLEN (curr)] = L'=';
            SQLSetConfigMode (systemDSN? ODBC_SYSTEM_DSN: ODBC_USER_DSN);
            SQLGetPrivateProfileStringW (szDSN, curr, L"", cour +
                                         WCSLEN (curr) + 1, size - WCSLEN (curr) - 1, NULL);
            size -= (WCSLEN (cour) + 1);
        }
        
        *cour = L'\0';
        
        if (!SQLConfigDataSourceW ((void*)1L, systemDSN? ODBC_CONFIG_SYS_DSN:ODBC_CONFIG_DSN,
                                   szDriver, connstr))
        {
            if (SQLInstallerErrorW (1, &error, connstr,
                                    sizeof (connstr)/sizeof(wchar_t), NULL) !=
                SQL_NO_DATA && error != ODBC_ERROR_REQUEST_FAILED)
                create_errorw ((void*)1L, szDSN,
                               L"An error occurred when trying to configure the DSN : ",
                               connstr);
            goto done;
        }
        
        ret = TRUE;
    }
done:
    if (szDSN) free(szDSN);
    if (szDSN) free(szDriver);
    return ret;
}


void test_dsn(BOOL systemDSN, NSString *dsn, NSString *driver)
{
    wchar_t *szDSN = conv_NSString_to_wchar(dsn);
    wchar_t *szDriver = conv_NSString_to_wchar(driver);
    wchar_t connstr[4096] = { L'\0' }, outconnstr[4096] = { L'\0' };
    HENV henv;
    HDBC hdbc;
    SWORD buflen;
    
    
    if (szDSN && szDriver){
        /* Make the connection */
#if (ODBCVER < 0x300)
        if (SQLAllocEnv (&henv) != SQL_SUCCESS)
#else
        if (SQLAllocHandle (SQL_HANDLE_ENV, NULL, &henv) != SQL_SUCCESS)
#endif
        {
            _iodbcdm_nativeerrorbox ((void*)1L, henv,
                                     SQL_NULL_HDBC, SQL_NULL_HSTMT);
            goto done;
        }
        
#if (ODBCVER < 0x300)
        if (SQLAllocConnect (henv, &hdbc) != SQL_SUCCESS)
#else
        SQLSetEnvAttr (henv, SQL_ATTR_ODBC_VERSION,
                       (SQLPOINTER) SQL_OV_ODBC3, SQL_IS_UINTEGER);
        if (SQLAllocHandle (SQL_HANDLE_DBC, henv, &hdbc) != SQL_SUCCESS)
#endif
        {
            _iodbcdm_nativeerrorbox ((void*)1L, henv, hdbc,
                                     SQL_NULL_HSTMT);
            SQLFreeEnv (henv);
            goto done;
        }
        
        WCSCPY(connstr, L"DSN=");
        WCSCAT(connstr, szDSN);
        
        SQLSetConfigMode (systemDSN?ODBC_SYSTEM_DSN: ODBC_USER_DSN);
        if (SQLDriverConnectW (hdbc, (void*)1L, connstr, SQL_NTS,
                               outconnstr, sizeof (outconnstr) / sizeof(wchar_t), &buflen,
                               SQL_DRIVER_PROMPT) != SQL_SUCCESS)
        {
            _iodbcdm_nativeerrorbox ((void*)1L, henv, hdbc, SQL_NULL_HSTMT);
        }
        else
        {
            create_messagew ((void*)1L, szDSN,
                          L"The connection DSN was tested successfully, and can be used at this time.");
            SQLDisconnect (hdbc);
        }
        
#if (ODBCVER < 0x300)
        SQLFreeConnect (hdbc);
        SQLFreeEnv (henv);
#else
        SQLFreeHandle (SQL_HANDLE_DBC, hdbc);
        SQLFreeHandle (SQL_HANDLE_ENV, henv);
#endif

    }
done:
    if (szDSN) free(szDSN);
    if (szDSN) free(szDriver);
}


BOOL remove_file_dsn(NSString *cur_dir, NSString *dsn)
{
    if (cur_dir && dsn) {
        NSString *nspath = [NSString stringWithFormat:@"%@/%@", cur_dir, dsn];
        char *path = conv_NSString_to_char(nspath);
        
        if (path)
        {
            if (create_confirm ((void*)1L, path,
                                "Are you sure you want to remove this File DSN ?"))
            {
                /* Call the right function */
                if (unlink(path) < 0)
                {
                    create_error ((void*)1L, NULL,
                                  "Error removing file DSN:", strerror (errno));
                }
            }
            free(path);
            return TRUE;
        }
    }
    return FALSE;
}

void test_file_dsn(NSString *cur_dir, NSString *dsn)
{
    if (cur_dir && dsn) {
        NSString *nspath = [NSString stringWithFormat:@"%@/%@", cur_dir, dsn];
        char *path = conv_NSString_to_char(nspath);
        
        if (path)
        {
            char connstr[4096] = { 0 };
            /* Create connection string and connect to data source */
            snprintf (connstr, sizeof (connstr), "FILEDSN=%s", path);
            if (test_driver_connect(connstr))
            {
                _iodbcdm_messagebox ((void*)1L, path,
                                     "The connection DSN was tested successfully, and can be used at this time.");
            }
            free(path);
        }
    }
}


BOOL configure_file_dsn(NSString *cur_dir, NSString *dsn)
{
    BOOL ret = FALSE;
    char *path = NULL;
    char *drv = NULL;
    char *attrs = NULL;
    char *_attrs = NULL;	/* attr list */
    size_t len = 0;	/* current attr list length (w/o list-terminating NUL) */
    char *p, *p_next;
    WORD read_len;
    char entries[4096];
    
    
    if (cur_dir && dsn) {
        NSString *nspath = [NSString stringWithFormat:@"%@/%@", cur_dir, dsn];
        path = conv_NSString_to_char(nspath);
        
        if (path)
        {
            /* Get list of entries in .dsn file */
            if (!SQLReadFileDSN (path, "ODBC", NULL,
                                 entries, sizeof (entries), &read_len))
            {
                create_error ((void*)1L, NULL, "SQLReadFileDSN failed", NULL);
                goto done;
            }
            
            /* add params from the .dsn file */
            for (p = entries; *p != '\0'; p = p_next)
            {
                char *tmp;
                size_t add_len;		/* length of added attribute */
                char value[1024];
                
                /* get next entry */
                p_next = strchr (p, ';');
                if (p_next)
                    *p_next++ = '\0';
                
                if (!SQLReadFileDSN (path, "ODBC", p, value, sizeof(value), &read_len))
                {
                    create_error ((void*)1L, NULL, "SQLReadFileDSN failed", NULL);
                    goto done;
                }
                
                if (!strcasecmp (p, "DRIVER"))
                {
                    /* got driver keyword */
                    add_len = strlen ("DRIVER=") + strlen (value) + 1;
                    drv = malloc (add_len);
                    snprintf (drv, add_len, "DRIVER=%s", value);
                    continue;
                }
                
                /* +1 for '=', +1 for NUL */
                add_len = strlen (p) + 1 + strlen (value) + 1;
                /* +1 for list-terminating NUL */;
                tmp = realloc (attrs, len + add_len + 1);
                if (tmp == NULL)
                {
                    create_error ((void*)1L, NULL, "Error adding file DSN:",
                                  strerror (errno));
                    goto done;
                }
                attrs = tmp;
                snprintf (attrs + len, add_len, "%s=%s", p, value);
                len += add_len;
            }
            
            if (drv == NULL)
            {
                /* no driver found, probably unshareable file data source */
                create_error ((void*)1L, NULL,
                              "Can't configure file DSN without DRIVER keyword (probably unshareable data source?)", NULL);
                goto done;
            }
            
            if (attrs == NULL)
                attrs = "\0\0";
            else
            {
                /* NUL-terminate the list */
                attrs[len] = '\0';
                _attrs = attrs;
            }
            
            /* Configure file DSN */
            filedsn_configure (drv, path, attrs, FALSE, TRUE);
        }
    }
done:
    if (path)
        free(path);
    return ret;
}


void setdir_file_dsn(NSString *cur_dir)
{
    char msg[4096];
    char *path = conv_NSString_to_char(cur_dir);

    if (path){
        /* confirm setting a directory */
        snprintf (msg, sizeof (msg),
                  "Are you sure that you want to make '%s' the default file DSN directory?",
                  path);
        if (!create_confirm ((void*)1L, NULL, msg))
            goto done;
        
        /* write FileDSNPath value */
        if (!SQLWritePrivateProfileString ("ODBC", "FileDSNPath",
                                           path, "odbcinst.ini"))
        {
            create_error ((void*)1L, NULL,
                          "Error setting default file DSN directory", NULL);
            goto done;
        }
        free(path);
    }
done:
    if (path)
        free(path);
}

BOOL add_file_dsn(NSString *_curr_dir)
{
    TFDRIVERCHOOSER drvchoose_t;
    BOOL ret = FALSE;
    
    /* Try first to get the driver name */
    SQLSetConfigMode (ODBC_USER_DSN);
    
    drvchoose_t.attrs = NULL;
    drvchoose_t.dsn = NULL;
    drvchoose_t.driver = NULL;
    drvchoose_t.curr_dir = (char*)_curr_dir.UTF8String;

    create_fdriverchooser ((void*)1L, &drvchoose_t);
    
    /* Check output parameters */
    if (drvchoose_t.ok == TRUE)
    {
        ret = TRUE;
        NSString *drv = [NSString stringWithFormat:@"DRIVER=%@", [conv_wchar_to_NSString(drvchoose_t.driver) autorelease]];
        char *attrs = drvchoose_t.attrs;
        
        filedsn_configure ((char*)drv.UTF8String, drvchoose_t.dsn,
                           attrs ? attrs :"\0\0", TRUE, drvchoose_t.verify_conn);
    }
    
    if (drvchoose_t.driver)
        free (drvchoose_t.driver);
    if (drvchoose_t.attrs)
        free (drvchoose_t.attrs);
    if (drvchoose_t.dsn)
        free (drvchoose_t.dsn);
    
    return ret;
}


static BOOL
test_driver_connect (char *connstr)
{
    HENV henv;
    HDBC hdbc;
    
#if (ODBCVER < 0x300)
    if (SQLAllocEnv (&henv) != SQL_SUCCESS)
#else
        if (SQLAllocHandle (SQL_HANDLE_ENV, NULL, &henv) != SQL_SUCCESS)
#endif
        {
            _iodbcdm_nativeerrorbox ((void*)1L,
                                     henv, SQL_NULL_HDBC, SQL_NULL_HSTMT);
            return FALSE;
        }
    
#if (ODBCVER < 0x300)
    if (SQLAllocConnect (henv, &hdbc) != SQL_SUCCESS)
#else
        SQLSetEnvAttr (henv, SQL_ATTR_ODBC_VERSION,
                       (SQLPOINTER) SQL_OV_ODBC3, SQL_IS_UINTEGER);
    if (SQLAllocHandle (SQL_HANDLE_DBC, henv, &hdbc) != SQL_SUCCESS)
#endif
    {
        _iodbcdm_nativeerrorbox ((void*)1L, henv, hdbc, SQL_NULL_HSTMT);
        SQLFreeEnv (henv);
        return FALSE;
    }
    
    SQLSetConfigMode (ODBC_BOTH_DSN);
    
    if (SQLDriverConnect (hdbc, (void*)1L, (SQLCHAR*)connstr, SQL_NTS,
                          NULL, 0, NULL, SQL_DRIVER_PROMPT) != SQL_SUCCESS)
    {
        _iodbcdm_nativeerrorbox ((void*)1L, henv, hdbc, SQL_NULL_HSTMT);
        SQLFreeEnv (henv);
        return FALSE;
    }
    else
    {
        SQLDisconnect (hdbc);
    }
    
#if (ODBCVER < 0x300)
    SQLFreeConnect (hdbc);
    SQLFreeEnv (henv);
#else
    SQLFreeHandle (SQL_HANDLE_DBC, hdbc);
    SQLFreeHandle (SQL_HANDLE_ENV, henv);
#endif
    
    return TRUE;
}


static void
filedsn_configure (char *drv, char *dsn, char *in_attrs, BOOL b_add, BOOL verify_conn)
{
    char *connstr = NULL;
    size_t len;			/* current connstr len    */
    size_t add_len;		/* len of appended string */
    LPSTR attrs = NULL, curr, tmp, attr_lst = NULL;
    BOOL b_Save = TRUE;
    
    attrs = in_attrs;
    
    if (!b_add && !_CheckDriverLoginDlg(drv + STRLEN("DRIVER=")))
    {
        /*  Get DSN name and additional attributes  */
        attr_lst = create_gensetup ((void*)1L, dsn, in_attrs,
                                    b_add, &verify_conn);
        attrs = attr_lst;
    }
    
    if (!attrs)
    {
        create_error ((void*)1L, NULL, "Error adding File DSN:",
                      strerror (ENOMEM));
        return;
    }
    if (attrs == (LPSTR) - 1L)
        return;
    
    
    /* Build the connection string */
    connstr = strdup (drv);
    len = strlen (connstr);
    for (curr = attrs; *curr; curr += (STRLEN (curr) + 1))
    {
        if (!strncasecmp (curr, "DSN=", STRLEN ("DSN=")))
        {
            if (dsn == NULL)
            {
                /* got dsn name */
                dsn = curr + STRLEN ("DSN=");
            }
            continue;
        }
        
        /* append attr */
        add_len = 1 + strlen (curr);			/* +1 for ';' */
        tmp = realloc (connstr, len + add_len + 1);	/* +1 for NUL */
        if (tmp == NULL)
        {
            create_error ((void*)1L, NULL, "Error adding File DSN:",
                          strerror (errno));
            goto done;
        }
        connstr = tmp;
        snprintf (connstr + len, add_len + 1, ";%s", curr);
        len += add_len;
    }
    
    /* Nothing to do if no DSN */
    if (!dsn || STRLEN (dsn) == 0)
        goto done;
    
    if (verify_conn)
    {
        BOOL ret;
        
        /* Append SAVEFILE */
        add_len = strlen (";SAVEFILE=") + strlen (dsn);
        tmp = realloc (connstr, len + add_len + 1);		/* +1 for NUL */
        if (tmp == NULL)
        {
            create_error ((void*)1L, NULL, "Error adding file DSN:",
                          strerror (errno));
            goto done;
        }
        connstr = tmp;
        snprintf (connstr + len, add_len + 1, ";SAVEFILE=%s", dsn);
        len += add_len;
        
        /* Connect to data source */
        ret = test_driver_connect (connstr);
        if (!ret && b_add)
        {
            if (create_confirm ((void*)1L, dsn,
                                "Can't check the connection. Do you want to store the FileDSN without verification ?"))
                b_Save = TRUE;
            else
                b_Save = FALSE;
        }
        else
            b_Save = FALSE;
    }
    
    if (b_Save)
    {
        char key[512];
        char *p;
        size_t sz;
        
        if (drv)
        {
            p = strchr(drv, '=');
            if (!SQLWriteFileDSN (dsn, "ODBC", "DRIVER", p + 1))
            {
                create_error ((void*)1L, NULL, "Error adding File DSN:",
                              strerror (errno));
                goto done;
            }
        }
        
        for (curr = attrs; *curr; curr += (STRLEN (curr) + 1))
        {
            if (!strncasecmp (curr, "DSN=", STRLEN ("DSN=")))
                continue;
            else if (!strncasecmp (curr, "PWD=", STRLEN ("PWD=")))
                continue;
            else if (!strncasecmp (curr, "SAVEFILE=", STRLEN ("SAVEFILE=")))
                continue;
            else if (!strncasecmp (curr, "FILEDSN=", STRLEN ("FILEDSN=")))
                continue;
            
            p = strchr(curr, '=');
            sz = p - curr < sizeof(key) ? p - curr : sizeof(key);
            memset(key, 0, sizeof(key));
            strncpy(key, curr, sz);
            
            if (!SQLWriteFileDSN (dsn, "ODBC", key, p + 1))
            {
                create_error ((void*)1L, NULL, "Error adding File DSN:",
                              strerror (errno));
                goto done;
            }
        }
    }
    
done:
    if (attr_lst != NULL)
        free (attr_lst);
    if (connstr != NULL)
        free (connstr);
}



#define CHECK_DRVCONN_DIALBOX(path) \
    if (path) \
    { \
        CFBundleRef bundle_dll = NULL; \
        char *tmp_path = strdup(path); \
        if (tmp_path) { \
            char *ptr = strstr(tmp_path, "/Contents/MacOS/"); \
            if (ptr) \
                *ptr = 0; \
            CFURLRef liburl = CFURLCreateFromFileSystemRepresentation (NULL, (UInt8*)tmp_path, strlen(tmp_path), FALSE); \
            CFArrayRef arr = CFBundleCopyExecutableArchitecturesForURL(liburl); \
            if (arr) \
                bundle_dll = CFBundleCreate (NULL, liburl); \
            if (arr) \
                CFRelease(arr); \
            if (liburl) \
                CFRelease(liburl); \
        } \
        MEM_FREE(tmp_path); \
        if (bundle_dll != NULL) \
        { \
            if (CFBundleGetFunctionPointerForName(bundle_dll, CFSTR("_iodbcdm_drvconn_dialboxw")) != NULL) \
            { \
                retVal = TRUE; \
                goto quit; \
            } \
            else if (CFBundleGetFunctionPointerForName(bundle_dll, CFSTR("_iodbcdm_drvconn_dialbox")) != NULL) \
            { \
                retVal = TRUE; \
                goto quit; \
            } \
        } \
    }



static BOOL _CheckDriverLoginDlg (char *drv)
{
    char drvbuf[4096] = { L'\0'};
    BOOL retVal = FALSE;
    
    if (!drv)
        return FALSE;
    
    SQLSetConfigMode (ODBC_USER_DSN);
    if (!access (drv, X_OK))
        { CHECK_DRVCONN_DIALBOX (drv); }
    if (SQLGetPrivateProfileString (drv, "Driver", "", drvbuf, sizeof (drvbuf), "odbcinst.ini"))
        { CHECK_DRVCONN_DIALBOX (drvbuf); }
    if (SQLGetPrivateProfileString (drv, "Setup", "", drvbuf, sizeof (drvbuf), "odbcinst.ini"))
        { CHECK_DRVCONN_DIALBOX (drvbuf); }
    
    SQLSetConfigMode (ODBC_SYSTEM_DSN);
    if (!access (drv, X_OK))
        { CHECK_DRVCONN_DIALBOX (drv); }
    if (SQLGetPrivateProfileString (drv, "Driver", "", drvbuf, sizeof (drvbuf), "odbcinst.ini"))
        { CHECK_DRVCONN_DIALBOX (drvbuf); }
    if (SQLGetPrivateProfileString (drv, "Setup", "", drvbuf, sizeof (drvbuf), "odbcinst.ini"))
        { CHECK_DRVCONN_DIALBOX (drvbuf); }
    
quit:
    return retVal;
}

