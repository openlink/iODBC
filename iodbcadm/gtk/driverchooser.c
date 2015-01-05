/*
 *  driverchooser.c
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
#include <sys/stat.h>
#include <unistd.h>

#include "../gui.h"
#include "img.xpm"

#include "dlf.h"
#include "dlproc.h"


char *szDriverColumnNames[] = {
  "Name",
  "File",
  "Date",
  "Size",
  "Version"
};


void
adddrivers_to_list (GtkWidget *widget, GtkWidget *dlg)
{
  SQLCHAR drvdesc[1024], drvattrs[1024], driver[1024], size[64];
  SQLCHAR *data[4];
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

  if (!GTK_IS_CLIST (widget))
    return;
  gtk_clist_clear (GTK_CLIST (widget));

  /* Create a HENV to get the list of drivers then */
  ret = SQLAllocHandle (SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
  if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
    {
      _iodbcdm_nativeerrorbox (dlg, henv, SQL_NULL_HANDLE, SQL_NULL_HANDLE);
      goto end;
    }

  /* Set the version ODBC API to use */
  SQLSetEnvAttr (henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER) SQL_OV_ODBC3,
      SQL_IS_INTEGER);

  /* Get the list of drivers */
  ret =
      SQLDrivers (henv, SQL_FETCH_FIRST, drvdesc,
      sizeof (drvdesc) / sizeof (SQLTCHAR), &len, drvattrs,
      sizeof (drvattrs) / sizeof (SQLTCHAR), &len1);
  if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO
      && ret != SQL_NO_DATA)
    {
      _iodbcdm_nativeerrorbox (dlg, henv, SQL_NULL_HANDLE, SQL_NULL_HANDLE);
      goto error;
    }

  while (ret != SQL_NO_DATA)
    {
      data[0] = drvdesc;

      /* Get the driver library name */
      SQLSetConfigMode (ODBC_BOTH_DSN);
      SQLGetPrivateProfileString (drvdesc, "Driver", "", driver,
	  sizeof (driver) / sizeof (SQLTCHAR), "odbcinst.ini");
      if (driver[0] == '\0')
	SQLGetPrivateProfileString ("Default", "Driver", "", driver,
	    sizeof (driver) / sizeof (SQLTCHAR), "odbcinst.ini");
      if (driver[0] == '\0')
	{
	  data[0] = NULL;
	  goto skip;
	}

      data[1] = driver;

      drv_hdbc = NULL;
      drv_henv = NULL;

      if ((handle = (void *) DLL_OPEN (driver)) != NULL)
	{
	  if ((allocHdl =
		  (pSQLAllocHandle) DLL_PROC (handle,
		      "SQLAllocHandle")) != NULL)
	    {
	      ret = allocHdl (SQL_HANDLE_ENV, SQL_NULL_HANDLE, &drv_henv);
	      if (ret == SQL_ERROR)
		goto nodriverver;
	      ret = allocHdl (SQL_HANDLE_DBC, drv_henv, &drv_hdbc);
	      if (ret == SQL_ERROR)
		goto nodriverver;
	    }
	  else
	    {
	      if ((allocEnvHdl =
		      (pSQLAllocEnv) DLL_PROC (handle,
			  "SQLAllocEnv")) != NULL)
		{
		  ret = allocEnvHdl (&drv_henv);
		  if (ret == SQL_ERROR)
		    goto nodriverver;
		}
	      else
		goto nodriverver;

	      if ((allocConnectHdl =
		      (pSQLAllocConnect) DLL_PROC (handle,
			  "SQLAllocConnect")) != NULL)
		{
		  ret = allocConnectHdl (drv_henv, &drv_hdbc);
		  if (ret == SQL_ERROR)
		    goto nodriverver;
		}
	      else
		goto nodriverver;
	    }

	  if ((funcHdl =
		  (pSQLGetInfoFunc) DLL_PROC (handle, "SQLGetInfo")) != NULL)
	    {
	      /* Retrieve some information */
	      ret =
		  funcHdl (drv_hdbc, SQL_DRIVER_VER, drvattrs,
		  sizeof (drvattrs), &len);
	      if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
		{
		  unsigned int z;
		  /* Drop the description if one provided */
		  for (z = 0; ((char *) drvattrs)[z]; z++)
		    if (((char *) drvattrs)[z] == ' ')
		      ((char *) drvattrs)[z] = '\0';
		  data[2] = drvattrs;
		}
	      else
		goto nodriverver;
	    }
	  else
	    goto nodriverver;
	}
      else
	{
	nodriverver:
	  data[2] = "##.##";
	}

      if (drv_hdbc || drv_henv)
	{
	  if (allocConnectHdl &&
	      (freeConnectHdl =
		  (pSQLFreeConnect) DLL_PROC (handle,
		      "SQLFreeConnect")) != NULL)
	    {
	      freeConnectHdl (drv_hdbc);
	      drv_hdbc = NULL;
	    }

	  if (allocEnvHdl &&
	      (freeEnvHdl =
		  (pSQLFreeEnv) DLL_PROC (handle, "SQLFreeEnv")) != NULL)
	    {
	      freeEnvHdl (drv_henv);
	      drv_henv = NULL;
	    }
	}

      if ((drv_hdbc || drv_henv) &&
	  (freeHdl =
	      (pSQLFreeHandle) DLL_PROC (handle, "SQLFreeHandle")) != NULL)
	{
	  if (drv_hdbc)
	    freeHdl (SQL_HANDLE_DBC, drv_hdbc);
	  if (drv_henv)
	    freeHdl (SQL_HANDLE_ENV, drv_henv);
	}

      if (handle)
        DLL_CLOSE (handle);

      /* Get the size of the driver */
      if (!stat (driver, &_stat))
	{
	  sprintf (size, "%d Kb", (int) (_stat.st_size / 1024));
	  data[3] = size;
	}
      else
	data[3] = "-";

      gtk_clist_append (GTK_CLIST (widget), (gchar**)data);

    skip:
      ret = SQLDrivers (henv, SQL_FETCH_NEXT, drvdesc,
	  sizeof (drvdesc) / sizeof (SQLTCHAR), &len, drvattrs,
	  sizeof (drvattrs) / sizeof (SQLTCHAR), &len1);
      if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO
	  && ret != SQL_NO_DATA)
	{
	  _iodbcdm_nativeerrorbox (dlg, henv, SQL_NULL_HANDLE,
	      SQL_NULL_HANDLE);
	  goto error;
	}
    }

error:
  /* Clean all that */
  SQLFreeHandle (SQL_HANDLE_ENV, henv);

end:
  if (GTK_CLIST (widget)->rows > 0)
    {
      gtk_clist_columns_autosize (GTK_CLIST (widget));
      gtk_clist_sort (GTK_CLIST (widget));
    }
}


static void
driver_list_select (GtkWidget *widget, gint row, gint column,
    GdkEvent *event, TDRIVERCHOOSER *choose_t)
{
  LPSTR driver = NULL;

  if (choose_t)
    {
      /* Get the directory name */
      gtk_clist_get_text (GTK_CLIST (choose_t->driverlist), row, 0, &driver);

      if (driver && event && event->type == GDK_2BUTTON_PRESS)
	gtk_signal_emit_by_name (GTK_OBJECT (choose_t->b_finish), "clicked",
	    choose_t);
    }
}


