/*
 *  administrator.c
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

#ifdef __linux
# ifndef _GNU_SOURCE
#  define _GNU_SOURCE	/* make sure dladdr is declared */
# endif
# define HAVE_DL_INFO 1
#endif

#include <iodbc.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include "../gui.h"
#include "odbc4.xpm"


#if defined (HAVE_DLADDR) && !defined(HAVE_DL_INFO)
typedef struct
{
  const char *dli_fname;	/* File name of defining object.  */
  void *dli_fbase;		/* Load address of that object.  */
  const char *dli_sname;	/* Name of nearest symbol.  */
  void *dli_saddr;		/* Exact value of nearest symbol.  */
} Dl_info;

#endif /* HAVE_DL_INFO */

static char *szDriverButtons[] = {
  "_Add a driver",
  "_Remove the driver",
  "Confi_gure the driver"
};

static char *szCpLabels[] = {
  "Name",
  "Pool timeout (seconds)",
  "Probe query"
};

static struct
{
  char *lib_name;
  char *lib_desc;
  char *lib_ver_sym;
} iODBC_Components[] =
{
  {"libiodbc.so.2", "iODBC Driver Manager", "iodbc_version"},
  {"libiodbcadm.so.2", "iODBC Administrator", "iodbcadm_version"},
  {"libiodbcinst.so.2", "iODBC Installer", "iodbcinst_version"},
  {"libdrvproxy.so.2", "iODBC Driver Setup Proxy", "iodbcproxy_version"},
  {"libtranslator.so.2", "iODBC Translation Manager", "iodbctrans_version"}
};


static void
addcomponents_to_list (GtkWidget *widget)
{
  char _date[1024], _size[1024];
  char *data[5];
  struct stat _stat;
#if defined(HAVE_DLADDR)
  Dl_info info;
#endif
  void *handle, *proc;
  int i;

  if (!GTK_IS_CLIST (widget))
    return;

  gtk_clist_clear (GTK_CLIST (widget));

  for (i = 0; i < sizeof (iODBC_Components) / sizeof (iODBC_Components[0]);
      i++)
    {
      /*
       *  Collect basic info on the components
       */
      data[0] = iODBC_Components[i].lib_desc;
      data[1] = VERSION;
      data[2] = iODBC_Components[i].lib_name;
      data[3] = "";		/* Modification Date */
      data[4] = "";		/* Size */

      if ((handle = dlopen (iODBC_Components[i].lib_name, RTLD_LAZY)))
	{
	  /* Find out the version of the library */
	  if ((proc = dlsym (handle, iODBC_Components[i].lib_ver_sym)))
	    data[1] = *(char **) proc;

	  /* Check the size and modification date of the library */
#ifdef HAVE_DLADDR
	  dladdr (proc, &info);
	  if (!stat (info.dli_fname, &_stat))
	    {
	      sprintf (_size, "%lu Kb",
		  (unsigned long) _stat.st_size / 1024L);
	      sprintf (_date, "%s", ctime (&_stat.st_mtime));
	      _date[strlen (_date) - 1] = '\0';	/* remove last \n */
	      data[3] = _date;
	      data[4] = _size;
	    }
#endif
	  gtk_clist_append (GTK_CLIST (widget), data);

	  dlclose (handle);
	}
    }

  if (GTK_CLIST (widget)->rows > 0)
    {
      gtk_clist_columns_autosize (GTK_CLIST (widget));
      gtk_clist_sort (GTK_CLIST (widget));
    }
}


static void
addconnectionpool_to_list (GtkWidget *widget)
{
  char *curr, *buffer = (char *) malloc (sizeof (char) * 65536), *szDriver;
  char cptime[128], cpprobe[1024];
  char *data[3];
  int len, i;
  BOOL careabout;
  UWORD confMode = ODBC_USER_DSN;

  if (!buffer || !GTK_IS_CLIST (widget))
    return;
  gtk_clist_clear (GTK_CLIST (widget));

  /* Get the current config mode */
  while (confMode != ODBC_SYSTEM_DSN + 1)
    {
      /* Get the list of drivers in the user context */
      SQLSetConfigMode (confMode);
#ifdef WIN32
      len =
	  SQLGetPrivateProfileString ("ODBC 32 bit Drivers", NULL, "", buffer,
	  65535, "odbcinst.ini");
#else
      len =
	  SQLGetPrivateProfileString ("ODBC Drivers", NULL, "", buffer, 65535,
	  "odbcinst.ini");
#endif
      if (len)
	goto process;

      goto end;

    process:
      for (curr = buffer; *curr; curr += (STRLEN (curr) + 1))
	{
	  /* Shadowing system odbcinst.ini */
	  for (i = 0, careabout = TRUE; i < GTK_CLIST (widget)->rows; i++)
	    {
	      gtk_clist_get_text (GTK_CLIST (widget), i, 0, &szDriver);
	      if (!strcmp (szDriver, curr))
		{
		  careabout = FALSE;
		  break;
		}
	    }

	  if (!careabout)
	    continue;

	  SQLSetConfigMode (confMode);
#ifdef WIN32
	  SQLGetPrivateProfileString ("ODBC 32 bit Drivers", curr, "", cpprobe,
	      sizeof (cpprobe), "odbcinst.ini");
#else
	  SQLGetPrivateProfileString ("ODBC Drivers", curr, "", cpprobe,
	      sizeof (cpprobe), "odbcinst.ini");
#endif

	  /* Check if the driver is installed */
	  if (strcasecmp (cpprobe, "Installed"))
	    goto end;

	  /* Get the driver library name */
	  SQLSetConfigMode (confMode);
	  if (!SQLGetPrivateProfileString (curr, "CPTimeout", "<Not pooled>",
		  cptime, sizeof (cptime), "odbcinst.ini"))
	    {
	      SQLSetConfigMode (confMode);
	      SQLGetPrivateProfileString ("Default", "CPTimeout",
		  "<Not pooled>", cptime, sizeof (cptime), "odbcinst.ini");
	    }
	  if (!SQLGetPrivateProfileString (curr, "CPProbe", "",
		  cpprobe, sizeof (cpprobe), "odbcinst.ini"))
	    {
	      SQLSetConfigMode (confMode);
	      SQLGetPrivateProfileString ("Default", "CPProbe",
		  "", cpprobe, sizeof (cpprobe), "odbcinst.ini");
	    }

	  if (STRLEN (curr) && STRLEN (cptime))
	    {
	      data[0] = curr;
	      data[1] = cptime;
	      data[2] = cpprobe;
	      gtk_clist_append (GTK_CLIST (widget), data);
	    }
	}

    end:
      if (confMode == ODBC_USER_DSN)
	confMode = ODBC_SYSTEM_DSN;
      else
	confMode = ODBC_SYSTEM_DSN + 1;
    }

  if (GTK_CLIST (widget)->rows > 0)
    {
      gtk_clist_columns_autosize (GTK_CLIST (widget));
      gtk_clist_sort (GTK_CLIST (widget));
    }

  /* Make the clean up */
  free (buffer);
}


static void
admin_apply_tracing (TTRACING *tracing_t)
{
  /* Write keywords for tracing in the ini file */
  SQLWritePrivateProfileString ("ODBC", "Trace",
      (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (tracing_t->
		  allthetime_rb)) == TRUE
	  || gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (tracing_t->
		  onetime_rb)) == TRUE) ? "1" : "0", NULL);
  SQLWritePrivateProfileString ("ODBC", "TraceAutoStop",
      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (tracing_t->
	      onetime_rb)) ? "1" : "0", NULL);
  SQLWritePrivateProfileString ("ODBC", "TraceFile",
      gtk_entry_get_text (GTK_ENTRY (tracing_t->logfile_entry)), NULL);
}


static void
admin_switch_page (GtkNotebook *notebook, GtkNotebookPage *page,
    gint page_num, void **inparams)
{
  TDSNCHOOSER *dsnchoose_t = (inparams) ? inparams[0] : NULL;
  TDRIVERCHOOSER *driverchoose_t = (inparams) ? inparams[1] : NULL;
  TTRACING *tracing_t = (inparams) ? inparams[2] : NULL;
  TCOMPONENT *component_t = (inparams) ? inparams[3] : NULL;
  TCONNECTIONPOOLING *connectionpool_t = (inparams) ? inparams[4] : NULL;
  BOOL trace = FALSE, traceauto = FALSE, perfmon = FALSE;
  char tokenstr[4096] = { 0 };

  switch (page_num)
    {
      /* The User DSN panel */
    case 0:
      if (dsnchoose_t)
	{
	  dsnchoose_t->type_dsn = USER_DSN;
	  adddsns_to_list (dsnchoose_t->udsnlist, FALSE);
	}
      break;

      /* The System DSN panel */
    case 1:
      if (dsnchoose_t)
	{
	  dsnchoose_t->type_dsn = SYSTEM_DSN;
	  adddsns_to_list (dsnchoose_t->sdsnlist, TRUE);
	}
      break;

      /* The File DSN panel */
    case 2:
      if (dsnchoose_t)
        {
          dsnchoose_t->type_dsn = FILE_DSN;

          addlistofdir_to_optionmenu(dsnchoose_t->dir_combo,
	      dsnchoose_t->curr_dir, dsnchoose_t);
          adddirectories_to_list(dsnchoose_t->mainwnd,
	      dsnchoose_t->dir_list, dsnchoose_t->curr_dir);
          addfiles_to_list(dsnchoose_t->mainwnd,
	      dsnchoose_t->file_list, dsnchoose_t->curr_dir);
        }
       break;

      /* The ODBC Driver panel */
    case 3:
      if (driverchoose_t)
	{
	  adddrivers_to_list (driverchoose_t->driverlist,
	      driverchoose_t->mainwnd);
	  gtk_widget_set_sensitive (driverchoose_t->b_remove, FALSE);
	  gtk_widget_set_sensitive (driverchoose_t->b_configure, FALSE);
	}
      break;

      /* The Connection Pooling */
    case 4:
      if (!connectionpool_t->changed)
	{
	  /* Get the connection pooling options */
	  SQLGetPrivateProfileString ("ODBC Connection Pooling", "Perfmon",
	      "", tokenstr, sizeof (tokenstr), "odbcinst.ini");
	  if (!strcasecmp (tokenstr, "1") || !strcasecmp (tokenstr, "On"))
	    perfmon = TRUE;
	  SQLGetPrivateProfileString ("ODBC Connection Pooling", "Retry Wait",
	      "", tokenstr, sizeof (tokenstr), "odbcinst.ini");

	  if (perfmon)
	    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
		(connectionpool_t->enperfmon_rb), 1);
	  else
	    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
		(connectionpool_t->disperfmon_rb), 1);

	  gtk_entry_set_text (GTK_ENTRY (connectionpool_t->retwait_entry),
	      tokenstr);

	  connectionpool_t->changed = TRUE;
	}

      addconnectionpool_to_list (connectionpool_t->driverlist);

      break;

      /* The tracing panel */
    case 5:
      if (!tracing_t->changed)
	{
	  /* Get the traces options */
	  SQLGetPrivateProfileString ("ODBC", "Trace", "", tokenstr,
	      sizeof (tokenstr), NULL);
	  if (!strcasecmp (tokenstr, "1") || !strcasecmp (tokenstr, "On"))
	    trace = TRUE;
	  SQLGetPrivateProfileString ("ODBC", "TraceAutoStop", "", tokenstr,
	      sizeof (tokenstr), NULL);
	  if (!strcasecmp (tokenstr, "1") || !strcasecmp (tokenstr, "On"))
	    traceauto = TRUE;
	  SQLGetPrivateProfileString ("ODBC", "TraceFile", "", tokenstr,
	      sizeof (tokenstr), NULL);

	  /* Set the widgets */
	  if (trace)
	    {
	      if (!traceauto)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tracing_t->
			allthetime_rb), 1);
	      else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tracing_t->
			onetime_rb), 1);
	    }
	  else
	    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tracing_t->
		    donttrace_rb), 1);

	  gtk_entry_set_text (GTK_ENTRY (tracing_t->logfile_entry),
	      (STRLEN (tokenstr)) ? tokenstr : "sql.log");

	  tracing_t->changed = TRUE;
	}
      break;

      /* The About panel */
    case 6:
      if (component_t)
	addcomponents_to_list (component_t->componentlist);
      break;
    };

  if (dsnchoose_t)
    {
      if (dsnchoose_t->uremove)
	gtk_widget_set_sensitive (dsnchoose_t->uremove, FALSE);
      if (dsnchoose_t->uconfigure)
	gtk_widget_set_sensitive (dsnchoose_t->uconfigure, FALSE);
      if (dsnchoose_t->utest)
	gtk_widget_set_sensitive (dsnchoose_t->utest, FALSE);
      if (dsnchoose_t->sremove)
	gtk_widget_set_sensitive (dsnchoose_t->sremove, FALSE);
      if (dsnchoose_t->sconfigure)
	gtk_widget_set_sensitive (dsnchoose_t->sconfigure, FALSE);
      if (dsnchoose_t->stest)
	gtk_widget_set_sensitive (dsnchoose_t->stest, FALSE);
      if (dsnchoose_t->fremove)
	gtk_widget_set_sensitive(dsnchoose_t->fremove, FALSE);
      if (dsnchoose_t->fconfigure)
	gtk_widget_set_sensitive(dsnchoose_t->fconfigure, FALSE);
      if (dsnchoose_t->ftest)
	gtk_widget_set_sensitive(dsnchoose_t->ftest, FALSE);
    }
}


static void
tracing_start_clicked (GtkWidget *widget, TTRACING *tracing_t)
{
  if (tracing_t)
    admin_apply_tracing (tracing_t);
}


static void
tracing_logfile_choosen (GtkWidget *widget, TTRACING *tracing_t)
{
  if (tracing_t)
    {
      gtk_entry_set_text (GTK_ENTRY (tracing_t->logfile_entry),
	  gtk_file_selection_get_filename (GTK_FILE_SELECTION (tracing_t->
		  filesel)));
      tracing_t->filesel = NULL;
    }
}


static void
tracing_browse_clicked (GtkWidget *widget, TTRACING *tracing_t)
{
  GtkWidget *filesel;

  if (tracing_t)
    {
      filesel = gtk_file_selection_new ("Choose your log file ...");
      gtk_window_set_modal (GTK_WINDOW (filesel), TRUE);
      gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel),
	  gtk_entry_get_text (GTK_ENTRY (tracing_t->logfile_entry)));
      /* Ok button events */
      gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->
	      ok_button), "clicked",
	  GTK_SIGNAL_FUNC (tracing_logfile_choosen), tracing_t);
      gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->
	      ok_button), "clicked", GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
      /* Cancel button events */
      gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->
	      cancel_button), "clicked", GTK_SIGNAL_FUNC (gtk_main_quit),
	  NULL);
      /* Close window button events */
      gtk_signal_connect (GTK_OBJECT (filesel), "delete_event",
	  GTK_SIGNAL_FUNC (gtk_main_quit), NULL);

      tracing_t->filesel = filesel;

      gtk_widget_show_all (filesel);
      gtk_main ();
      gtk_widget_destroy (filesel);

      tracing_t->filesel = NULL;
    }
}


