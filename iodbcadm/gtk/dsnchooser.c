/*
 *  dsnchooser.c
 *
 *  $Id$
 *
 *  The iODBC driver manager.
 *
 *  Copyright (C) 1996-2015 by OpenLink Software <iodbc@openlinksw.com>
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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <dirent.h>
#include <errno.h>

#include <sqltypes.h>
#include <dlproc.h>

#include "../gui.h"
#include "odbc4.xpm"


#ifndef MAXPATHLEN
#define MAXPATHLEN	1024
#endif

/*
 *  Linux glibc has the function, but does not have the prototype
 *  with the default header files.
 */
#if defined(HAVE_ASPRINTF) && defined(__GLIBC__)
extern int asprintf (char **ret, const char *format, ...);
#endif


char *szDSNColumnNames[] = {
  "Name",
  "Description",
  "Driver"
};

char *szTabNames[] = {
  "User DSN",
  "System DSN",
  "File DSN",
  "ODBC Drivers",
  "Connection Pooling",
  "Tracing",
  "About"
};

char *szDSNButtons[] = {
  "_Add",
  "_Remove",
  "Confi_gure",
  "_Test",
  "_Set Dir",
};


void
addlistofdir_to_optionmenu (GtkWidget *widget, LPCSTR path,
    TDSNCHOOSER *choose_t)
{
  GtkWidget *menu, *menu_item;
  char *curr_dir, *prov, *dir;
  void **array;

  if (!path || !GTK_IS_OPTION_MENU (widget) || !(prov = strdup (path)))
    return;

  if (prov[STRLEN (prov) - 1] == '/' && STRLEN (prov) > 1)
    prov[STRLEN (prov) - 1] = 0;

  /* Add the root directory */
  menu = gtk_menu_new ();
  menu_item = gtk_menu_item_new_with_label ("/");
  gtk_widget_show (menu_item);
  gtk_menu_prepend (GTK_MENU (menu), menu_item);
  if (!(array = (void **) malloc (sizeof (void *) * 2)))
    return;
  array[0] = g_strdup ("/");
  array[1] = choose_t;
  gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
      GTK_SIGNAL_FUNC (filedsn_lookin_clicked), array);

  for (curr_dir = prov, dir = NULL; curr_dir;
      curr_dir = strchr (curr_dir + 1, '/'))
    {
      if (strchr (curr_dir + 1, '/'))
	{
	  dir = strchr (curr_dir + 1, '/');
	  *dir = 0;
	}

      menu_item = gtk_menu_item_new_with_label (prov);
      gtk_widget_show (menu_item);
      gtk_menu_prepend (GTK_MENU (menu), menu_item);
      if (!(array = (void **) malloc (sizeof (void *) * 2)))
	return;
      array[0] = g_strdup (prov);
      array[1] = choose_t;
      gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
	  GTK_SIGNAL_FUNC (filedsn_lookin_clicked), array);
      if (dir)
	*dir = '/';
    }

  gtk_option_menu_remove_menu (GTK_OPTION_MENU (widget));
  gtk_option_menu_set_menu (GTK_OPTION_MENU (widget), menu);

  free (prov);
}


void
adddirectories_to_list (HWND hwnd, GtkWidget *widget, LPCSTR path)
{
  DIR *dir;
#if defined (HAVE_ASPRINTF)
  char *path_buf;
#else
  char path_buf[MAXPATHLEN];
#endif
  struct dirent *dir_entry;
  struct stat fstat;
  char *data[1];

  if (!path || !GTK_IS_CLIST (widget))
    return;

  if ((dir = opendir (path)))
    {
      /* Remove all the entries */
      gtk_clist_clear (GTK_CLIST (widget));

      while ((dir_entry = readdir (dir)))
	{
#if defined (HAVE_ASPRINTF)
	  asprintf (&path_buf, "%s/%s", path, dir_entry->d_name);
#elif defined (HAVE_SNPRINTF)
	  snprintf (path_buf, sizeof (path_buf), "%s/%s",
	      path, dir_entry->d_name);
#else
	  STRCPY (path_buf, path);
	  if (path[STRLEN (path) - 1] != '/')
	    STRCAT (path_buf, "/");
	  STRCAT (path_buf, dir_entry->d_name);
#endif

	  if (stat ((LPCSTR) path_buf, &fstat) >= 0
	      && S_ISDIR (fstat.st_mode))
	    if (strcmp (path, "/") || strcmp (dir_entry->d_name, ".."))
	      {
		data[0] = dir_entry->d_name;
		gtk_clist_append (GTK_CLIST (widget), data);
	      }

#if defined (HAVE_ASPRINTF)
	  free (path_buf);
#endif
	}

      /* Close the directory entry */
      closedir (dir);

      if (GTK_CLIST (widget)->rows > 0)
	gtk_clist_sort (GTK_CLIST (widget));
    }
  else
    create_error (hwnd, NULL, "Error during accessing directory information:",
	strerror (errno));
}


void
addfiles_to_list (HWND hwnd, GtkWidget *widget, LPCSTR path)
{
  DIR *dir;
#if defined (HAVE_ASPRINTF)
  char *path_buf;
#else
  char path_buf[MAXPATHLEN];
#endif
  struct dirent *dir_entry;
  struct stat fstat;
  char *data[1];

  if (!path || !GTK_IS_CLIST (widget))
    return;

  if ((dir = opendir (path)))
    {
      /* Remove all the entries */
      gtk_clist_clear (GTK_CLIST (widget));

      while ((dir_entry = readdir (dir)))
	{
#if defined (HAVE_ASPRINTF)
	  asprintf (&path_buf, "%s/%s", path, dir_entry->d_name);
#elif defined (HAVE_SNPRINTF)
	  snprintf (path_buf, sizeof (path_buf), "%s/%s",
	      path, dir_entry->d_name);
#else
	  STRCPY (path_buf, path);
	  if (path[STRLEN (path) - 1] != '/')
	    STRCAT (path_buf, "/");
	  STRCAT (path_buf, dir_entry->d_name);
#endif

	  if (stat ((LPCSTR) path_buf, &fstat) >= 0
	      && !S_ISDIR (fstat.st_mode)
	      && strstr (dir_entry->d_name, ".dsn"))
	    {
	      data[0] = dir_entry->d_name;
	      gtk_clist_append (GTK_CLIST (widget), data);
	    }

#if defined (HAVE_ASPRINTF)
	  free (path_buf);
#endif
	}

      /* Close the directory entry */
      closedir (dir);

      if (GTK_CLIST (widget)->rows > 0)
	gtk_clist_sort (GTK_CLIST (widget));
    }
}


void
adddsns_to_list (GtkWidget *widget, BOOL systemDSN)
{
  char *curr, *buffer = (char *) malloc (sizeof (char) * 65536);
  char diz[1024], driver[1024];
  char *data[3];
  int len, _case = 0;

  if (!buffer || !GTK_IS_CLIST (widget))
    return;
  gtk_clist_clear (GTK_CLIST (widget));

  /* Get the list of DSN in the user context */
  SQLSetConfigMode (systemDSN ? ODBC_SYSTEM_DSN : ODBC_USER_DSN);
  len =
      SQLGetPrivateProfileString ("ODBC Data Sources", NULL, "", buffer,
      65536, NULL);
  if (len)
    goto process;

  _case = 1;
  SQLSetConfigMode (systemDSN ? ODBC_SYSTEM_DSN : ODBC_USER_DSN);
  len =
      SQLGetPrivateProfileString ("ODBC 32 bit Data Sources", NULL, "",
      buffer, 65536, NULL);
  if (len)
    goto process;

  goto end;

process:
  for (curr = buffer; *curr; curr += (STRLEN (curr) + 1))
    {
      SQLSetConfigMode (systemDSN ? ODBC_SYSTEM_DSN : ODBC_USER_DSN);
      SQLGetPrivateProfileString (curr, "Description", "", diz, sizeof (diz),
	  NULL);
      SQLSetConfigMode (systemDSN ? ODBC_SYSTEM_DSN : ODBC_USER_DSN);
      switch (_case)
	{
	case 0:
	  SQLGetPrivateProfileString ("ODBC Data Sources", curr, "", driver,
	      sizeof (driver), NULL);
	  break;
	case 1:
	  SQLGetPrivateProfileString ("ODBC 32 bit Data Sources", curr, "",
	      driver, sizeof (driver), NULL);
	  break;
	};

      if (STRLEN (curr) && STRLEN (driver))
	{
	  data[0] = curr;
	  data[1] = diz;
	  data[2] = driver;
	  gtk_clist_append (GTK_CLIST (widget), data);
	}
    }

end:
  SQLSetConfigMode (ODBC_BOTH_DSN);

  if (GTK_CLIST (widget)->rows > 0)
    {
      gtk_clist_columns_autosize (GTK_CLIST (widget));
      gtk_clist_sort (GTK_CLIST (widget));
    }

  /* Make the clean up */
  free (buffer);
}


static void
dsnchooser_switch_page (GtkNotebook *notebook, GtkNotebookPage *page,
    gint page_num, TDSNCHOOSER *choose_t)
{
  switch (page_num)
    {
    case 0:
      if (choose_t)
	{
	  choose_t->type_dsn = USER_DSN;
	  adddsns_to_list (choose_t->udsnlist, FALSE);
	}
      break;

    case 1:
      if (choose_t)
	{
	  choose_t->type_dsn = SYSTEM_DSN;
	  adddsns_to_list (choose_t->sdsnlist, TRUE);
	}
      break;

    case 2:
      if (choose_t)
	{
	  choose_t->type_dsn = FILE_DSN;
          addlistofdir_to_optionmenu(choose_t->dir_combo,
	      choose_t->curr_dir, choose_t);
          adddirectories_to_list(choose_t->mainwnd,
	      choose_t->dir_list, choose_t->curr_dir);
          addfiles_to_list(choose_t->mainwnd,
	      choose_t->file_list, choose_t->curr_dir);
	}
      break;
    };

  if (choose_t)
    {
      if (choose_t->uremove)
	gtk_widget_set_sensitive (choose_t->uremove, FALSE);
      if (choose_t->uconfigure)
	gtk_widget_set_sensitive (choose_t->uconfigure, FALSE);
      if (choose_t->utest)
	gtk_widget_set_sensitive (choose_t->utest, FALSE);
      if (choose_t->sremove)
	gtk_widget_set_sensitive (choose_t->sremove, FALSE);
      if (choose_t->sconfigure)
	gtk_widget_set_sensitive (choose_t->sconfigure, FALSE);
      if (choose_t->stest)
	gtk_widget_set_sensitive (choose_t->stest, FALSE);
      if (choose_t->fremove)
        gtk_widget_set_sensitive(choose_t->fremove, FALSE);
      if (choose_t->fconfigure)
        gtk_widget_set_sensitive(choose_t->fconfigure, FALSE);
      if (choose_t->ftest)
        gtk_widget_set_sensitive(choose_t->ftest, FALSE);
    }
}


