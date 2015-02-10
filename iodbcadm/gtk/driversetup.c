/*
 *  driversetup.c
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


#include "gui.h"


static char* STRCONN = "%s\0Driver=%s\0Setup=%s\0\0";
static int STRCONN_NB_TOKENS = 3;

char *szKeysColumnNames[] = {
  "Keyword",
  "Value"
};

char *szKeysButtons[] = {
  "_Add",
  "_Update"
};


static void
addkeywords_to_list (GtkWidget *widget, LPCSTR attrs,
    TDRIVERSETUP *driversetup_t)
{
  char *curr, *cour;
  char *data[2];

  if (!GTK_IS_CLIST (widget))
    return;
  gtk_clist_clear (GTK_CLIST (widget));

  for (curr = (LPSTR) attrs; *curr; curr += (STRLEN (curr) + 1))
    {
      if (!strncasecmp (curr, "Driver=", STRLEN ("Driver=")))
	{
	  gtk_entry_set_text (GTK_ENTRY (driversetup_t->driver_entry),
	      curr + STRLEN ("Driver="));
	  continue;
	}

      if (!strncasecmp (curr, "Setup=", STRLEN ("Setup=")))
	{
	  gtk_entry_set_text (GTK_ENTRY (driversetup_t->setup_entry),
	      curr + STRLEN ("Setup="));
	  continue;
	}

      data[0] = curr;

      if ((cour = strchr (curr, '=')))
	{
	  data[1] = cour + 1;
	  *cour = 0;
	  gtk_clist_append (GTK_CLIST (widget), data);
	  *cour = '=';
	}
      else
	{
	  data[1] = "";
	  gtk_clist_append (GTK_CLIST (widget), data);
	}
    }

  if (GTK_CLIST (widget)->rows > 0)
    gtk_clist_sort (GTK_CLIST (widget));
}


static void
parse_attribute_line (TDRIVERSETUP *driversetup_t, LPCSTR driver,
    LPCSTR attrs, BOOL add)
{
  if (driver)
    {
      gtk_entry_set_text (GTK_ENTRY (driversetup_t->name_entry), driver);
      if (add)
	gtk_entry_set_editable (GTK_ENTRY (driversetup_t->name_entry), FALSE);
      else
	gtk_entry_set_editable (GTK_ENTRY (driversetup_t->name_entry), TRUE);
    }

  addkeywords_to_list (driversetup_t->key_list, attrs, driversetup_t);
}


static void
driver_file_choosen (GtkWidget *widget, TDRIVERSETUP *driversetup_t)
{
  if (driversetup_t)
    {
      gtk_entry_set_text (GTK_ENTRY (driversetup_t->driver_entry),
	  gtk_file_selection_get_filename (GTK_FILE_SELECTION (driversetup_t->
		  filesel)));
      driversetup_t->filesel = NULL;
    }
}


static void
setup_file_choosen (GtkWidget *widget, TDRIVERSETUP *driversetup_t)
{
  if (driversetup_t)
    {
      gtk_entry_set_text (GTK_ENTRY (driversetup_t->setup_entry),
	  gtk_file_selection_get_filename (GTK_FILE_SELECTION (driversetup_t->
		  filesel)));
      driversetup_t->filesel = NULL;
    }
}


static void
driversetup_browsedriver_clicked (GtkWidget *widget,
    TDRIVERSETUP *driversetup_t)
{
  GtkWidget *filesel;

  if (driversetup_t)
    {
      filesel = gtk_file_selection_new ("Choose your driver library ...");
      gtk_window_set_modal (GTK_WINDOW (filesel), TRUE);
      gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel),
	  gtk_entry_get_text (GTK_ENTRY (driversetup_t->driver_entry)));
      /* Ok button events */
      gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->
	      ok_button), "clicked", GTK_SIGNAL_FUNC (driver_file_choosen),
	  driversetup_t);
      gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->
	      ok_button), "clicked", GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
      /* Cancel button events */
      gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->
	      cancel_button), "clicked", GTK_SIGNAL_FUNC (gtk_main_quit),
	  NULL);
      /* Close window button events */
      gtk_signal_connect (GTK_OBJECT (filesel), "delete_event",
	  GTK_SIGNAL_FUNC (gtk_main_quit), NULL);

      driversetup_t->filesel = filesel;

      gtk_widget_show_all (filesel);
      gtk_main ();
      gtk_widget_destroy (filesel);

      driversetup_t->filesel = NULL;
    }
}