static void
driverchooser_ok_clicked (GtkWidget *widget, TDRIVERCHOOSER *choose_t)
{
  char *szDriver;

  if (choose_t)
    {
      if (GTK_CLIST (choose_t->driverlist)->selection != NULL)
	{
	  gtk_clist_get_text (GTK_CLIST (choose_t->driverlist),
	      GPOINTER_TO_INT (GTK_CLIST (choose_t->driverlist)->selection->
		  data), 0, &szDriver);
	  choose_t->driver = dm_SQL_A2W(szDriver, SQL_NTS);
	}
      else
	choose_t->driver = NULL;

      choose_t->driverlist = NULL;

      gtk_signal_disconnect_by_func (GTK_OBJECT (choose_t->mainwnd),
	  GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
      gtk_main_quit ();
      gtk_widget_destroy (choose_t->mainwnd);
    }
}


static void
driverchooser_cancel_clicked (GtkWidget *widget, TDRIVERCHOOSER *choose_t)
{
  if (choose_t)
    {
      choose_t->driverlist = NULL;
      choose_t->driver = NULL;

      gtk_signal_disconnect_by_func (GTK_OBJECT (choose_t->mainwnd),
	  GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
      gtk_main_quit ();
      gtk_widget_destroy (choose_t->mainwnd);
    }
}


static gint
delete_event (GtkWidget *widget, GdkEvent *event, TDRIVERCHOOSER *choose_t)
{
  driverchooser_cancel_clicked (widget, choose_t);

  return FALSE;
}


void
create_driverchooser (HWND hwnd, TDRIVERCHOOSER *choose_t)
{
  GdkPixmap *pixmap;
  GdkBitmap *mask;
  GtkStyle *style;

  GtkWidget *driverchooser;
  GtkWidget *dialog_vbox2;
  GtkWidget *hbox25;
  GtkWidget *frame37;
  GtkWidget *alignment29;
  GtkWidget *pixmap1;
  GtkWidget *frame36;
  GtkWidget *alignment28;
  GtkWidget *scrolledwindow10;
  GtkWidget *clist1;
  GtkWidget *l_name;
  GtkWidget *l_file;
  GtkWidget *l_date;
  GtkWidget *l_size;
  GtkWidget *l_diz;
  GtkWidget *dialog_action_area2;
  GtkWidget *b_finish;
  GtkWidget *b_cancel;

  if (hwnd == NULL || !GTK_IS_WIDGET (hwnd))
    return;

  driverchooser = gtk_dialog_new ();
  gtk_widget_set_name (driverchooser, "driverchooser");
  gtk_widget_set_size_request (driverchooser, 515, 335);
  gtk_window_set_title (GTK_WINDOW (driverchooser), _("Choose an ODBC Driver"));
  gtk_window_set_position (GTK_WINDOW (driverchooser), GTK_WIN_POS_CENTER);
  gtk_window_set_modal (GTK_WINDOW (driverchooser), TRUE);
  gtk_window_set_default_size (GTK_WINDOW (driverchooser), 600, 450);
  gtk_window_set_type_hint (GTK_WINDOW (driverchooser), GDK_WINDOW_TYPE_HINT_DIALOG);

#if GTK_CHECK_VERSION(2,0,0)
  gtk_widget_show (driverchooser);
#endif

  dialog_vbox2 = GTK_DIALOG (driverchooser)->vbox;
  gtk_widget_set_name (dialog_vbox2, "dialog_vbox2");
  gtk_widget_show (dialog_vbox2);

  hbox25 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox25, "hbox25");
  gtk_widget_show (hbox25);
  gtk_box_pack_start (GTK_BOX (dialog_vbox2), hbox25, TRUE, TRUE, 0);

  frame37 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame37, "frame37");
  gtk_widget_show (frame37);
  gtk_box_pack_start (GTK_BOX (hbox25), frame37, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame37), 10);
  gtk_frame_set_shadow_type (GTK_FRAME (frame37), GTK_SHADOW_NONE);

  alignment29 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment29, "alignment29");
  gtk_widget_show (alignment29);
  gtk_container_add (GTK_CONTAINER (frame37), alignment29);
  gtk_widget_set_size_request (alignment29, 140, -1);

#if GTK_CHECK_VERSION(2,0,0)
  style = gtk_widget_get_style (driverchooser);
  pixmap =
      gdk_pixmap_create_from_xpm_d (driverchooser->window, &mask,
      &style->bg[GTK_STATE_NORMAL], (gchar **) img_xpm);
#else
  style = gtk_widget_get_style (GTK_WIDGET (hwnd));
  pixmap =
      gdk_pixmap_create_from_xpm_d (GTK_WIDGET (hwnd)->window, &mask,
      &style->bg[GTK_STATE_NORMAL], (gchar **) img_xpm);