static void
cpdriver_list_select (GtkWidget *widget, gint row, gint column,
    GdkEvent *event, TCONNECTIONPOOLING *connectionpool_t)
{
  char *szDriver = NULL, *cptimeout, *cpprobe;
  char sztime[1024] = { 0 };
  char szprobe[1024] = { 0 };
  TCONNECTIONPOOLING choose_t;

  if (connectionpool_t)
    {
      choose_t = *connectionpool_t;
      if (GTK_CLIST (connectionpool_t->driverlist)->selection != NULL)
	{
          memset(choose_t.timeout, 0, sizeof(choose_t.timeout)); 
          memset(choose_t.probe, 0, sizeof(choose_t.probe)); 

	  gtk_clist_get_text (GTK_CLIST (choose_t.driverlist),
	      GPOINTER_TO_INT (GTK_CLIST (choose_t.driverlist)->
		  selection->data), 0, &szDriver);
	  gtk_clist_get_text (GTK_CLIST (choose_t.driverlist),
	      GPOINTER_TO_INT (GTK_CLIST (choose_t.driverlist)->
		  selection->data), 1, &cptimeout);
	  gtk_clist_get_text (GTK_CLIST (choose_t.driverlist),
	      GPOINTER_TO_INT (GTK_CLIST (choose_t.driverlist)->
		  selection->data), 2, &cpprobe);

          strncpy (choose_t.timeout, cptimeout, sizeof(choose_t.timeout)-1);
          strncpy (choose_t.probe, cpprobe, sizeof(choose_t.probe)-1);
          strncpy (choose_t.driver, szDriver, sizeof(choose_t.driver)-1);
	}

      if (szDriver && event && event->type == GDK_2BUTTON_PRESS
	  && (create_connectionpool (connectionpool_t->mainwnd, &choose_t) == TRUE))
	{
	  sprintf (sztime, "CPTimeout=%s", choose_t.timeout);
	  if (!SQLConfigDriver (connectionpool_t->mainwnd, ODBC_CONFIG_DRIVER,
		  szDriver, sztime, NULL, 0, NULL))
	    _iodbcdm_errorbox (connectionpool_t->mainwnd, szDriver,
		"An error occured when trying to set the connection pooling time-out : ");

	  sprintf (szprobe, "CPProbe=%s", choose_t.probe);
	  if (!SQLConfigDriver (connectionpool_t->mainwnd, ODBC_CONFIG_DRIVER,
		  szDriver, szprobe, NULL, 0, NULL))
	    _iodbcdm_errorbox (connectionpool_t->mainwnd, szDriver,
		"An error occured when trying to set the connection probe query : ");

	  addconnectionpool_to_list (connectionpool_t->driverlist);
	}
    }
}


static void
driver_list_select (GtkWidget *widget, gint row, gint column,
    GdkEvent *event, TDRIVERCHOOSER *choose_t)
{
  char *szDriver = NULL;

  if (choose_t)
    {
      if (GTK_CLIST (choose_t->driverlist)->selection != NULL)
	gtk_clist_get_text (GTK_CLIST (choose_t->driverlist),
	    GPOINTER_TO_INT (GTK_CLIST (choose_t->driverlist)->selection->
		data), 0, &szDriver);

      if (szDriver && event && event->type == GDK_2BUTTON_PRESS)
	gtk_signal_emit_by_name (GTK_OBJECT (choose_t->b_configure),
	    "clicked", choose_t);
      else
	{
	  gtk_widget_set_sensitive (choose_t->b_remove, TRUE);
	  gtk_widget_set_sensitive (choose_t->b_configure, TRUE);
	}
    }
}


static void
driver_list_unselect (GtkWidget *widget, gint row, gint column,
    GdkEvent *event, TDRIVERCHOOSER *choose_t)
{
  if (choose_t)
    {
      gtk_widget_set_sensitive (choose_t->b_remove, FALSE);
      gtk_widget_set_sensitive (choose_t->b_configure, FALSE);
    }
}


static void
driver_add_clicked (GtkWidget *widget, TDRIVERCHOOSER *choose_t)
{
  char connstr[4096] = { 0 };
  char tokenstr[4096] = { 0 };
  char *cstr;

  if (choose_t)
    {
      cstr = create_driversetup (choose_t->mainwnd, NULL, connstr, FALSE, TRUE);

      if (cstr && cstr != connstr && cstr != (LPSTR) - 1L)
	{
	  SQLSetConfigMode (ODBC_USER_DSN);
	  if (!SQLInstallDriverEx (cstr, NULL, tokenstr, sizeof (tokenstr),
		  NULL, ODBC_INSTALL_COMPLETE, NULL))
	    {
	      _iodbcdm_errorbox (choose_t->mainwnd, NULL,
		  "An error occured when trying to add the driver : ");
	      goto done;
	    }

	  free (cstr);
	}

      adddrivers_to_list (choose_t->driverlist, choose_t->mainwnd);

    done:
      if (GTK_CLIST (choose_t->driverlist)->selection == NULL)
	{
	  if (choose_t->b_remove)
	    gtk_widget_set_sensitive (choose_t->b_remove, FALSE);
	  if (choose_t->b_configure)
	    gtk_widget_set_sensitive (choose_t->b_configure, FALSE);
	}
    }

  return;
}


static void
driver_remove_clicked (GtkWidget *widget, TDRIVERCHOOSER *choose_t)
{
  char *szDriver = NULL;

  if (choose_t)
    {
      /* Retrieve the driver name */
      if (GTK_CLIST (choose_t->driverlist)->selection != NULL)
	gtk_clist_get_text (GTK_CLIST (choose_t->driverlist),
	    GPOINTER_TO_INT (GTK_CLIST (choose_t->driverlist)->selection->
		data), 0, &szDriver);

      /* Call the right function */
      if (szDriver
	  && create_confirm (choose_t->mainwnd, szDriver,
	      "Are you sure you want to remove this driver ?"))
	{
	  if (!SQLRemoveDriver (szDriver, create_confirm (choose_t->mainwnd,
		      szDriver,
		      "Do you want to remove all the DSN associated to this driver ?"),
		  NULL))
	    {
	      _iodbcdm_errorbox (choose_t->mainwnd, szDriver,
		  "An error occured when trying to remove the driver : ");
	      goto done;
	    }

	  adddrivers_to_list (choose_t->driverlist, choose_t->mainwnd);
	}

    done:
      if (GTK_CLIST (choose_t->driverlist)->selection == NULL)
	{
	  if (choose_t->b_remove)
	    gtk_widget_set_sensitive (choose_t->b_remove, FALSE);
	  if (choose_t->b_configure)
	    gtk_widget_set_sensitive (choose_t->b_configure, FALSE);
	}
    }

  return;
}


static void
driver_configure_clicked (GtkWidget *widget, TDRIVERCHOOSER *choose_t)
{
  char connstr[4096] = { 0 };
  char tokenstr[4096] = { 0 };
  char *szDriver = NULL, *curr, *cour, *cstr;
  int size = sizeof (connstr);

  if (choose_t)
    {
      /* Retrieve the driver name */
      if (GTK_CLIST (choose_t->driverlist)->selection != NULL)
	gtk_clist_get_text (GTK_CLIST (choose_t->driverlist),
	    GPOINTER_TO_INT (GTK_CLIST (choose_t->driverlist)->selection->
		data), 0, &szDriver);

      /* Call the right function */
      if (szDriver)
	{
	  SQLSetConfigMode (ODBC_USER_DSN);
	  if (!SQLGetPrivateProfileString (szDriver, NULL, "", tokenstr,
		  sizeof (tokenstr), "odbcinst.ini"))
	    {
	      _iodbcdm_errorbox (choose_t->mainwnd, szDriver,
		  "An error occured when trying to configure the driver : ");
	      goto done;
	    }

	  for (curr = tokenstr, cour = connstr; *curr;
	      curr += (STRLEN (curr) + 1), cour += (STRLEN (cour) + 1))
	    {
	      STRCPY (cour, curr);
	      cour[STRLEN (curr)] = '=';
	      SQLSetConfigMode (ODBC_USER_DSN);
	      SQLGetPrivateProfileString (szDriver, curr, "",
		  cour + STRLEN (curr) + 1, size - STRLEN (curr) - 1,
		  "odbcinst.ini");
	      size -= (STRLEN (cour) + 1);
	    }

	  *cour = 0;

	  cstr =
	      create_driversetup (choose_t->mainwnd, szDriver, connstr,
	      FALSE, TRUE);

	  if (cstr && cstr != connstr && cstr != (LPSTR) - 1L)
	    {
	      SQLSetConfigMode (ODBC_USER_DSN);
	      if (!SQLInstallDriverEx (cstr, NULL, tokenstr,
		      sizeof (tokenstr), NULL, ODBC_INSTALL_COMPLETE, NULL))
		{
		  _iodbcdm_errorbox (choose_t->mainwnd, NULL,
		      "An error occured when trying to configure the driver : ");
		  goto done;
		}

	      free (cstr);
	    }

	  adddrivers_to_list (choose_t->driverlist, choose_t->mainwnd);
	}

    done:
      if (GTK_CLIST (choose_t->driverlist)->selection == NULL)
	{
	  if (choose_t->b_remove)
	    gtk_widget_set_sensitive (choose_t->b_remove, FALSE);
	  if (choose_t->b_configure)
	    gtk_widget_set_sensitive (choose_t->b_configure, FALSE);
	}
    }

  return;
}