static void
driversetup_browsesetup_clicked (GtkWidget *widget,
    TDRIVERSETUP *driversetup_t)
{
  GtkWidget *filesel;

  if (driversetup_t)
    {
      filesel = gtk_file_selection_new ("Choose your setup library ...");
      gtk_window_set_modal (GTK_WINDOW (filesel), TRUE);
      gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel),
	  gtk_entry_get_text (GTK_ENTRY (driversetup_t->setup_entry)));
      /* Ok button events */
      gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->
	      ok_button), "clicked", GTK_SIGNAL_FUNC (setup_file_choosen),
	  driversetup_t);
      gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->
	      ok_button), "clicked", GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
      /* Cancel button events */
      gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->
	      cancel_button), "clicked", GTK_SIGNAL_FUNC (gtk_main_quit),
	  NULL);
      /* Close window button events */
      gtk_signal_connect (GTK_OBJECT (filesel), "delete_event",
	  GTK_SIGNAL_FUNC (gtk_main_quit), NULL);

      driversetup_t->filesel = filesel;

      gtk_widget_show_all (filesel);
      gtk_main ();
      gtk_widget_destroy (filesel);

      driversetup_t->filesel = NULL;
    }
}


static void
driversetup_add_clicked (GtkWidget *widget, TDRIVERSETUP *driversetup_t)
{
  char *szKey;
  char *data[2];
  int i = 0;

  if (driversetup_t)
    {
      data[0] = (char*)gtk_entry_get_text (GTK_ENTRY (driversetup_t->key_entry));
      if (!STRLEN (data[0]))
	goto done;
      data[1] = (char*)gtk_entry_get_text (GTK_ENTRY (driversetup_t->value_entry));

      for (i = 0; i < GTK_CLIST (driversetup_t->key_list)->rows; i++)
	{
	  gtk_clist_get_text (GTK_CLIST (driversetup_t->key_list), i, 0,
	      &szKey);
	  if (!strcmp (szKey, data[0]))
	    {
	      gtk_clist_remove (GTK_CLIST (driversetup_t->key_list), i);
	      break;
	    }
	}

      gtk_clist_append (GTK_CLIST (driversetup_t->key_list), data);
      if (GTK_CLIST (driversetup_t->key_list)->rows > 0)
	gtk_clist_sort (GTK_CLIST (driversetup_t->key_list));

    done:
      gtk_entry_set_text (GTK_ENTRY (driversetup_t->key_entry), "");
      gtk_entry_set_text (GTK_ENTRY (driversetup_t->value_entry), "");
    }
}


static void
driversetup_update_clicked (GtkWidget *widget, TDRIVERSETUP *driversetup_t)
{
  char *data[2];
  int i;

  if (driversetup_t)
    {
      data[0] = (char*)gtk_entry_get_text (GTK_ENTRY (driversetup_t->key_entry));
      if (STRLEN (data[0]))
	{
	  data[1] = (char*)gtk_entry_get_text (GTK_ENTRY (driversetup_t->value_entry));

	  if (GTK_CLIST (driversetup_t->key_list)->selection != NULL)
	    i = GPOINTER_TO_INT (GTK_CLIST (driversetup_t->key_list)->selection->
		data);
	  else
	    i = 0;

	  /* An update operation */
	  if (i < GTK_CLIST (driversetup_t->key_list)->rows)
	    {
	      gtk_clist_set_text (GTK_CLIST (driversetup_t->key_list), i, 0,
		  data[0]);
	      gtk_clist_set_text (GTK_CLIST (driversetup_t->key_list), i, 1,
		  data[1]);
	    }
	}

      gtk_entry_set_text (GTK_ENTRY (driversetup_t->key_entry), "");
      gtk_entry_set_text (GTK_ENTRY (driversetup_t->value_entry), "");
    }
}