#endif

  pixmap1 = gtk_pixmap_new (pixmap, mask);
  gtk_widget_set_name (pixmap1, "pixmap1");
  gtk_widget_show (pixmap1);
  gtk_container_add (GTK_CONTAINER (alignment29), pixmap1);

  frame36 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame36, "frame36");
  gtk_widget_show (frame36);
  gtk_box_pack_start (GTK_BOX (hbox25), frame36, TRUE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame36), GTK_SHADOW_NONE);

  alignment28 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment28, "alignment28");
  gtk_widget_show (alignment28);
  gtk_container_add (GTK_CONTAINER (frame36), alignment28);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment28), 0, 10, 0, 0);

  scrolledwindow10 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_name (scrolledwindow10, "scrolledwindow10");
  gtk_widget_show (scrolledwindow10);
  gtk_container_add (GTK_CONTAINER (alignment28), scrolledwindow10);

  clist1 = gtk_clist_new (4);
  gtk_widget_set_name (clist1, "clist1");
  gtk_widget_show (clist1);
  gtk_container_add (GTK_CONTAINER (scrolledwindow10), clist1);
  gtk_clist_set_column_width (GTK_CLIST (clist1), 0, 165);
  gtk_clist_set_column_width (GTK_CLIST (clist1), 1, 118);
  gtk_clist_set_column_width (GTK_CLIST (clist1), 2, 80);
  gtk_clist_set_column_width (GTK_CLIST (clist1), 3, 80);
  gtk_clist_column_titles_show (GTK_CLIST (clist1));

  l_name = gtk_label_new (_("Name"));
  gtk_widget_set_name (l_name, "l_name");
  gtk_widget_show (l_name);
  gtk_clist_set_column_widget (GTK_CLIST (clist1), 0, l_name);
  gtk_widget_set_size_request (l_name, 165, -1);

  l_file = gtk_label_new (_("File"));
  gtk_widget_set_name (l_file, "l_file");
  gtk_widget_show (l_file);
  gtk_clist_set_column_widget (GTK_CLIST (clist1), 1, l_file);
  gtk_widget_set_size_request (l_file, 118, -1);

  l_date = gtk_label_new (_("Date"));
  gtk_widget_set_name (l_date, "l_date");
  gtk_widget_show (l_date);
  gtk_clist_set_column_widget (GTK_CLIST (clist1), 2, l_date);
  gtk_widget_set_size_request (l_date, 80, -1);

  l_size = gtk_label_new (_("Size"));
  gtk_widget_set_name (l_size, "l_size");
  gtk_widget_show (l_size);
  gtk_clist_set_column_widget (GTK_CLIST (clist1), 3, l_size);
  gtk_widget_set_size_request (l_size, 80, -1);

  l_diz = gtk_label_new (_("Select a driver for which you want to setup a data source"));
  gtk_widget_set_name (l_diz, "l_diz");
  gtk_widget_show (l_diz);
  gtk_frame_set_label_widget (GTK_FRAME (frame36), l_diz);
  gtk_label_set_use_markup (GTK_LABEL (l_diz), TRUE);

  dialog_action_area2 = GTK_DIALOG (driverchooser)->action_area;
  gtk_widget_set_name (dialog_action_area2, "dialog_action_area2");
  gtk_widget_show (dialog_action_area2);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area2), GTK_BUTTONBOX_END);

  b_finish = gtk_button_new_with_mnemonic (_("_Finish"));
  gtk_widget_set_name (b_finish, "b_finish");
  gtk_widget_show (b_finish);
  gtk_dialog_add_action_widget (GTK_DIALOG (driverchooser), b_finish, 0);
  GTK_WIDGET_SET_FLAGS (b_finish, GTK_CAN_DEFAULT);

  b_cancel = gtk_button_new_from_stock ("gtk-cancel");
  gtk_widget_set_name (b_cancel, "b_cancel");
  gtk_widget_show (b_cancel);
  gtk_dialog_add_action_widget (GTK_DIALOG (driverchooser), b_cancel, GTK_RESPONSE_CANCEL);
  GTK_WIDGET_SET_FLAGS (b_cancel, GTK_CAN_DEFAULT);

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF (driverchooser, driverchooser, "driverchooser");
  GLADE_HOOKUP_OBJECT_NO_REF (driverchooser, dialog_vbox2, "dialog_vbox2");
  GLADE_HOOKUP_OBJECT (driverchooser, hbox25, "hbox25");
  GLADE_HOOKUP_OBJECT (driverchooser, frame37, "frame37");
  GLADE_HOOKUP_OBJECT (driverchooser, alignment29, "alignment29");
  GLADE_HOOKUP_OBJECT (driverchooser, pixmap1, "pixmap1");
  GLADE_HOOKUP_OBJECT (driverchooser, frame36, "frame36");
  GLADE_HOOKUP_OBJECT (driverchooser, alignment28, "alignment28");
  GLADE_HOOKUP_OBJECT (driverchooser, scrolledwindow10, "scrolledwindow10");
  GLADE_HOOKUP_OBJECT (driverchooser, clist1, "clist1");
  GLADE_HOOKUP_OBJECT (driverchooser, l_name, "l_name");
  GLADE_HOOKUP_OBJECT (driverchooser, l_file, "l_file");
  GLADE_HOOKUP_OBJECT (driverchooser, l_date, "l_date");
  GLADE_HOOKUP_OBJECT (driverchooser, l_size, "l_size");
  GLADE_HOOKUP_OBJECT (driverchooser, l_diz, "l_diz");
  GLADE_HOOKUP_OBJECT_NO_REF (driverchooser, dialog_action_area2, "dialog_action_area2");
  GLADE_HOOKUP_OBJECT (driverchooser, b_finish, "b_finish");
  GLADE_HOOKUP_OBJECT (driverchooser, b_cancel, "b_cancel");

  /* Ok button events */
  gtk_signal_connect (GTK_OBJECT (b_finish), "clicked",
      GTK_SIGNAL_FUNC (driverchooser_ok_clicked), choose_t);
  /* Cancel button events */
  gtk_signal_connect (GTK_OBJECT (b_cancel), "clicked",
      GTK_SIGNAL_FUNC (driverchooser_cancel_clicked), choose_t);
  /* Close window button events */
  gtk_signal_connect (GTK_OBJECT (driverchooser), "delete_event",
      GTK_SIGNAL_FUNC (delete_event), choose_t);
  gtk_signal_connect (GTK_OBJECT (driverchooser), "destroy",
      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
  /* Driver list events */
  gtk_signal_connect (GTK_OBJECT (clist1), "select_row",
      GTK_SIGNAL_FUNC (driver_list_select), choose_t);

  adddrivers_to_list (clist1, driverchooser);

  choose_t->driverlist = clist1;
  choose_t->driver = NULL;
  choose_t->mainwnd = driverchooser;
  choose_t->b_finish = b_finish;

  gtk_widget_show_all (driverchooser);
  gtk_main ();
}



