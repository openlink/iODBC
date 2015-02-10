/*
 *  translator.c
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


#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include "gui.h"
#include "img.xpm"


static void
translator_list_select (GtkWidget *widget, gint row, gint column,
    GdkEvent *event, TTRANSLATORCHOOSER *choose_t)
{
  LPSTR translator = NULL;

  if (choose_t)
    {
      /* Get the directory name */
      gtk_clist_get_text (GTK_CLIST (choose_t->translatorlist), row, 0,
	  &translator);

      if (translator && event && event->type == GDK_2BUTTON_PRESS)
	gtk_signal_emit_by_name (GTK_OBJECT (choose_t->b_finish), "clicked",
	    choose_t);
    }
}


static void
translatorchooser_ok_clicked (GtkWidget *widget,
    TTRANSLATORCHOOSER *choose_t)
{
  char *szTranslator;

  if (choose_t)
    {
      if (GTK_CLIST (choose_t->translatorlist)->selection != NULL)
	{
	  gtk_clist_get_text (GTK_CLIST (choose_t->translatorlist),
	      GPOINTER_TO_INT (GTK_CLIST (choose_t->translatorlist)->
		  selection->data), 0, &szTranslator);
	  choose_t->translator = dm_SQL_A2W (szTranslator, SQL_NTS);
	}
      else
	choose_t->translator = NULL;

      choose_t->translatorlist = NULL;

      gtk_signal_disconnect_by_func (GTK_OBJECT (choose_t->mainwnd),
	  GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
      gtk_main_quit ();
      gtk_widget_destroy (choose_t->mainwnd);
    }
}


static void
translatorchooser_cancel_clicked (GtkWidget *widget,
    TTRANSLATORCHOOSER *choose_t)
{
  if (choose_t)
    {
      choose_t->translatorlist = NULL;

      gtk_signal_disconnect_by_func (GTK_OBJECT (choose_t->mainwnd),
	  GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
      gtk_main_quit ();
      gtk_widget_destroy (choose_t->mainwnd);
    }
}


static gint
delete_event (GtkWidget *widget,
    GdkEvent *event, TTRANSLATORCHOOSER *choose_t)
{
  translatorchooser_cancel_clicked (widget, choose_t);

  return FALSE;
}

void
addtranslators_to_list (GtkWidget *widget, GtkWidget *dlg)
{
  char *curr, *buffer = (char *) malloc (sizeof (char) * 65536), *szDriver;
  char driver[1024], _date[1024], _size[1024];
  char *data[4];
  int len, i;
  BOOL careabout;
  UWORD confMode = ODBC_USER_DSN;
  struct stat _stat;

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
	  SQLGetPrivateProfileString ("ODBC 32 bit Translators",
	  NULL, "", buffer, 65535, "odbcinst.ini");
#else
      len =
	  SQLGetPrivateProfileString ("ODBC Translators",
	  NULL, "", buffer, 65535, "odbcinst.ini");
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
	  SQLGetPrivateProfileString ("ODBC 32 bit Translators",
	      curr, "", driver, sizeof (driver), "odbcinst.ini");
#else
	  SQLGetPrivateProfileString ("ODBC Translators",
	      curr, "", driver, sizeof (driver), "odbcinst.ini");
#endif

	  /* Check if the driver is installed */
	  if (strcasecmp (driver, "Installed"))
	    goto end;

	  /* Get the driver library name */
	  SQLSetConfigMode (confMode);
	  if (!SQLGetPrivateProfileString (curr,
		  "Translator", "", driver, sizeof (driver), "odbcinst.ini"))
	    {
	      SQLSetConfigMode (confMode);
	      SQLGetPrivateProfileString ("Default",
		  "Translator", "", driver, sizeof (driver), "odbcinst.ini");
	    }

	  if (STRLEN (curr) && STRLEN (driver))
	    {
	      data[0] = curr;
	      data[1] = driver;

	      /* Get some information about the driver */
	      if (!stat (driver, &_stat))
		{
		  sprintf (_size, "%lu Kb",
		      (unsigned long) _stat.st_size / 1024L);
		  sprintf (_date, "%s", ctime (&_stat.st_mtime));
		  data[2] = _date;
		  data[3] = _size;
		  gtk_clist_append (GTK_CLIST (widget), data);
		}
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


void
create_translatorchooser (HWND hwnd, TTRANSLATORCHOOSER *choose_t)
{
  GdkPixmap *pixmap;
  GdkBitmap *mask;
  GtkStyle *style;

  GtkWidget *translatorchooser;
  GtkWidget *vbox41;
  GtkWidget *hbox57;
  GtkWidget *frame97;
  GtkWidget *alignment81;
  GtkWidget *pixmap1;
  GtkWidget *frame98;
  GtkWidget *alignment82;
  GtkWidget *scrolledwindow21;
  GtkWidget *clist1;
  GtkWidget *label158;
  GtkWidget *label159;
  GtkWidget *label160;
  GtkWidget *label161;
  GtkWidget *label162;
  GtkWidget *hbuttonbox3;
  GtkWidget *b_finish;
  GtkWidget *b_cancel;

  if (hwnd == NULL || !GTK_IS_WIDGET (hwnd))
    return;

  translatorchooser = gtk_dialog_new ();
  gtk_widget_set_name (translatorchooser, "translatorchooser");
  gtk_widget_set_size_request (translatorchooser, 515, 335);
  gtk_window_set_title (GTK_WINDOW (translatorchooser), _("Choose a Translator"));
  gtk_window_set_position (GTK_WINDOW (translatorchooser), GTK_WIN_POS_CENTER_ON_PARENT);
  gtk_window_set_modal (GTK_WINDOW (translatorchooser), TRUE);
  gtk_window_set_default_size (GTK_WINDOW (translatorchooser), 600, 450);
  gtk_window_set_type_hint (GTK_WINDOW (translatorchooser), GDK_WINDOW_TYPE_HINT_DIALOG);

#if GTK_CHECK_VERSION(2,0,0)
  gtk_widget_show (translatorchooser);
#endif

  vbox41 = GTK_DIALOG (translatorchooser)->vbox;
  gtk_widget_set_name (vbox41, "vbox41");
  gtk_widget_show (vbox41);

  hbox57 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox57, "hbox57");
  gtk_widget_show (hbox57);
  gtk_box_pack_start (GTK_BOX (vbox41), hbox57, TRUE, TRUE, 0);

  frame97 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame97, "frame97");
  gtk_widget_show (frame97);
  gtk_box_pack_start (GTK_BOX (hbox57), frame97, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame97), 10);
  gtk_frame_set_shadow_type (GTK_FRAME (frame97), GTK_SHADOW_NONE);

  alignment81 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment81, "alignment81");
  gtk_widget_show (alignment81);
  gtk_container_add (GTK_CONTAINER (frame97), alignment81);
  gtk_widget_set_size_request (alignment81, 140, -1);

#if GTK_CHECK_VERSION(2,0,0)
  style = gtk_widget_get_style (translatorchooser);
  pixmap =
      gdk_pixmap_create_from_xpm_d (translatorchooser->window, &mask,
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
  gtk_container_add (GTK_CONTAINER (alignment81), pixmap1);

  frame98 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame98, "frame98");
  gtk_widget_show (frame98);
  gtk_box_pack_start (GTK_BOX (hbox57), frame98, TRUE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame98), GTK_SHADOW_NONE);

  alignment82 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment82, "alignment82");
  gtk_widget_show (alignment82);
  gtk_container_add (GTK_CONTAINER (frame98), alignment82);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment82), 0, 10, 0, 0);

  scrolledwindow21 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_name (scrolledwindow21, "scrolledwindow21");
  gtk_widget_show (scrolledwindow21);
  gtk_container_add (GTK_CONTAINER (alignment82), scrolledwindow21);

  clist1 = gtk_clist_new (4);
  gtk_widget_set_name (clist1, "clist1");
  gtk_widget_show (clist1);
  gtk_container_add (GTK_CONTAINER (scrolledwindow21), clist1);
  gtk_clist_set_column_width (GTK_CLIST (clist1), 0, 165);
  gtk_clist_set_column_width (GTK_CLIST (clist1), 1, 118);
  gtk_clist_set_column_width (GTK_CLIST (clist1), 2, 80);
  gtk_clist_set_column_width (GTK_CLIST (clist1), 3, 80);
  gtk_clist_column_titles_show (GTK_CLIST (clist1));

  label158 = gtk_label_new (_("Name"));
  gtk_widget_set_name (label158, "label158");
  gtk_widget_show (label158);
  gtk_clist_set_column_widget (GTK_CLIST (clist1), 0, label158);
  gtk_widget_set_size_request (label158, 165, -1);

  label159 = gtk_label_new (_("File"));
  gtk_widget_set_name (label159, "label159");
  gtk_widget_show (label159);
  gtk_clist_set_column_widget (GTK_CLIST (clist1), 1, label159);
  gtk_widget_set_size_request (label159, 118, -1);

  label160 = gtk_label_new (_("Date"));
  gtk_widget_set_name (label160, "label160");
  gtk_widget_show (label160);
  gtk_clist_set_column_widget (GTK_CLIST (clist1), 2, label160);
  gtk_widget_set_size_request (label160, 80, -1);

  label161 = gtk_label_new (_("Size"));
  gtk_widget_set_name (label161, "label161");
  gtk_widget_show (label161);
  gtk_clist_set_column_widget (GTK_CLIST (clist1), 3, label161);
  gtk_widget_set_size_request (label161, 80, -1);

  label162 = gtk_label_new (_("Select which ODBC Translator you want to use"));
  gtk_widget_set_name (label162, "label162");
  gtk_widget_show (label162);
  gtk_frame_set_label_widget (GTK_FRAME (frame98), label162);
  gtk_label_set_use_markup (GTK_LABEL (label162), TRUE);

  hbuttonbox3 = GTK_DIALOG (translatorchooser)->action_area;
  gtk_widget_set_name (hbuttonbox3, "hbuttonbox3");
  gtk_widget_show (hbuttonbox3);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox3), GTK_BUTTONBOX_END);

  b_finish = gtk_button_new_from_stock ("gtk-ok");
  gtk_widget_set_name (b_finish, "b_finish");
  gtk_widget_show (b_finish);
  gtk_dialog_add_action_widget (GTK_DIALOG (translatorchooser), b_finish, GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS (b_finish, GTK_CAN_DEFAULT);

  b_cancel = gtk_button_new_from_stock ("gtk-cancel");
  gtk_widget_set_name (b_cancel, "b_cancel");
  gtk_widget_show (b_cancel);
  gtk_dialog_add_action_widget (GTK_DIALOG (translatorchooser), b_cancel, GTK_RESPONSE_CANCEL);
  GTK_WIDGET_SET_FLAGS (b_cancel, GTK_CAN_DEFAULT);

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF (translatorchooser, translatorchooser, "translatorchooser");
  GLADE_HOOKUP_OBJECT_NO_REF (translatorchooser, vbox41, "vbox41");
  GLADE_HOOKUP_OBJECT (translatorchooser, hbox57, "hbox57");
  GLADE_HOOKUP_OBJECT (translatorchooser, frame97, "frame97");
  GLADE_HOOKUP_OBJECT (translatorchooser, alignment81, "alignment81");
  GLADE_HOOKUP_OBJECT (translatorchooser, pixmap1, "pixmap1");
  GLADE_HOOKUP_OBJECT (translatorchooser, frame98, "frame98");
  GLADE_HOOKUP_OBJECT (translatorchooser, alignment82, "alignment82");
  GLADE_HOOKUP_OBJECT (translatorchooser, scrolledwindow21, "scrolledwindow21");
  GLADE_HOOKUP_OBJECT (translatorchooser, clist1, "clist1");
  GLADE_HOOKUP_OBJECT (translatorchooser, label158, "label158");
  GLADE_HOOKUP_OBJECT (translatorchooser, label159, "label159");
  GLADE_HOOKUP_OBJECT (translatorchooser, label160, "label160");
  GLADE_HOOKUP_OBJECT (translatorchooser, label161, "label161");
  GLADE_HOOKUP_OBJECT (translatorchooser, label162, "label162");
  GLADE_HOOKUP_OBJECT_NO_REF (translatorchooser, hbuttonbox3, "hbuttonbox3");
  GLADE_HOOKUP_OBJECT (translatorchooser, b_finish, "b_finish");
  GLADE_HOOKUP_OBJECT (translatorchooser, b_cancel, "b_cancel");

  /* Ok button events */
  gtk_signal_connect (GTK_OBJECT (b_finish), "clicked",
      GTK_SIGNAL_FUNC (translatorchooser_ok_clicked), choose_t);
  /* Cancel button events */
  gtk_signal_connect (GTK_OBJECT (b_cancel), "clicked",
      GTK_SIGNAL_FUNC (translatorchooser_cancel_clicked), choose_t);
  /* Close window button events */
  gtk_signal_connect (GTK_OBJECT (translatorchooser), "delete_event",
      GTK_SIGNAL_FUNC (delete_event), choose_t);
  gtk_signal_connect (GTK_OBJECT (translatorchooser), "destroy",
      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
  /* Translator list events */
  gtk_signal_connect (GTK_OBJECT (clist1), "select_row",
      GTK_SIGNAL_FUNC (translator_list_select), choose_t);

  addtranslators_to_list (clist1, translatorchooser);

  choose_t->translatorlist = clist1;
  choose_t->translator = NULL;
  choose_t->mainwnd = translatorchooser;
  choose_t->b_finish = b_finish;

  gtk_widget_show_all (translatorchooser);
  gtk_main ();
}