static void
driversetup_list_select (GtkWidget *widget, gint row, gint column,
    GdkEvent *event, TDRIVERSETUP *driversetup_t)
{
  char *szKey, *szValue;

  if (driversetup_t && GTK_CLIST (driversetup_t->key_list)->selection != NULL)
    {
      gtk_clist_get_text (GTK_CLIST (driversetup_t->key_list),
	  GPOINTER_TO_INT (GTK_CLIST (driversetup_t->key_list)->selection->
	      data), 0, &szKey);
      gtk_clist_get_text (GTK_CLIST (driversetup_t->key_list),
	  GPOINTER_TO_INT (GTK_CLIST (driversetup_t->key_list)->selection->
	      data), 1, &szValue);
      gtk_entry_set_text (GTK_ENTRY (driversetup_t->key_entry), szKey);
      gtk_entry_set_text (GTK_ENTRY (driversetup_t->value_entry), szValue);
      gtk_widget_set_sensitive (driversetup_t->bupdate, TRUE);
    }
}


static void
driversetup_list_unselect (GtkWidget *widget, gint row, gint column,
    GdkEvent *event, TDRIVERSETUP *driversetup_t)
{
  if (driversetup_t)
    {
      gtk_widget_set_sensitive (driversetup_t->bupdate, FALSE);
      gtk_entry_set_text (GTK_ENTRY (driversetup_t->key_entry), "");
      gtk_entry_set_text (GTK_ENTRY (driversetup_t->value_entry), "");
    }
}