static void
fdriverchooser_switch_page (GtkNotebook * notebook, GtkNotebookPage * page,
    gint page_num, TFDRIVERCHOOSER * choose_t)
{
  guint len;
  char  buff[1024];
  char *dsn = {""};
  char *drv = {""};
  char *curr;

  if (choose_t)
    {
      switch (page_num)
	{
	case 0:
	  if (choose_t->b_back)
	    gtk_widget_set_sensitive (choose_t->b_back, FALSE);
	  if (choose_t->b_continue)
            gtk_label_parse_uline(GTK_LABEL(GTK_BIN (choose_t->b_continue)->child),
               "Continue");
	  break;

	case 1:
	  if (choose_t->driverlist && choose_t->tab_panel && GTK_CLIST (choose_t->driverlist)->selection == NULL)
	    {
	      _iodbcdm_messagebox(choose_t->mainwnd, NULL, "Driver wasn't selected!");
              gtk_notebook_set_page (GTK_NOTEBOOK (choose_t->tab_panel), 0);
              break;
	    }
	  if (choose_t->b_back)
	    gtk_widget_set_sensitive (choose_t->b_back, TRUE);
	  if (choose_t->b_continue)
            gtk_label_parse_uline(GTK_LABEL(GTK_BIN (choose_t->b_continue)->child),
               "Co_ntinue");
	  break;
	case 2:
	  if (choose_t->driverlist && choose_t->tab_panel && choose_t->dsn_entry)
	    {
              if (GTK_CLIST (choose_t->driverlist)->selection != NULL)
                {
	          gtk_clist_get_text (GTK_CLIST (choose_t->driverlist),
	            GPOINTER_TO_INT(GTK_CLIST(choose_t->driverlist)->selection->data),
	            0, &drv);
	        }
	      else
	        {
	          _iodbcdm_messagebox(choose_t->mainwnd, NULL, "Driver wasn't selected!");
                  gtk_notebook_set_page (GTK_NOTEBOOK (choose_t->tab_panel), 0);
                  break;
	        }

              dsn = (char*)gtk_entry_get_text(GTK_ENTRY(choose_t->dsn_entry));
              if (strlen(dsn) < 1)
                {
	          _iodbcdm_messagebox(choose_t->mainwnd, NULL, "Enter File DSN Name...");
                  gtk_notebook_set_page (GTK_NOTEBOOK (choose_t->tab_panel), 1);
                  break;
                }
            }

	  if (choose_t->b_back)
	    gtk_widget_set_sensitive (choose_t->b_back, TRUE);
	  if (choose_t->b_continue)
            gtk_label_parse_uline(GTK_LABEL(GTK_BIN (choose_t->b_continue)->child),
                 "_Finish");

          if (choose_t->mess_entry)
            {
#if GTK_CHECK_VERSION(2,0,0)
              GtkTextBuffer *gbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(choose_t->mess_entry));
              GtkTextIter *iter;

              gtk_text_buffer_set_text(gbuf, "", 0);

              if (strchr(dsn, '/') != NULL)
                snprintf(buff, sizeof(buff), "Filename: %s\n", dsn);
              else
                snprintf(buff, sizeof(buff), "Filename: %s/%s\n", choose_t->curr_dir, dsn);

              gtk_text_buffer_insert_at_cursor(gbuf, buff, -1);

              snprintf(buff, sizeof(buff), "Driver: %s\n", drv);
              gtk_text_buffer_insert_at_cursor(gbuf, buff, -1);

              gtk_text_buffer_insert_at_cursor(gbuf, "Driver-specific Keywords:\n", -1);

              if (choose_t->attrs)
                {
                  for (curr = choose_t->attrs; *curr; curr += (STRLEN (curr) + 1))
                    {
                      if (!strncasecmp (curr, "PWD=", STRLEN ("PWD=")))
	                continue;

                      if (curr)
                        gtk_text_buffer_insert_at_cursor(gbuf, curr, -1);

                      gtk_text_buffer_insert_at_cursor(gbuf, "\n", -1);
                    }
                }
#else
              gtk_text_set_point(GTK_TEXT(choose_t->mess_entry), 0);
              len = gtk_text_get_length(GTK_TEXT(choose_t->mess_entry));
              gtk_text_forward_delete(GTK_TEXT(choose_t->mess_entry), len);
              gtk_text_insert(GTK_TEXT(choose_t->mess_entry), NULL, NULL, NULL, "File Data Source\n", -1);

              if (strchr(dsn, '/') != NULL)
                snprintf(buff, sizeof(buff), "Filename: %s\n", dsn);
              else
                snprintf(buff, sizeof(buff), "Filename: %s/%s\n", choose_t->curr_dir, dsn);
              gtk_text_insert(GTK_TEXT(choose_t->mess_entry), NULL, NULL, NULL, buff, -1);

              snprintf(buff, sizeof(buff), "Driver: %s\n", drv);
              gtk_text_insert(GTK_TEXT(choose_t->mess_entry), NULL, NULL, NULL, buff, -1);

              gtk_text_insert(GTK_TEXT(choose_t->mess_entry), NULL, NULL, NULL, "Driver-specific Keywords:\n", -1);

              if (choose_t->attrs)
                {
                  for (curr = choose_t->attrs; *curr; curr += (STRLEN (curr) + 1))
                    {
                      if (!strncasecmp (curr, "PWD=", STRLEN ("PWD=")))
                        {
	                  continue;
   	                }
                      gtk_text_insert(GTK_TEXT(choose_t->mess_entry), NULL, NULL, NULL, curr, -1);
                      gtk_text_insert(GTK_TEXT(choose_t->mess_entry), NULL, NULL, NULL, "\n", -1);
                    }
                }
#endif
            }
	  break;
	}
    }
}


static void
fdriverchooser_finish_clicked (GtkWidget *widget, TFDRIVERCHOOSER *choose_t)
{
  char *szDriver;
  char *dsn;
  char buff[1024];

  if (choose_t)
    {
      if (GTK_CLIST (choose_t->driverlist)->selection != NULL)
	{
	  gtk_clist_get_text (GTK_CLIST (choose_t->driverlist),
	      GPOINTER_TO_INT (GTK_CLIST (choose_t->driverlist)->selection->
		  data), 0, &szDriver);
	  choose_t->driver = dm_SQL_A2W(szDriver, SQL_NTS);
	}
      else
	choose_t->driver = NULL;

      dsn = (char*)gtk_entry_get_text((GtkEntry*)choose_t->dsn_entry);
      if (strchr(dsn, '/') != NULL)
        snprintf(buff, sizeof(buff), "%s", dsn);
      else
        snprintf(buff, sizeof(buff), "%s/%s", choose_t->curr_dir, dsn);

      choose_t->dsn = strdup(buff);
      choose_t->driverlist = NULL;
      choose_t->dsn_entry = NULL;
      choose_t->b_back = NULL;
      choose_t->b_continue = NULL;
      choose_t->mess_entry = NULL;
      choose_t->tab_panel = NULL;
      choose_t->browse_sel = NULL;

      choose_t->ok = (choose_t->driver ? TRUE : FALSE);

      gtk_signal_disconnect_by_func (GTK_OBJECT (choose_t->mainwnd),
	  GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
      gtk_main_quit ();
      gtk_widget_destroy (choose_t->mainwnd);
    }
}


static void
fdriverchooser_cancel_clicked (GtkWidget *widget, TFDRIVERCHOOSER *choose_t)
{
  if (choose_t)
    {
      choose_t->driverlist = NULL;
      choose_t->driver = NULL;
      choose_t->ok = FALSE;
      choose_t->driverlist = NULL;
      choose_t->dsn_entry = NULL;
      choose_t->b_back = NULL;
      choose_t->b_continue = NULL;
      choose_t->mess_entry = NULL;
      choose_t->tab_panel = NULL;
      choose_t->browse_sel = NULL;

      gtk_signal_disconnect_by_func (GTK_OBJECT (choose_t->mainwnd),
	  GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
      gtk_main_quit ();
      gtk_widget_destroy (choose_t->mainwnd);
    }
}


static void
fdriverchooser_next_clicked (GtkWidget * widget, TFDRIVERCHOOSER * choose_t)
{
  if (choose_t)
    {
      gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK (choose_t->tab_panel));
      if (page == 2) /* Last Page */
        fdriverchooser_finish_clicked (widget, choose_t);
      else
        gtk_notebook_next_page (GTK_NOTEBOOK (choose_t->tab_panel));
    }
}

static void
fdriverchooser_prev_clicked (GtkWidget * widget, TFDRIVERCHOOSER * choose_t)
{
  if (choose_t)
    {
      gtk_notebook_prev_page (GTK_NOTEBOOK (choose_t->tab_panel));
    }
}


static gint
fdelete_event (GtkWidget *widget, GdkEvent *event, TFDRIVERCHOOSER *choose_t)
{
  fdriverchooser_cancel_clicked (widget, choose_t);

  return FALSE;
}


static void
fdriver_list_select (GtkWidget *widget, gint row, gint column,
    GdkEvent *event, TFDRIVERCHOOSER *choose_t)
{
  LPSTR driver = NULL;

  if (choose_t)
    {
      /* Get the directory name */
      gtk_clist_get_text (GTK_CLIST (choose_t->driverlist), row, 0, &driver);

      if (driver && event && event->type == GDK_2BUTTON_PRESS)
	gtk_signal_emit_by_name (GTK_OBJECT (choose_t->b_continue), "clicked",
	    choose_t);
    }
}


static void
fdsn_choosen(GtkWidget *widget, TFDRIVERCHOOSER *choose_t)
{
  if (choose_t)
    {
      gtk_entry_set_text (GTK_ENTRY (choose_t->dsn_entry),
	  gtk_file_selection_get_filename (GTK_FILE_SELECTION (choose_t->
		  browse_sel)));
      choose_t->browse_sel = NULL;
    }
}


static void
fdriverchooser_browse_clicked (GtkWidget * widget, TFDRIVERCHOOSER * choose_t)
{
  GtkWidget *filesel;
  char *dsn;
  char buff[1024];

  if (choose_t)
    {
      filesel = gtk_file_selection_new ("Save as ...");
      gtk_window_set_modal (GTK_WINDOW (filesel), TRUE);

      dsn = (char*)gtk_entry_get_text((GtkEntry*)choose_t->dsn_entry);
      if (strchr(dsn, '/') != NULL)
        snprintf(buff, sizeof(buff), "%s", dsn);
      else
        snprintf(buff, sizeof(buff), "%s/%s", choose_t->curr_dir, dsn);

      gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel), buff);
      /* Ok button events */
      gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button), 
          "clicked", GTK_SIGNAL_FUNC (fdsn_choosen), choose_t);
      gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button), 
          "clicked", GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
      /* Cancel button events */
      gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button), 
          "clicked", GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
      /* Close window button events */
      gtk_signal_connect (GTK_OBJECT (filesel), "delete_event",
	  GTK_SIGNAL_FUNC (gtk_main_quit), NULL);

      choose_t->browse_sel = filesel;
      gtk_widget_show_all (filesel);
      gtk_main ();
      gtk_widget_destroy (filesel);

      choose_t->browse_sel = NULL;
    }
}