static BOOL
test_driver_connect (TDSNCHOOSER *choose_t, char *connstr)
{
  HENV henv;
  HDBC hdbc;

#if (ODBCVER < 0x300)
  if (SQLAllocEnv (&henv) != SQL_SUCCESS)
#else
  if (SQLAllocHandle (SQL_HANDLE_ENV, NULL, &henv) != SQL_SUCCESS)
#endif
    {
      _iodbcdm_nativeerrorbox (choose_t->mainwnd,
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
      _iodbcdm_nativeerrorbox (choose_t->mainwnd, henv, hdbc, SQL_NULL_HSTMT);
      SQLFreeEnv (henv);
      return FALSE;
    }

  switch (choose_t->type_dsn)
    {
    case USER_DSN:
      SQLSetConfigMode (ODBC_USER_DSN);
      break;
    case SYSTEM_DSN:
      SQLSetConfigMode (ODBC_SYSTEM_DSN);
      break;
    case FILE_DSN:
      SQLSetConfigMode (ODBC_BOTH_DSN);
      break;
    }
  if (SQLDriverConnect (hdbc, choose_t->mainwnd, connstr, SQL_NTS,
          NULL, 0, NULL, SQL_DRIVER_PROMPT) != SQL_SUCCESS)
    {
      _iodbcdm_nativeerrorbox (choose_t->mainwnd, henv, hdbc, SQL_NULL_HSTMT);
      SQLFreeEnv (henv);
      return FALSE;
    }
  else
    SQLDisconnect (hdbc);

#if (ODBCVER < 0x300)
  SQLFreeConnect (hdbc);
  SQLFreeEnv (henv);
#else
  SQLFreeHandle (SQL_HANDLE_DBC, hdbc);
  SQLFreeHandle (SQL_HANDLE_ENV, henv);
#endif

  return TRUE;
}


void
userdsn_add_clicked (GtkWidget *widget, TDSNCHOOSER *choose_t)
{
  char connstr[4096] = { "" };
  char drv[1024] = { ""};
  int sqlstat;
  DWORD error;

  if (choose_t)
    {
      /* Try first to get the driver name */
      SQLSetConfigMode (ODBC_USER_DSN);
      if (_iodbcdm_drvchoose_dialbox (choose_t->mainwnd, drv, 
      	sizeof (drv), &sqlstat) == SQL_SUCCESS)
	{
	  SQLSetConfigMode (ODBC_USER_DSN);
	  if (!SQLConfigDataSource (choose_t->mainwnd, ODBC_ADD_DSN,
		  drv + STRLEN ("DRIVER="), connstr))
	    {
	      if (SQLInstallerError (1, &error, connstr, 
	         sizeof (connstr), NULL) != SQL_NO_DATA)
		_iodbcdm_errorbox (choose_t->mainwnd, NULL,
		    "An error occurred when trying to add the DSN : ");
	      goto done;
	    }

	  adddsns_to_list (choose_t->udsnlist, FALSE);
	}

    done:
      if (GTK_CLIST (choose_t->udsnlist)->selection == NULL)
	{
	  if (choose_t->uremove)
	    gtk_widget_set_sensitive (choose_t->uremove, FALSE);
	  if (choose_t->uconfigure)
	    gtk_widget_set_sensitive (choose_t->uconfigure, FALSE);
	  if (choose_t->utest)
	    gtk_widget_set_sensitive (choose_t->utest, FALSE);
	}
    }

  return;
}


void
userdsn_remove_clicked (GtkWidget *widget, TDSNCHOOSER *choose_t)
{
  char dsn[1024] = { "\0" };
  char *szDSN = NULL, *szDriver = NULL;

  if (choose_t)
    {
      /* Retrieve the DSN name */
      if (GTK_CLIST (choose_t->udsnlist)->selection != NULL)
	{
	  gtk_clist_get_text (GTK_CLIST (choose_t->udsnlist),
	      GPOINTER_TO_INT (GTK_CLIST (choose_t->udsnlist)->selection->
		  data), 0, &szDSN);
	  gtk_clist_get_text (GTK_CLIST (choose_t->udsnlist),
	      GPOINTER_TO_INT (GTK_CLIST (choose_t->udsnlist)->selection->
		  data), 2, &szDriver);
	}

      /* Call the right function */
      if (szDSN
	  && create_confirm (choose_t->mainwnd, szDSN,
	      "Are you sure you want to remove this DSN ?"))
	{
	  sprintf (dsn, "DSN=%s", szDSN);
	  if (!SQLConfigDataSource (choose_t->mainwnd, ODBC_REMOVE_DSN,
		  szDriver, dsn))
	    _iodbcdm_errorbox (choose_t->mainwnd, szDSN,
		"An error occurred when trying to remove the DSN : ");
	  adddsns_to_list (choose_t->udsnlist, FALSE);
	}

      if (GTK_CLIST (choose_t->udsnlist)->selection == NULL)
	{
	  if (choose_t->uremove)
	    gtk_widget_set_sensitive (choose_t->uremove, FALSE);
	  if (choose_t->uconfigure)
	    gtk_widget_set_sensitive (choose_t->uconfigure, FALSE);
	  if (choose_t->utest)
	    gtk_widget_set_sensitive (choose_t->utest, FALSE);
	}
    }
}


void
userdsn_configure_clicked (GtkWidget *widget, TDSNCHOOSER *choose_t)
{
  char connstr[4096] = { 0 };
  char tokenstr[4096] = { 0 };
  char *szDSN = NULL, *szDriver = NULL, *curr, *cour;
  int size = sizeof (connstr);
  DWORD error;

  if (choose_t)
    {
      /* Retrieve the DSN name */
      if (GTK_CLIST (choose_t->udsnlist)->selection != NULL)
	{
	  gtk_clist_get_text (GTK_CLIST (choose_t->udsnlist),
	      GPOINTER_TO_INT (GTK_CLIST (choose_t->udsnlist)->selection->
		  data), 0, &szDSN);
	  gtk_clist_get_text (GTK_CLIST (choose_t->udsnlist),
	      GPOINTER_TO_INT (GTK_CLIST (choose_t->udsnlist)->selection->
		  data), 2, &szDriver);
	}

      /* Call the right function */
      if (szDSN)
	{
	  sprintf (connstr, "DSN=%s", szDSN);
	  size -= (STRLEN (connstr) + 1);

	  SQLSetConfigMode (ODBC_USER_DSN);
	  if (!SQLGetPrivateProfileString (szDSN, NULL, "", tokenstr,
		  sizeof (tokenstr), NULL))
	    {
	      _iodbcdm_errorbox (choose_t->mainwnd, szDSN,
		  "An error occurred when trying to configure the DSN : ");
	      goto done;
	    }

	  for (curr = tokenstr, cour = connstr + STRLEN (connstr) + 1; *curr;
	      curr += (STRLEN (curr) + 1), cour += (STRLEN (cour) + 1))
	    {
	      STRCPY (cour, curr);
	      cour[STRLEN (curr)] = '=';
	      SQLSetConfigMode (ODBC_USER_DSN);
	      SQLGetPrivateProfileString (szDSN, curr, "",
		  cour + STRLEN (curr) + 1, size - STRLEN (curr) - 1, NULL);
	      size -= (STRLEN (cour) + 1);
	    }

	  *cour = 0;

	  SQLSetConfigMode (ODBC_USER_DSN);
	  if (!SQLConfigDataSource (choose_t->mainwnd, ODBC_CONFIG_DSN,
		  szDriver, connstr))
	    {
	      if (SQLInstallerError (1, &error, connstr, sizeof (connstr),
		      NULL) != SQL_NO_DATA
		  && error != ODBC_ERROR_REQUEST_FAILED)
		_iodbcdm_errorbox (choose_t->mainwnd, szDSN,
		    "An error occurred when trying to configure the DSN : ");
	      goto done;
	    }

	  adddsns_to_list (choose_t->udsnlist, FALSE);
	}

    done:
      if (GTK_CLIST (choose_t->udsnlist)->selection == NULL)
	{
	  if (choose_t->uremove)
	    gtk_widget_set_sensitive (choose_t->uremove, FALSE);
	  if (choose_t->uconfigure)
	    gtk_widget_set_sensitive (choose_t->uconfigure, FALSE);
	  if (choose_t->utest)
	    gtk_widget_set_sensitive (choose_t->utest, FALSE);
	}
    }

  return;
}


void
userdsn_test_clicked (GtkWidget *widget, TDSNCHOOSER *choose_t)
{
  HENV henv;
  HDBC hdbc;
  SWORD buflen;
  char connstr[4096] = { 0 };
  char outconnstr[4096] = { 0 };
  char *szDSN;

  if (choose_t)
    {
      /* Retrieve the DSN name */
      if (GTK_CLIST (choose_t->udsnlist)->selection != NULL)
	gtk_clist_get_text (GTK_CLIST (choose_t->udsnlist),
	    GPOINTER_TO_INT (GTK_CLIST (choose_t->udsnlist)->selection->data),
	    0, &szDSN);

      if (szDSN && STRLEN (szDSN))
	{
	  snprintf (connstr, sizeof (connstr), "DSN=%s", szDSN);
	  if (test_driver_connect (choose_t, connstr))
	    {
	      _iodbcdm_messagebox (choose_t->mainwnd, szDSN,
		  "The connection DSN was tested successfully, and can be used at this time.");
	    }
	}

      if (GTK_CLIST (choose_t->udsnlist)->selection == NULL)
	{
	  if (choose_t->uremove)
	    gtk_widget_set_sensitive (choose_t->uremove, FALSE);
	  if (choose_t->uconfigure)
	    gtk_widget_set_sensitive (choose_t->uconfigure, FALSE);
	  if (choose_t->utest)
	    gtk_widget_set_sensitive (choose_t->utest, FALSE);
	}
    }
}


void
systemdsn_add_clicked (GtkWidget *widget, TDSNCHOOSER *choose_t)
{
  char connstr[4096] = { 0 };
  char drv[1024] = { 0 };
  int sqlstat;
  DWORD error;

  if (choose_t)
    {
      /* Try first to get the driver name */
      SQLSetConfigMode (ODBC_SYSTEM_DSN);
      if (_iodbcdm_drvchoose_dialbox (choose_t->mainwnd, drv, sizeof (drv),
	      &sqlstat) == SQL_SUCCESS)
	{
	  if (!SQLConfigDataSource (choose_t->mainwnd, ODBC_ADD_SYS_DSN,
		  drv + STRLEN ("DRIVER="), connstr))
	    {
	      if (SQLInstallerError (1, &error, connstr, sizeof (connstr),
		      NULL) != SQL_NO_DATA)
		_iodbcdm_errorbox (choose_t->mainwnd, NULL,
		    "An error occurred when trying to add the DSN : ");
	      goto done;
	    }

	  adddsns_to_list (choose_t->sdsnlist, TRUE);
	}

    done:
      if (GTK_CLIST (choose_t->sdsnlist)->selection == NULL)
	{
	  if (choose_t->sremove)
	    gtk_widget_set_sensitive (choose_t->sremove, FALSE);
	  if (choose_t->sconfigure)
	    gtk_widget_set_sensitive (choose_t->sconfigure, FALSE);
	  if (choose_t->stest)
	    gtk_widget_set_sensitive (choose_t->stest, FALSE);
	}
    }

  return;
}


void
systemdsn_remove_clicked (GtkWidget *widget, TDSNCHOOSER *choose_t)
{
  char dsn[1024] = { 0 };
  char *szDSN = NULL, *szDriver = NULL;

  if (choose_t)
    {
      /* Retrieve the DSN name */
      if (GTK_CLIST (choose_t->sdsnlist)->selection != NULL)
	{
	  gtk_clist_get_text (GTK_CLIST (choose_t->sdsnlist),
	      GPOINTER_TO_INT (GTK_CLIST (choose_t->sdsnlist)->selection->
		  data), 0, &szDSN);
	  gtk_clist_get_text (GTK_CLIST (choose_t->sdsnlist),
	      GPOINTER_TO_INT (GTK_CLIST (choose_t->sdsnlist)->selection->
		  data), 2, &szDriver);
	}

      /* Call the right function */
      if (szDSN
	  && create_confirm (choose_t->mainwnd, szDSN,
	      "Are you sure you want to remove this DSN ?"))
	{
	  sprintf (dsn, "DSN=%s", szDSN);
	  if (!SQLConfigDataSource (choose_t->mainwnd, ODBC_REMOVE_SYS_DSN,
		  szDriver, dsn))
	    _iodbcdm_errorbox (choose_t->mainwnd, szDSN,
		"An error occurred when trying to remove the DSN : ");
	  adddsns_to_list (choose_t->sdsnlist, TRUE);
	}

      if (GTK_CLIST (choose_t->sdsnlist)->selection == NULL)
	{
	  if (choose_t->sremove)
	    gtk_widget_set_sensitive (choose_t->sremove, FALSE);
	  if (choose_t->sconfigure)
	    gtk_widget_set_sensitive (choose_t->sconfigure, FALSE);
	  if (choose_t->stest)
	    gtk_widget_set_sensitive (choose_t->stest, FALSE);
	}
    }
}


void
systemdsn_configure_clicked (GtkWidget *widget, TDSNCHOOSER *choose_t)
{
  char connstr[4096] = { 0 };
  char tokenstr[4096] = { 0 };
  char *szDSN = NULL, *szDriver = NULL, *curr, *cour;
  int size = sizeof (connstr);
  DWORD error;

  if (choose_t)
    {
      /* Retrieve the DSN name */
      if (GTK_CLIST (choose_t->sdsnlist)->selection != NULL)
	{
	  gtk_clist_get_text (GTK_CLIST (choose_t->sdsnlist),
	      GPOINTER_TO_INT (GTK_CLIST (choose_t->sdsnlist)->selection->
		  data), 0, &szDSN);
	  gtk_clist_get_text (GTK_CLIST (choose_t->sdsnlist),
	      GPOINTER_TO_INT (GTK_CLIST (choose_t->sdsnlist)->selection->
		  data), 2, &szDriver);
	}

      /* Call the right function */
      if (szDSN)
	{
	  sprintf (connstr, "DSN=%s", szDSN);
	  size -= (STRLEN (connstr) + 1);

	  SQLSetConfigMode (ODBC_SYSTEM_DSN);
	  if (!SQLGetPrivateProfileString (szDSN, NULL, "", tokenstr,
		  sizeof (tokenstr), NULL))
	    {
	      _iodbcdm_errorbox (choose_t->mainwnd, szDSN,
		  "An error occurred when trying to configure the DSN : ");
	      goto done;
	    }

	  for (curr = tokenstr, cour = connstr + STRLEN (connstr) + 1; *curr;
	      curr += (STRLEN (curr) + 1), cour += (STRLEN (cour) + 1))
	    {
	      STRCPY (cour, curr);
	      cour[STRLEN (curr)] = '=';
	      SQLSetConfigMode (ODBC_SYSTEM_DSN);
	      SQLGetPrivateProfileString (szDSN, curr, "",
		  cour + STRLEN (curr) + 1, size - STRLEN (curr) - 1, NULL);
	      size -= (STRLEN (cour) + 1);
	    }

	  *cour = 0;

	  if (!SQLConfigDataSource (choose_t->mainwnd, ODBC_CONFIG_SYS_DSN,
		  szDriver, connstr))
	    {
	      if (SQLInstallerError (1, &error, connstr, sizeof (connstr),
		      NULL) != SQL_NO_DATA
		  && error != ODBC_ERROR_REQUEST_FAILED)
		_iodbcdm_errorbox (choose_t->mainwnd, szDSN,
		    "An error occurred when trying to configure the DSN : ");
	      goto done;
	    }

	  adddsns_to_list (choose_t->sdsnlist, TRUE);
	}

    done:
      if (GTK_CLIST (choose_t->sdsnlist)->selection == NULL)
	{
	  if (choose_t->sremove)
	    gtk_widget_set_sensitive (choose_t->sremove, FALSE);
	  if (choose_t->sconfigure)
	    gtk_widget_set_sensitive (choose_t->sconfigure, FALSE);
	  if (choose_t->stest)
	    gtk_widget_set_sensitive (choose_t->stest, FALSE);
	}
    }

  return;
}


void
systemdsn_test_clicked (GtkWidget * widget, TDSNCHOOSER * choose_t)
{
  HENV henv;
  HDBC hdbc;
  SWORD buflen;
  SQLCHAR connstr[4096] = { 0 };
  SQLCHAR outconnstr[4096] = { 0 };
  char *szDSN = NULL;

  if (choose_t)
    {
      /* Retrieve the DSN name */
      if (GTK_CLIST (choose_t->sdsnlist)->selection != NULL)
	gtk_clist_get_text (GTK_CLIST (choose_t->sdsnlist),
	    GPOINTER_TO_INT (GTK_CLIST (choose_t->sdsnlist)->selection->data),
	    0, &szDSN);

      if (szDSN && STRLEN (szDSN))
	{
	  snprintf (connstr, sizeof (connstr), "DSN=%s", szDSN);
	  if (test_driver_connect (choose_t, connstr))
	    {
	      _iodbcdm_messagebox (choose_t->mainwnd, szDSN,
		  "The connection DSN was tested successfully, and can be used at this time.");
	    }
        }

      if (GTK_CLIST (choose_t->sdsnlist)->selection == NULL)
	{
	  if (choose_t->sremove)
	    gtk_widget_set_sensitive (choose_t->sremove, FALSE);
	  if (choose_t->sconfigure)
	    gtk_widget_set_sensitive (choose_t->sconfigure, FALSE);
	  if (choose_t->stest)
	    gtk_widget_set_sensitive (choose_t->stest, FALSE);
	}

    }
}


static void
filedsn_get_dsn (char *filename, char *dsn, size_t dsn_sz)
{
  char *p;

  /* Cut everything up to the last slash */
  p = strrchr (filename, '/');
  if (p != NULL)
    p++;
  else
    p = filename;
  snprintf (dsn, dsn_sz, "%s", p);

  /* Cut .dsn extension */
  p = strrchr (dsn, '.');
  if (p != NULL && !strcasecmp (p, ".dsn"))
    *p = '\0';
}


static BOOL _CheckDriverLoginDlg (char *drv);

static void
filedsn_configure (TDSNCHOOSER *choose_t, char *drv, char *dsn, char *in_attrs,
	BOOL b_add, BOOL verify_conn)
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
      attr_lst = create_fgensetup (choose_t->mainwnd, dsn, in_attrs, 
         b_add, &verify_conn);
      attrs = attr_lst;
    }

  if (!attrs)
    {
      create_error (choose_t->mainwnd, NULL, "Error adding File DSN:",
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
          create_error (choose_t->mainwnd, NULL, "Error adding File DSN:",
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
          create_error (choose_t->mainwnd, NULL, "Error adding file DSN:",
	      strerror (errno));
          goto done;
        }
      connstr = tmp;
      snprintf (connstr + len, add_len + 1, ";SAVEFILE=%s", dsn);
      len += add_len;

      /* Connect to data source */
      ret = test_driver_connect (choose_t, connstr);
      if (!ret && b_add)
        { 
	  if (create_confirm (choose_t->mainwnd, dsn,
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
              create_error (choose_t->mainwnd, NULL, "Error writing File DSN:",
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
              create_error (choose_t->mainwnd, NULL, "Error writing File DSN:",
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


static void
filedsn_update_file_list (TDSNCHOOSER *choose_t)
{
  /* Reset current file */
  gtk_entry_set_text (GTK_ENTRY (choose_t->file_entry), "");
  if (choose_t->fremove)
    gtk_widget_set_sensitive(choose_t->fremove, FALSE);
  if (choose_t->fconfigure)
    gtk_widget_set_sensitive(choose_t->fconfigure, FALSE);
  if (choose_t->ftest)
    gtk_widget_set_sensitive(choose_t->ftest, FALSE);

  /* Update file list */
  addfiles_to_list (choose_t->mainwnd, choose_t->file_list,
    choose_t->curr_dir);
}


void
filedsn_add_clicked (GtkWidget *widget, TDSNCHOOSER *choose_t)
{
  SQLCHAR drv[1024] = { 0 };
  int sqlstat;
  LPSTR s, attrs;
  TFDRIVERCHOOSER drvchoose_t;

  if (!choose_t)
    return;

  /* Try first to get the driver name */
  SQLSetConfigMode (ODBC_USER_DSN);

  drvchoose_t.attrs = NULL;
  drvchoose_t.dsn = NULL;
  drvchoose_t.driver = NULL;
  drvchoose_t.curr_dir = choose_t->curr_dir;

  create_fdriverchooser (choose_t->mainwnd, &drvchoose_t);

  /* Check output parameters */
  if (drvchoose_t.ok)
    {
      if (sizeof(drv) > WCSLEN(drvchoose_t.driver) + strlen("DRIVER="))
	{
          s = strcpy(drv, "DRIVER=");
          s += strlen("DRIVER=");
          dm_strcpy_W2A(s, drvchoose_t.driver);
          attrs = drvchoose_t.attrs;

          filedsn_configure(choose_t, drv, drvchoose_t.dsn, 
          	attrs ? attrs :"\0\0", TRUE, drvchoose_t.verify_conn);
          filedsn_update_file_list(choose_t);
	}
    }

  if (drvchoose_t.driver)
    free (drvchoose_t.driver);
  if (drvchoose_t.attrs)
    free (drvchoose_t.attrs);
  if (drvchoose_t.dsn)
    free (drvchoose_t.dsn);


}


void
filedsn_remove_clicked (GtkWidget *widget, TDSNCHOOSER *choose_t)
{
  char msg[4096];
  char *filedsn;

  if (!choose_t)
    return;

  /* Retrieve filedsn file name */
  filedsn = (char*)gtk_entry_get_text (GTK_ENTRY (choose_t->file_entry));

  /* Confirm removing a file dsn */
  snprintf (msg, sizeof (msg),
      "Are you sure you want to remove the '%s' data source?",
      filedsn);
  if (!create_confirm (choose_t->mainwnd, NULL, msg))
    return;

  /* Remove file */
  if (unlink (filedsn) < 0)
    {
      create_error (choose_t->mainwnd, NULL, "Error removing file DSN:",
	  strerror (errno));
      return;
    }

  /* Update file list */
  filedsn_update_file_list(choose_t);
}


void
filedsn_configure_clicked (GtkWidget *widget, TDSNCHOOSER *choose_t)
{
  char dsn[1024];
  char *filedsn;
  char *drv = NULL;
  char *attrs = NULL;
  char *_attrs = NULL;	/* attr list */
  size_t len = 0;	/* current attr list length (w/o list-terminating NUL) */
  char *p, *p_next;
  WORD read_len;
  char entries[1024];

  if (!choose_t)
    return;

  /* Retrieve filedsn file name */
  filedsn = (char*)gtk_entry_get_text (GTK_ENTRY (choose_t->file_entry));
  filedsn_get_dsn (filedsn, dsn, sizeof (dsn));

  /* Get list of entries in .dsn file */
  if (!SQLReadFileDSN (filedsn, "ODBC", NULL,
		       entries, sizeof (entries), &read_len))
    {
      create_error (choose_t->mainwnd, NULL, "SQLReadFileDSN failed", NULL);
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

      if (!SQLReadFileDSN (filedsn, "ODBC", p, value, sizeof(value), &read_len))
        {
          create_error (choose_t->mainwnd, NULL, "SQLReadFileDSN failed", NULL);
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
          create_error (choose_t->mainwnd, NULL, "Error adding file DSN:",
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
      create_error (choose_t->mainwnd, NULL,
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
  filedsn_configure (choose_t, drv, dsn, attrs, FALSE, TRUE);

done:
  if (drv != NULL)
    free (drv);
  if (_attrs != NULL)
    free (_attrs);
}


void
filedsn_test_clicked (GtkWidget *widget, TDSNCHOOSER *choose_t)
{
  char dsn[1024];
  char connstr[4096] = { 0 };
  char *filedsn;

  if (!choose_t)
    return;

  /* Retrieve filedsn file name */
  filedsn = (char*)gtk_entry_get_text (GTK_ENTRY (choose_t->file_entry));
  filedsn_get_dsn (filedsn, dsn, sizeof (dsn));

  /* Create connection string and connect to data source */
  snprintf (connstr, sizeof (connstr), "FILEDSN=%s", filedsn);
  if (test_driver_connect(choose_t, connstr))
    {
      _iodbcdm_messagebox (choose_t->mainwnd, filedsn,
        "The connection DSN was tested successfully, and can be used at this time.");
    }
}


void
filedsn_setdir_clicked (GtkWidget *widget, TDSNCHOOSER *choose_t)
{
  char msg[4096];

  if (!choose_t)
    return;

  /* confirm setting a directory */
  snprintf (msg, sizeof (msg),
      "Are you sure that you want to make '%s' the default file DSN directory?",
      choose_t->curr_dir);
  if (!create_confirm (choose_t->mainwnd, NULL, msg))
    return;

  /* write FileDSNPath value */
  if (!SQLWritePrivateProfileString ("ODBC", "FileDSNPath",
	   choose_t->curr_dir, "odbcinst.ini"))
    {
      create_error (choose_t->mainwnd, NULL,
	  "Error setting default file DSN directory", NULL);
      return;
    }
}


void
filedsn_filelist_select (GtkWidget *widget, gint row, gint column,
    GdkEvent *event, TDSNCHOOSER *choose_t)
{
  LPSTR filename = NULL, temp = NULL;

  if (choose_t)
    {
      /* Get the file name */
      gtk_clist_get_text (GTK_CLIST (choose_t->file_list), row, 0, &filename);

      /* Update the directory and file list */
      temp =
	  (LPSTR) malloc (STRLEN (filename) + STRLEN (choose_t->curr_dir) +
	  2);

      if (temp)
	{
	  STRCPY (temp, choose_t->curr_dir);
	  if (temp[STRLEN (temp) - 1] != '/')
	    STRCAT (temp, "/");
	  STRCAT (temp, filename);

	  /* Check if it's a valid file */
	  gtk_entry_set_text (GTK_ENTRY (choose_t->file_entry), temp);

	  /* And activate buttons */
	  if (choose_t->fremove)
	    gtk_widget_set_sensitive (choose_t->fremove, TRUE);
	  if (choose_t->fconfigure)
	    gtk_widget_set_sensitive (choose_t->fconfigure, TRUE);
	  if (choose_t->ftest)
	    gtk_widget_set_sensitive (choose_t->ftest, TRUE);

	  free (temp);
	}
    }
}


void
filedsn_filelist_unselect (GtkWidget *widget, gint row, gint column,
    GdkEvent *event, TDSNCHOOSER *choose_t)
{
  if (choose_t)
    {
      /* Check if it's a valid file */
      gtk_entry_set_text (GTK_ENTRY (choose_t->file_entry), "");

      /* And des-activate buttons */
      if (choose_t->fremove)
	gtk_widget_set_sensitive (choose_t->fremove, FALSE);
      if (choose_t->fconfigure)
	gtk_widget_set_sensitive (choose_t->fconfigure, FALSE);
      if (choose_t->ftest)
	gtk_widget_set_sensitive (choose_t->ftest, FALSE);
    }
}


void
filedsn_dirlist_select (GtkWidget *widget, gint row, gint column,
    GdkEvent *event, TDSNCHOOSER *choose_t)
{
  LPSTR filename = NULL, temp = NULL;
  int i;

  if (choose_t)
    {
      /* Get the directory name */
      gtk_clist_get_text (GTK_CLIST (choose_t->dir_list), row, 0, &filename);

      if (filename && event && event->type == GDK_2BUTTON_PRESS)
	{
	  /* Update the directory and file list */
	  temp =
	      (LPSTR) malloc (STRLEN (filename) +
	      STRLEN (choose_t->curr_dir) + 2);

	  if (temp)
	    {
	      if (!strcmp (filename, "."))
		STRCPY (temp, choose_t->curr_dir);
	      else if (!strcmp (filename, ".."))
		{
		  STRCPY (temp, choose_t->curr_dir);
		  for (i = STRLEN (temp) - 1; i - 1 && temp[i] != '/'; i--);
		  temp[i] = 0;
		}
	      else
		{
		  STRCPY (temp, choose_t->curr_dir);
		  if (temp[STRLEN (temp) - 1] != '/')
		    STRCAT (temp, "/");
		  STRCAT (temp, filename);
		}

	      strncpy(choose_t->curr_dir, temp, sizeof(choose_t->curr_dir));
	      addlistofdir_to_optionmenu (choose_t->dir_combo,
		  choose_t->curr_dir, choose_t);
	      adddirectories_to_list (choose_t->mainwnd, choose_t->dir_list,
		  choose_t->curr_dir);
	      addfiles_to_list (choose_t->mainwnd, choose_t->file_list,
		  choose_t->curr_dir);
	    }
	}
    }
}


void
filedsn_lookin_clicked (GtkWidget *widget, void **array)
{
  if (array && array[0] && array[1] && ((TDSNCHOOSER *) array[1])->curr_dir
      && strcmp (((TDSNCHOOSER *) array[1])->curr_dir, array[0]))
    {
      TDSNCHOOSER *choose_t = (TDSNCHOOSER *) array[1];
      /* Update the directory and file list */
      strncpy(choose_t->curr_dir, array[0], sizeof(choose_t->curr_dir));
      addlistofdir_to_optionmenu (choose_t->dir_combo,
	  (LPCSTR) array[0], choose_t);
      adddirectories_to_list (choose_t->mainwnd,
	  choose_t->dir_list, (LPCSTR) array[0]);
      addfiles_to_list (choose_t->mainwnd, choose_t->file_list, (LPCSTR) array[0]);
    }
}


void
userdsn_list_select (GtkWidget *widget, gint row, gint column,
    GdkEvent *event, TDSNCHOOSER *choose_t)
{
  char *szDSN = NULL;

  if (choose_t)
    {
      if (GTK_CLIST (choose_t->udsnlist)->selection != NULL)
	gtk_clist_get_text (GTK_CLIST (choose_t->udsnlist),
	    GPOINTER_TO_INT (GTK_CLIST (choose_t->udsnlist)->selection->data),
	    0, &szDSN);

      if (szDSN && event && event->type == GDK_2BUTTON_PRESS)
	gtk_signal_emit_by_name (GTK_OBJECT (choose_t->uconfigure), "clicked",
	    choose_t);

      gtk_widget_set_sensitive (choose_t->uremove, TRUE);
      gtk_widget_set_sensitive (choose_t->uconfigure, TRUE);
      gtk_widget_set_sensitive (choose_t->utest, TRUE);
    }
}


void
userdsn_list_unselect (GtkWidget *widget, gint row, gint column,
    GdkEvent *event, TDSNCHOOSER *choose_t)
{
  if (choose_t)
    {
      gtk_widget_set_sensitive (choose_t->uremove, FALSE);
      gtk_widget_set_sensitive (choose_t->uconfigure, FALSE);
      gtk_widget_set_sensitive (choose_t->utest, FALSE);
    }
}


void
systemdsn_list_select (GtkWidget *widget, gint row, gint column,
    GdkEvent *event, TDSNCHOOSER *choose_t)
{
  char *szDSN = NULL;

  if (choose_t)
    {
      if (GTK_CLIST (choose_t->sdsnlist)->selection != NULL)
	gtk_clist_get_text (GTK_CLIST (choose_t->sdsnlist),
	    GPOINTER_TO_INT (GTK_CLIST (choose_t->sdsnlist)->selection->data),
	    0, &szDSN);

      if (szDSN && event && event->type == GDK_2BUTTON_PRESS)
	gtk_signal_emit_by_name (GTK_OBJECT (choose_t->sconfigure), "clicked",
	    choose_t);

      gtk_widget_set_sensitive (choose_t->sremove, TRUE);
      gtk_widget_set_sensitive (choose_t->sconfigure, TRUE);
      gtk_widget_set_sensitive (choose_t->stest, TRUE);
    }
}


void
systemdsn_list_unselect (GtkWidget *widget, gint row, gint column,
    GdkEvent *event, TDSNCHOOSER *choose_t)
{
  if (choose_t)
    {
      gtk_widget_set_sensitive (choose_t->sremove, FALSE);
      gtk_widget_set_sensitive (choose_t->sconfigure, FALSE);
      gtk_widget_set_sensitive (choose_t->stest, FALSE);
    }
}


static void
dsnchooser_ok_clicked (GtkWidget *widget, TDSNCHOOSER *choose_t)
{
  char *szDSN;

  if (choose_t)
    {
      switch (choose_t->type_dsn)
	{
	case USER_DSN:
	  if (GTK_CLIST (choose_t->udsnlist)->selection != NULL)
	    {
	      gtk_clist_get_text (GTK_CLIST (choose_t->udsnlist),
		  GPOINTER_TO_INT (GTK_CLIST (choose_t->udsnlist)->selection->
		      data), 0, &szDSN);
	      choose_t->dsn = dm_SQL_A2W(szDSN, SQL_NTS);
	    }
	  else
	    choose_t->dsn = NULL;
	  break;

	case SYSTEM_DSN:
	  if (GTK_CLIST (choose_t->sdsnlist)->selection != NULL)
	    {
	      gtk_clist_get_text (GTK_CLIST (choose_t->sdsnlist),
		  GPOINTER_TO_INT (GTK_CLIST (choose_t->sdsnlist)->selection->
		      data), 0, &szDSN);
	      choose_t->dsn = dm_SQL_A2W (szDSN, SQL_NTS);
	    }
	  else
	    choose_t->dsn = NULL;
	  break;

	default:
	  choose_t->dsn = NULL;
	  break;
	};

    done:
      choose_t->udsnlist = choose_t->sdsnlist = NULL;
      choose_t->uadd = choose_t->uremove = choose_t->utest =
	  choose_t->uconfigure = NULL;
      choose_t->sadd = choose_t->sremove = choose_t->stest =
	  choose_t->sconfigure = NULL;

      gtk_signal_disconnect_by_func (GTK_OBJECT (choose_t->mainwnd),
	  GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
      gtk_main_quit ();
      gtk_widget_destroy (choose_t->mainwnd);
    }
}


static void
dsnchooser_cancel_clicked (GtkWidget *widget, TDSNCHOOSER *choose_t)
{
  if (choose_t)
    {
      choose_t->udsnlist = choose_t->sdsnlist = NULL;
      choose_t->uadd = choose_t->uremove = choose_t->utest =
	  choose_t->uconfigure = NULL;
      choose_t->sadd = choose_t->sremove = choose_t->stest =
	  choose_t->sconfigure = NULL;
      choose_t->type_dsn = -1;
      choose_t->dsn = NULL;

      gtk_signal_disconnect_by_func (GTK_OBJECT (choose_t->mainwnd),
	  GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
      gtk_main_quit ();
      gtk_widget_destroy (choose_t->mainwnd);
    }
}


static gint
delete_event (GtkWidget *widget, GdkEvent *event, TDSNCHOOSER *choose_t)
{
  dsnchooser_cancel_clicked (widget, choose_t);

  return FALSE;
}


void
create_dsnchooser (HWND hwnd, TDSNCHOOSER * choose_t)
{
  GdkPixmap *pixmap;
  GdkBitmap *mask;
  GtkStyle *style;

  GtkWidget *dsnchooser;
  GtkWidget *vbox27;
  GtkWidget *notebook1;
  GtkWidget *vbox28;
  GtkWidget *frame60;
  GtkWidget *alignment52;
  GtkWidget *hbox36;
  GtkWidget *scrolledwindow17;
  GtkWidget *clist1;
  GtkWidget *label104;
  GtkWidget *label105;
  GtkWidget *label106;
  GtkWidget *vbox29;
  GtkWidget *b_add;
  GtkWidget *b_remove;
  GtkWidget *b_configure;
  GtkWidget *b_test;
  GtkWidget *frame61;
  GtkWidget *alignment53;
  GtkWidget *label107;
  GtkWidget *frame62;
  GtkWidget *alignment54;
  GtkWidget *hbox37;
  GtkWidget *pixmap1;
  GtkWidget *label108;
  GtkWidget *label109;
  GtkWidget *vbox30;
  GtkWidget *frame63;
  GtkWidget *alignment55;
  GtkWidget *hbox38;
  GtkWidget *scrolledwindow18;
  GtkWidget *clist2;
  GtkWidget *label110;
  GtkWidget *label111;
  GtkWidget *label112;
  GtkWidget *vbox31;
  GtkWidget *bs_add;
  GtkWidget *bs_remove;
  GtkWidget *bs_configure;
  GtkWidget *bs_test;
  GtkWidget *frame64;
  GtkWidget *alignment56;
  GtkWidget *label113;
  GtkWidget *frame65;
  GtkWidget *alignment57;
  GtkWidget *hbox39;
  GtkWidget *pixmap2;
  GtkWidget *label114;
  GtkWidget *label115;
  GtkWidget *vbox32;
  GtkWidget *frame66;
  GtkWidget *alignment58;
  GtkWidget *hbox40;
  GtkWidget *vbox33;
  GtkWidget *hbox41;
  GtkWidget *frame67;
  GtkWidget *alignment59;
  GtkWidget *hbox42;
  GtkWidget *label116;
  GtkWidget *optionmenu1;
  GtkWidget *menu2;
  GtkWidget *hbox43;
  GtkWidget *scrolledwindow19;
  GtkWidget *clist3;
  GtkWidget *label117;
  GtkWidget *scrolledwindow20;
  GtkWidget *clist4;
  GtkWidget *label118;
  GtkWidget *frame68;
  GtkWidget *alignment60;
  GtkWidget *hbox44;
  GtkWidget *label119;
  GtkWidget *t_fileselected;
  GtkWidget *vbox34;
  GtkWidget *bf_add;
  GtkWidget *bf_remove;
  GtkWidget *bf_configure;
  GtkWidget *bf_test;
  GtkWidget *bf_setdir;
  GtkWidget *frame69;
  GtkWidget *alignment61;
  GtkWidget *frame70;
  GtkWidget *alignment62;
  GtkWidget *hbox45;
  GtkWidget *pixmap3;
  GtkWidget *label120;
  GtkWidget *label121;
  GtkWidget *hbuttonbox2;
  GtkWidget *b_cancel;
  GtkWidget *b_ok;
  GtkAccelGroup *accel_group;

  if (!GTK_IS_WIDGET (hwnd))
    {
      gtk_init (0, NULL);
      hwnd = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    }

  if (hwnd == NULL || !GTK_IS_WIDGET (hwnd))
    return;

  accel_group = gtk_accel_group_new ();

  dsnchooser = gtk_dialog_new ();
  gtk_widget_set_name (dsnchooser, "dsnchooser");
  gtk_widget_set_size_request (dsnchooser, 570, 420);
  gtk_window_set_title (GTK_WINDOW (dsnchooser), _("Select Data Source"));
  gtk_window_set_position (GTK_WINDOW (dsnchooser), GTK_WIN_POS_CENTER);
  gtk_window_set_modal (GTK_WINDOW (dsnchooser), TRUE);
  gtk_window_set_default_size (GTK_WINDOW (dsnchooser), 600, 450);
  gtk_window_set_type_hint (GTK_WINDOW (dsnchooser), GDK_WINDOW_TYPE_HINT_DIALOG);

#if GTK_CHECK_VERSION(2,0,0)
  gtk_widget_show (dsnchooser);
#endif

  vbox27 = GTK_DIALOG (dsnchooser)->vbox;
  gtk_widget_set_name (vbox27, "vbox27");
  gtk_widget_show (vbox27);

  notebook1 = gtk_notebook_new ();
  gtk_widget_set_name (notebook1, "notebook1");
  gtk_widget_show (notebook1);
  gtk_box_pack_start (GTK_BOX (vbox27), notebook1, TRUE, TRUE, 0);

  vbox28 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox28, "vbox28");
  gtk_widget_show (vbox28);
  gtk_container_add (GTK_CONTAINER (notebook1), vbox28);

  frame60 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame60, "frame60");
  gtk_widget_show (frame60);
  gtk_box_pack_start (GTK_BOX (vbox28), frame60, TRUE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame60), GTK_SHADOW_NONE);

  alignment52 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment52, "alignment52");
  gtk_widget_show (alignment52);
  gtk_container_add (GTK_CONTAINER (frame60), alignment52);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment52), 0, 0, 4, 0);

  hbox36 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox36, "hbox36");
  gtk_widget_show (hbox36);
  gtk_container_add (GTK_CONTAINER (alignment52), hbox36);

  scrolledwindow17 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_name (scrolledwindow17, "scrolledwindow17");
  gtk_widget_show (scrolledwindow17);
  gtk_box_pack_start (GTK_BOX (hbox36), scrolledwindow17, TRUE, TRUE, 0);
  gtk_widget_set_size_request (scrolledwindow17, 440, 219);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow17), GTK_SHADOW_IN);

  clist1 = gtk_clist_new (3);
  gtk_widget_set_name (clist1, "clist1");
  gtk_widget_show (clist1);
  gtk_container_add (GTK_CONTAINER (scrolledwindow17), clist1);
  gtk_clist_set_column_width (GTK_CLIST (clist1), 0, 100);
  gtk_clist_set_column_width (GTK_CLIST (clist1), 1, 162);
  gtk_clist_set_column_width (GTK_CLIST (clist1), 2, 80);
  gtk_clist_column_titles_show (GTK_CLIST (clist1));

  label104 = gtk_label_new (_("Name"));
  gtk_widget_set_name (label104, "label104");
  gtk_widget_show (label104);
  gtk_clist_set_column_widget (GTK_CLIST (clist1), 0, label104);
  gtk_widget_set_size_request (label104, 100, -1);

  label105 = gtk_label_new (_("Description"));
  gtk_widget_set_name (label105, "label105");
  gtk_widget_show (label105);
  gtk_clist_set_column_widget (GTK_CLIST (clist1), 1, label105);
  gtk_widget_set_size_request (label105, 162, -1);

  label106 = gtk_label_new (_("Driver"));
  gtk_widget_set_name (label106, "label106");
  gtk_widget_show (label106);
  gtk_clist_set_column_widget (GTK_CLIST (clist1), 2, label106);
  gtk_widget_set_size_request (label106, 80, -1);

  vbox29 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox29, "vbox29");
  gtk_widget_show (vbox29);
  gtk_box_pack_start (GTK_BOX (hbox36), vbox29, FALSE, TRUE, 0);

  b_add = gtk_button_new_from_stock ("gtk-add");
  gtk_widget_set_name (b_add, "b_add");
  gtk_widget_show (b_add);
  gtk_box_pack_start (GTK_BOX (vbox29), b_add, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (b_add), 4);
  GTK_WIDGET_SET_FLAGS (b_add, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (b_add, "clicked", accel_group,
                              GDK_A, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);

  b_remove = gtk_button_new_from_stock ("gtk-remove");
  gtk_widget_set_name (b_remove, "b_remove");
  gtk_widget_show (b_remove);
  gtk_box_pack_start (GTK_BOX (vbox29), b_remove, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (b_remove), 4);
  gtk_widget_set_sensitive (b_remove, FALSE);
  GTK_WIDGET_SET_FLAGS (b_remove, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (b_remove, "clicked", accel_group,
                              GDK_R, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);

  b_configure = gtk_button_new_with_mnemonic (_("Confi_gure"));
  gtk_widget_set_name (b_configure, "b_configure");
  gtk_widget_show (b_configure);
  gtk_box_pack_start (GTK_BOX (vbox29), b_configure, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (b_configure), 4);
  gtk_widget_set_sensitive (b_configure, FALSE);
  GTK_WIDGET_SET_FLAGS (b_configure, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (b_configure, "clicked", accel_group,
                              GDK_G, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (b_configure, "clicked", accel_group,
                              GDK_g, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);

  b_test = gtk_button_new_with_mnemonic (_("_Test"));
  gtk_widget_set_name (b_test, "b_test");
  gtk_widget_show (b_test);
  gtk_box_pack_start (GTK_BOX (vbox29), b_test, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (b_test), 4);
  gtk_widget_set_sensitive (b_test, FALSE);
  GTK_WIDGET_SET_FLAGS (b_test, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (b_test, "clicked", accel_group,
                              GDK_T, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);

  frame61 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame61, "frame61");
  gtk_widget_show (frame61);
  gtk_box_pack_start (GTK_BOX (vbox29), frame61, TRUE, TRUE, 0);
  gtk_widget_set_size_request (frame61, -1, 80);
  gtk_frame_set_shadow_type (GTK_FRAME (frame61), GTK_SHADOW_NONE);

  alignment53 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment53, "alignment53");
  gtk_widget_show (alignment53);
  gtk_container_add (GTK_CONTAINER (frame61), alignment53);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment53), 0, 0, 12, 0);

  label107 = gtk_label_new (_(" User Data Sources:"));
  gtk_widget_set_name (label107, "label107");
  gtk_widget_show (label107);
  gtk_frame_set_label_widget (GTK_FRAME (frame60), label107);
  gtk_label_set_use_markup (GTK_LABEL (label107), TRUE);

  frame62 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame62, "frame62");
  gtk_widget_show (frame62);
  gtk_box_pack_start (GTK_BOX (vbox28), frame62, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame62), 3);

  alignment54 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment54, "alignment54");
  gtk_widget_show (alignment54);
  gtk_container_add (GTK_CONTAINER (frame62), alignment54);

  hbox37 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox37, "hbox37");
  gtk_widget_show (hbox37);
  gtk_container_add (GTK_CONTAINER (alignment54), hbox37);

#if GTK_CHECK_VERSION(2,0,0)
  style = gtk_widget_get_style (dsnchooser);
  pixmap =
      gdk_pixmap_create_from_xpm_d (dsnchooser->window, &mask,
      &style->bg[GTK_STATE_NORMAL], (gchar **) odbc4_xpm);
#else
  style = gtk_widget_get_style (GTK_WIDGET (hwnd));
  pixmap =
      gdk_pixmap_create_from_xpm_d (GTK_WIDGET (hwnd)->window, &mask,
      &style->bg[GTK_STATE_NORMAL], (gchar **) odbc4_xpm);
#endif

  pixmap1 = gtk_pixmap_new (pixmap, mask);
  gtk_widget_set_name (pixmap1, "pixmap1");
  gtk_widget_show (pixmap1);
  gtk_box_pack_start (GTK_BOX (hbox37), pixmap1, FALSE, TRUE, 10);

  label108 = gtk_label_new (_("An ODBC User data source stores information about to connect to\nthe indicated data provider. A User data source is only available to you,\nand can only be used on the current machine."));
  gtk_widget_set_name (label108, "label108");
  gtk_widget_show (label108);
  gtk_box_pack_start (GTK_BOX (hbox37), label108, FALSE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (label108), GTK_JUSTIFY_FILL);

  label109 = gtk_label_new (_("User DSN"));
  gtk_widget_set_name (label109, "label109");
  gtk_widget_show (label109);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook1), 0), label109);


  choose_t->uadd = b_add;
  choose_t->uremove = b_remove;
  choose_t->utest = b_test;
  choose_t->uconfigure = b_configure;


  
  vbox30 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox30, "vbox30");
  gtk_widget_show (vbox30);
  gtk_container_add (GTK_CONTAINER (notebook1), vbox30);

  frame63 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame63, "frame63");
  gtk_widget_show (frame63);
  gtk_box_pack_start (GTK_BOX (vbox30), frame63, TRUE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame63), GTK_SHADOW_NONE);

  alignment55 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment55, "alignment55");
  gtk_widget_show (alignment55);
  gtk_container_add (GTK_CONTAINER (frame63), alignment55);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment55), 0, 0, 4, 0);

  hbox38 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox38, "hbox38");
  gtk_widget_show (hbox38);
  gtk_container_add (GTK_CONTAINER (alignment55), hbox38);

  scrolledwindow18 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_name (scrolledwindow18, "scrolledwindow18");
  gtk_widget_show (scrolledwindow18);
  gtk_box_pack_start (GTK_BOX (hbox38), scrolledwindow18, TRUE, TRUE, 0);
  gtk_widget_set_size_request (scrolledwindow18, 440, 219);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow18), GTK_SHADOW_IN);

  clist2 = gtk_clist_new (3);
  gtk_widget_set_name (clist2, "clist2");
  gtk_widget_show (clist2);
  gtk_container_add (GTK_CONTAINER (scrolledwindow18), clist2);
  gtk_clist_set_column_width (GTK_CLIST (clist2), 0, 100);
  gtk_clist_set_column_width (GTK_CLIST (clist2), 1, 163);
  gtk_clist_set_column_width (GTK_CLIST (clist2), 2, 80);
  gtk_clist_column_titles_show (GTK_CLIST (clist2));

  label110 = gtk_label_new (_("Name"));
  gtk_widget_set_name (label110, "label110");
  gtk_widget_show (label110);
  gtk_clist_set_column_widget (GTK_CLIST (clist2), 0, label110);
  gtk_widget_set_size_request (label110, 100, -1);

  label111 = gtk_label_new (_("Description"));
  gtk_widget_set_name (label111, "label111");
  gtk_widget_show (label111);
  gtk_clist_set_column_widget (GTK_CLIST (clist2), 1, label111);
  gtk_widget_set_size_request (label111, 162, -1);

  label112 = gtk_label_new (_("Driver"));
  gtk_widget_set_name (label112, "label112");
  gtk_widget_show (label112);
  gtk_clist_set_column_widget (GTK_CLIST (clist2), 2, label112);
  gtk_widget_set_size_request (label112, 80, -1);

  vbox31 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox31, "vbox31");
  gtk_widget_show (vbox31);
  gtk_box_pack_start (GTK_BOX (hbox38), vbox31, FALSE, TRUE, 0);

  bs_add = gtk_button_new_from_stock ("gtk-add");
  gtk_widget_set_name (bs_add, "bs_add");
  gtk_widget_show (bs_add);
  gtk_box_pack_start (GTK_BOX (vbox31), bs_add, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (bs_add), 4);
  GTK_WIDGET_SET_FLAGS (bs_add, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (bs_add, "clicked", accel_group,
                              GDK_A, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);

  bs_remove = gtk_button_new_from_stock ("gtk-remove");
  gtk_widget_set_name (bs_remove, "bs_remove");
  gtk_widget_show (bs_remove);
  gtk_box_pack_start (GTK_BOX (vbox31), bs_remove, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (bs_remove), 4);
  gtk_widget_set_sensitive (bs_remove, FALSE);
  GTK_WIDGET_SET_FLAGS (bs_remove, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (bs_remove, "clicked", accel_group,
                              GDK_R, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);

  bs_configure = gtk_button_new_with_mnemonic (_("Confi_gure"));
  gtk_widget_set_name (bs_configure, "bs_configure");
  gtk_widget_show (bs_configure);
  gtk_box_pack_start (GTK_BOX (vbox31), bs_configure, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (bs_configure), 4);
  gtk_widget_set_sensitive (bs_configure, FALSE);
  GTK_WIDGET_SET_FLAGS (bs_configure, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (bs_configure, "clicked", accel_group,
                              GDK_G, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (bs_configure, "clicked", accel_group,
                              GDK_g, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);

  bs_test = gtk_button_new_with_mnemonic (_("_Test"));
  gtk_widget_set_name (bs_test, "bs_test");
  gtk_widget_show (bs_test);
  gtk_box_pack_start (GTK_BOX (vbox31), bs_test, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (bs_test), 4);
  gtk_widget_set_sensitive (bs_test, FALSE);
  GTK_WIDGET_SET_FLAGS (bs_test, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (bs_test, "clicked", accel_group,
                              GDK_T, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);

  frame64 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame64, "frame64");
  gtk_widget_show (frame64);
  gtk_box_pack_start (GTK_BOX (vbox31), frame64, TRUE, TRUE, 0);
  gtk_widget_set_size_request (frame64, -1, 80);
  gtk_frame_set_shadow_type (GTK_FRAME (frame64), GTK_SHADOW_NONE);

  alignment56 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment56, "alignment56");
  gtk_widget_show (alignment56);
  gtk_container_add (GTK_CONTAINER (frame64), alignment56);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment56), 0, 0, 12, 0);

  label113 = gtk_label_new (_(" System Data Sources:"));
  gtk_widget_set_name (label113, "label113");
  gtk_widget_show (label113);
  gtk_frame_set_label_widget (GTK_FRAME (frame63), label113);
  gtk_label_set_use_markup (GTK_LABEL (label113), TRUE);

  frame65 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame65, "frame65");
  gtk_widget_show (frame65);
  gtk_box_pack_start (GTK_BOX (vbox30), frame65, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame65), 3);

  alignment57 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment57, "alignment57");
  gtk_widget_show (alignment57);
  gtk_container_add (GTK_CONTAINER (frame65), alignment57);

  hbox39 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox39, "hbox39");
  gtk_widget_show (hbox39);
  gtk_container_add (GTK_CONTAINER (alignment57), hbox39);
  gtk_container_set_border_width (GTK_CONTAINER (hbox39), 10);

  pixmap2 = gtk_pixmap_new (pixmap, mask);
  gtk_widget_set_name (pixmap2, "pixmap2");
  gtk_widget_show (pixmap2);
  gtk_box_pack_start (GTK_BOX (hbox39), pixmap2, FALSE, TRUE, 10);

  label114 = gtk_label_new (_("An ODBC System data source stores information about to connect to\nthe indicated data provider. A System data source is visible to all\nusers on this machine, including daemons."));
  gtk_widget_set_name (label114, "label114");
  gtk_widget_show (label114);
  gtk_box_pack_start (GTK_BOX (hbox39), label114, FALSE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (label114), GTK_JUSTIFY_FILL);

  label115 = gtk_label_new (_("System DSN"));
  gtk_widget_set_name (label115, "label115");
  gtk_widget_show (label115);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook1), 1), label115);



  choose_t->sadd = bs_add;
  choose_t->sremove = bs_remove;
  choose_t->stest = bs_test;
  choose_t->sconfigure = bs_configure;



  vbox32 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox32, "vbox32");
  gtk_widget_show (vbox32);
  gtk_container_add (GTK_CONTAINER (notebook1), vbox32);

  frame66 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame66, "frame66");
  gtk_widget_show (frame66);
  gtk_box_pack_start (GTK_BOX (vbox32), frame66, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame66), 2);
  gtk_frame_set_shadow_type (GTK_FRAME (frame66), GTK_SHADOW_NONE);

  alignment58 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment58, "alignment58");
  gtk_widget_show (alignment58);
  gtk_container_add (GTK_CONTAINER (frame66), alignment58);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment58), 0, 0, 4, 0);

  hbox40 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox40, "hbox40");
  gtk_widget_show (hbox40);
  gtk_container_add (GTK_CONTAINER (alignment58), hbox40);

  vbox33 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox33, "vbox33");
  gtk_widget_show (vbox33);
  gtk_box_pack_start (GTK_BOX (hbox40), vbox33, TRUE, TRUE, 0);
  gtk_widget_set_size_request (vbox33, 436, 250);

  hbox41 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox41, "hbox41");
  gtk_widget_show (hbox41);
  gtk_box_pack_start (GTK_BOX (vbox33), hbox41, FALSE, FALSE, 0);

  frame67 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame67, "frame67");
  gtk_widget_show (frame67);
  gtk_box_pack_start (GTK_BOX (hbox41), frame67, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame67), 4);
  gtk_frame_set_shadow_type (GTK_FRAME (frame67), GTK_SHADOW_NONE);

  alignment59 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment59, "alignment59");
  gtk_widget_show (alignment59);
  gtk_container_add (GTK_CONTAINER (frame67), alignment59);
  gtk_container_set_border_width (GTK_CONTAINER (alignment59), 2);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment59), 0, 0, 6, 0);

  hbox42 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox42, "hbox42");
  gtk_widget_show (hbox42);
  gtk_container_add (GTK_CONTAINER (alignment59), hbox42);

  label116 = gtk_label_new (_("Look in : "));
  gtk_widget_set_name (label116, "label116");
  gtk_widget_show (label116);
  gtk_box_pack_start (GTK_BOX (hbox42), label116, FALSE, FALSE, 0);

  optionmenu1 = gtk_option_menu_new ();
  gtk_widget_set_name (optionmenu1, "optionmenu1");
  gtk_widget_show (optionmenu1);
  gtk_box_pack_start (GTK_BOX (hbox42), optionmenu1, TRUE, TRUE, 0);

  menu2 = gtk_menu_new ();
  gtk_widget_set_name (menu2, "menu2");

  gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu1), menu2);

  hbox43 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox43, "hbox43");
  gtk_widget_show (hbox43);
  gtk_box_pack_start (GTK_BOX (vbox33), hbox43, TRUE, TRUE, 0);

  scrolledwindow19 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_name (scrolledwindow19, "scrolledwindow19");
  gtk_widget_show (scrolledwindow19);
  gtk_box_pack_start (GTK_BOX (hbox43), scrolledwindow19, TRUE, TRUE, 0);
  gtk_widget_set_size_request (scrolledwindow19, 96, -1);
  gtk_container_set_border_width (GTK_CONTAINER (scrolledwindow19), 4);

  clist3 = gtk_clist_new (1);
  gtk_widget_set_name (clist3, "clist3");
  gtk_widget_show (clist3);
  gtk_container_add (GTK_CONTAINER (scrolledwindow19), clist3);
  gtk_clist_set_column_width (GTK_CLIST (clist3), 0, 80);
  gtk_clist_column_titles_show (GTK_CLIST (clist3));

  label117 = gtk_label_new (_("Directories"));
  gtk_widget_set_name (label117, "label117");
  gtk_widget_show (label117);
  gtk_clist_set_column_widget (GTK_CLIST (clist3), 0, label117);

  scrolledwindow20 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_name (scrolledwindow20, "scrolledwindow20");
  gtk_widget_show (scrolledwindow20);
  gtk_box_pack_start (GTK_BOX (hbox43), scrolledwindow20, TRUE, TRUE, 0);
  gtk_widget_set_size_request (scrolledwindow20, 102, -1);
  gtk_container_set_border_width (GTK_CONTAINER (scrolledwindow20), 4);

  clist4 = gtk_clist_new (1);
  gtk_widget_set_name (clist4, "clist4");
  gtk_widget_show (clist4);
  gtk_container_add (GTK_CONTAINER (scrolledwindow20), clist4);
  gtk_clist_set_column_width (GTK_CLIST (clist4), 0, 80);
  gtk_clist_column_titles_show (GTK_CLIST (clist4));

  label118 = gtk_label_new (_("Files"));
  gtk_widget_set_name (label118, "label118");
  gtk_widget_show (label118);
  gtk_clist_set_column_widget (GTK_CLIST (clist4), 0, label118);

  frame68 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame68, "frame68");
  gtk_widget_show (frame68);
  gtk_box_pack_start (GTK_BOX (vbox33), frame68, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame68), 4);
  gtk_frame_set_shadow_type (GTK_FRAME (frame68), GTK_SHADOW_NONE);

  alignment60 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment60, "alignment60");
  gtk_widget_show (alignment60);
  gtk_container_add (GTK_CONTAINER (frame68), alignment60);
  gtk_container_set_border_width (GTK_CONTAINER (alignment60), 2);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment60), 0, 0, 6, 0);

  hbox44 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox44, "hbox44");
  gtk_widget_show (hbox44);
  gtk_container_add (GTK_CONTAINER (alignment60), hbox44);

  label119 = gtk_label_new (_("File selected:"));
  gtk_widget_set_name (label119, "label119");
  gtk_widget_show (label119);
  gtk_box_pack_start (GTK_BOX (hbox44), label119, FALSE, FALSE, 0);

  t_fileselected = gtk_entry_new ();
  gtk_widget_set_name (t_fileselected, "t_fileselected");
  gtk_widget_show (t_fileselected);
  gtk_box_pack_start (GTK_BOX (hbox44), t_fileselected, TRUE, TRUE, 0);

  vbox34 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox34, "vbox34");
  gtk_widget_show (vbox34);
  gtk_box_pack_start (GTK_BOX (hbox40), vbox34, FALSE, TRUE, 0);

  bf_add = gtk_button_new_from_stock ("gtk-add");
  gtk_widget_set_name (bf_add, "bf_add");
  gtk_widget_show (bf_add);
  gtk_box_pack_start (GTK_BOX (vbox34), bf_add, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (bf_add), 4);
  GTK_WIDGET_SET_FLAGS (bf_add, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (bf_add, "clicked", accel_group,
                              GDK_A, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);

  bf_remove = gtk_button_new_from_stock ("gtk-remove");
  gtk_widget_set_name (bf_remove, "bf_remove");
  gtk_widget_show (bf_remove);
  gtk_box_pack_start (GTK_BOX (vbox34), bf_remove, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (bf_remove), 4);
  GTK_WIDGET_SET_FLAGS (bf_remove, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (bf_remove, "clicked", accel_group,
                              GDK_R, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);

  bf_configure = gtk_button_new_with_mnemonic (_("Confi_gure"));
  gtk_widget_set_name (bf_configure, "bf_configure");
  gtk_widget_show (bf_configure);
  gtk_box_pack_start (GTK_BOX (vbox34), bf_configure, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (bf_configure), 4);
  GTK_WIDGET_SET_FLAGS (bf_configure, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (bf_configure, "clicked", accel_group,
                              GDK_G, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (bf_configure, "clicked", accel_group,
                              GDK_g, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);

  bf_test = gtk_button_new_with_mnemonic (_("_Test"));
  gtk_widget_set_name (bf_test, "bf_test");
  gtk_widget_show (bf_test);
  gtk_box_pack_start (GTK_BOX (vbox34), bf_test, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (bf_test), 4);
  GTK_WIDGET_SET_FLAGS (bf_test, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (bf_test, "clicked", accel_group,
                              GDK_T, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);

  bf_setdir = gtk_button_new_with_mnemonic (_("_Set Dir"));
  gtk_widget_set_name (bf_setdir, "bf_setdir");
  gtk_widget_show (bf_setdir);
  gtk_box_pack_start (GTK_BOX (vbox34), bf_setdir, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (bf_setdir), 4);
  GTK_WIDGET_SET_FLAGS (bf_setdir, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (bf_setdir, "clicked", accel_group,
                              GDK_S, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);

  frame69 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame69, "frame69");
  gtk_widget_show (frame69);
  gtk_box_pack_start (GTK_BOX (vbox34), frame69, TRUE, TRUE, 0);
  gtk_widget_set_size_request (frame69, -1, 80);
  gtk_frame_set_shadow_type (GTK_FRAME (frame69), GTK_SHADOW_NONE);

  alignment61 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment61, "alignment61");
  gtk_widget_show (alignment61);
  gtk_container_add (GTK_CONTAINER (frame69), alignment61);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment61), 0, 0, 12, 0);

  frame70 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame70, "frame70");
  gtk_widget_show (frame70);
  gtk_box_pack_start (GTK_BOX (vbox32), frame70, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame70), 3);

  alignment62 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment62, "alignment62");
  gtk_widget_show (alignment62);
  gtk_container_add (GTK_CONTAINER (frame70), alignment62);

  hbox45 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox45, "hbox45");
  gtk_widget_show (hbox45);
  gtk_container_add (GTK_CONTAINER (alignment62), hbox45);
  gtk_container_set_border_width (GTK_CONTAINER (hbox45), 10);

  pixmap3 = gtk_pixmap_new (pixmap, mask);
  gtk_widget_set_name (pixmap3, "pixmap3");
  gtk_widget_show (pixmap3);
  gtk_box_pack_start (GTK_BOX (hbox45), pixmap3, FALSE, TRUE, 10);

  label120 = gtk_label_new (_("Select the file data source that describes the driver that you wish to\nconnect to. You can use any file data source that refers to an ODBC\ndriver which is installed on your machine."));
  gtk_widget_set_name (label120, "label120");
  gtk_widget_show (label120);
  gtk_box_pack_start (GTK_BOX (hbox45), label120, FALSE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (label120), GTK_JUSTIFY_FILL);

  label121 = gtk_label_new (_("File DSN"));
  gtk_widget_set_name (label121, "label121");
  gtk_widget_show (label121);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook1), 2), label121);



  choose_t->fadd = bf_add; 
  choose_t->fremove = bf_remove; 
  choose_t->fconfigure = bf_configure;
  choose_t->ftest = bf_test; 
  choose_t->dir_list = clist3; 
  choose_t->dir_combo = optionmenu1;
  choose_t->file_list = clist4; 
  choose_t->file_entry = t_fileselected;
  choose_t->fsetdir = bf_setdir;

  
  hbuttonbox2 = GTK_DIALOG (dsnchooser)->action_area;
  gtk_widget_set_name (hbuttonbox2, "hbuttonbox2");
  gtk_widget_show (hbuttonbox2);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox2), GTK_BUTTONBOX_END);

  b_cancel = gtk_button_new_from_stock ("gtk-cancel");
  gtk_widget_set_name (b_cancel, "b_cancel");
  gtk_widget_show (b_cancel);
  gtk_dialog_add_action_widget (GTK_DIALOG (dsnchooser), b_cancel, GTK_RESPONSE_CANCEL);
  GTK_WIDGET_SET_FLAGS (b_cancel, GTK_CAN_DEFAULT);

  b_ok = gtk_button_new_from_stock ("gtk-ok");
  gtk_widget_set_name (b_ok, "b_ok");
  gtk_widget_show (b_ok);
  gtk_dialog_add_action_widget (GTK_DIALOG (dsnchooser), b_ok, GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS (b_ok, GTK_CAN_DEFAULT);

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF (dsnchooser, dsnchooser, "dsnchooser");
  GLADE_HOOKUP_OBJECT_NO_REF (dsnchooser, vbox27, "vbox27");
  GLADE_HOOKUP_OBJECT (dsnchooser, notebook1, "notebook1");
  GLADE_HOOKUP_OBJECT (dsnchooser, vbox28, "vbox28");
  GLADE_HOOKUP_OBJECT (dsnchooser, frame60, "frame60");
  GLADE_HOOKUP_OBJECT (dsnchooser, alignment52, "alignment52");
  GLADE_HOOKUP_OBJECT (dsnchooser, hbox36, "hbox36");
  GLADE_HOOKUP_OBJECT (dsnchooser, scrolledwindow17, "scrolledwindow17");
  GLADE_HOOKUP_OBJECT (dsnchooser, clist1, "clist1");
  GLADE_HOOKUP_OBJECT (dsnchooser, label104, "label104");
  GLADE_HOOKUP_OBJECT (dsnchooser, label105, "label105");
  GLADE_HOOKUP_OBJECT (dsnchooser, label106, "label106");
  GLADE_HOOKUP_OBJECT (dsnchooser, vbox29, "vbox29");
  GLADE_HOOKUP_OBJECT (dsnchooser, b_add, "b_add");
  GLADE_HOOKUP_OBJECT (dsnchooser, b_remove, "b_remove");
  GLADE_HOOKUP_OBJECT (dsnchooser, b_configure, "b_configure");
  GLADE_HOOKUP_OBJECT (dsnchooser, b_test, "b_test");
  GLADE_HOOKUP_OBJECT (dsnchooser, frame61, "frame61");
  GLADE_HOOKUP_OBJECT (dsnchooser, alignment53, "alignment53");
  GLADE_HOOKUP_OBJECT (dsnchooser, label107, "label107");
  GLADE_HOOKUP_OBJECT (dsnchooser, frame62, "frame62");
  GLADE_HOOKUP_OBJECT (dsnchooser, alignment54, "alignment54");
  GLADE_HOOKUP_OBJECT (dsnchooser, hbox37, "hbox37");
  GLADE_HOOKUP_OBJECT (dsnchooser, pixmap1, "pixmap1");
  GLADE_HOOKUP_OBJECT (dsnchooser, label108, "label108");
  GLADE_HOOKUP_OBJECT (dsnchooser, label109, "label109");
  GLADE_HOOKUP_OBJECT (dsnchooser, vbox30, "vbox30");
  GLADE_HOOKUP_OBJECT (dsnchooser, frame63, "frame63");
  GLADE_HOOKUP_OBJECT (dsnchooser, alignment55, "alignment55");
  GLADE_HOOKUP_OBJECT (dsnchooser, hbox38, "hbox38");
  GLADE_HOOKUP_OBJECT (dsnchooser, scrolledwindow18, "scrolledwindow18");
  GLADE_HOOKUP_OBJECT (dsnchooser, clist2, "clist2");
  GLADE_HOOKUP_OBJECT (dsnchooser, label110, "label110");
  GLADE_HOOKUP_OBJECT (dsnchooser, label111, "label111");
  GLADE_HOOKUP_OBJECT (dsnchooser, label112, "label112");
  GLADE_HOOKUP_OBJECT (dsnchooser, vbox31, "vbox31");
  GLADE_HOOKUP_OBJECT (dsnchooser, bs_add, "bs_add");
  GLADE_HOOKUP_OBJECT (dsnchooser, bs_remove, "bs_remove");
  GLADE_HOOKUP_OBJECT (dsnchooser, bs_configure, "bs_configure");
  GLADE_HOOKUP_OBJECT (dsnchooser, bs_test, "bs_test");
  GLADE_HOOKUP_OBJECT (dsnchooser, frame64, "frame64");
  GLADE_HOOKUP_OBJECT (dsnchooser, alignment56, "alignment56");
  GLADE_HOOKUP_OBJECT (dsnchooser, label113, "label113");
  GLADE_HOOKUP_OBJECT (dsnchooser, frame65, "frame65");
  GLADE_HOOKUP_OBJECT (dsnchooser, alignment57, "alignment57");
  GLADE_HOOKUP_OBJECT (dsnchooser, hbox39, "hbox39");
  GLADE_HOOKUP_OBJECT (dsnchooser, pixmap2, "pixmap2");
  GLADE_HOOKUP_OBJECT (dsnchooser, label114, "label114");
  GLADE_HOOKUP_OBJECT (dsnchooser, label115, "label115");
  GLADE_HOOKUP_OBJECT (dsnchooser, vbox32, "vbox32");
  GLADE_HOOKUP_OBJECT (dsnchooser, frame66, "frame66");
  GLADE_HOOKUP_OBJECT (dsnchooser, alignment58, "alignment58");
  GLADE_HOOKUP_OBJECT (dsnchooser, hbox40, "hbox40");
  GLADE_HOOKUP_OBJECT (dsnchooser, vbox33, "vbox33");
  GLADE_HOOKUP_OBJECT (dsnchooser, hbox41, "hbox41");
  GLADE_HOOKUP_OBJECT (dsnchooser, frame67, "frame67");
  GLADE_HOOKUP_OBJECT (dsnchooser, alignment59, "alignment59");
  GLADE_HOOKUP_OBJECT (dsnchooser, hbox42, "hbox42");
  GLADE_HOOKUP_OBJECT (dsnchooser, label116, "label116");
  GLADE_HOOKUP_OBJECT (dsnchooser, optionmenu1, "optionmenu1");
  GLADE_HOOKUP_OBJECT (dsnchooser, menu2, "menu2");
  GLADE_HOOKUP_OBJECT (dsnchooser, hbox43, "hbox43");
  GLADE_HOOKUP_OBJECT (dsnchooser, scrolledwindow19, "scrolledwindow19");
  GLADE_HOOKUP_OBJECT (dsnchooser, clist3, "clist3");
  GLADE_HOOKUP_OBJECT (dsnchooser, label117, "label117");
  GLADE_HOOKUP_OBJECT (dsnchooser, scrolledwindow20, "scrolledwindow20");
  GLADE_HOOKUP_OBJECT (dsnchooser, clist4, "clist4");
  GLADE_HOOKUP_OBJECT (dsnchooser, label118, "label118");
  GLADE_HOOKUP_OBJECT (dsnchooser, frame68, "frame68");
  GLADE_HOOKUP_OBJECT (dsnchooser, alignment60, "alignment60");
  GLADE_HOOKUP_OBJECT (dsnchooser, hbox44, "hbox44");
  GLADE_HOOKUP_OBJECT (dsnchooser, label119, "label119");
  GLADE_HOOKUP_OBJECT (dsnchooser, t_fileselected, "t_fileselected");
  GLADE_HOOKUP_OBJECT (dsnchooser, vbox34, "vbox34");
  GLADE_HOOKUP_OBJECT (dsnchooser, bf_add, "bf_add");
  GLADE_HOOKUP_OBJECT (dsnchooser, bf_remove, "bf_remove");
  GLADE_HOOKUP_OBJECT (dsnchooser, bf_configure, "bf_configure");
  GLADE_HOOKUP_OBJECT (dsnchooser, bf_test, "bf_test");
  GLADE_HOOKUP_OBJECT (dsnchooser, bf_setdir, "bf_setdir");
  GLADE_HOOKUP_OBJECT (dsnchooser, frame69, "frame69");
  GLADE_HOOKUP_OBJECT (dsnchooser, alignment61, "alignment61");
  GLADE_HOOKUP_OBJECT (dsnchooser, frame70, "frame70");
  GLADE_HOOKUP_OBJECT (dsnchooser, alignment62, "alignment62");
  GLADE_HOOKUP_OBJECT (dsnchooser, hbox45, "hbox45");
  GLADE_HOOKUP_OBJECT (dsnchooser, pixmap3, "pixmap3");
  GLADE_HOOKUP_OBJECT (dsnchooser, label120, "label120");
  GLADE_HOOKUP_OBJECT (dsnchooser, label121, "label121");
  GLADE_HOOKUP_OBJECT_NO_REF (dsnchooser, hbuttonbox2, "hbuttonbox2");
  GLADE_HOOKUP_OBJECT (dsnchooser, b_cancel, "b_cancel");
  GLADE_HOOKUP_OBJECT (dsnchooser, b_ok, "b_ok");

  /* Notebook events */
  gtk_signal_connect_after (GTK_OBJECT (notebook1), "switch_page",
      GTK_SIGNAL_FUNC (dsnchooser_switch_page), choose_t);
  /* Ok button events */
  gtk_signal_connect (GTK_OBJECT (b_ok), "clicked",
      GTK_SIGNAL_FUNC (dsnchooser_ok_clicked), choose_t);
  /* Cancel button events */
  gtk_signal_connect (GTK_OBJECT (b_cancel), "clicked",
      GTK_SIGNAL_FUNC (dsnchooser_cancel_clicked), choose_t);
  /* Close window button events */
  gtk_signal_connect (GTK_OBJECT (dsnchooser), "delete_event",
      GTK_SIGNAL_FUNC (delete_event), choose_t);
  gtk_signal_connect (GTK_OBJECT (dsnchooser), "destroy",
      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
  /* Add user DSN button events */
  gtk_signal_connect (GTK_OBJECT (choose_t->uadd), "clicked",
      GTK_SIGNAL_FUNC (userdsn_add_clicked), choose_t);
  /* Remove user DSN button events */
  gtk_signal_connect (GTK_OBJECT (choose_t->uremove), "clicked",
      GTK_SIGNAL_FUNC (userdsn_remove_clicked), choose_t);
  /* Test user DSN button events */
  gtk_signal_connect (GTK_OBJECT (choose_t->utest), "clicked",
      GTK_SIGNAL_FUNC (userdsn_test_clicked), choose_t);
  /* Configure user DSN button events */
  gtk_signal_connect (GTK_OBJECT (choose_t->uconfigure), "clicked",
      GTK_SIGNAL_FUNC (userdsn_configure_clicked), choose_t);
  /* Add system DSN button events */
  gtk_signal_connect (GTK_OBJECT (choose_t->sadd), "clicked",
      GTK_SIGNAL_FUNC (systemdsn_add_clicked), choose_t);
  /* Remove system DSN button events */
  gtk_signal_connect (GTK_OBJECT (choose_t->sremove), "clicked",
      GTK_SIGNAL_FUNC (systemdsn_remove_clicked), choose_t);
  /* Test system DSN button events */
  gtk_signal_connect (GTK_OBJECT (choose_t->stest), "clicked",
      GTK_SIGNAL_FUNC (systemdsn_test_clicked), choose_t);
  /* Configure system DSN button events */
  gtk_signal_connect (GTK_OBJECT (choose_t->sconfigure), "clicked",
      GTK_SIGNAL_FUNC (systemdsn_configure_clicked), choose_t);
  /* User DSN list events */
  gtk_signal_connect (GTK_OBJECT (clist1), "select_row",
      GTK_SIGNAL_FUNC (userdsn_list_select), choose_t);
  gtk_signal_connect (GTK_OBJECT (clist1), "unselect_row",
      GTK_SIGNAL_FUNC (userdsn_list_unselect), choose_t);
  /* System DSN list events */
  gtk_signal_connect (GTK_OBJECT (clist2), "select_row",
      GTK_SIGNAL_FUNC (systemdsn_list_select), choose_t);
  gtk_signal_connect (GTK_OBJECT (clist2), "unselect_row",
      GTK_SIGNAL_FUNC (systemdsn_list_unselect), choose_t);

  /* Add file DSN button events */
  gtk_signal_connect (GTK_OBJECT (choose_t->fadd), "clicked",
      GTK_SIGNAL_FUNC (filedsn_add_clicked),
      choose_t);
  /* Remove file DSN button events */
  gtk_signal_connect (GTK_OBJECT (choose_t->fremove), "clicked",
      GTK_SIGNAL_FUNC (filedsn_remove_clicked),
      choose_t);
  /* Test file DSN button events */
  gtk_signal_connect (GTK_OBJECT (choose_t->ftest), "clicked",
     GTK_SIGNAL_FUNC (filedsn_test_clicked),
     choose_t);
  /* Configure file DSN button events */
  gtk_signal_connect (GTK_OBJECT (choose_t->fconfigure), "clicked",
     GTK_SIGNAL_FUNC (filedsn_configure_clicked),
     choose_t);
  /* Configure file DSN button events */
  gtk_signal_connect (GTK_OBJECT (choose_t->fsetdir), "clicked",
     GTK_SIGNAL_FUNC (filedsn_setdir_clicked),
     choose_t);
  /* Directories file DSN list events */
  gtk_signal_connect (GTK_OBJECT (clist3), "select_row",
     GTK_SIGNAL_FUNC (filedsn_dirlist_select),
     choose_t);
  /* Files file DSN list events */
  gtk_signal_connect (GTK_OBJECT (clist4), "select_row",
     GTK_SIGNAL_FUNC (filedsn_filelist_select),
     choose_t);
  gtk_signal_connect (GTK_OBJECT (clist4), "unselect_row",
     GTK_SIGNAL_FUNC (filedsn_filelist_unselect),
     choose_t);

  gtk_window_add_accel_group (GTK_WINDOW (dsnchooser), accel_group);

  SQLSetConfigMode (ODBC_BOTH_DSN);
  if (!SQLGetPrivateProfileString("ODBC", "FileDSNPath", "", 
      choose_t->curr_dir, sizeof(choose_t->curr_dir), "odbcinst.ini"))
    strcpy(choose_t->curr_dir, DEFAULT_FILEDSNPATH);

  adddsns_to_list (clist1, FALSE);

  choose_t->udsnlist = clist1;
  choose_t->sdsnlist = clist2;
  choose_t->type_dsn = USER_DSN;
  choose_t->mainwnd = dsnchooser;

  gtk_widget_show_all (dsnchooser);
  gtk_main ();
}


#define CHECK_DRVCONN_DIALBOX(path) \
  { \
    if ((handle = DLL_OPEN(path)) != NULL) \
      { \
        if (DLL_PROC(handle, "_iodbcdm_drvconn_dialboxw") != NULL) \
          { \
            DLL_CLOSE(handle); \
            retVal = TRUE; \
            goto quit; \
          } \
        else \
          { \
            if (DLL_PROC(handle, "_iodbcdm_drvconn_dialbox") != NULL) \
              { \
                DLL_CLOSE(handle); \
                retVal = TRUE; \
                goto quit; \
              } \
          } \
        DLL_CLOSE(handle); \
      } \
  }



static BOOL
_CheckDriverLoginDlg (
    char *drv
)
{
  char drvbuf[4096] = { L'\0'};
  HDLL handle;
  BOOL retVal = FALSE;


  if (!drv)
    return FALSE;

  SQLSetConfigMode (ODBC_USER_DSN);
  if (!access (drv, X_OK))
    { CHECK_DRVCONN_DIALBOX (drv); }
  if (SQLGetPrivateProfileString (drv, "Driver", "", drvbuf,
    sizeof (drvbuf), "odbcinst.ini"))
    { CHECK_DRVCONN_DIALBOX (drvbuf); }
  if (SQLGetPrivateProfileString (drv, "Setup", "", drvbuf,
    sizeof (drvbuf), "odbcinst.ini"))
    { CHECK_DRVCONN_DIALBOX (drvbuf); }

  SQLSetConfigMode (ODBC_SYSTEM_DSN);
  if (!access (drv, X_OK))
    { CHECK_DRVCONN_DIALBOX (drv); }
  if (SQLGetPrivateProfileString (drv, "Driver", "", drvbuf,
    sizeof (drvbuf), "odbcinst.ini"))
    { CHECK_DRVCONN_DIALBOX (drvbuf); }
  if (SQLGetPrivateProfileString (drv, "Setup", "", drvbuf,
    sizeof (drvbuf), "odbcinst.ini"))
    { CHECK_DRVCONN_DIALBOX (drvbuf); }

quit:
  return retVal;
}