static void
admin_ok_clicked (GtkWidget *widget, void **inparams)
{
  TDSNCHOOSER *dsnchoose_t = (inparams) ? inparams[0] : NULL;
  TDRIVERCHOOSER *driverchoose_t = (inparams) ? inparams[1] : NULL;
  TTRACING *tracing_t = (inparams) ? inparams[2] : NULL;
  TCOMPONENT *component_t = (inparams) ? inparams[3] : NULL;
  TCONNECTIONPOOLING *connectionpool_t = (inparams) ? inparams[4] : NULL;
  GtkWidget *mainwnd = (inparams) ? inparams[5] : NULL;

  if (dsnchoose_t)
    {
      dsnchoose_t->udsnlist = dsnchoose_t->sdsnlist = dsnchoose_t->dir_list =
	  NULL;
      dsnchoose_t->fadd = dsnchoose_t->fremove = dsnchoose_t->ftest =
	  dsnchoose_t->fconfigure = dsnchoose_t->fsetdir = NULL;
      dsnchoose_t->uadd = dsnchoose_t->uremove = dsnchoose_t->utest =
	  dsnchoose_t->uconfigure = NULL;
      dsnchoose_t->sadd = dsnchoose_t->sremove = dsnchoose_t->stest =
	  dsnchoose_t->sconfigure = NULL;
      dsnchoose_t->file_list = dsnchoose_t->file_entry =
	  dsnchoose_t->dir_combo = NULL;
      dsnchoose_t->type_dsn = -1;
      dsnchoose_t->dsn = NULL;
    }

  if (driverchoose_t)
    driverchoose_t->driverlist = NULL;

  if (component_t)
    component_t->componentlist = NULL;

  if (tracing_t)
    {
      if (tracing_t->changed)
	admin_apply_tracing (tracing_t);

      tracing_t->logfile_entry = tracing_t->tracelib_entry =
	  tracing_t->b_start_stop = NULL;
      tracing_t->donttrace_rb = tracing_t->allthetime_rb =
	  tracing_t->onetime_rb = NULL;
    }

  if (connectionpool_t)
    {
      if (connectionpool_t->changed)
	{
	  /* Write keywords for tracing in the ini file */
	  SQLWritePrivateProfileString ("ODBC Connection Pooling", "PerfMon",
	      (GTK_TOGGLE_BUTTON (connectionpool_t->enperfmon_rb)->
		  active) ? "1" : "0", "odbcinst.ini");
	  SQLWritePrivateProfileString ("ODBC Connection Pooling",
	      "Retry Wait",
	      gtk_entry_get_text (GTK_ENTRY (connectionpool_t->
		      retwait_entry)), "odbcinst.ini");
	}

      connectionpool_t->driverlist = connectionpool_t->mainwnd = NULL;
      connectionpool_t->enperfmon_rb = connectionpool_t->disperfmon_rb = NULL;
      connectionpool_t->retwait_entry = NULL;
    }

  if (mainwnd)
    {
      gtk_signal_disconnect_by_func (GTK_OBJECT (mainwnd),
	  GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
      gtk_main_quit ();
      gtk_widget_destroy (mainwnd);
    }
}


static void
admin_cancel_clicked (GtkWidget *widget, void **inparams)
{
  TDSNCHOOSER *dsnchoose_t = (inparams) ? inparams[0] : NULL;
  TDRIVERCHOOSER *driverchoose_t = (inparams) ? inparams[1] : NULL;
  TTRACING *tracing_t = (inparams) ? inparams[2] : NULL;
  TCOMPONENT *component_t = (inparams) ? inparams[3] : NULL;
  TCONNECTIONPOOLING *connectionpool_t = (inparams) ? inparams[4] : NULL;
  GtkWidget *mainwnd = (inparams) ? inparams[5] : NULL;

  if (dsnchoose_t)
    {
      dsnchoose_t->udsnlist = dsnchoose_t->sdsnlist = dsnchoose_t->dir_list =
	  NULL;
      dsnchoose_t->fadd = dsnchoose_t->fremove = dsnchoose_t->ftest =
	  dsnchoose_t->fconfigure = dsnchoose_t->fsetdir = NULL;
      dsnchoose_t->uadd = dsnchoose_t->uremove = dsnchoose_t->utest =
	  dsnchoose_t->uconfigure = NULL;
      dsnchoose_t->sadd = dsnchoose_t->sremove = dsnchoose_t->stest =
	  dsnchoose_t->sconfigure = NULL;
      dsnchoose_t->file_list = dsnchoose_t->file_entry =
	  dsnchoose_t->dir_combo = NULL;
      dsnchoose_t->type_dsn = -1;
      dsnchoose_t->dsn = NULL;
    }

  if (driverchoose_t)
    driverchoose_t->driverlist = NULL;

  if (component_t)
    component_t->componentlist = NULL;

  if (tracing_t)
    {
      tracing_t->logfile_entry = tracing_t->tracelib_entry =
	  tracing_t->b_start_stop = NULL;
      tracing_t->donttrace_rb = tracing_t->allthetime_rb =
	  tracing_t->onetime_rb = NULL;
    }

  if (connectionpool_t)
    {
      connectionpool_t->driverlist = connectionpool_t->mainwnd = NULL;
      connectionpool_t->enperfmon_rb = connectionpool_t->disperfmon_rb = NULL;
      connectionpool_t->retwait_entry = NULL;
    }

  if (mainwnd)
    {
      gtk_signal_disconnect_by_func (GTK_OBJECT (mainwnd),
	  GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
      gtk_main_quit ();
      gtk_widget_destroy (mainwnd);
    }
}


static gint
delete_event (GtkWidget *widget, GdkEvent *event, void **inparams)
{
  admin_cancel_clicked (widget, inparams);
  return FALSE;
}



void
create_administrator (HWND hwnd)
{
  GdkPixmap *pixmap;
  GdkBitmap *mask;
  GtkStyle *style;

  
  GtkWidget *admin;
  GtkWidget *dialog_vbox1;
  GtkWidget *notebook1;
  GtkWidget *vbox1;
  GtkWidget *frame1;
  GtkWidget *alignment1;
  GtkWidget *hbox2;
  GtkWidget *scrolledwindow1;
  GtkWidget *lst_usources;
  GtkWidget *lu_name;
  GtkWidget *lu_description;
  GtkWidget *lu_driver;
  GtkWidget *vbox3;
  GtkWidget *bu_add;
  GtkWidget *bu_remove;
  GtkWidget *bu_config;
  GtkWidget *bu_test;
  GtkWidget *frame18;
  GtkWidget *alignment18;
  GtkWidget *l_usdsn;
  GtkWidget *frame2;
  GtkWidget *alignment2;
  GtkWidget *hbox3;
  GtkWidget *pixmap1;
  GtkWidget *label14;
  GtkWidget *label1;
  GtkWidget *vbox4;
  GtkWidget *frame3;
  GtkWidget *alignment3;
  GtkWidget *hbox4;
  GtkWidget *scrolledwindow2;
  GtkWidget *lst_ssources;
  GtkWidget *ls_name;
  GtkWidget *ls_description;
  GtkWidget *ls_driver;
  GtkWidget *vbox5;
  GtkWidget *bs_add;
  GtkWidget *bs_remove;
  GtkWidget *bs_config;
  GtkWidget *bs_test;
  GtkWidget *frame19;
  GtkWidget *alignment19;
  GtkWidget *label18;
  GtkWidget *frame4;
  GtkWidget *alignment4;
  GtkWidget *hbox5;
  GtkWidget *pixmap2;
  GtkWidget *label19;
  GtkWidget *label2;
  GtkWidget *vbox6;
  GtkWidget *frame5;
  GtkWidget *alignment5;
  GtkWidget *hbox6;
  GtkWidget *vbox8;
  GtkWidget *hbox8;
  GtkWidget *frame7;
  GtkWidget *alignment7;
  GtkWidget *hbox9;
  GtkWidget *l_lookin;
  GtkWidget *optionmenu1;
  GtkWidget *menu1;
  GtkWidget *hbox11;
  GtkWidget *scrolledwindow3;
  GtkWidget *lst_fdir;
  GtkWidget *l_directory;
  GtkWidget *scrolledwindow4;
  GtkWidget *lst_ffiles;
  GtkWidget *l_files;
  GtkWidget *frame8;
  GtkWidget *alignment8;
  GtkWidget *hbox10;
  GtkWidget *l_selected;
  GtkWidget *t_fileselected;
  GtkWidget *vbox7;
  GtkWidget *bf_add;
  GtkWidget *bf_remove;
  GtkWidget *bf_config;
  GtkWidget *bf_test;
  GtkWidget *bf_setdir;
  GtkWidget *frame20;
  GtkWidget *alignment20;
  GtkWidget *frame6;
  GtkWidget *alignment6;
  GtkWidget *hbox7;
  GtkWidget *pixmap3;
  GtkWidget *label24;
  GtkWidget *label53;
  GtkWidget *vbox9;
  GtkWidget *frame9;
  GtkWidget *alignment9;
  GtkWidget *hbox12;
  GtkWidget *scrolledwindow5;
  GtkWidget *lst_drivers;
  GtkWidget *ld_name;
  GtkWidget *ld_file;
  GtkWidget *ld_date;
  GtkWidget *ld_size;
  GtkWidget *label32;
  GtkWidget *frame10;
  GtkWidget *alignment10;
  GtkWidget *hbox13;
  GtkWidget *b_add_driver;
  GtkWidget *b_remove_driver;
  GtkWidget *b_configure_driver;
  GtkWidget *frame17;
  GtkWidget *alignment17;
  GtkWidget *hbox20;
  GtkWidget *pixmap4;
  GtkWidget *label54;
  GtkWidget *label4;
  GtkWidget *vbox11;
  GtkWidget *hbox21;
  GtkWidget *frame21;
  GtkWidget *alignment21;
  GtkWidget *frame24;
  GtkWidget *alignment24;
  GtkWidget *scrolledwindow9;
  GtkWidget *lst_pool;
  GtkWidget *lp_name;
  GtkWidget *lp_timeout;
  GtkWidget *lp_probe;
  GtkWidget *label58;
  GtkWidget *label55;
  GtkWidget *vbox17;
  GtkWidget *frame22;
  GtkWidget *alignment22;
  GtkWidget *vbox18;
  GtkWidget *br_enable;
  GSList *br_enable_group = NULL;
  GtkWidget *br_disable;
  GtkWidget *label56;
  GtkWidget *frame23;
  GtkWidget *alignment23;
  GtkWidget *t_retrywait;
  GtkWidget *label57;
  GtkWidget *frame12;
  GtkWidget *alignment12;
  GtkWidget *hbox15;
  GtkWidget *pixmap5;
  GtkWidget *label38;
  GtkWidget *label5;
  GtkWidget *vbox13;
  GtkWidget *table1;
  GtkWidget *frame30;
  GtkWidget *frame25;
  GtkWidget *alignment25;
  GtkWidget *table2;
  GtkWidget *b_donottrace;
  GSList *b_donottrace_group = NULL;
  GtkWidget *b_allthetime;
  GtkWidget *b_onetime;
  GtkWidget *b_start;
  GtkWidget *frame28;
  GtkWidget *label59;
  GtkWidget *frame27;
  GtkWidget *alignment27;
  GtkWidget *vbox20;
  GtkWidget *t_tracelib;
  GtkWidget *hbox23;
  GtkWidget *frame34;
  GtkWidget *bt_select_library;
  GtkWidget *frame35;
  GtkWidget *frame31;
  GtkWidget *label61;
  GtkWidget *frame26;
  GtkWidget *alignment26;
  GtkWidget *vbox19;
  GtkWidget *t_logfile;
  GtkWidget *hbox22;
  GtkWidget *frame32;
  GtkWidget *bt_browse;
  GtkWidget *frame33;
  GtkWidget *frame29;
  GtkWidget *label60;
  GtkWidget *frame14;
  GtkWidget *alignment14;
  GtkWidget *hbox17;
  GtkWidget *pixmap6;
  GtkWidget *label43;
  GtkWidget *label6;
  GtkWidget *vbox15;
  GtkWidget *frame15;
  GtkWidget *alignment15;
  GtkWidget *hbox18;
  GtkWidget *scrolledwindow8;
  GtkWidget *lst_about;
  GtkWidget *la_name;
  GtkWidget *la_version;
  GtkWidget *la_file;
  GtkWidget *la_date;
  GtkWidget *la_size;
  GtkWidget *label47;
  GtkWidget *frame16;
  GtkWidget *alignment16;
  GtkWidget *hbox19;
  GtkWidget *pixmap7;
  GtkWidget *label48;
  GtkWidget *label7;
  GtkWidget *dialog_action_area1;
  GtkWidget *cancelbutton1;
  GtkWidget *okbutton1;
  GtkAccelGroup *accel_group;
  TDSNCHOOSER dsnchoose_t;
  TDRIVERCHOOSER driverchoose_t;
  TCOMPONENT component_t;
  TTRACING tracing_t;
  TCONNECTIONPOOLING connectionpool_t;
  void *inparams[6];

  accel_group = gtk_accel_group_new ();

  admin = gtk_dialog_new ();
  gtk_widget_set_name (admin, "admin");
  gtk_widget_set_size_request (admin, 600, 450);
  gtk_window_set_title (GTK_WINDOW (admin), _("iODBC Data Source Administrator"));
  gtk_window_set_position (GTK_WINDOW (admin), GTK_WIN_POS_CENTER);
  gtk_window_set_modal (GTK_WINDOW (admin), TRUE);
  gtk_window_set_default_size (GTK_WINDOW (admin), 600, 450);
  gtk_window_set_type_hint (GTK_WINDOW (admin), GDK_WINDOW_TYPE_HINT_DIALOG);

#if GTK_CHECK_VERSION(2,0,0)
  gtk_widget_show (admin);
#endif

  dialog_vbox1 = GTK_DIALOG (admin)->vbox;
  gtk_widget_set_name (dialog_vbox1, "dialog_vbox1");
  gtk_widget_show (dialog_vbox1);

  notebook1 = gtk_notebook_new ();
  gtk_widget_set_name (notebook1, "notebook1");
  gtk_widget_show (notebook1);
  gtk_box_pack_start (GTK_BOX (dialog_vbox1), notebook1, TRUE, TRUE, 0);

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox1, "vbox1");
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (notebook1), vbox1);

  frame1 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame1, "frame1");
  gtk_widget_show (frame1);
  gtk_box_pack_start (GTK_BOX (vbox1), frame1, TRUE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame1), GTK_SHADOW_NONE);

  alignment1 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment1, "alignment1");
  gtk_widget_show (alignment1);
  gtk_container_add (GTK_CONTAINER (frame1), alignment1);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment1), 0, 0, 4, 0);

  hbox2 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox2, "hbox2");
  gtk_widget_show (hbox2);
  gtk_container_add (GTK_CONTAINER (alignment1), hbox2);

  scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_name (scrolledwindow1, "scrolledwindow1");
  gtk_widget_show (scrolledwindow1);
  gtk_box_pack_start (GTK_BOX (hbox2), scrolledwindow1, TRUE, TRUE, 0);
  gtk_widget_set_size_request (scrolledwindow1, 440, 219);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_SHADOW_IN);

  lst_usources = gtk_clist_new (3);
  gtk_widget_set_name (lst_usources, "lst_usources");
  gtk_widget_show (lst_usources);
  gtk_container_add (GTK_CONTAINER (scrolledwindow1), lst_usources);
  gtk_clist_set_column_width (GTK_CLIST (lst_usources), 0, 80);
  gtk_clist_set_column_width (GTK_CLIST (lst_usources), 1, 80);
  gtk_clist_set_column_width (GTK_CLIST (lst_usources), 2, 80);
  gtk_clist_column_titles_show (GTK_CLIST (lst_usources));

  lu_name = gtk_label_new (_("Name"));
  gtk_widget_set_name (lu_name, "lu_name");
  gtk_widget_show (lu_name);
  gtk_clist_set_column_widget (GTK_CLIST (lst_usources), 0, lu_name);
  gtk_widget_set_size_request (lu_name, 100, -1);

  lu_description = gtk_label_new (_("Description"));
  gtk_widget_set_name (lu_description, "lu_description");
  gtk_widget_show (lu_description);
  gtk_clist_set_column_widget (GTK_CLIST (lst_usources), 1, lu_description);
  gtk_widget_set_size_request (lu_description, 162, -1);

  lu_driver = gtk_label_new (_("Driver"));
  gtk_widget_set_name (lu_driver, "lu_driver");
  gtk_widget_show (lu_driver);
  gtk_clist_set_column_widget (GTK_CLIST (lst_usources), 2, lu_driver);
  gtk_widget_set_size_request (lu_driver, 80, -1);

  vbox3 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox3, "vbox3");
  gtk_widget_show (vbox3);
  gtk_box_pack_start (GTK_BOX (hbox2), vbox3, FALSE, TRUE, 0);

  bu_add = gtk_button_new_from_stock ("gtk-add");
  gtk_widget_set_name (bu_add, "bu_add");
  gtk_widget_show (bu_add);
  gtk_box_pack_start (GTK_BOX (vbox3), bu_add, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (bu_add), 4);
  GTK_WIDGET_SET_FLAGS (bu_add, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (bu_add, "clicked", accel_group,
                              GDK_A, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);

  bu_remove = gtk_button_new_from_stock ("gtk-remove");
  gtk_widget_set_name (bu_remove, "bu_remove");
  gtk_widget_show (bu_remove);
  gtk_box_pack_start (GTK_BOX (vbox3), bu_remove, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (bu_remove), 4);
  gtk_widget_set_sensitive (bu_remove, FALSE);
  GTK_WIDGET_SET_FLAGS (bu_remove, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (bu_remove, "clicked", accel_group,
                              GDK_R, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);

  bu_config = gtk_button_new_with_mnemonic (_("Confi_gure"));
  gtk_widget_set_name (bu_config, "bu_config");
  gtk_widget_show (bu_config);
  gtk_box_pack_start (GTK_BOX (vbox3), bu_config, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (bu_config), 4);
  gtk_widget_set_sensitive (bu_config, FALSE);
  GTK_WIDGET_SET_FLAGS (bu_config, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (bu_config, "clicked", accel_group,
                              GDK_G, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (bu_config, "clicked", accel_group,
                              GDK_g, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);

  bu_test = gtk_button_new_with_mnemonic (_("_Test"));
  gtk_widget_set_name (bu_test, "bu_test");
  gtk_widget_show (bu_test);
  gtk_box_pack_start (GTK_BOX (vbox3), bu_test, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (bu_test), 4);
  gtk_widget_set_sensitive (bu_test, FALSE);
  GTK_WIDGET_SET_FLAGS (bu_test, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (bu_test, "clicked", accel_group,
                              GDK_T, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);

  frame18 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame18, "frame18");
  gtk_widget_show (frame18);
  gtk_box_pack_start (GTK_BOX (vbox3), frame18, TRUE, TRUE, 0);
  gtk_widget_set_size_request (frame18, -1, 80);
  gtk_frame_set_shadow_type (GTK_FRAME (frame18), GTK_SHADOW_NONE);

  alignment18 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment18, "alignment18");
  gtk_widget_show (alignment18);
  gtk_container_add (GTK_CONTAINER (frame18), alignment18);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment18), 0, 0, 12, 0);

  l_usdsn = gtk_label_new (_(" User Data Sources:"));
  gtk_widget_set_name (l_usdsn, "l_usdsn");
  gtk_widget_show (l_usdsn);
  gtk_frame_set_label_widget (GTK_FRAME (frame1), l_usdsn);
  gtk_label_set_use_markup (GTK_LABEL (l_usdsn), TRUE);

  frame2 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame2, "frame2");
  gtk_widget_show (frame2);
  gtk_box_pack_start (GTK_BOX (vbox1), frame2, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame2), 3);

  alignment2 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment2, "alignment2");
  gtk_widget_show (alignment2);
  gtk_container_add (GTK_CONTAINER (frame2), alignment2);

  hbox3 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox3, "hbox3");
  gtk_widget_show (hbox3);
  gtk_container_add (GTK_CONTAINER (alignment2), hbox3);
  gtk_container_set_border_width (GTK_CONTAINER (hbox3), 10);