static void
fdriverchooser_advanced_clicked (GtkWidget * widget, TFDRIVERCHOOSER * choose_t)
{
  if (choose_t)
    {
      LPSTR attr_lst = NULL;
      LPSTR in_attrs = choose_t->attrs ? choose_t->attrs : "\0\0";

      attr_lst = create_keyval (choose_t->mainwnd, in_attrs, &choose_t->verify_conn);
      if (attr_lst && attr_lst != (LPSTR)-1L)
        {
          if (choose_t->attrs)
            free(choose_t->attrs);
          choose_t->attrs = attr_lst;
        }
    }
}


//??DONE
void
create_fdriverchooser (HWND hwnd, TFDRIVERCHOOSER *choose_t)
{
  GdkPixmap *pixmap;
  GdkBitmap *mask;
  GtkStyle *style;

  GtkWidget *fdriverchooser;
  GtkWidget *dialog_vbox3;
  GtkWidget *notebook2;
  GtkWidget *hbox26;
  GtkWidget *frame38;
  GtkWidget *alignment30;
  GtkWidget *pixmap1;
  GtkWidget *vbox21;
  GtkWidget *frame39;
  GtkWidget *alignment31;
  GtkWidget *scrolledwindow11;
  GtkWidget *clist2;
  GtkWidget *l_name;
  GtkWidget *l_file;
  GtkWidget *l_date;
  GtkWidget *l_size;
  GtkWidget *label70;
  GtkWidget *hbox27;
  GtkWidget *frame40;
  GtkWidget *alignment32;
  GtkWidget *b_advanced;
  GtkWidget *label67;
  GtkWidget *hbox28;
  GtkWidget *frame41;
  GtkWidget *alignment33;
  GtkWidget *pixmap2;
  GtkWidget *vbox22;
  GtkWidget *frame42;
  GtkWidget *alignment34;
  GtkWidget *label79;
  GtkWidget *frame43;
  GtkWidget *alignment35;
  GtkWidget *hbox30;
  GtkWidget *fdsn_entry;
  GtkWidget *b_browse;
  GtkWidget *frame47;
  GtkWidget *alignment39;
  GtkWidget *label68;
  GtkWidget *hbox29;
  GtkWidget *frame44;
  GtkWidget *alignment36;
  GtkWidget *pixmap3;
  GtkWidget *vbox23;
  GtkWidget *frame45;
  GtkWidget *alignment37;
  GtkWidget *label80;
  GtkWidget *frame46;
  GtkWidget *alignment38;
  GtkWidget *scrolledwindow13;
  GtkWidget *result_text;
  GtkWidget *label69;
  GtkWidget *dialog_action_area3;
  GtkWidget *b_cancel;
  GtkWidget *b_back;
  GtkWidget *b_continue;

  if (hwnd == NULL || !GTK_IS_WIDGET (hwnd))
    return;

  fdriverchooser = gtk_dialog_new ();
  gtk_widget_set_name (fdriverchooser, "fdriverchooser");
  gtk_widget_set_size_request (fdriverchooser, 512, 384);
  gtk_window_set_title (GTK_WINDOW (fdriverchooser), _("Create New File Data Source"));
  gtk_window_set_modal (GTK_WINDOW (fdriverchooser), TRUE);
  gtk_window_set_default_size (GTK_WINDOW (fdriverchooser), 600, 450);
  gtk_window_set_position (GTK_WINDOW (fdriverchooser), GTK_WIN_POS_CENTER);
  gtk_window_set_type_hint (GTK_WINDOW (fdriverchooser), GDK_WINDOW_TYPE_HINT_DIALOG);

#if GTK_CHECK_VERSION(2,0,0)
  gtk_widget_show (fdriverchooser);
#endif

  dialog_vbox3 = GTK_DIALOG (fdriverchooser)->vbox;
  gtk_widget_set_name (dialog_vbox3, "dialog_vbox3");
  gtk_widget_show (dialog_vbox3);

  notebook2 = gtk_notebook_new ();
  gtk_widget_set_name (notebook2, "notebook2");
  gtk_widget_show (notebook2);
  gtk_box_pack_start (GTK_BOX (dialog_vbox3), notebook2, TRUE, TRUE, 0);

  hbox26 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox26, "hbox26");
  gtk_widget_show (hbox26);
  gtk_container_add (GTK_CONTAINER (notebook2), hbox26);

  frame38 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame38, "frame38");
  gtk_widget_show (frame38);
  gtk_box_pack_start (GTK_BOX (hbox26), frame38, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame38), 10);
  gtk_frame_set_shadow_type (GTK_FRAME (frame38), GTK_SHADOW_NONE);

  alignment30 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment30, "alignment30");
  gtk_widget_show (alignment30);
  gtk_container_add (GTK_CONTAINER (frame38), alignment30);
  gtk_widget_set_size_request (alignment30, 140, -1);

#if GTK_CHECK_VERSION(2,0,0)
  style = gtk_widget_get_style (fdriverchooser);
  pixmap =
      gdk_pixmap_create_from_xpm_d (fdriverchooser->window, &mask,
      &style->bg[GTK_STATE_NORMAL], (gchar **) img_xpm);