static void
driversetup_ok_clicked (GtkWidget *widget, TDRIVERSETUP *driversetup_t)
{
  char *curr, *cour, *szKey, *szValue;
  int i = 0, size;

  if (driversetup_t)
    {
      driversetup_t->connstr = (char *) malloc (sizeof (char) * (size =
	      ((STRLEN (gtk_entry_get_text (GTK_ENTRY (driversetup_t->
				  name_entry))) ?
		      STRLEN (gtk_entry_get_text (GTK_ENTRY (driversetup_t->
				  name_entry))) + 1 : 0) +
		  (STRLEN (gtk_entry_get_text (GTK_ENTRY (driversetup_t->
				  driver_entry))) ?
		      STRLEN (gtk_entry_get_text (GTK_ENTRY (driversetup_t->
				  driver_entry))) + STRLEN ("Driver=") +
		      1 : 0) +
		  (STRLEN (gtk_entry_get_text (GTK_ENTRY (driversetup_t->
				  setup_entry))) ?
		      STRLEN (gtk_entry_get_text (GTK_ENTRY (driversetup_t->
				  setup_entry))) + STRLEN ("Setup=") +
		      1 : 0) + 1)));

      if (driversetup_t->connstr)
	{
	  for (curr = STRCONN, cour = driversetup_t->connstr;
	      i < STRCONN_NB_TOKENS; i++, curr += (STRLEN (curr) + 1))
	    switch (i)
	      {
	      case 0:
		if (STRLEN (gtk_entry_get_text (GTK_ENTRY (driversetup_t->
				name_entry))))
		  {
		    sprintf (cour, curr,
			gtk_entry_get_text (GTK_ENTRY (driversetup_t->
				name_entry)));
		    cour += (STRLEN (cour) + 1);
		  }
		break;
	      case 1:
		if (STRLEN (gtk_entry_get_text (GTK_ENTRY (driversetup_t->
				driver_entry))))
		  {
		    sprintf (cour, curr,
			gtk_entry_get_text (GTK_ENTRY (driversetup_t->
				driver_entry)));
		    cour += (STRLEN (cour) + 1);
		  }
		break;
	      case 2:
		if (STRLEN (gtk_entry_get_text (GTK_ENTRY (driversetup_t->
				setup_entry))))
		  {
		    sprintf (cour, curr,
			gtk_entry_get_text (GTK_ENTRY (driversetup_t->
				setup_entry)));
		    cour += (STRLEN (cour) + 1);
		  }
		break;
	      };

	  for (i = 0; i < GTK_CLIST (driversetup_t->key_list)->rows; i++)
	    {
	      gtk_clist_get_text (GTK_CLIST (driversetup_t->key_list), i, 0,
		  &szKey);
	      gtk_clist_get_text (GTK_CLIST (driversetup_t->key_list), i, 1,
		  &szValue);

	      cour = (char *) driversetup_t->connstr;
	      driversetup_t->connstr =
		  (LPSTR) malloc (size + STRLEN (szKey) + STRLEN (szValue) +
		  2);
	      if (driversetup_t->connstr)
		{
		  memcpy (driversetup_t->connstr, cour, size);
		  sprintf (driversetup_t->connstr + size - 1, "%s=%s", szKey,
		      szValue);
		  free (cour);
		  size += STRLEN (szKey) + STRLEN (szValue) + 2;
		}
	      else
		driversetup_t->connstr = cour;
	    }

	  driversetup_t->connstr[size - 1] = 0;
	}

      driversetup_t->name_entry = driversetup_t->driver_entry =
	  driversetup_t->setup_entry = NULL;
      driversetup_t->key_list = driversetup_t->filesel = NULL;

      gtk_signal_disconnect_by_func (GTK_OBJECT (driversetup_t->mainwnd),
	  GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
      gtk_main_quit ();
      gtk_widget_destroy (driversetup_t->mainwnd);
    }
}


static void
driversetup_cancel_clicked (GtkWidget *widget, TDRIVERSETUP *driversetup_t)
{
  if (driversetup_t)
    {
      driversetup_t->connstr = (LPSTR) - 1L;

      driversetup_t->name_entry = driversetup_t->driver_entry =
	  driversetup_t->setup_entry = NULL;
      driversetup_t->key_list = driversetup_t->filesel = NULL;

      gtk_signal_disconnect_by_func (GTK_OBJECT (driversetup_t->mainwnd),
	  GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
      gtk_main_quit ();
      gtk_widget_destroy (driversetup_t->mainwnd);
    }
}


static gint
delete_event (GtkWidget *widget, GdkEvent *event,
    TDRIVERSETUP *driversetup_t)
{
  driversetup_cancel_clicked (widget, driversetup_t);

  return FALSE;
}


LPSTR
create_driversetup (HWND hwnd, LPCSTR driver, LPCSTR attrs, BOOL add, BOOL user)
{
  GdkPixmap *pixmap;
  GdkBitmap *mask;
  GtkStyle *style;


  GtkWidget *driversetup;
  GtkWidget *dialog_vbox6;
  GtkWidget *vbox26;
  GtkWidget *frame57;
  GtkWidget *alignment49;
  GtkWidget *table6;
  GtkWidget *label99;
  GtkWidget *label100;
  GtkWidget *label101;
  GtkWidget *t_name;
  GtkWidget *t_driver;
  GtkWidget *t_setup;
  GtkWidget *b_browse;
  GtkWidget *b_browse1;
  GtkWidget *frame58;
  GtkWidget *alignment50;
  GtkWidget *scrolledwindow16;
  GtkWidget *clist1;
  GtkWidget *label97;
  GtkWidget *label98;
  GtkWidget *frame59;
  GtkWidget *alignment51;
  GtkWidget *table7;
  GtkWidget *label102;
  GtkWidget *label103;
  GtkWidget *t_keyword;
  GtkWidget *t_value;
  GtkWidget *b_add;
  GtkWidget *b_update;
  GtkWidget *dialog_action_area6;
  GtkWidget *b_cancel;
  GtkWidget *b_ok;
  TDRIVERSETUP driversetup_t;


  if (hwnd == NULL || !GTK_IS_WIDGET (hwnd))
    return (LPSTR) attrs;

  driversetup = gtk_dialog_new ();
  gtk_widget_set_name (driversetup, "driversetup");
  gtk_widget_set_size_request (driversetup, 505, 480);
  gtk_window_set_title (GTK_WINDOW (driversetup), _("ODBC Driver Add/Setup"));
  gtk_window_set_position (GTK_WINDOW (driversetup), GTK_WIN_POS_CENTER);
  gtk_window_set_modal (GTK_WINDOW (driversetup), TRUE);
  gtk_window_set_default_size (GTK_WINDOW (driversetup), 600, 450);
  gtk_window_set_type_hint (GTK_WINDOW (driversetup), GDK_WINDOW_TYPE_HINT_DIALOG);

#if GTK_CHECK_VERSION(2,0,0)
  gtk_widget_show (driversetup);
#endif

  dialog_vbox6 = GTK_DIALOG (driversetup)->vbox;
  gtk_widget_set_name (dialog_vbox6, "dialog_vbox6");
  gtk_widget_show (dialog_vbox6);

  vbox26 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox26, "vbox26");
  gtk_widget_show (vbox26);
  gtk_box_pack_start (GTK_BOX (dialog_vbox6), vbox26, TRUE, TRUE, 0);

  frame57 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame57, "frame57");
  gtk_widget_show (frame57);
  gtk_box_pack_start (GTK_BOX (vbox26), frame57, FALSE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame57), GTK_SHADOW_NONE);

  alignment49 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment49, "alignment49");
  gtk_widget_show (alignment49);
  gtk_container_add (GTK_CONTAINER (frame57), alignment49);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment49), 16, 0, 4, 4);

  table6 = gtk_table_new (3, 3, FALSE);
  gtk_widget_set_name (table6, "table6");
  gtk_widget_show (table6);
  gtk_container_add (GTK_CONTAINER (alignment49), table6);
  gtk_table_set_row_spacings (GTK_TABLE (table6), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table6), 10);

  label99 = gtk_label_new (_("Description of the driver :"));
  gtk_widget_set_name (label99, "label99");
  gtk_widget_show (label99);
  gtk_table_attach (GTK_TABLE (table6), label99, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label99), 0, 0.5);

  label100 = gtk_label_new (_("            Driver file name :"));
  gtk_widget_set_name (label100, "label100");
  gtk_widget_show (label100);
  gtk_table_attach (GTK_TABLE (table6), label100, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_label_set_justify (GTK_LABEL (label100), GTK_JUSTIFY_RIGHT);
  gtk_misc_set_alignment (GTK_MISC (label100), 0, 0.5);

  label101 = gtk_label_new (_("            Setup file name :"));
  gtk_widget_set_name (label101, "label101");
  gtk_widget_show (label101);
  gtk_table_attach (GTK_TABLE (table6), label101, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_label_set_justify (GTK_LABEL (label101), GTK_JUSTIFY_RIGHT);
  gtk_misc_set_alignment (GTK_MISC (label101), 0, 0.5);

  t_name = gtk_entry_new ();
  gtk_widget_set_name (t_name, "t_name");
  gtk_widget_show (t_name);
  gtk_table_attach (GTK_TABLE (table6), t_name, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  t_driver = gtk_entry_new ();
  gtk_widget_set_name (t_driver, "t_driver");
  gtk_widget_show (t_driver);
  gtk_table_attach (GTK_TABLE (table6), t_driver, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  t_setup = gtk_entry_new ();
  gtk_widget_set_name (t_setup, "t_setup");
  gtk_widget_show (t_setup);
  gtk_table_attach (GTK_TABLE (table6), t_setup, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  b_browse = gtk_button_new_with_mnemonic (_("Browse . . ."));
  gtk_widget_set_name (b_browse, "b_browse");
  gtk_widget_show (b_browse);
  gtk_table_attach (GTK_TABLE (table6), b_browse, 2, 3, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  b_browse1 = gtk_button_new_with_mnemonic (_("  Browse . . . "));
  gtk_widget_set_name (b_browse1, "b_browse1");
  gtk_widget_show (b_browse1);
  gtk_table_attach (GTK_TABLE (table6), b_browse1, 2, 3, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  frame58 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame58, "frame58");
  gtk_widget_show (frame58);
  gtk_box_pack_start (GTK_BOX (vbox26), frame58, TRUE, TRUE, 0);
  gtk_widget_set_size_request (frame58, -1, 220);
  gtk_frame_set_shadow_type (GTK_FRAME (frame58), GTK_SHADOW_NONE);

  alignment50 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment50, "alignment50");
  gtk_widget_show (alignment50);
  gtk_container_add (GTK_CONTAINER (frame58), alignment50);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment50), 0, 0, 4, 4);

  scrolledwindow16 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_name (scrolledwindow16, "scrolledwindow16");
  gtk_widget_show (scrolledwindow16);
  gtk_container_add (GTK_CONTAINER (alignment50), scrolledwindow16);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow16), GTK_SHADOW_IN);

  clist1 = gtk_clist_new (2);
  gtk_widget_set_name (clist1, "clist1");
  gtk_widget_show (clist1);
  gtk_container_add (GTK_CONTAINER (scrolledwindow16), clist1);
  gtk_clist_set_column_width (GTK_CLIST (clist1), 0, 134);
  gtk_clist_set_column_width (GTK_CLIST (clist1), 1, 80);
  gtk_clist_column_titles_show (GTK_CLIST (clist1));

  label97 = gtk_label_new (_("Keyword"));
  gtk_widget_set_name (label97, "label97");
  gtk_widget_show (label97);
  gtk_clist_set_column_widget (GTK_CLIST (clist1), 0, label97);
  gtk_widget_set_size_request (label97, 134, -1);

  label98 = gtk_label_new (_("Value"));
  gtk_widget_set_name (label98, "label98");
  gtk_widget_show (label98);
  gtk_clist_set_column_widget (GTK_CLIST (clist1), 1, label98);
  gtk_widget_set_size_request (label98, 80, -1);

  frame59 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame59, "frame59");
  gtk_widget_show (frame59);
  gtk_box_pack_start (GTK_BOX (vbox26), frame59, FALSE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame59), GTK_SHADOW_NONE);

  alignment51 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment51, "alignment51");
  gtk_widget_show (alignment51);
  gtk_container_add (GTK_CONTAINER (frame59), alignment51);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment51), 10, 10, 4, 4);

  table7 = gtk_table_new (2, 3, FALSE);
  gtk_widget_set_name (table7, "table7");
  gtk_widget_show (table7);
  gtk_container_add (GTK_CONTAINER (alignment51), table7);
  gtk_table_set_row_spacings (GTK_TABLE (table7), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table7), 10);

  label102 = gtk_label_new (_("Keyword :"));
  gtk_widget_set_name (label102, "label102");
  gtk_widget_show (label102);
  gtk_table_attach (GTK_TABLE (table7), label102, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label102), 0, 0.5);

  label103 = gtk_label_new (_("    Value :"));
  gtk_widget_set_name (label103, "label103");
  gtk_widget_show (label103);
  gtk_table_attach (GTK_TABLE (table7), label103, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label103), 0, 0.5);

  t_keyword = gtk_entry_new ();
  gtk_widget_set_name (t_keyword, "t_keyword");
  gtk_widget_show (t_keyword);
  gtk_table_attach (GTK_TABLE (table7), t_keyword, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  t_value = gtk_entry_new ();
  gtk_widget_set_name (t_value, "t_value");
  gtk_widget_show (t_value);
  gtk_table_attach (GTK_TABLE (table7), t_value, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  b_add = gtk_button_new_from_stock ("gtk-add");
  gtk_widget_set_name (b_add, "b_add");
  gtk_widget_show (b_add);
  gtk_table_attach (GTK_TABLE (table7), b_add, 2, 3, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  b_update = gtk_button_new_with_mnemonic (_("_Update"));
  gtk_widget_set_name (b_update, "b_update");
  gtk_widget_show (b_update);
  gtk_table_attach (GTK_TABLE (table7), b_update, 2, 3, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  dialog_action_area6 = GTK_DIALOG (driversetup)->action_area;
  gtk_widget_set_name (dialog_action_area6, "dialog_action_area6");
  gtk_widget_show (dialog_action_area6);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area6), GTK_BUTTONBOX_END);

  b_cancel = gtk_button_new_from_stock ("gtk-cancel");
  gtk_widget_set_name (b_cancel, "b_cancel");
  gtk_widget_show (b_cancel);
  gtk_dialog_add_action_widget (GTK_DIALOG (driversetup), b_cancel, GTK_RESPONSE_CANCEL);
  GTK_WIDGET_SET_FLAGS (b_cancel, GTK_CAN_DEFAULT);

  b_ok = gtk_button_new_from_stock ("gtk-ok");
  gtk_widget_set_name (b_ok, "b_ok");
  gtk_widget_show (b_ok);
  gtk_dialog_add_action_widget (GTK_DIALOG (driversetup), b_ok, GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS (b_ok, GTK_CAN_DEFAULT);

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF (driversetup, driversetup, "driversetup");
  GLADE_HOOKUP_OBJECT_NO_REF (driversetup, dialog_vbox6, "dialog_vbox6");
  GLADE_HOOKUP_OBJECT (driversetup, vbox26, "vbox26");
  GLADE_HOOKUP_OBJECT (driversetup, frame57, "frame57");
  GLADE_HOOKUP_OBJECT (driversetup, alignment49, "alignment49");
  GLADE_HOOKUP_OBJECT (driversetup, table6, "table6");
  GLADE_HOOKUP_OBJECT (driversetup, label99, "label99");
  GLADE_HOOKUP_OBJECT (driversetup, label100, "label100");
  GLADE_HOOKUP_OBJECT (driversetup, label101, "label101");
  GLADE_HOOKUP_OBJECT (driversetup, t_name, "t_name");
  GLADE_HOOKUP_OBJECT (driversetup, t_driver, "t_driver");
  GLADE_HOOKUP_OBJECT (driversetup, t_setup, "t_setup");
  GLADE_HOOKUP_OBJECT (driversetup, b_browse, "b_browse");
  GLADE_HOOKUP_OBJECT (driversetup, b_browse1, "b_browse1");
  GLADE_HOOKUP_OBJECT (driversetup, frame58, "frame58");
  GLADE_HOOKUP_OBJECT (driversetup, alignment50, "alignment50");
  GLADE_HOOKUP_OBJECT (driversetup, scrolledwindow16, "scrolledwindow16");
  GLADE_HOOKUP_OBJECT (driversetup, clist1, "clist1");
  GLADE_HOOKUP_OBJECT (driversetup, label97, "label97");
  GLADE_HOOKUP_OBJECT (driversetup, label98, "label98");
  GLADE_HOOKUP_OBJECT (driversetup, frame59, "frame59");
  GLADE_HOOKUP_OBJECT (driversetup, alignment51, "alignment51");
  GLADE_HOOKUP_OBJECT (driversetup, table7, "table7");
  GLADE_HOOKUP_OBJECT (driversetup, label102, "label102");
  GLADE_HOOKUP_OBJECT (driversetup, label103, "label103");
  GLADE_HOOKUP_OBJECT (driversetup, t_keyword, "t_keyword");
  GLADE_HOOKUP_OBJECT (driversetup, t_value, "t_value");
  GLADE_HOOKUP_OBJECT (driversetup, b_add, "b_add");
  GLADE_HOOKUP_OBJECT (driversetup, b_update, "b_update");
  GLADE_HOOKUP_OBJECT_NO_REF (driversetup, dialog_action_area6, "dialog_action_area6");
  GLADE_HOOKUP_OBJECT (driversetup, b_cancel, "b_cancel");
  GLADE_HOOKUP_OBJECT (driversetup, b_ok, "b_ok");

  /* Ok button events */
  gtk_signal_connect (GTK_OBJECT (b_ok), "clicked",
      GTK_SIGNAL_FUNC (driversetup_ok_clicked), &driversetup_t);
  /* Cancel button events */
  gtk_signal_connect (GTK_OBJECT (b_cancel), "clicked",
      GTK_SIGNAL_FUNC (driversetup_cancel_clicked), &driversetup_t);
  /* Add button events */
  gtk_signal_connect (GTK_OBJECT (b_add), "clicked",
      GTK_SIGNAL_FUNC (driversetup_add_clicked), &driversetup_t);
  /* Update button events */
  gtk_signal_connect (GTK_OBJECT (b_update), "clicked",
      GTK_SIGNAL_FUNC (driversetup_update_clicked), &driversetup_t);
  /* Close window button events */
  gtk_signal_connect (GTK_OBJECT (driversetup), "delete_event",
      GTK_SIGNAL_FUNC (delete_event), &driversetup_t);
  gtk_signal_connect (GTK_OBJECT (driversetup), "destroy",
      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
  /* List events */
  gtk_signal_connect (GTK_OBJECT (clist1), "select_row",
      GTK_SIGNAL_FUNC (driversetup_list_select), &driversetup_t);
  gtk_signal_connect (GTK_OBJECT (clist1), "unselect_row",
      GTK_SIGNAL_FUNC (driversetup_list_unselect), &driversetup_t);
  /* Browse button events */
  gtk_signal_connect (GTK_OBJECT (b_browse), "clicked",
      GTK_SIGNAL_FUNC (driversetup_browsedriver_clicked), &driversetup_t);
  gtk_signal_connect (GTK_OBJECT (b_browse1), "clicked",
      GTK_SIGNAL_FUNC (driversetup_browsesetup_clicked), &driversetup_t);

  driversetup_t.name_entry = t_name;
  driversetup_t.driver_entry = t_driver;
  driversetup_t.key_list = clist1;
  driversetup_t.bupdate = b_update;
  driversetup_t.key_entry = t_keyword;
  driversetup_t.value_entry = t_value;
  driversetup_t.mainwnd = driversetup;
  driversetup_t.setup_entry = t_setup;

  /* Parse the attributes line */
  parse_attribute_line (&driversetup_t, driver, attrs, add);

  gtk_widget_show_all (driversetup);
  gtk_main ();

  return driversetup_t.connstr;
}