#if GTK_CHECK_VERSION(2,0,0)
  style = gtk_widget_get_style (admin);
  pixmap =
      gdk_pixmap_create_from_xpm_d (admin->window, &mask,
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
  gtk_box_pack_start (GTK_BOX (hbox3), pixmap1, FALSE, TRUE, 10);

  label14 = gtk_label_new (_("An ODBC User data source stores information about to connect to\nthe indicated data provider. A User data source is only available to you,\nand can only be used on the current machine."));
  gtk_widget_set_name (label14, "label14");
  gtk_widget_show (label14);
  gtk_box_pack_start (GTK_BOX (hbox3), label14, FALSE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (label14), GTK_JUSTIFY_FILL);

  label1 = gtk_label_new (_("User DSN"));
  gtk_widget_set_name (label1, "label1");
  gtk_widget_show (label1);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook1), 0), label1);


  
  dsnchoose_t.uadd = bu_add;
  dsnchoose_t.uremove = bu_remove;
  dsnchoose_t.utest = bu_test;
  dsnchoose_t.uconfigure = bu_config;



  vbox4 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox4, "vbox4");
  gtk_widget_show (vbox4);
  gtk_container_add (GTK_CONTAINER (notebook1), vbox4);

  frame3 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame3, "frame3");
  gtk_widget_show (frame3);
  gtk_box_pack_start (GTK_BOX (vbox4), frame3, TRUE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame3), GTK_SHADOW_NONE);

  alignment3 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment3, "alignment3");
  gtk_widget_show (alignment3);
  gtk_container_add (GTK_CONTAINER (frame3), alignment3);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment3), 0, 0, 4, 0);

  hbox4 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox4, "hbox4");
  gtk_widget_show (hbox4);
  gtk_container_add (GTK_CONTAINER (alignment3), hbox4);

  scrolledwindow2 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_name (scrolledwindow2, "scrolledwindow2");
  gtk_widget_show (scrolledwindow2);
  gtk_box_pack_start (GTK_BOX (hbox4), scrolledwindow2, TRUE, TRUE, 0);
  gtk_widget_set_size_request (scrolledwindow2, 440, 219);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow2), GTK_SHADOW_IN);

  lst_ssources = gtk_clist_new (3);
  gtk_widget_set_name (lst_ssources, "lst_ssources");
  gtk_widget_show (lst_ssources);
  gtk_container_add (GTK_CONTAINER (scrolledwindow2), lst_ssources);
  gtk_clist_set_column_width (GTK_CLIST (lst_ssources), 0, 80);
  gtk_clist_set_column_width (GTK_CLIST (lst_ssources), 1, 80);
  gtk_clist_set_column_width (GTK_CLIST (lst_ssources), 2, 80);
  gtk_clist_column_titles_show (GTK_CLIST (lst_ssources));

  ls_name = gtk_label_new (_("Name"));
  gtk_widget_set_name (ls_name, "ls_name");
  gtk_widget_show (ls_name);
  gtk_clist_set_column_widget (GTK_CLIST (lst_ssources), 0, ls_name);
  gtk_widget_set_size_request (ls_name, 100, -1);

  ls_description = gtk_label_new (_("Description"));
  gtk_widget_set_name (ls_description, "ls_description");
  gtk_widget_show (ls_description);
  gtk_clist_set_column_widget (GTK_CLIST (lst_ssources), 1, ls_description);
  gtk_widget_set_size_request (ls_description, 162, -1);

  ls_driver = gtk_label_new (_("Driver"));
  gtk_widget_set_name (ls_driver, "ls_driver");
  gtk_widget_show (ls_driver);
  gtk_clist_set_column_widget (GTK_CLIST (lst_ssources), 2, ls_driver);
  gtk_widget_set_size_request (ls_driver, 80, -1);

  vbox5 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox5, "vbox5");
  gtk_widget_show (vbox5);
  gtk_box_pack_start (GTK_BOX (hbox4), vbox5, FALSE, TRUE, 0);

  bs_add = gtk_button_new_from_stock ("gtk-add");
  gtk_widget_set_name (bs_add, "bs_add");
  gtk_widget_show (bs_add);
  gtk_box_pack_start (GTK_BOX (vbox5), bs_add, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (bs_add), 4);
  GTK_WIDGET_SET_FLAGS (bs_add, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (bs_add, "clicked", accel_group,
                              GDK_A, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);

  bs_remove = gtk_button_new_from_stock ("gtk-remove");
  gtk_widget_set_name (bs_remove, "bs_remove");
  gtk_widget_show (bs_remove);
  gtk_box_pack_start (GTK_BOX (vbox5), bs_remove, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (bs_remove), 4);
  gtk_widget_set_sensitive (bs_remove, FALSE);
  GTK_WIDGET_SET_FLAGS (bs_remove, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (bs_remove, "clicked", accel_group,
                              GDK_R, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);

  bs_config = gtk_button_new_with_mnemonic (_("Confi_gure"));
  gtk_widget_set_name (bs_config, "bs_config");
  gtk_widget_show (bs_config);
  gtk_box_pack_start (GTK_BOX (vbox5), bs_config, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (bs_config), 4);
  gtk_widget_set_sensitive (bs_config, FALSE);
  GTK_WIDGET_SET_FLAGS (bs_config, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (bs_config, "clicked", accel_group,
                              GDK_G, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (bs_config, "clicked", accel_group,
                              GDK_g, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);

  bs_test = gtk_button_new_with_mnemonic (_("_Test"));
  gtk_widget_set_name (bs_test, "bs_test");
  gtk_widget_show (bs_test);
  gtk_box_pack_start (GTK_BOX (vbox5), bs_test, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (bs_test), 4);
  gtk_widget_set_sensitive (bs_test, FALSE);
  GTK_WIDGET_SET_FLAGS (bs_test, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (bs_test, "clicked", accel_group,
                              GDK_T, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);

  frame19 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame19, "frame19");
  gtk_widget_show (frame19);
  gtk_box_pack_start (GTK_BOX (vbox5), frame19, TRUE, TRUE, 0);
  gtk_widget_set_size_request (frame19, -1, 80);
  gtk_frame_set_shadow_type (GTK_FRAME (frame19), GTK_SHADOW_NONE);

  alignment19 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment19, "alignment19");
  gtk_widget_show (alignment19);
  gtk_container_add (GTK_CONTAINER (frame19), alignment19);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment19), 0, 0, 12, 0);

  label18 = gtk_label_new (_(" System Data Sources:"));
  gtk_widget_set_name (label18, "label18");
  gtk_widget_show (label18);
  gtk_frame_set_label_widget (GTK_FRAME (frame3), label18);
  gtk_label_set_use_markup (GTK_LABEL (label18), TRUE);

  frame4 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame4, "frame4");
  gtk_widget_show (frame4);
  gtk_box_pack_start (GTK_BOX (vbox4), frame4, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame4), 3);

  alignment4 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment4, "alignment4");
  gtk_widget_show (alignment4);
  gtk_container_add (GTK_CONTAINER (frame4), alignment4);

  hbox5 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox5, "hbox5");
  gtk_widget_show (hbox5);
  gtk_container_add (GTK_CONTAINER (alignment4), hbox5);
  gtk_container_set_border_width (GTK_CONTAINER (hbox5), 10);

  pixmap2 = gtk_pixmap_new (pixmap, mask);
  gtk_widget_set_name (pixmap2, "pixmap2");
  gtk_widget_show (pixmap2);
  gtk_box_pack_start (GTK_BOX (hbox5), pixmap2, FALSE, TRUE, 10);

  label19 = gtk_label_new (_("An ODBC System data source stores information about to connect to\nthe indicated data provider. A System data source is visible to all\nusers on this machine, including daemons."));
  gtk_widget_set_name (label19, "label19");
  gtk_widget_show (label19);
  gtk_box_pack_start (GTK_BOX (hbox5), label19, FALSE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (label19), GTK_JUSTIFY_FILL);

  label2 = gtk_label_new (_("System DSN"));
  gtk_widget_set_name (label2, "label2");
  gtk_widget_show (label2);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook1), 1), label2);


  
  dsnchoose_t.sadd = bs_add;
  dsnchoose_t.sremove = bs_remove;
  dsnchoose_t.stest = bs_test;
  dsnchoose_t.sconfigure = bs_config;


  
  vbox6 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox6, "vbox6");
  gtk_widget_show (vbox6);
  gtk_container_add (GTK_CONTAINER (notebook1), vbox6);

  frame5 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame5, "frame5");
  gtk_widget_show (frame5);
  gtk_box_pack_start (GTK_BOX (vbox6), frame5, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame5), 2);
  gtk_frame_set_shadow_type (GTK_FRAME (frame5), GTK_SHADOW_NONE);

  alignment5 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment5, "alignment5");
  gtk_widget_show (alignment5);
  gtk_container_add (GTK_CONTAINER (frame5), alignment5);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment5), 0, 0, 4, 0);

  hbox6 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox6, "hbox6");
  gtk_widget_show (hbox6);
  gtk_container_add (GTK_CONTAINER (alignment5), hbox6);

  vbox8 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox8, "vbox8");
  gtk_widget_show (vbox8);
  gtk_box_pack_start (GTK_BOX (hbox6), vbox8, TRUE, TRUE, 0);
  gtk_widget_set_size_request (vbox8, 436, 250);

  hbox8 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox8, "hbox8");
  gtk_widget_show (hbox8);
  gtk_box_pack_start (GTK_BOX (vbox8), hbox8, FALSE, FALSE, 0);

  frame7 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame7, "frame7");
  gtk_widget_show (frame7);
  gtk_box_pack_start (GTK_BOX (hbox8), frame7, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame7), 4);
  gtk_frame_set_shadow_type (GTK_FRAME (frame7), GTK_SHADOW_NONE);

  alignment7 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment7, "alignment7");
  gtk_widget_show (alignment7);
  gtk_container_add (GTK_CONTAINER (frame7), alignment7);
  gtk_container_set_border_width (GTK_CONTAINER (alignment7), 2);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment7), 0, 0, 6, 0);

  hbox9 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox9, "hbox9");
  gtk_widget_show (hbox9);
  gtk_container_add (GTK_CONTAINER (alignment7), hbox9);

  l_lookin = gtk_label_new (_("Look in : "));
  gtk_widget_set_name (l_lookin, "l_lookin");
  gtk_widget_show (l_lookin);
  gtk_box_pack_start (GTK_BOX (hbox9), l_lookin, FALSE, FALSE, 0);

  optionmenu1 = gtk_option_menu_new ();
  gtk_widget_set_name (optionmenu1, "optionmenu1");
  gtk_widget_show (optionmenu1);
  gtk_box_pack_start (GTK_BOX (hbox9), optionmenu1, TRUE, TRUE, 0);

  menu1 = gtk_menu_new ();
  gtk_widget_set_name (menu1, "menu1");

  gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu1), menu1);

  hbox11 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox11, "hbox11");
  gtk_widget_show (hbox11);
  gtk_box_pack_start (GTK_BOX (vbox8), hbox11, TRUE, TRUE, 0);

  scrolledwindow3 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_name (scrolledwindow3, "scrolledwindow3");
  gtk_widget_show (scrolledwindow3);
  gtk_box_pack_start (GTK_BOX (hbox11), scrolledwindow3, TRUE, TRUE, 0);
  gtk_widget_set_size_request (scrolledwindow3, 96, -1);
  gtk_container_set_border_width (GTK_CONTAINER (scrolledwindow3), 4);

  lst_fdir = gtk_clist_new (1);
  gtk_widget_set_name (lst_fdir, "lst_fdir");
  gtk_widget_show (lst_fdir);
  gtk_container_add (GTK_CONTAINER (scrolledwindow3), lst_fdir);
  gtk_clist_set_column_width (GTK_CLIST (lst_fdir), 0, 80);
  gtk_clist_column_titles_show (GTK_CLIST (lst_fdir));

  l_directory = gtk_label_new (_("Directories"));
  gtk_widget_set_name (l_directory, "l_directory");
  gtk_widget_show (l_directory);
  gtk_clist_set_column_widget (GTK_CLIST (lst_fdir), 0, l_directory);

  scrolledwindow4 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_name (scrolledwindow4, "scrolledwindow4");
  gtk_widget_show (scrolledwindow4);
  gtk_box_pack_start (GTK_BOX (hbox11), scrolledwindow4, TRUE, TRUE, 0);
  gtk_widget_set_size_request (scrolledwindow4, 102, -1);
  gtk_container_set_border_width (GTK_CONTAINER (scrolledwindow4), 4);

  lst_ffiles = gtk_clist_new (1);
  gtk_widget_set_name (lst_ffiles, "lst_ffiles");
  gtk_widget_show (lst_ffiles);
  gtk_container_add (GTK_CONTAINER (scrolledwindow4), lst_ffiles);
  gtk_clist_set_column_width (GTK_CLIST (lst_ffiles), 0, 80);
  gtk_clist_column_titles_show (GTK_CLIST (lst_ffiles));

  l_files = gtk_label_new (_("Files"));
  gtk_widget_set_name (l_files, "l_files");
  gtk_widget_show (l_files);
  gtk_clist_set_column_widget (GTK_CLIST (lst_ffiles), 0, l_files);

  frame8 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame8, "frame8");
  gtk_widget_show (frame8);
  gtk_box_pack_start (GTK_BOX (vbox8), frame8, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame8), 4);
  gtk_frame_set_shadow_type (GTK_FRAME (frame8), GTK_SHADOW_NONE);

  alignment8 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment8, "alignment8");
  gtk_widget_show (alignment8);
  gtk_container_add (GTK_CONTAINER (frame8), alignment8);
  gtk_container_set_border_width (GTK_CONTAINER (alignment8), 2);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment8), 0, 0, 6, 0);

  hbox10 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox10, "hbox10");
  gtk_widget_show (hbox10);
  gtk_container_add (GTK_CONTAINER (alignment8), hbox10);

  l_selected = gtk_label_new (_("File selected:"));
  gtk_widget_set_name (l_selected, "l_selected");
  gtk_widget_show (l_selected);
  gtk_box_pack_start (GTK_BOX (hbox10), l_selected, FALSE, FALSE, 0);

  t_fileselected = gtk_entry_new ();
  gtk_widget_set_name (t_fileselected, "t_fileselected");
  gtk_widget_show (t_fileselected);
  gtk_box_pack_start (GTK_BOX (hbox10), t_fileselected, TRUE, TRUE, 0);

  vbox7 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox7, "vbox7");
  gtk_widget_show (vbox7);
  gtk_box_pack_start (GTK_BOX (hbox6), vbox7, FALSE, TRUE, 0);

  bf_add = gtk_button_new_from_stock ("gtk-add");
  gtk_widget_set_name (bf_add, "bf_add");
  gtk_widget_show (bf_add);
  gtk_box_pack_start (GTK_BOX (vbox7), bf_add, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (bf_add), 4);
  GTK_WIDGET_SET_FLAGS (bf_add, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (bf_add, "clicked", accel_group,
                              GDK_A, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);

  bf_remove = gtk_button_new_from_stock ("gtk-remove");
  gtk_widget_set_name (bf_remove, "bf_remove");
  gtk_widget_show (bf_remove);
  gtk_box_pack_start (GTK_BOX (vbox7), bf_remove, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (bf_remove), 4);
  GTK_WIDGET_SET_FLAGS (bf_remove, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (bf_remove, "clicked", accel_group,
                              GDK_R, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);

  bf_config = gtk_button_new_with_mnemonic (_("Confi_gure"));
  gtk_widget_set_name (bf_config, "bf_config");
  gtk_widget_show (bf_config);
  gtk_box_pack_start (GTK_BOX (vbox7), bf_config, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (bf_config), 4);
  GTK_WIDGET_SET_FLAGS (bf_config, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (bf_config, "clicked", accel_group,
                              GDK_G, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (bf_config, "clicked", accel_group,
                              GDK_g, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);

  bf_test = gtk_button_new_with_mnemonic (_("_Test"));
  gtk_widget_set_name (bf_test, "bf_test");
  gtk_widget_show (bf_test);
  gtk_box_pack_start (GTK_BOX (vbox7), bf_test, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (bf_test), 4);
  GTK_WIDGET_SET_FLAGS (bf_test, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (bf_test, "clicked", accel_group,
                              GDK_T, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);

  bf_setdir = gtk_button_new_with_mnemonic (_("_Set Dir"));
  gtk_widget_set_name (bf_setdir, "bf_setdir");
  gtk_widget_show (bf_setdir);
  gtk_box_pack_start (GTK_BOX (vbox7), bf_setdir, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (bf_setdir), 4);
  GTK_WIDGET_SET_FLAGS (bf_setdir, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (bf_setdir, "clicked", accel_group,
                              GDK_S, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);

  frame20 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame20, "frame20");
  gtk_widget_show (frame20);
  gtk_box_pack_start (GTK_BOX (vbox7), frame20, TRUE, TRUE, 0);
  gtk_widget_set_size_request (frame20, -1, 80);
  gtk_frame_set_shadow_type (GTK_FRAME (frame20), GTK_SHADOW_NONE);

  alignment20 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment20, "alignment20");
  gtk_widget_show (alignment20);
  gtk_container_add (GTK_CONTAINER (frame20), alignment20);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment20), 0, 0, 12, 0);

  frame6 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame6, "frame6");
  gtk_widget_show (frame6);
  gtk_box_pack_start (GTK_BOX (vbox6), frame6, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame6), 3);

  alignment6 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment6, "alignment6");
  gtk_widget_show (alignment6);
  gtk_container_add (GTK_CONTAINER (frame6), alignment6);

  hbox7 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox7, "hbox7");
  gtk_widget_show (hbox7);
  gtk_container_add (GTK_CONTAINER (alignment6), hbox7);
  gtk_container_set_border_width (GTK_CONTAINER (hbox7), 10);

  pixmap3 = gtk_pixmap_new (pixmap, mask);
  gtk_widget_set_name (pixmap3, "pixmap3");
  gtk_widget_show (pixmap3);
  gtk_box_pack_start (GTK_BOX (hbox7), pixmap3, FALSE, TRUE, 10);

  label24 = gtk_label_new (_("Select the file data source that describes the driver that you wish to\nconnect to. You can use any file data source that refers to an ODBC\ndriver which is installed on your machine."));
  gtk_widget_set_name (label24, "label24");
  gtk_widget_show (label24);
  gtk_box_pack_start (GTK_BOX (hbox7), label24, FALSE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (label24), GTK_JUSTIFY_FILL);

  label53 = gtk_label_new (_("File DSN"));
  gtk_widget_set_name (label53, "label53");
  gtk_widget_show (label53);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook1), 2), label53);


  
  dsnchoose_t.fadd = bf_add; 
  dsnchoose_t.fremove = bf_remove; 
  dsnchoose_t.fconfigure = bf_config;
  dsnchoose_t.ftest = bf_test; 
  dsnchoose_t.dir_list = lst_fdir; 
  dsnchoose_t.dir_combo = optionmenu1;
  dsnchoose_t.file_list = lst_ffiles; 
  dsnchoose_t.file_entry = t_fileselected;
  dsnchoose_t.fsetdir = bf_setdir;


  dsnchoose_t.udsnlist = lst_usources;
  dsnchoose_t.sdsnlist = lst_ssources;
  dsnchoose_t.type_dsn = 0;
  dsnchoose_t.mainwnd = admin;

  
  vbox9 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox9, "vbox9");
  gtk_widget_show (vbox9);
  gtk_container_add (GTK_CONTAINER (notebook1), vbox9);

  frame9 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame9, "frame9");
  gtk_widget_show (frame9);
  gtk_box_pack_start (GTK_BOX (vbox9), frame9, TRUE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame9), GTK_SHADOW_NONE);

  alignment9 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment9, "alignment9");
  gtk_widget_show (alignment9);
  gtk_container_add (GTK_CONTAINER (frame9), alignment9);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment9), 0, 0, 4, 0);

  hbox12 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox12, "hbox12");
  gtk_widget_show (hbox12);
  gtk_container_add (GTK_CONTAINER (alignment9), hbox12);

  scrolledwindow5 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_name (scrolledwindow5, "scrolledwindow5");
  gtk_widget_show (scrolledwindow5);
  gtk_box_pack_start (GTK_BOX (hbox12), scrolledwindow5, TRUE, TRUE, 0);
  gtk_widget_set_size_request (scrolledwindow5, 440, 185);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow5), GTK_SHADOW_IN);

  lst_drivers = gtk_clist_new (4);
  gtk_widget_set_name (lst_drivers, "lst_drivers");
  gtk_widget_show (lst_drivers);
  gtk_container_add (GTK_CONTAINER (scrolledwindow5), lst_drivers);
  gtk_clist_set_column_width (GTK_CLIST (lst_drivers), 0, 80);
  gtk_clist_set_column_width (GTK_CLIST (lst_drivers), 1, 80);
  gtk_clist_set_column_width (GTK_CLIST (lst_drivers), 2, 80);
  gtk_clist_set_column_width (GTK_CLIST (lst_drivers), 3, 80);
  gtk_clist_column_titles_show (GTK_CLIST (lst_drivers));

  ld_name = gtk_label_new (_("Name"));
  gtk_widget_set_name (ld_name, "ld_name");
  gtk_widget_show (ld_name);
  gtk_clist_set_column_widget (GTK_CLIST (lst_drivers), 0, ld_name);
  gtk_widget_set_size_request (ld_name, 188, -1);

  ld_file = gtk_label_new (_("File"));
  gtk_widget_set_name (ld_file, "ld_file");
  gtk_widget_show (ld_file);
  gtk_clist_set_column_widget (GTK_CLIST (lst_drivers), 1, ld_file);
  gtk_widget_set_size_request (ld_file, 170, -1);

  ld_date = gtk_label_new (_("Date"));
  gtk_widget_set_name (ld_date, "ld_date");
  gtk_widget_show (ld_date);
  gtk_clist_set_column_widget (GTK_CLIST (lst_drivers), 2, ld_date);
  gtk_widget_set_size_request (ld_date, 134, -1);

  ld_size = gtk_label_new (_("Size"));
  gtk_widget_set_name (ld_size, "ld_size");
  gtk_widget_show (ld_size);
  gtk_clist_set_column_widget (GTK_CLIST (lst_drivers), 3, ld_size);
  gtk_widget_set_size_request (ld_size, 80, -1);

  label32 = gtk_label_new (_(" ODBC Drivers that are installed on your system :"));
  gtk_widget_set_name (label32, "label32");
  gtk_widget_show (label32);
  gtk_frame_set_label_widget (GTK_FRAME (frame9), label32);
  gtk_label_set_use_markup (GTK_LABEL (label32), TRUE);

  frame10 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame10, "frame10");
  gtk_widget_show (frame10);
  gtk_box_pack_start (GTK_BOX (vbox9), frame10, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame10), 3);

  alignment10 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment10, "alignment10");
  gtk_widget_show (alignment10);
  gtk_container_add (GTK_CONTAINER (frame10), alignment10);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment10), 0, 0, 10, 10);

  hbox13 = gtk_hbox_new (FALSE, 50);
  gtk_widget_set_name (hbox13, "hbox13");
  gtk_widget_show (hbox13);
  gtk_container_add (GTK_CONTAINER (alignment10), hbox13);
  gtk_container_set_border_width (GTK_CONTAINER (hbox13), 4);

  b_add_driver = gtk_button_new_with_mnemonic (_("    _Add a driver    "));
  gtk_widget_set_name (b_add_driver, "b_add_driver");
  gtk_widget_show (b_add_driver);
  gtk_box_pack_start (GTK_BOX (hbox13), b_add_driver, FALSE, TRUE, 0);
  GTK_WIDGET_SET_FLAGS (b_add_driver, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (b_add_driver, "clicked", accel_group,
                              GDK_A, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);

  b_remove_driver = gtk_button_new_with_mnemonic (_("    _Remove the driver    "));
  gtk_widget_set_name (b_remove_driver, "b_remove_driver");
  gtk_widget_show (b_remove_driver);
  gtk_box_pack_start (GTK_BOX (hbox13), b_remove_driver, FALSE, TRUE, 0);
  GTK_WIDGET_SET_FLAGS (b_remove_driver, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (b_remove_driver, "clicked", accel_group,
                              GDK_R, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);

  b_configure_driver = gtk_button_new_with_mnemonic (_("  Confi_gure the driver  "));
  gtk_widget_set_name (b_configure_driver, "b_configure_driver");
  gtk_widget_show (b_configure_driver);
  gtk_box_pack_start (GTK_BOX (hbox13), b_configure_driver, FALSE, TRUE, 0);
  GTK_WIDGET_SET_FLAGS (b_configure_driver, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (b_configure_driver, "clicked", accel_group,
                              GDK_G, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (b_configure_driver, "clicked", accel_group,
                              GDK_g, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);

  frame17 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame17, "frame17");
  gtk_widget_show (frame17);
  gtk_box_pack_start (GTK_BOX (vbox9), frame17, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame17), 3);

  alignment17 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment17, "alignment17");
  gtk_widget_show (alignment17);
  gtk_container_add (GTK_CONTAINER (frame17), alignment17);

  hbox20 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox20, "hbox20");
  gtk_widget_show (hbox20);
  gtk_container_add (GTK_CONTAINER (alignment17), hbox20);
  gtk_container_set_border_width (GTK_CONTAINER (hbox20), 10);

  pixmap4 = gtk_pixmap_new (pixmap, mask);
  gtk_widget_set_name (pixmap4, "pixmap4");
  gtk_widget_show (pixmap4);
  gtk_box_pack_start (GTK_BOX (hbox20), pixmap4, FALSE, TRUE, 10);

  label54 = gtk_label_new (_("An ODBC driver allows ODBC-enabled programs to get information from\nODBC data sources. To install new drivers, use the driver's setup\nprogram if available, or add it with the 'Add' button."));
  gtk_widget_set_name (label54, "label54");
  gtk_widget_show (label54);
  gtk_box_pack_start (GTK_BOX (hbox20), label54, FALSE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (label54), GTK_JUSTIFY_FILL);

  label4 = gtk_label_new (_("ODBC Drivers"));
  gtk_widget_set_name (label4, "label4");
  gtk_widget_show (label4);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook1), 3), label4);



  driverchoose_t.driverlist = lst_drivers;
  driverchoose_t.mainwnd = admin;
  driverchoose_t.b_add = b_add_driver;
  driverchoose_t.b_remove = b_remove_driver;
  driverchoose_t.b_configure = b_configure_driver;


  
  vbox11 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox11, "vbox11");
  gtk_widget_show (vbox11);
  gtk_container_add (GTK_CONTAINER (notebook1), vbox11);

  hbox21 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox21, "hbox21");
  gtk_widget_show (hbox21);
  gtk_box_pack_start (GTK_BOX (vbox11), hbox21, TRUE, TRUE, 0);

  frame21 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame21, "frame21");
  gtk_widget_show (frame21);
  gtk_box_pack_start (GTK_BOX (hbox21), frame21, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame21), 2);

  alignment21 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment21, "alignment21");
  gtk_widget_show (alignment21);
  gtk_container_add (GTK_CONTAINER (frame21), alignment21);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment21), 0, 0, 6, 0);

  frame24 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame24, "frame24");
  gtk_widget_show (frame24);
  gtk_container_add (GTK_CONTAINER (alignment21), frame24);
  gtk_frame_set_shadow_type (GTK_FRAME (frame24), GTK_SHADOW_NONE);

  alignment24 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment24, "alignment24");
  gtk_widget_show (alignment24);
  gtk_container_add (GTK_CONTAINER (frame24), alignment24);

  scrolledwindow9 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_name (scrolledwindow9, "scrolledwindow9");
  gtk_widget_show (scrolledwindow9);
  gtk_container_add (GTK_CONTAINER (alignment24), scrolledwindow9);
  gtk_widget_set_size_request (scrolledwindow9, 427, 160);

  lst_pool = gtk_clist_new (3);
  gtk_widget_set_name (lst_pool, "lst_pool");
  gtk_widget_show (lst_pool);
  gtk_container_add (GTK_CONTAINER (scrolledwindow9), lst_pool);
  gtk_clist_set_column_width (GTK_CLIST (lst_pool), 0, 80);
  gtk_clist_set_column_width (GTK_CLIST (lst_pool), 1, 80);
  gtk_clist_set_column_width (GTK_CLIST (lst_pool), 2, 80);
  gtk_clist_column_titles_show (GTK_CLIST (lst_pool));

  lp_name = gtk_label_new (_("Name"));
  gtk_widget_set_name (lp_name, "lp_name");
  gtk_widget_show (lp_name);
  gtk_clist_set_column_widget (GTK_CLIST (lst_pool), 0, lp_name);

  lp_timeout = gtk_label_new (_("Pool timeout"));
  gtk_widget_set_name (lp_timeout, "lp_timeout");
  gtk_widget_show (lp_timeout);
  gtk_clist_set_column_widget (GTK_CLIST (lst_pool), 1, lp_timeout);

  lp_probe = gtk_label_new (_("Probe query"));
  gtk_widget_set_name (lp_probe, "lp_probe");
  gtk_widget_show (lp_probe);
  gtk_clist_set_column_widget (GTK_CLIST (lst_pool), 2, lp_probe);

  label58 = gtk_label_new (_("ODBC Drivers"));
  gtk_widget_set_name (label58, "label58");
  gtk_widget_show (label58);
  gtk_frame_set_label_widget (GTK_FRAME (frame24), label58);
  gtk_label_set_use_markup (GTK_LABEL (label58), TRUE);

  label55 = gtk_label_new (_(" Connection Pooling Timeout "));
  gtk_widget_set_name (label55, "label55");
  gtk_widget_show (label55);
  gtk_frame_set_label_widget (GTK_FRAME (frame21), label55);
  gtk_label_set_use_markup (GTK_LABEL (label55), TRUE);

  vbox17 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox17, "vbox17");
  gtk_widget_show (vbox17);
  gtk_box_pack_start (GTK_BOX (hbox21), vbox17, TRUE, TRUE, 0);

  frame22 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame22, "frame22");
  gtk_widget_show (frame22);
  gtk_box_pack_start (GTK_BOX (vbox17), frame22, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame22), 3);

  alignment22 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment22, "alignment22");
  gtk_widget_show (alignment22);
  gtk_container_add (GTK_CONTAINER (frame22), alignment22);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment22), 0, 0, 12, 0);

  vbox18 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox18, "vbox18");
  gtk_widget_show (vbox18);
  gtk_container_add (GTK_CONTAINER (alignment22), vbox18);

  br_enable = gtk_radio_button_new_with_mnemonic (NULL, _("_Enable"));
  gtk_widget_set_name (br_enable, "br_enable");
  gtk_widget_show (br_enable);
  gtk_box_pack_start (GTK_BOX (vbox18), br_enable, FALSE, FALSE, 0);
  GTK_WIDGET_SET_FLAGS (br_enable, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (br_enable, "clicked", accel_group,
                              GDK_E, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);
  gtk_radio_button_set_group (GTK_RADIO_BUTTON (br_enable), br_enable_group);
  br_enable_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (br_enable));

  br_disable = gtk_radio_button_new_with_mnemonic (NULL, _("_Disable"));
  gtk_widget_set_name (br_disable, "br_disable");
  gtk_widget_show (br_disable);
  gtk_box_pack_start (GTK_BOX (vbox18), br_disable, FALSE, FALSE, 0);
  GTK_WIDGET_SET_FLAGS (br_disable, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (br_disable, "clicked", accel_group,
                              GDK_D, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);
  gtk_radio_button_set_group (GTK_RADIO_BUTTON (br_disable), br_enable_group);
  br_enable_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (br_disable));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (br_disable), TRUE);

  label56 = gtk_label_new (_(" PerfMon "));
  gtk_widget_set_name (label56, "label56");
  gtk_widget_show (label56);
  gtk_frame_set_label_widget (GTK_FRAME (frame22), label56);
  gtk_label_set_use_markup (GTK_LABEL (label56), TRUE);

  frame23 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame23, "frame23");
  gtk_widget_show (frame23);
  gtk_box_pack_start (GTK_BOX (vbox17), frame23, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame23), 3);

  alignment23 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment23, "alignment23");
  gtk_widget_show (alignment23);
  gtk_container_add (GTK_CONTAINER (frame23), alignment23);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment23), 0, 0, 12, 0);

  t_retrywait = gtk_entry_new ();
  gtk_widget_set_name (t_retrywait, "t_retrywait");
  gtk_widget_show (t_retrywait);
  gtk_container_add (GTK_CONTAINER (alignment23), t_retrywait);

  label57 = gtk_label_new (_(" Retry Wait time "));
  gtk_widget_set_name (label57, "label57");
  gtk_widget_show (label57);
  gtk_frame_set_label_widget (GTK_FRAME (frame23), label57);
  gtk_label_set_use_markup (GTK_LABEL (label57), TRUE);

  frame12 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame12, "frame12");
  gtk_widget_show (frame12);
  gtk_box_pack_start (GTK_BOX (vbox11), frame12, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame12), 3);

  alignment12 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment12, "alignment12");
  gtk_widget_show (alignment12);
  gtk_container_add (GTK_CONTAINER (frame12), alignment12);

  hbox15 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox15, "hbox15");
  gtk_widget_show (hbox15);
  gtk_container_add (GTK_CONTAINER (alignment12), hbox15);
  gtk_container_set_border_width (GTK_CONTAINER (hbox15), 15);

  pixmap5 = gtk_pixmap_new (pixmap, mask);
  gtk_widget_set_name (pixmap5, "pixmap5");
  gtk_widget_show (pixmap5);
  gtk_box_pack_start (GTK_BOX (hbox15), pixmap5, FALSE, TRUE, 10);

  label38 = gtk_label_new (_("Connection pooling allows an application to reuse open connection\nhandles, which saves round-trips to the server."));
  gtk_widget_set_name (label38, "label38");
  gtk_widget_show (label38);
  gtk_box_pack_start (GTK_BOX (hbox15), label38, FALSE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (label38), GTK_JUSTIFY_FILL);

  label5 = gtk_label_new (_("Connection Pooling"));
  gtk_widget_set_name (label5, "label5");
  gtk_widget_show (label5);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook1), 4), label5);


  
  connectionpool_t.driverlist = lst_pool;
  connectionpool_t.enperfmon_rb = br_enable;
  connectionpool_t.disperfmon_rb = br_disable;
  connectionpool_t.retwait_entry = t_retrywait;
  connectionpool_t.changed = FALSE;
  connectionpool_t.mainwnd = admin;

  
  
  vbox13 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox13, "vbox13");
  gtk_widget_show (vbox13);
  gtk_container_add (GTK_CONTAINER (notebook1), vbox13);

  table1 = gtk_table_new (3, 2, FALSE);
  gtk_widget_set_name (table1, "table1");
  gtk_widget_show (table1);
  gtk_box_pack_start (GTK_BOX (vbox13), table1, TRUE, TRUE, 0);
  gtk_widget_set_size_request (table1, -1, 250);

  frame30 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame30, "frame30");
  gtk_widget_show (frame30);
  gtk_table_attach (GTK_TABLE (table1), frame30, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame30), GTK_SHADOW_NONE);

  frame25 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame25, "frame25");
  gtk_widget_show (frame25);
  gtk_table_attach (GTK_TABLE (table1), frame25, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame25), 3);

  alignment25 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment25, "alignment25");
  gtk_widget_show (alignment25);
  gtk_container_add (GTK_CONTAINER (frame25), alignment25);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment25), 0, 0, 12, 0);

  table2 = gtk_table_new (4, 2, FALSE);
  gtk_widget_set_name (table2, "table2");
  gtk_widget_show (table2);
  gtk_container_add (GTK_CONTAINER (alignment25), table2);
  gtk_container_set_border_width (GTK_CONTAINER (table2), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table2), 13);

  b_donottrace = gtk_radio_button_new_with_mnemonic (NULL, _("_Don't trace"));
  gtk_widget_set_name (b_donottrace, "b_donottrace");
  gtk_widget_show (b_donottrace);
  gtk_table_attach (GTK_TABLE (table2), b_donottrace, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND), 0, 0);
  gtk_radio_button_set_group (GTK_RADIO_BUTTON (b_donottrace), b_donottrace_group);
  b_donottrace_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (b_donottrace));

  b_allthetime = gtk_radio_button_new_with_mnemonic (NULL, _("All the t_ime"));
  gtk_widget_set_name (b_allthetime, "b_allthetime");
  gtk_widget_show (b_allthetime);
  gtk_table_attach (GTK_TABLE (table2), b_allthetime, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND), 0, 0);
  gtk_radio_button_set_group (GTK_RADIO_BUTTON (b_allthetime), b_donottrace_group);
  b_donottrace_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (b_allthetime));

  b_onetime = gtk_radio_button_new_with_mnemonic (NULL, _("One-_time only"));
  gtk_widget_set_name (b_onetime, "b_onetime");
  gtk_widget_show (b_onetime);
  gtk_table_attach (GTK_TABLE (table2), b_onetime, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND), 0, 0);
  gtk_radio_button_set_group (GTK_RADIO_BUTTON (b_onetime), b_donottrace_group);
  b_donottrace_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (b_onetime));

  b_start = gtk_button_new_with_mnemonic (_("_Apply tracing settings"));
  gtk_widget_set_name (b_start, "b_start");
  gtk_widget_show (b_start);
  gtk_table_attach (GTK_TABLE (table2), b_start, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND), 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (b_start), 2);

  frame28 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame28, "frame28");
  gtk_widget_show (frame28);
  gtk_table_attach (GTK_TABLE (table2), frame28, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame28), GTK_SHADOW_NONE);

  label59 = gtk_label_new (_(" When to trace "));
  gtk_widget_set_name (label59, "label59");
  gtk_widget_show (label59);
  gtk_frame_set_label_widget (GTK_FRAME (frame25), label59);
  gtk_label_set_use_markup (GTK_LABEL (label59), TRUE);

  frame27 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame27, "frame27");
  gtk_widget_show (frame27);
  gtk_table_attach (GTK_TABLE (table1), frame27, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_widget_set_size_request (frame27, -1, 110);
  gtk_container_set_border_width (GTK_CONTAINER (frame27), 3);

  alignment27 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment27, "alignment27");
  gtk_widget_show (alignment27);
  gtk_container_add (GTK_CONTAINER (frame27), alignment27);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment27), 0, 0, 10, 0);

  vbox20 = gtk_vbox_new (FALSE, 5);
  gtk_widget_set_name (vbox20, "vbox20");
  gtk_widget_show (vbox20);
  gtk_container_add (GTK_CONTAINER (alignment27), vbox20);
  gtk_widget_set_size_request (vbox20, -1, 70);
  gtk_container_set_border_width (GTK_CONTAINER(vbox20), 2);

  t_tracelib = gtk_entry_new ();
  gtk_widget_set_name (t_tracelib, "t_tracelib");
  gtk_widget_show (t_tracelib);
  gtk_box_pack_start (GTK_BOX (vbox20), t_tracelib, FALSE, TRUE, 0);

  hbox23 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox23, "hbox23");
  gtk_widget_show (hbox23);
  gtk_box_pack_start (GTK_BOX (vbox20), hbox23, FALSE, TRUE, 0);

  frame34 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame34, "frame34");
  gtk_widget_show (frame34);
  gtk_box_pack_start (GTK_BOX (hbox23), frame34, TRUE, TRUE, 0);
  gtk_widget_set_size_request (frame34, -1, 30);
  gtk_frame_set_shadow_type (GTK_FRAME (frame34), GTK_SHADOW_NONE);

  bt_select_library = gtk_button_new_with_mnemonic (_("_Select library"));
  gtk_widget_set_name (bt_select_library, "bt_select_library");
  gtk_widget_show (bt_select_library);
  gtk_box_pack_start (GTK_BOX (hbox23), bt_select_library, FALSE, TRUE, 0);
  GTK_WIDGET_SET_FLAGS (bt_select_library, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (bt_select_library, "clicked", accel_group,
                              GDK_S, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);

  frame35 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame35, "frame35");
  gtk_widget_show (frame35);
  gtk_box_pack_start (GTK_BOX (hbox23), frame35, TRUE, TRUE, 0);
  gtk_widget_set_size_request (frame35, -1, 30);
  gtk_frame_set_shadow_type (GTK_FRAME (frame35), GTK_SHADOW_NONE);

  frame31 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame31, "frame31");
  gtk_widget_show (frame31);
  gtk_box_pack_start (GTK_BOX (vbox20), frame31, TRUE, TRUE, 0);
  gtk_widget_set_size_request (frame31, -1, 30);
  gtk_frame_set_shadow_type (GTK_FRAME (frame31), GTK_SHADOW_NONE);

  label61 = gtk_label_new (_(" Custom trace library "));
  gtk_widget_set_name (label61, "label61");
  gtk_widget_show (label61);
  gtk_frame_set_label_widget (GTK_FRAME (frame27), label61);
  gtk_label_set_use_markup (GTK_LABEL (label61), TRUE);

  frame26 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame26, "frame26");
  gtk_widget_show (frame26);
  gtk_table_attach (GTK_TABLE (table1), frame26, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame26), 3);

  alignment26 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment26, "alignment26");
  gtk_widget_show (alignment26);
  gtk_container_add (GTK_CONTAINER (frame26), alignment26);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment26), 0, 0, 12, 0);

  vbox19 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox19, "vbox19");
  gtk_widget_show (vbox19);
  gtk_container_add (GTK_CONTAINER (alignment26), vbox19);

  t_logfile = gtk_entry_new ();
  gtk_widget_set_name (t_logfile, "t_logfile");
  gtk_widget_show (t_logfile);
  gtk_box_pack_start (GTK_BOX (vbox19), t_logfile, TRUE, TRUE, 0);
  gtk_entry_set_text (GTK_ENTRY (t_logfile), _("sql.log"));

  hbox22 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox22, "hbox22");
  gtk_widget_show (hbox22);
  gtk_box_pack_start (GTK_BOX (vbox19), hbox22, FALSE, TRUE, 0);

  frame32 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame32, "frame32");
  gtk_widget_show (frame32);
  gtk_box_pack_start (GTK_BOX (hbox22), frame32, TRUE, TRUE, 0);
  gtk_widget_set_size_request (frame32, -1, 30);
  gtk_frame_set_shadow_type (GTK_FRAME (frame32), GTK_SHADOW_NONE);

  bt_browse = gtk_button_new_with_mnemonic (_("_Browse"));
  gtk_widget_set_name (bt_browse, "bt_browse");
  gtk_widget_show (bt_browse);
  gtk_box_pack_start (GTK_BOX (hbox22), bt_browse, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER(bt_browse), 2);
  GTK_WIDGET_SET_FLAGS (bt_browse, GTK_CAN_DEFAULT);
  gtk_widget_add_accelerator (bt_browse, "clicked", accel_group,
                              GDK_B, (GdkModifierType) GDK_MOD1_MASK,
                              GTK_ACCEL_VISIBLE);

  frame33 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame33, "frame33");
  gtk_widget_show (frame33);
  gtk_box_pack_start (GTK_BOX (hbox22), frame33, TRUE, TRUE, 0);
  gtk_widget_set_size_request (frame33, -1, 30);
  gtk_frame_set_shadow_type (GTK_FRAME (frame33), GTK_SHADOW_NONE);

  frame29 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame29, "frame29");
  gtk_widget_show (frame29);
  gtk_box_pack_start (GTK_BOX (vbox19), frame29, TRUE, TRUE, 0);
  gtk_widget_set_size_request (frame29, -1, 30);
  gtk_frame_set_shadow_type (GTK_FRAME (frame29), GTK_SHADOW_NONE);

  label60 = gtk_label_new (_(" Log file path "));
  gtk_widget_set_name (label60, "label60");
  gtk_widget_show (label60);
  gtk_frame_set_label_widget (GTK_FRAME (frame26), label60);
  gtk_label_set_use_markup (GTK_LABEL (label60), TRUE);

  frame14 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame14, "frame14");
  gtk_widget_show (frame14);
  gtk_box_pack_start (GTK_BOX (vbox13), frame14, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame14), 3);

  alignment14 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment14, "alignment14");
  gtk_widget_show (alignment14);
  gtk_container_add (GTK_CONTAINER (frame14), alignment14);

  hbox17 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox17, "hbox17");
  gtk_widget_show (hbox17);
  gtk_container_add (GTK_CONTAINER (alignment14), hbox17);
  gtk_container_set_border_width (GTK_CONTAINER (hbox17), 10);

  pixmap6 = gtk_pixmap_new (pixmap, mask);
  gtk_widget_set_name (pixmap6, "pixmap6");
  gtk_widget_show (pixmap6);
  gtk_box_pack_start (GTK_BOX (hbox17), pixmap6, FALSE, TRUE, 10);

  label43 = gtk_label_new (_("ODBC tracing allows you to create logs of the calls to ODBC drivers for\nuse by support personnel or to aid you in debugging your applications.\nNote: log files can become very large."));
  gtk_widget_set_name (label43, "label43");
  gtk_widget_show (label43);
  gtk_box_pack_start (GTK_BOX (hbox17), label43, FALSE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (label43), GTK_JUSTIFY_FILL);

  label6 = gtk_label_new (_("Tracing"));
  gtk_widget_set_name (label6, "label6");
  gtk_widget_show (label6);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook1), 5), label6);


  
  tracing_t.logfile_entry = t_logfile;
  tracing_t.tracelib_entry = t_tracelib;
  tracing_t.b_start_stop = b_start;
  tracing_t.donttrace_rb = b_donottrace;
  tracing_t.allthetime_rb = b_allthetime;
  tracing_t.onetime_rb = b_onetime;
  tracing_t.changed = FALSE;


  
  vbox15 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox15, "vbox15");
  gtk_widget_show (vbox15);
  gtk_container_add (GTK_CONTAINER (notebook1), vbox15);

  frame15 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame15, "frame15");
  gtk_widget_show (frame15);
  gtk_box_pack_start (GTK_BOX (vbox15), frame15, TRUE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame15), GTK_SHADOW_NONE);

  alignment15 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment15, "alignment15");
  gtk_widget_show (alignment15);
  gtk_container_add (GTK_CONTAINER (frame15), alignment15);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment15), 0, 0, 4, 0);

  hbox18 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox18, "hbox18");
  gtk_widget_show (hbox18);
  gtk_container_add (GTK_CONTAINER (alignment15), hbox18);

  scrolledwindow8 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_name (scrolledwindow8, "scrolledwindow8");
  gtk_widget_show (scrolledwindow8);
  gtk_box_pack_start (GTK_BOX (hbox18), scrolledwindow8, TRUE, TRUE, 0);
  gtk_widget_set_size_request (scrolledwindow8, 440, 219);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow8), GTK_SHADOW_IN);

  lst_about = gtk_clist_new (5);
  gtk_widget_set_name (lst_about, "lst_about");
  gtk_widget_show (lst_about);
  gtk_container_add (GTK_CONTAINER (scrolledwindow8), lst_about);
  gtk_clist_set_column_width (GTK_CLIST (lst_about), 0, 80);
  gtk_clist_set_column_width (GTK_CLIST (lst_about), 1, 80);
  gtk_clist_set_column_width (GTK_CLIST (lst_about), 2, 80);
  gtk_clist_set_column_width (GTK_CLIST (lst_about), 3, 80);
  gtk_clist_set_column_width (GTK_CLIST (lst_about), 4, 80);
  gtk_clist_column_titles_show (GTK_CLIST (lst_about));

  la_name = gtk_label_new (_("Name"));
  gtk_widget_set_name (la_name, "la_name");
  gtk_widget_show (la_name);
  gtk_clist_set_column_widget (GTK_CLIST (lst_about), 0, la_name);

  la_version = gtk_label_new (_("Version"));
  gtk_widget_set_name (la_version, "la_version");
  gtk_widget_show (la_version);
  gtk_clist_set_column_widget (GTK_CLIST (lst_about), 1, la_version);

  la_file = gtk_label_new (_("File"));
  gtk_widget_set_name (la_file, "la_file");
  gtk_widget_show (la_file);
  gtk_clist_set_column_widget (GTK_CLIST (lst_about), 2, la_file);

  la_date = gtk_label_new (_("Date"));
  gtk_widget_set_name (la_date, "la_date");
  gtk_widget_show (la_date);
  gtk_clist_set_column_widget (GTK_CLIST (lst_about), 3, la_date);

  la_size = gtk_label_new (_("Size"));
  gtk_widget_set_name (la_size, "la_size");
  gtk_widget_show (la_size);
  gtk_clist_set_column_widget (GTK_CLIST (lst_about), 4, la_size);

  label47 = gtk_label_new (_(" ODBC components installed on your system :"));
  gtk_widget_set_name (label47, "label47");
  gtk_widget_show (label47);
  gtk_frame_set_label_widget (GTK_FRAME (frame15), label47);
  gtk_label_set_use_markup (GTK_LABEL (label47), TRUE);

  frame16 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame16, "frame16");
  gtk_widget_show (frame16);
  gtk_box_pack_start (GTK_BOX (vbox15), frame16, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame16), 3);

  alignment16 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment16, "alignment16");
  gtk_widget_show (alignment16);
  gtk_container_add (GTK_CONTAINER (frame16), alignment16);

  hbox19 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox19, "hbox19");
  gtk_widget_show (hbox19);
  gtk_container_add (GTK_CONTAINER (alignment16), hbox19);
  gtk_container_set_border_width (GTK_CONTAINER (hbox19), 10);

  pixmap7 = gtk_pixmap_new (pixmap, mask);
  gtk_widget_set_name (pixmap7, "pixmap7");
  gtk_widget_show (pixmap7);
  gtk_box_pack_start (GTK_BOX (hbox19), pixmap7, FALSE, TRUE, 10);

  label48 = gtk_label_new (_("ODBC is a programming interface that enables applications to access\ndata in database management systems that use Structured Query\nLanguage (SQL) as a data access standard."));
  gtk_widget_set_name (label48, "label48");
  gtk_widget_show (label48);
  gtk_box_pack_start (GTK_BOX (hbox19), label48, FALSE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (label48), GTK_JUSTIFY_FILL);

  label7 = gtk_label_new (_("About"));
  gtk_widget_set_name (label7, "label7");
  gtk_widget_show (label7);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook1), 6), label7);

  dialog_action_area1 = GTK_DIALOG (admin)->action_area;
  gtk_widget_set_name (dialog_action_area1, "dialog_action_area1");
  gtk_widget_show (dialog_action_area1);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1), GTK_BUTTONBOX_END);

  cancelbutton1 = gtk_button_new_from_stock ("gtk-cancel");
  gtk_widget_set_name (cancelbutton1, "cancelbutton1");
  gtk_widget_show (cancelbutton1);
  gtk_dialog_add_action_widget (GTK_DIALOG (admin), cancelbutton1, GTK_RESPONSE_CANCEL);
  GTK_WIDGET_SET_FLAGS (cancelbutton1, GTK_CAN_DEFAULT);

  okbutton1 = gtk_button_new_from_stock ("gtk-ok");
  gtk_widget_set_name (okbutton1, "okbutton1");
  gtk_widget_show (okbutton1);
  gtk_dialog_add_action_widget (GTK_DIALOG (admin), okbutton1, GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS (okbutton1, GTK_CAN_DEFAULT);

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF (admin, admin, "admin");
  GLADE_HOOKUP_OBJECT_NO_REF (admin, dialog_vbox1, "dialog_vbox1");
  GLADE_HOOKUP_OBJECT (admin, notebook1, "notebook1");
  GLADE_HOOKUP_OBJECT (admin, vbox1, "vbox1");
  GLADE_HOOKUP_OBJECT (admin, frame1, "frame1");
  GLADE_HOOKUP_OBJECT (admin, alignment1, "alignment1");
  GLADE_HOOKUP_OBJECT (admin, hbox2, "hbox2");
  GLADE_HOOKUP_OBJECT (admin, scrolledwindow1, "scrolledwindow1");
  GLADE_HOOKUP_OBJECT (admin, lst_usources, "lst_usources");
  GLADE_HOOKUP_OBJECT (admin, lu_name, "lu_name");
  GLADE_HOOKUP_OBJECT (admin, lu_description, "lu_description");
  GLADE_HOOKUP_OBJECT (admin, lu_driver, "lu_driver");
  GLADE_HOOKUP_OBJECT (admin, vbox3, "vbox3");
  GLADE_HOOKUP_OBJECT (admin, bu_add, "bu_add");
  GLADE_HOOKUP_OBJECT (admin, bu_remove, "bu_remove");
  GLADE_HOOKUP_OBJECT (admin, bu_config, "bu_config");
  GLADE_HOOKUP_OBJECT (admin, bu_test, "bu_test");
  GLADE_HOOKUP_OBJECT (admin, frame18, "frame18");
  GLADE_HOOKUP_OBJECT (admin, alignment18, "alignment18");
  GLADE_HOOKUP_OBJECT (admin, l_usdsn, "l_usdsn");
  GLADE_HOOKUP_OBJECT (admin, frame2, "frame2");
  GLADE_HOOKUP_OBJECT (admin, alignment2, "alignment2");
  GLADE_HOOKUP_OBJECT (admin, hbox3, "hbox3");
  GLADE_HOOKUP_OBJECT (admin, pixmap1, "pixmap1");
  GLADE_HOOKUP_OBJECT (admin, label14, "label14");
  GLADE_HOOKUP_OBJECT (admin, label1, "label1");
  GLADE_HOOKUP_OBJECT (admin, vbox4, "vbox4");
  GLADE_HOOKUP_OBJECT (admin, frame3, "frame3");
  GLADE_HOOKUP_OBJECT (admin, alignment3, "alignment3");
  GLADE_HOOKUP_OBJECT (admin, hbox4, "hbox4");
  GLADE_HOOKUP_OBJECT (admin, scrolledwindow2, "scrolledwindow2");
  GLADE_HOOKUP_OBJECT (admin, lst_ssources, "lst_ssources");
  GLADE_HOOKUP_OBJECT (admin, ls_name, "ls_name");
  GLADE_HOOKUP_OBJECT (admin, ls_description, "ls_description");
  GLADE_HOOKUP_OBJECT (admin, ls_driver, "ls_driver");
  GLADE_HOOKUP_OBJECT (admin, vbox5, "vbox5");
  GLADE_HOOKUP_OBJECT (admin, bs_add, "bs_add");
  GLADE_HOOKUP_OBJECT (admin, bs_remove, "bs_remove");
  GLADE_HOOKUP_OBJECT (admin, bs_config, "bs_config");
  GLADE_HOOKUP_OBJECT (admin, bs_test, "bs_test");
  GLADE_HOOKUP_OBJECT (admin, frame19, "frame19");
  GLADE_HOOKUP_OBJECT (admin, alignment19, "alignment19");
  GLADE_HOOKUP_OBJECT (admin, label18, "label18");
  GLADE_HOOKUP_OBJECT (admin, frame4, "frame4");
  GLADE_HOOKUP_OBJECT (admin, alignment4, "alignment4");
  GLADE_HOOKUP_OBJECT (admin, hbox5, "hbox5");
  GLADE_HOOKUP_OBJECT (admin, pixmap2, "pixmap2");
  GLADE_HOOKUP_OBJECT (admin, label19, "label19");
  GLADE_HOOKUP_OBJECT (admin, label2, "label2");
  GLADE_HOOKUP_OBJECT (admin, vbox6, "vbox6");
  GLADE_HOOKUP_OBJECT (admin, frame5, "frame5");
  GLADE_HOOKUP_OBJECT (admin, alignment5, "alignment5");
  GLADE_HOOKUP_OBJECT (admin, hbox6, "hbox6");
  GLADE_HOOKUP_OBJECT (admin, vbox8, "vbox8");
  GLADE_HOOKUP_OBJECT (admin, hbox8, "hbox8");
  GLADE_HOOKUP_OBJECT (admin, frame7, "frame7");
  GLADE_HOOKUP_OBJECT (admin, alignment7, "alignment7");
  GLADE_HOOKUP_OBJECT (admin, hbox9, "hbox9");
  GLADE_HOOKUP_OBJECT (admin, l_lookin, "l_lookin");
  GLADE_HOOKUP_OBJECT (admin, optionmenu1, "optionmenu1");
  GLADE_HOOKUP_OBJECT (admin, menu1, "menu1");
  GLADE_HOOKUP_OBJECT (admin, hbox11, "hbox11");
  GLADE_HOOKUP_OBJECT (admin, scrolledwindow3, "scrolledwindow3");
  GLADE_HOOKUP_OBJECT (admin, lst_fdir, "lst_fdir");
  GLADE_HOOKUP_OBJECT (admin, l_directory, "l_directory");
  GLADE_HOOKUP_OBJECT (admin, scrolledwindow4, "scrolledwindow4");
  GLADE_HOOKUP_OBJECT (admin, lst_ffiles, "lst_ffiles");
  GLADE_HOOKUP_OBJECT (admin, l_files, "l_files");
  GLADE_HOOKUP_OBJECT (admin, frame8, "frame8");
  GLADE_HOOKUP_OBJECT (admin, alignment8, "alignment8");
  GLADE_HOOKUP_OBJECT (admin, hbox10, "hbox10");
  GLADE_HOOKUP_OBJECT (admin, l_selected, "l_selected");
  GLADE_HOOKUP_OBJECT (admin, t_fileselected, "t_fileselected");
  GLADE_HOOKUP_OBJECT (admin, vbox7, "vbox7");
  GLADE_HOOKUP_OBJECT (admin, bf_add, "bf_add");
  GLADE_HOOKUP_OBJECT (admin, bf_remove, "bf_remove");
  GLADE_HOOKUP_OBJECT (admin, bf_config, "bf_config");
  GLADE_HOOKUP_OBJECT (admin, bf_test, "bf_test");
  GLADE_HOOKUP_OBJECT (admin, bf_setdir, "bf_setdir");
  GLADE_HOOKUP_OBJECT (admin, frame20, "frame20");
  GLADE_HOOKUP_OBJECT (admin, alignment20, "alignment20");
  GLADE_HOOKUP_OBJECT (admin, frame6, "frame6");
  GLADE_HOOKUP_OBJECT (admin, alignment6, "alignment6");
  GLADE_HOOKUP_OBJECT (admin, hbox7, "hbox7");
  GLADE_HOOKUP_OBJECT (admin, pixmap3, "pixmap3");
  GLADE_HOOKUP_OBJECT (admin, label24, "label24");
  GLADE_HOOKUP_OBJECT (admin, label53, "label53");
  GLADE_HOOKUP_OBJECT (admin, vbox9, "vbox9");
  GLADE_HOOKUP_OBJECT (admin, frame9, "frame9");
  GLADE_HOOKUP_OBJECT (admin, alignment9, "alignment9");
  GLADE_HOOKUP_OBJECT (admin, hbox12, "hbox12");
  GLADE_HOOKUP_OBJECT (admin, scrolledwindow5, "scrolledwindow5");
  GLADE_HOOKUP_OBJECT (admin, lst_drivers, "lst_drivers");
  GLADE_HOOKUP_OBJECT (admin, ld_name, "ld_name");
  GLADE_HOOKUP_OBJECT (admin, ld_file, "ld_file");
  GLADE_HOOKUP_OBJECT (admin, ld_date, "ld_date");
  GLADE_HOOKUP_OBJECT (admin, ld_size, "ld_size");
  GLADE_HOOKUP_OBJECT (admin, label32, "label32");
  GLADE_HOOKUP_OBJECT (admin, frame10, "frame10");
  GLADE_HOOKUP_OBJECT (admin, alignment10, "alignment10");
  GLADE_HOOKUP_OBJECT (admin, hbox13, "hbox13");
  GLADE_HOOKUP_OBJECT (admin, b_add_driver, "b_add_driver");
  GLADE_HOOKUP_OBJECT (admin, b_remove_driver, "b_remove_driver");
  GLADE_HOOKUP_OBJECT (admin, b_configure_driver, "b_configure_driver");
  GLADE_HOOKUP_OBJECT (admin, frame17, "frame17");
  GLADE_HOOKUP_OBJECT (admin, alignment17, "alignment17");
  GLADE_HOOKUP_OBJECT (admin, hbox20, "hbox20");
  GLADE_HOOKUP_OBJECT (admin, pixmap4, "pixmap4");
  GLADE_HOOKUP_OBJECT (admin, label54, "label54");
  GLADE_HOOKUP_OBJECT (admin, label4, "label4");
  GLADE_HOOKUP_OBJECT (admin, vbox11, "vbox11");
  GLADE_HOOKUP_OBJECT (admin, hbox21, "hbox21");
  GLADE_HOOKUP_OBJECT (admin, frame21, "frame21");
  GLADE_HOOKUP_OBJECT (admin, alignment21, "alignment21");
  GLADE_HOOKUP_OBJECT (admin, frame24, "frame24");
  GLADE_HOOKUP_OBJECT (admin, alignment24, "alignment24");
  GLADE_HOOKUP_OBJECT (admin, scrolledwindow9, "scrolledwindow9");
  GLADE_HOOKUP_OBJECT (admin, lst_pool, "lst_pool");
  GLADE_HOOKUP_OBJECT (admin, lp_name, "lp_name");
  GLADE_HOOKUP_OBJECT (admin, lp_timeout, "lp_timeout");
  GLADE_HOOKUP_OBJECT (admin, lp_probe, "lp_probe");
  GLADE_HOOKUP_OBJECT (admin, label58, "label58");
  GLADE_HOOKUP_OBJECT (admin, label55, "label55");
  GLADE_HOOKUP_OBJECT (admin, vbox17, "vbox17");
  GLADE_HOOKUP_OBJECT (admin, frame22, "frame22");
  GLADE_HOOKUP_OBJECT (admin, alignment22, "alignment22");
  GLADE_HOOKUP_OBJECT (admin, vbox18, "vbox18");
  GLADE_HOOKUP_OBJECT (admin, br_enable, "br_enable");
  GLADE_HOOKUP_OBJECT (admin, br_disable, "br_disable");
  GLADE_HOOKUP_OBJECT (admin, label56, "label56");
  GLADE_HOOKUP_OBJECT (admin, frame23, "frame23");
  GLADE_HOOKUP_OBJECT (admin, alignment23, "alignment23");
  GLADE_HOOKUP_OBJECT (admin, t_retrywait, "t_retrywait");
  GLADE_HOOKUP_OBJECT (admin, label57, "label57");
  GLADE_HOOKUP_OBJECT (admin, frame12, "frame12");
  GLADE_HOOKUP_OBJECT (admin, alignment12, "alignment12");
  GLADE_HOOKUP_OBJECT (admin, hbox15, "hbox15");
  GLADE_HOOKUP_OBJECT (admin, pixmap5, "pixmap5");
  GLADE_HOOKUP_OBJECT (admin, label38, "label38");
  GLADE_HOOKUP_OBJECT (admin, label5, "label5");
  GLADE_HOOKUP_OBJECT (admin, vbox13, "vbox13");
  GLADE_HOOKUP_OBJECT (admin, table1, "table1");
  GLADE_HOOKUP_OBJECT (admin, frame25, "frame25");
  GLADE_HOOKUP_OBJECT (admin, alignment25, "alignment25");
  GLADE_HOOKUP_OBJECT (admin, table2, "table2");
  GLADE_HOOKUP_OBJECT (admin, b_donottrace, "b_donottrace");
  GLADE_HOOKUP_OBJECT (admin, b_allthetime, "b_allthetime");
  GLADE_HOOKUP_OBJECT (admin, b_onetime, "b_onetime");
  GLADE_HOOKUP_OBJECT (admin, b_start, "b_start");
  GLADE_HOOKUP_OBJECT (admin, frame28, "frame28");
  GLADE_HOOKUP_OBJECT (admin, label59, "label59");
  GLADE_HOOKUP_OBJECT (admin, frame26, "frame26");
  GLADE_HOOKUP_OBJECT (admin, alignment26, "alignment26");
  GLADE_HOOKUP_OBJECT (admin, vbox19, "vbox19");
  GLADE_HOOKUP_OBJECT (admin, t_logfile, "t_logfile");
  GLADE_HOOKUP_OBJECT (admin, hbox22, "hbox22");
  GLADE_HOOKUP_OBJECT (admin, frame32, "frame32");
  GLADE_HOOKUP_OBJECT (admin, bt_browse, "bt_browse");
  GLADE_HOOKUP_OBJECT (admin, frame33, "frame33");
  GLADE_HOOKUP_OBJECT (admin, frame29, "frame29");
  GLADE_HOOKUP_OBJECT (admin, label60, "label60");
  GLADE_HOOKUP_OBJECT (admin, frame27, "frame27");
  GLADE_HOOKUP_OBJECT (admin, alignment27, "alignment27");
  GLADE_HOOKUP_OBJECT (admin, vbox20, "vbox20");
  GLADE_HOOKUP_OBJECT (admin, t_tracelib, "t_tracelib");
  GLADE_HOOKUP_OBJECT (admin, hbox23, "hbox23");
  GLADE_HOOKUP_OBJECT (admin, frame34, "frame34");
  GLADE_HOOKUP_OBJECT (admin, bt_select_library, "bt_select_library");
  GLADE_HOOKUP_OBJECT (admin, frame35, "frame35");
  GLADE_HOOKUP_OBJECT (admin, frame31, "frame31");
  GLADE_HOOKUP_OBJECT (admin, label61, "label61");
  GLADE_HOOKUP_OBJECT (admin, frame30, "frame30");
  GLADE_HOOKUP_OBJECT (admin, frame14, "frame14");
  GLADE_HOOKUP_OBJECT (admin, alignment14, "alignment14");
  GLADE_HOOKUP_OBJECT (admin, hbox17, "hbox17");
  GLADE_HOOKUP_OBJECT (admin, pixmap6, "pixmap6");
  GLADE_HOOKUP_OBJECT (admin, label43, "label43");
  GLADE_HOOKUP_OBJECT (admin, label6, "label6");
  GLADE_HOOKUP_OBJECT (admin, vbox15, "vbox15");
  GLADE_HOOKUP_OBJECT (admin, frame15, "frame15");
  GLADE_HOOKUP_OBJECT (admin, alignment15, "alignment15");
  GLADE_HOOKUP_OBJECT (admin, hbox18, "hbox18");
  GLADE_HOOKUP_OBJECT (admin, scrolledwindow8, "scrolledwindow8");
  GLADE_HOOKUP_OBJECT (admin, lst_about, "lst_about");
  GLADE_HOOKUP_OBJECT (admin, la_name, "la_name");
  GLADE_HOOKUP_OBJECT (admin, la_version, "la_version");
  GLADE_HOOKUP_OBJECT (admin, la_file, "la_file");
  GLADE_HOOKUP_OBJECT (admin, la_date, "la_date");
  GLADE_HOOKUP_OBJECT (admin, la_size, "la_size");
  GLADE_HOOKUP_OBJECT (admin, label47, "label47");
  GLADE_HOOKUP_OBJECT (admin, frame16, "frame16");
  GLADE_HOOKUP_OBJECT (admin, alignment16, "alignment16");
  GLADE_HOOKUP_OBJECT (admin, hbox19, "hbox19");
  GLADE_HOOKUP_OBJECT (admin, pixmap7, "pixmap7");
  GLADE_HOOKUP_OBJECT (admin, label48, "label48");
  GLADE_HOOKUP_OBJECT (admin, label7, "label7");
  GLADE_HOOKUP_OBJECT_NO_REF (admin, dialog_action_area1, "dialog_action_area1");
  GLADE_HOOKUP_OBJECT (admin, cancelbutton1, "cancelbutton1");
  GLADE_HOOKUP_OBJECT (admin, okbutton1, "okbutton1");


  /* Notebook events */
  gtk_signal_connect_after (GTK_OBJECT (notebook1), "switch_page",
      GTK_SIGNAL_FUNC (admin_switch_page), inparams);
  /* Ok button events */
  gtk_signal_connect (GTK_OBJECT (okbutton1), "clicked",
      GTK_SIGNAL_FUNC (admin_ok_clicked), inparams);
  /* Cancel button events */
  gtk_signal_connect (GTK_OBJECT (cancelbutton1), "clicked",
      GTK_SIGNAL_FUNC (admin_cancel_clicked), inparams);
  /* Close window button events */
  gtk_signal_connect (GTK_OBJECT (admin), "delete_event",
      GTK_SIGNAL_FUNC (delete_event), inparams);
  gtk_signal_connect (GTK_OBJECT (admin), "destroy",
      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
  /* Add user DSN button events */
  gtk_signal_connect (GTK_OBJECT (dsnchoose_t.uadd), "clicked",
      GTK_SIGNAL_FUNC (userdsn_add_clicked), &dsnchoose_t);
  /* Remove user DSN button events */
  gtk_signal_connect (GTK_OBJECT (dsnchoose_t.uremove), "clicked",
      GTK_SIGNAL_FUNC (userdsn_remove_clicked), &dsnchoose_t);
  /* Test user DSN button events */
  gtk_signal_connect (GTK_OBJECT (dsnchoose_t.utest), "clicked",
      GTK_SIGNAL_FUNC (userdsn_test_clicked), &dsnchoose_t);
  /* Configure user DSN button events */
  gtk_signal_connect (GTK_OBJECT (dsnchoose_t.uconfigure), "clicked",
      GTK_SIGNAL_FUNC (userdsn_configure_clicked), &dsnchoose_t);
  /* Add system DSN button events */
  gtk_signal_connect (GTK_OBJECT (dsnchoose_t.sadd), "clicked",
      GTK_SIGNAL_FUNC (systemdsn_add_clicked), &dsnchoose_t);
  /* Remove system DSN button events */
  gtk_signal_connect (GTK_OBJECT (dsnchoose_t.sremove), "clicked",
      GTK_SIGNAL_FUNC (systemdsn_remove_clicked), &dsnchoose_t);
  /* Test system DSN button events */
  gtk_signal_connect (GTK_OBJECT (dsnchoose_t.stest), "clicked",
      GTK_SIGNAL_FUNC (systemdsn_test_clicked), &dsnchoose_t);
  /* Configure system DSN button events */
  gtk_signal_connect (GTK_OBJECT (dsnchoose_t.sconfigure), "clicked",
      GTK_SIGNAL_FUNC (systemdsn_configure_clicked), &dsnchoose_t);

  /* Add file DSN button events */
  gtk_signal_connect (GTK_OBJECT (dsnchoose_t.fadd), "clicked",
      GTK_SIGNAL_FUNC (filedsn_add_clicked),
      &dsnchoose_t);
  /* Remove file DSN button events */
  gtk_signal_connect (GTK_OBJECT (dsnchoose_t.fremove), "clicked",
      GTK_SIGNAL_FUNC (filedsn_remove_clicked),
      &dsnchoose_t);
  /* Test file DSN button events */
  gtk_signal_connect (GTK_OBJECT (dsnchoose_t.ftest), "clicked",
     GTK_SIGNAL_FUNC (filedsn_test_clicked),
     &dsnchoose_t);
  /* Configure file DSN button events */
  gtk_signal_connect (GTK_OBJECT (dsnchoose_t.fconfigure), "clicked",
     GTK_SIGNAL_FUNC (filedsn_configure_clicked),
     &dsnchoose_t);
  /* Configure file DSN button events */
  gtk_signal_connect (GTK_OBJECT (dsnchoose_t.fsetdir), "clicked",
     GTK_SIGNAL_FUNC (filedsn_setdir_clicked),
     &dsnchoose_t);
  /* Add driver button events */
  gtk_signal_connect (GTK_OBJECT (driverchoose_t.b_add), "clicked",
      GTK_SIGNAL_FUNC (driver_add_clicked), &driverchoose_t);
  /* Remove driver button events */
  gtk_signal_connect (GTK_OBJECT (driverchoose_t.b_remove), "clicked",
      GTK_SIGNAL_FUNC (driver_remove_clicked), &driverchoose_t);
  /* Configure driver button events */
  gtk_signal_connect (GTK_OBJECT (driverchoose_t.b_configure), "clicked",
      GTK_SIGNAL_FUNC (driver_configure_clicked), &driverchoose_t);
  /* User DSN list events */
  gtk_signal_connect (GTK_OBJECT (lst_usources), "select_row",
      GTK_SIGNAL_FUNC (userdsn_list_select), &dsnchoose_t);
  gtk_signal_connect (GTK_OBJECT (lst_usources), "unselect_row",
      GTK_SIGNAL_FUNC (userdsn_list_unselect), &dsnchoose_t);
  /* System DSN list events */
  gtk_signal_connect (GTK_OBJECT (lst_ssources), "select_row",
      GTK_SIGNAL_FUNC (systemdsn_list_select), &dsnchoose_t);
  gtk_signal_connect (GTK_OBJECT (lst_ssources), "unselect_row",
      GTK_SIGNAL_FUNC (systemdsn_list_unselect), &dsnchoose_t);
  /* Directories file DSN list events */
  gtk_signal_connect (GTK_OBJECT (lst_fdir), "select_row",
     GTK_SIGNAL_FUNC (filedsn_dirlist_select),
     &dsnchoose_t);
  /* Files file DSN list events */
  gtk_signal_connect (GTK_OBJECT (lst_ffiles), "select_row",
     GTK_SIGNAL_FUNC (filedsn_filelist_select),
     &dsnchoose_t);
  gtk_signal_connect (GTK_OBJECT (lst_ffiles), "unselect_row",
     GTK_SIGNAL_FUNC (filedsn_filelist_unselect),
     &dsnchoose_t);
  /* Start tracing button events */
  gtk_signal_connect (GTK_OBJECT (b_start), "clicked",
      GTK_SIGNAL_FUNC (tracing_start_clicked), &tracing_t);
  /* Browse file entry events */
  gtk_signal_connect (GTK_OBJECT (bt_browse), "clicked",
      GTK_SIGNAL_FUNC (tracing_browse_clicked), &tracing_t);
  /* Driver list events */
  gtk_signal_connect (GTK_OBJECT (lst_drivers), "select_row",
      GTK_SIGNAL_FUNC (driver_list_select), &driverchoose_t);
  gtk_signal_connect (GTK_OBJECT (lst_drivers), "unselect_row",
      GTK_SIGNAL_FUNC (driver_list_unselect), &driverchoose_t);
  /* Connection pooling list events */
  gtk_signal_connect (GTK_OBJECT (lst_pool), "select_row",
      GTK_SIGNAL_FUNC (cpdriver_list_select), &connectionpool_t);




  SQLSetConfigMode (ODBC_BOTH_DSN);
  if (!SQLGetPrivateProfileString("ODBC", "FileDSNPath", "", 
      dsnchoose_t.curr_dir, sizeof(dsnchoose_t.curr_dir), "odbcinst.ini"))
    strcpy(dsnchoose_t.curr_dir, DEFAULT_FILEDSNPATH);

  adddsns_to_list (lst_usources, FALSE);
  component_t.componentlist = lst_about;

  inparams[0] = &dsnchoose_t;
  inparams[1] = &driverchoose_t;
  inparams[2] = &tracing_t;
  inparams[3] = &component_t;
  inparams[4] = &connectionpool_t;
  inparams[5] = admin;

  gtk_widget_show_all (admin);
  gtk_main ();
}