#else
  style = gtk_widget_get_style (GTK_WIDGET (hwnd));
  pixmap =
      gdk_pixmap_create_from_xpm_d (GTK_WIDGET (hwnd)->window, &mask,
      &style->bg[GTK_STATE_NORMAL], (gchar **) img_xpm);
#endif

  pixmap1 = gtk_pixmap_new (pixmap, mask);
  gtk_widget_set_name (pixmap1, "pixmap1");
  gtk_widget_show (pixmap1);
  gtk_container_add (GTK_CONTAINER (alignment30), pixmap1);

  vbox21 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox21, "vbox21");
  gtk_widget_show (vbox21);
  gtk_box_pack_start (GTK_BOX (hbox26), vbox21, TRUE, TRUE, 0);

  frame39 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame39, "frame39");
  gtk_widget_show (frame39);
  gtk_box_pack_start (GTK_BOX (vbox21), frame39, TRUE, TRUE, 0);
  gtk_widget_set_size_request (frame39, -1, 270);
  gtk_frame_set_shadow_type (GTK_FRAME (frame39), GTK_SHADOW_NONE);

  alignment31 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment31, "alignment31");
  gtk_widget_show (alignment31);
  gtk_container_add (GTK_CONTAINER (frame39), alignment31);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment31), 0, 0, 4, 0);

  scrolledwindow11 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_name (scrolledwindow11, "scrolledwindow11");
  gtk_widget_show (scrolledwindow11);
  gtk_container_add (GTK_CONTAINER (alignment31), scrolledwindow11);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow11), GTK_SHADOW_IN);

  clist2 = gtk_clist_new (4);
  gtk_widget_set_name (clist2, "clist2");
  gtk_widget_show (clist2);
  gtk_container_add (GTK_CONTAINER (scrolledwindow11), clist2);
  gtk_clist_set_column_width (GTK_CLIST (clist2), 0, 165);
  gtk_clist_set_column_width (GTK_CLIST (clist2), 1, 118);
  gtk_clist_set_column_width (GTK_CLIST (clist2), 2, 80);
  gtk_clist_set_column_width (GTK_CLIST (clist2), 3, 80);
  gtk_clist_column_titles_show (GTK_CLIST (clist2));

  l_name = gtk_label_new (_("Name"));
  gtk_widget_set_name (l_name, "l_name");
  gtk_widget_show (l_name);
  gtk_clist_set_column_widget (GTK_CLIST (clist2), 0, l_name);
  gtk_widget_set_size_request (l_name, 165, -1);

  l_file = gtk_label_new (_("File"));
  gtk_widget_set_name (l_file, "l_file");
  gtk_widget_show (l_file);
  gtk_clist_set_column_widget (GTK_CLIST (clist2), 1, l_file);
  gtk_widget_set_size_request (l_file, 118, -1);

  l_date = gtk_label_new (_("Date"));
  gtk_widget_set_name (l_date, "l_date");
  gtk_widget_show (l_date);
  gtk_clist_set_column_widget (GTK_CLIST (clist2), 2, l_date);
  gtk_widget_set_size_request (l_date, 80, -1);

  l_size = gtk_label_new (_("Size"));
  gtk_widget_set_name (l_size, "l_size");
  gtk_widget_show (l_size);
  gtk_clist_set_column_widget (GTK_CLIST (clist2), 3, l_size);
  gtk_widget_set_size_request (l_size, 80, -1);

  label70 = gtk_label_new (_("Select a driver for which you want to setup a data source"));
  gtk_widget_set_name (label70, "label70");
  gtk_widget_show (label70);
  gtk_frame_set_label_widget (GTK_FRAME (frame39), label70);
  gtk_label_set_use_markup (GTK_LABEL (label70), TRUE);

  hbox27 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox27, "hbox27");
  gtk_widget_show (hbox27);
  gtk_box_pack_start (GTK_BOX (vbox21), hbox27, FALSE, TRUE, 0);

  frame40 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame40, "frame40");
  gtk_widget_show (frame40);
  gtk_box_pack_start (GTK_BOX (hbox27), frame40, TRUE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame40), GTK_SHADOW_NONE);

  alignment32 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment32, "alignment32");
  gtk_widget_show (alignment32);
  gtk_container_add (GTK_CONTAINER (frame40), alignment32);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment32), 0, 10, 0, 0);

  b_advanced = gtk_button_new_with_mnemonic (_("_Advanced..."));
  gtk_widget_set_name (b_advanced, "b_advanced");
  gtk_widget_show (b_advanced);
  gtk_box_pack_start (GTK_BOX (hbox27), b_advanced, FALSE, TRUE, 0);
  gtk_widget_set_size_request (b_advanced, -1, 45);
  gtk_container_set_border_width (GTK_CONTAINER (b_advanced), 8);

  label67 = gtk_label_new (_("   Drivers   "));
  gtk_widget_set_name (label67, "label67");
  gtk_widget_show (label67);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook2), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook2), 0), label67);

  hbox28 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox28, "hbox28");
  gtk_widget_show (hbox28);
  gtk_container_add (GTK_CONTAINER (notebook2), hbox28);

  frame41 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame41, "frame41");
  gtk_widget_show (frame41);
  gtk_box_pack_start (GTK_BOX (hbox28), frame41, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame41), 10);
  gtk_frame_set_shadow_type (GTK_FRAME (frame41), GTK_SHADOW_NONE);

  alignment33 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment33, "alignment33");
  gtk_widget_show (alignment33);
  gtk_container_add (GTK_CONTAINER (frame41), alignment33);
  gtk_widget_set_size_request (alignment33, 140, -1);

  pixmap2 = gtk_pixmap_new (pixmap, mask);
  gtk_widget_set_name (pixmap2, "pixmap2");
  gtk_widget_show (pixmap2);
  gtk_container_add (GTK_CONTAINER (alignment33), pixmap2);

  vbox22 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox22, "vbox22");
  gtk_widget_show (vbox22);
  gtk_box_pack_start (GTK_BOX (hbox28), vbox22, TRUE, TRUE, 0);

  frame42 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame42, "frame42");
  gtk_widget_show (frame42);
  gtk_box_pack_start (GTK_BOX (vbox22), frame42, FALSE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame42), GTK_SHADOW_NONE);

  alignment34 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment34, "alignment34");
  gtk_widget_show (alignment34);
  gtk_container_add (GTK_CONTAINER (frame42), alignment34);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment34), 0, 0, 4, 0);

  label79 = gtk_label_new (_("Type the name of the data source you want to\nsave this connection to. Or, find the location to\nsave to by clicking Browse."));
  gtk_widget_set_name (label79, "label79");
  gtk_widget_show (label79);
  gtk_container_add (GTK_CONTAINER (alignment34), label79);

  frame43 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame43, "frame43");
  gtk_widget_show (frame43);
  gtk_box_pack_start (GTK_BOX (vbox22), frame43, FALSE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame43), GTK_SHADOW_NONE);

  alignment35 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment35, "alignment35");
  gtk_widget_show (alignment35);
  gtk_container_add (GTK_CONTAINER (frame43), alignment35);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment35), 0, 0, 12, 0);

  hbox30 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox30, "hbox30");
  gtk_widget_show (hbox30);
  gtk_container_add (GTK_CONTAINER (alignment35), hbox30);

  fdsn_entry = gtk_entry_new ();
  gtk_widget_set_name (fdsn_entry, "fdsn_entry");
  gtk_widget_show (fdsn_entry);
  gtk_box_pack_start (GTK_BOX (hbox30), fdsn_entry, TRUE, TRUE, 0);

  b_browse = gtk_button_new_with_mnemonic (_("   Browse   "));
  gtk_widget_set_name (b_browse, "b_browse");
  gtk_widget_show (b_browse);
  gtk_box_pack_start (GTK_BOX (hbox30), b_browse, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (b_browse), 10);

  frame47 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame47, "frame47");
  gtk_widget_show (frame47);
  gtk_box_pack_start (GTK_BOX (vbox22), frame47, TRUE, TRUE, 0);
  gtk_widget_set_size_request (frame47, -1, 200);
  gtk_frame_set_shadow_type (GTK_FRAME (frame47), GTK_SHADOW_NONE);

  alignment39 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment39, "alignment39");
  gtk_widget_show (alignment39);
  gtk_container_add (GTK_CONTAINER (frame47), alignment39);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment39), 0, 0, 12, 0);

  label68 = gtk_label_new (_("   FileDSN Name   "));
  gtk_widget_set_name (label68, "label68");
  gtk_widget_show (label68);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook2), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook2), 1), label68);

  hbox29 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox29, "hbox29");
  gtk_widget_show (hbox29);
  gtk_container_add (GTK_CONTAINER (notebook2), hbox29);

  frame44 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame44, "frame44");
  gtk_widget_show (frame44);
  gtk_box_pack_start (GTK_BOX (hbox29), frame44, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame44), 10);
  gtk_frame_set_shadow_type (GTK_FRAME (frame44), GTK_SHADOW_NONE);

  alignment36 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment36, "alignment36");
  gtk_widget_show (alignment36);
  gtk_container_add (GTK_CONTAINER (frame44), alignment36);
  gtk_widget_set_size_request (alignment36, 140, -1);

  pixmap3 = gtk_pixmap_new (pixmap, mask);
  gtk_widget_set_name (pixmap3, "pixmap3");
  gtk_widget_show (pixmap3);
  gtk_container_add (GTK_CONTAINER (alignment36), pixmap3);

  vbox23 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox23, "vbox23");
  gtk_widget_show (vbox23);
  gtk_box_pack_start (GTK_BOX (hbox29), vbox23, TRUE, TRUE, 0);

  frame45 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame45, "frame45");
  gtk_widget_show (frame45);
  gtk_box_pack_start (GTK_BOX (vbox23), frame45, FALSE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame45), GTK_SHADOW_NONE);

  alignment37 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment37, "alignment37");
  gtk_widget_show (alignment37);
  gtk_container_add (GTK_CONTAINER (frame45), alignment37);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment37), 0, 0, 4, 0);

  label80 = gtk_label_new (_("When you click Finish, you will create the data\nsource which you have just configured. The driver\nmay prompt you more information."));
  gtk_widget_set_name (label80, "label80");
  gtk_widget_show (label80);
  gtk_container_add (GTK_CONTAINER (alignment37), label80);

  frame46 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame46, "frame46");
  gtk_widget_show (frame46);
  gtk_box_pack_start (GTK_BOX (vbox23), frame46, TRUE, TRUE, 0);
  gtk_widget_set_size_request (frame46, -1, 250);
  gtk_frame_set_shadow_type (GTK_FRAME (frame46), GTK_SHADOW_NONE);

  alignment38 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment38, "alignment38");
  gtk_widget_show (alignment38);
  gtk_container_add (GTK_CONTAINER (frame46), alignment38);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment38), 0, 10, 0, 0);

  scrolledwindow13 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_name (scrolledwindow13, "scrolledwindow13");
  gtk_widget_show (scrolledwindow13);
  gtk_container_add (GTK_CONTAINER (alignment38), scrolledwindow13);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow13), GTK_SHADOW_IN);

#if GTK_CHECK_VERSION(2,0,0)
  result_text = gtk_text_view_new ();
#else
  result_text = gtk_text_new (NULL, NULL);
#endif
  gtk_widget_set_name (result_text, "result_text");
  gtk_widget_show (result_text);
  gtk_container_add (GTK_CONTAINER (scrolledwindow13), result_text);
  gtk_text_view_set_editable (GTK_TEXT_VIEW (result_text), FALSE);
  gtk_text_view_set_overwrite (GTK_TEXT_VIEW (result_text), TRUE);

  label69 = gtk_label_new (_("   Results   "));
  gtk_widget_set_name (label69, "label69");
  gtk_widget_show (label69);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook2), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook2), 2), label69);

  dialog_action_area3 = GTK_DIALOG (fdriverchooser)->action_area;
  gtk_widget_set_name (dialog_action_area3, "dialog_action_area3");
  gtk_widget_show (dialog_action_area3);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area3), GTK_BUTTONBOX_END);

  b_cancel = gtk_button_new_from_stock ("gtk-cancel");
  gtk_widget_set_name (b_cancel, "b_cancel");
  gtk_widget_show (b_cancel);
  gtk_dialog_add_action_widget (GTK_DIALOG (fdriverchooser), b_cancel, GTK_RESPONSE_CANCEL);
  GTK_WIDGET_SET_FLAGS (b_cancel, GTK_CAN_DEFAULT);

  b_back = gtk_button_new_from_stock ("gtk-go-back");
  gtk_widget_set_name (b_back, "b_back");
  gtk_widget_show (b_back);
  gtk_dialog_add_action_widget (GTK_DIALOG (fdriverchooser), b_back, 0);
  gtk_widget_set_sensitive (b_back, FALSE);
  GTK_WIDGET_SET_FLAGS (b_back, GTK_CAN_DEFAULT);

  b_continue = gtk_button_new_with_mnemonic (_("Co_ntinue"));
  gtk_widget_set_name (b_continue, "b_continue");
  gtk_widget_show (b_continue);
  gtk_dialog_add_action_widget (GTK_DIALOG (fdriverchooser), b_continue, 0);
  GTK_WIDGET_SET_FLAGS (b_continue, GTK_CAN_DEFAULT);

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF (fdriverchooser, fdriverchooser, "fdriverchooser");
  GLADE_HOOKUP_OBJECT_NO_REF (fdriverchooser, dialog_vbox3, "dialog_vbox3");
  GLADE_HOOKUP_OBJECT (fdriverchooser, notebook2, "notebook2");
  GLADE_HOOKUP_OBJECT (fdriverchooser, hbox26, "hbox26");
  GLADE_HOOKUP_OBJECT (fdriverchooser, frame38, "frame38");
  GLADE_HOOKUP_OBJECT (fdriverchooser, alignment30, "alignment30");
  GLADE_HOOKUP_OBJECT (fdriverchooser, pixmap1, "pixmap1");
  GLADE_HOOKUP_OBJECT (fdriverchooser, vbox21, "vbox21");
  GLADE_HOOKUP_OBJECT (fdriverchooser, frame39, "frame39");
  GLADE_HOOKUP_OBJECT (fdriverchooser, alignment31, "alignment31");
  GLADE_HOOKUP_OBJECT (fdriverchooser, scrolledwindow11, "scrolledwindow11");
  GLADE_HOOKUP_OBJECT (fdriverchooser, clist2, "clist2");
  GLADE_HOOKUP_OBJECT (fdriverchooser, l_name, "l_name");
  GLADE_HOOKUP_OBJECT (fdriverchooser, l_file, "l_file");
  GLADE_HOOKUP_OBJECT (fdriverchooser, l_date, "l_date");
  GLADE_HOOKUP_OBJECT (fdriverchooser, l_size, "l_size");
  GLADE_HOOKUP_OBJECT (fdriverchooser, label70, "label70");
  GLADE_HOOKUP_OBJECT (fdriverchooser, hbox27, "hbox27");
  GLADE_HOOKUP_OBJECT (fdriverchooser, frame40, "frame40");
  GLADE_HOOKUP_OBJECT (fdriverchooser, alignment32, "alignment32");
  GLADE_HOOKUP_OBJECT (fdriverchooser, b_advanced, "b_advanced");
  GLADE_HOOKUP_OBJECT (fdriverchooser, label67, "label67");
  GLADE_HOOKUP_OBJECT (fdriverchooser, hbox28, "hbox28");
  GLADE_HOOKUP_OBJECT (fdriverchooser, frame41, "frame41");
  GLADE_HOOKUP_OBJECT (fdriverchooser, alignment33, "alignment33");
  GLADE_HOOKUP_OBJECT (fdriverchooser, pixmap2, "pixmap2");
  GLADE_HOOKUP_OBJECT (fdriverchooser, vbox22, "vbox22");
  GLADE_HOOKUP_OBJECT (fdriverchooser, frame42, "frame42");
  GLADE_HOOKUP_OBJECT (fdriverchooser, alignment34, "alignment34");
  GLADE_HOOKUP_OBJECT (fdriverchooser, label79, "label79");
  GLADE_HOOKUP_OBJECT (fdriverchooser, frame43, "frame43");
  GLADE_HOOKUP_OBJECT (fdriverchooser, alignment35, "alignment35");
  GLADE_HOOKUP_OBJECT (fdriverchooser, hbox30, "hbox30");
  GLADE_HOOKUP_OBJECT (fdriverchooser, fdsn_entry, "fdsn_entry");
  GLADE_HOOKUP_OBJECT (fdriverchooser, b_browse, "b_browse");
  GLADE_HOOKUP_OBJECT (fdriverchooser, frame47, "frame47");
  GLADE_HOOKUP_OBJECT (fdriverchooser, alignment39, "alignment39");
  GLADE_HOOKUP_OBJECT (fdriverchooser, label68, "label68");
  GLADE_HOOKUP_OBJECT (fdriverchooser, hbox29, "hbox29");
  GLADE_HOOKUP_OBJECT (fdriverchooser, frame44, "frame44");
  GLADE_HOOKUP_OBJECT (fdriverchooser, alignment36, "alignment36");
  GLADE_HOOKUP_OBJECT (fdriverchooser, pixmap3, "pixmap3");
  GLADE_HOOKUP_OBJECT (fdriverchooser, vbox23, "vbox23");
  GLADE_HOOKUP_OBJECT (fdriverchooser, frame45, "frame45");
  GLADE_HOOKUP_OBJECT (fdriverchooser, alignment37, "alignment37");
  GLADE_HOOKUP_OBJECT (fdriverchooser, label80, "label80");
  GLADE_HOOKUP_OBJECT (fdriverchooser, frame46, "frame46");
  GLADE_HOOKUP_OBJECT (fdriverchooser, alignment38, "alignment38");
  GLADE_HOOKUP_OBJECT (fdriverchooser, scrolledwindow13, "scrolledwindow13");
  GLADE_HOOKUP_OBJECT (fdriverchooser, result_text, "result_text");
  GLADE_HOOKUP_OBJECT (fdriverchooser, label69, "label69");
  GLADE_HOOKUP_OBJECT_NO_REF (fdriverchooser, dialog_action_area3, "dialog_action_area3");
  GLADE_HOOKUP_OBJECT (fdriverchooser, b_cancel, "b_cancel");
  GLADE_HOOKUP_OBJECT (fdriverchooser, b_back, "b_back");
  GLADE_HOOKUP_OBJECT (fdriverchooser, b_continue, "b_continue");

  /* Notebook events */
  gtk_signal_connect_after (GTK_OBJECT (notebook2), "switch_page",
      GTK_SIGNAL_FUNC (fdriverchooser_switch_page), choose_t);
  /* Cancel button events */
  gtk_signal_connect (GTK_OBJECT (b_cancel), "clicked",
      GTK_SIGNAL_FUNC (fdriverchooser_cancel_clicked), choose_t);
  /* Continue button events */
  gtk_signal_connect (GTK_OBJECT (b_continue), "clicked",
      GTK_SIGNAL_FUNC (fdriverchooser_next_clicked), choose_t);
  /* Back button events */
  gtk_signal_connect (GTK_OBJECT (b_back), "clicked",
      GTK_SIGNAL_FUNC (fdriverchooser_prev_clicked), choose_t);
  /* Browse button events */
  gtk_signal_connect (GTK_OBJECT (b_browse), "clicked",
      GTK_SIGNAL_FUNC (fdriverchooser_browse_clicked), choose_t);
  /* Advanced button events */
  gtk_signal_connect (GTK_OBJECT (b_advanced), "clicked",
      GTK_SIGNAL_FUNC (fdriverchooser_advanced_clicked), choose_t);
  /* Driver list events */
  gtk_signal_connect (GTK_OBJECT (clist2), "select_row",
      GTK_SIGNAL_FUNC (fdriver_list_select), choose_t);
  /* Close window button events */
  gtk_signal_connect (GTK_OBJECT (fdriverchooser), "delete_event",
      GTK_SIGNAL_FUNC (fdelete_event), choose_t);
  gtk_signal_connect (GTK_OBJECT (fdriverchooser), "destroy",
      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
  
  gtk_widget_grab_default (b_continue);

  adddrivers_to_list (clist2, fdriverchooser);

  choose_t->driverlist = clist2;
  choose_t->driver = NULL;
  choose_t->mainwnd = fdriverchooser;
  choose_t->b_continue = b_continue;
  choose_t->b_back = b_back;
  choose_t->tab_panel = notebook2;
  choose_t->dsn_entry = fdsn_entry;
  choose_t->mess_entry = result_text;

  gtk_widget_show_all (fdriverchooser);
  gtk_main ();

}
