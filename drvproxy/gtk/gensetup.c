/*
 *  gensetup.c
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


static char* STRCONN = "DSN=%s\0Description=%s\0\0";
static int STRCONN_NB_TOKENS = 2;

static char *szKeysColumnNames[] = {
  "Keyword",
  "Value"
};

static char *szKeysButtons[] = {
  "_Add",
  "_Update"
};


static void
addkeywords_to_list(GtkWidget* widget, LPCSTR attrs, TGENSETUP *gensetup_t)
{
  char *curr, *cour;
  char *data[2];

  if (!GTK_IS_CLIST (widget))
    return;
  gtk_clist_clear (GTK_CLIST (widget));

  for (curr = (LPSTR) attrs; *curr; curr += (STRLEN (curr) + 1))
    {
      if (!strncasecmp (curr, "Description=", STRLEN ("Description=")))
	gtk_entry_set_text (GTK_ENTRY (gensetup_t->comment_entry),
	    curr + STRLEN ("Description="));

      if (!strncasecmp (curr, "DSN=", STRLEN ("DSN=")) ||
	  !strncasecmp (curr, "Driver=", STRLEN ("Driver=")) ||
	  !strncasecmp (curr, "Description=", STRLEN ("Description=")))
	continue;

      if ((cour = strchr (curr, '=')))
	{
	  *cour = '\0';
	  data[0] = curr;
	  data[1] = cour + 1;
	  gtk_clist_append (GTK_CLIST (widget), data);
	  *cour = '=';
	}
      else
	{
	  data[0] = "";
	  gtk_clist_append (GTK_CLIST (widget), data);
	}
    }

  if (GTK_CLIST (widget)->rows > 0)
    gtk_clist_sort (GTK_CLIST (widget));
}


static void
parse_attribute_line(TGENSETUP *gensetup_t, LPCSTR dsn, LPCSTR attrs, BOOL add)
{
  if (dsn)
    {
      gtk_entry_set_text (GTK_ENTRY (gensetup_t->dsn_entry), dsn);
      if (add)
	gtk_widget_set_sensitive (gensetup_t->dsn_entry, TRUE);
      else
	gtk_widget_set_sensitive (gensetup_t->dsn_entry, FALSE);
    }

  addkeywords_to_list (gensetup_t->key_list, attrs, gensetup_t);
}


static void
gensetup_add_clicked(GtkWidget* widget, TGENSETUP *gensetup_t)
{
  char *szKey;
  char *data[2];
  int i = 0;

  if (gensetup_t)
    {
      data[0] = (char*)gtk_entry_get_text (GTK_ENTRY (gensetup_t->key_entry));
      if (STRLEN (data[0]))
	{
	  data[1] = (char*)gtk_entry_get_text (GTK_ENTRY (gensetup_t->value_entry));

	  /* Try to see if the keyword already exists */
	  for (i = 0; i < GTK_CLIST (gensetup_t->key_list)->rows; i++)
	    {
	      gtk_clist_get_text (GTK_CLIST (gensetup_t->key_list), i, 0,
		  &szKey);
	      if (STREQ (data[0], szKey))
		goto done;
	    }

	  /* An update operation */
	  if (i < GTK_CLIST (gensetup_t->key_list)->rows)
	    {
	      gtk_clist_set_text (GTK_CLIST (gensetup_t->key_list), i, 1,
		  data[1]);
	    }
	  else if (STRLEN (data[1]))
	    {
	      gtk_clist_append (GTK_CLIST (gensetup_t->key_list), data);
	    }
	}

      gtk_clist_sort (GTK_CLIST (gensetup_t->key_list));

    done:
      gtk_entry_set_text (GTK_ENTRY (gensetup_t->key_entry), "");
      gtk_entry_set_text (GTK_ENTRY (gensetup_t->value_entry), "");
    }
}


static void
gensetup_update_clicked(GtkWidget* widget, TGENSETUP *gensetup_t)
{
  char *data[2];
  int i;

  if (gensetup_t)
    {
      data[0] = (char*)gtk_entry_get_text (GTK_ENTRY (gensetup_t->key_entry));
      if (STRLEN (data[0]))
	{
	  data[1] = (char*)gtk_entry_get_text (GTK_ENTRY (gensetup_t->value_entry));

	  if (GTK_CLIST (gensetup_t->key_list)->selection != NULL)
	    i = GPOINTER_TO_INT (GTK_CLIST (gensetup_t->key_list)->selection->
		data);
	  else
	    i = 0;

	  /* An update operation */
	  if (i < GTK_CLIST (gensetup_t->key_list)->rows)
	    {
	      gtk_clist_set_text (GTK_CLIST (gensetup_t->key_list), i, 0,
		  data[0]);
	      gtk_clist_set_text (GTK_CLIST (gensetup_t->key_list), i, 1,
		  data[1]);
	    }
	}

      gtk_entry_set_text (GTK_ENTRY (gensetup_t->key_entry), "");
      gtk_entry_set_text (GTK_ENTRY (gensetup_t->value_entry), "");
    }
}


static void
gensetup_list_select(GtkWidget* widget, gint row, gint column, GdkEvent *event, TGENSETUP *gensetup_t)
{
  char *szKey, *szValue;

  if (gensetup_t && GTK_CLIST (gensetup_t->key_list)->selection != NULL)
    {
      gtk_clist_get_text (GTK_CLIST (gensetup_t->key_list),
	  GPOINTER_TO_INT (GTK_CLIST (gensetup_t->key_list)->selection->data),
	  0, &szKey);
      gtk_clist_get_text (GTK_CLIST (gensetup_t->key_list),
	  GPOINTER_TO_INT (GTK_CLIST (gensetup_t->key_list)->selection->data),
	  1, &szValue);
      gtk_entry_set_text (GTK_ENTRY (gensetup_t->key_entry), szKey);
      gtk_entry_set_text (GTK_ENTRY (gensetup_t->value_entry), szValue);
      gtk_widget_set_sensitive (gensetup_t->bupdate, TRUE);
    }
}


static void
gensetup_list_unselect(GtkWidget* widget, gint row, gint column, GdkEvent *event, TGENSETUP *gensetup_t)
{
  if (gensetup_t)
    {
      gtk_widget_set_sensitive (gensetup_t->bupdate, FALSE);
      gtk_entry_set_text (GTK_ENTRY (gensetup_t->key_entry), "");
      gtk_entry_set_text (GTK_ENTRY (gensetup_t->value_entry), "");
    }
}


static void
gensetup_ok_clicked(GtkWidget* widget, TGENSETUP *gensetup_t)
{
  char *curr, *cour, *szKey, *szValue;
  int i = 0, size = 0;

  if (gensetup_t)
    {
      /* What is the size of the block to malloc */
      size +=
	  STRLEN (gtk_entry_get_text (GTK_ENTRY (gensetup_t->dsn_entry))) +
	  STRLEN ("DSN=") + 1;
      size +=
	  STRLEN (gtk_entry_get_text (GTK_ENTRY (gensetup_t->
		  comment_entry))) + STRLEN ("Description=") + 1;
      /* Malloc it (+1 for list-terminating NUL) */
      if ((gensetup_t->connstr = (char *) malloc (size + 1)))
	{
	  for (curr = STRCONN, cour = gensetup_t->connstr;
	      i < STRCONN_NB_TOKENS; i++, curr += (STRLEN (curr) + 1))
	    switch (i)
	      {
	      case 0:
		sprintf (cour, curr,
		    gtk_entry_get_text (GTK_ENTRY (gensetup_t->dsn_entry)));
		cour += (STRLEN (cour) + 1);
		break;
	      case 1:
		sprintf (cour, curr,
		    gtk_entry_get_text (GTK_ENTRY (gensetup_t->
			    comment_entry)));
		cour += (STRLEN (cour) + 1);
		break;
	      };

	  for (i = 0; i < GTK_CLIST (gensetup_t->key_list)->rows; i++)
	    {
	      gtk_clist_get_text (GTK_CLIST (gensetup_t->key_list), i, 0,
		  &szKey);
	      gtk_clist_get_text (GTK_CLIST (gensetup_t->key_list), i, 1,
		  &szValue);

	      cour = gensetup_t->connstr;
	      gensetup_t->connstr =
		  (char *) malloc (size + STRLEN (szKey) + STRLEN (szValue) +
		  2);
	      if (gensetup_t->connstr)
		{
		  memcpy (gensetup_t->connstr, cour, size);
		  sprintf (gensetup_t->connstr + size, "%s=%s", szKey, szValue);
		  free (cour);
		  size += STRLEN (szKey) + STRLEN (szValue) + 2;
		}
	      else
		gensetup_t->connstr = cour;
	    }

	  /* add list-terminating NUL */
	  gensetup_t->connstr[size] = '\0';
	}

      gensetup_t->dsn_entry = gensetup_t->comment_entry = NULL;
      gensetup_t->key_list = NULL;

      gtk_signal_disconnect_by_func (GTK_OBJECT (gensetup_t->mainwnd),
	  GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
      gtk_main_quit ();
      gtk_widget_destroy (gensetup_t->mainwnd);
    }
}


static void
gensetup_cancel_clicked(GtkWidget* widget, TGENSETUP *gensetup_t)
{
  if (gensetup_t)
    {
      gensetup_t->connstr = (LPSTR) - 1L;

      gensetup_t->dsn_entry = gensetup_t->comment_entry = NULL;
      gensetup_t->key_list = NULL;

      gtk_signal_disconnect_by_func (GTK_OBJECT (gensetup_t->mainwnd),
	  GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
      gtk_main_quit ();
      gtk_widget_destroy (gensetup_t->mainwnd);
    }
}


static gint delete_event( GtkWidget *widget,
	GdkEvent *event, TGENSETUP *gensetup_t)
{
  gensetup_cancel_clicked (widget, gensetup_t);

  return FALSE;
}


LPSTR
create_gensetup (HWND hwnd, LPCSTR dsn, LPCSTR attrs, BOOL add)
{
  GtkWidget *dgensetup;
  GtkWidget *vbox42;
  GtkWidget *vbox43;
  GtkWidget *frame100;
  GtkWidget *alignment84;
  GtkWidget *label166;
  GtkWidget *frame103;
  GtkWidget *alignment87;
  GtkWidget *table11;
  GtkWidget *label171;
  GtkWidget *label172;
  GtkWidget *t_dsn;
  GtkWidget *t_comment;
  GtkWidget *frame101;
  GtkWidget *alignment85;
  GtkWidget *scrolledwindow22;
  GtkWidget *clist1;
  GtkWidget *label167;
  GtkWidget *label168;
  GtkWidget *frame102;
  GtkWidget *alignment86;
  GtkWidget *hbox58;
  GtkWidget *table10;
  GtkWidget *label169;
  GtkWidget *t_value;
  GtkWidget *b_update;
  GtkWidget *b_add;
  GtkWidget *t_keyword;
  GtkWidget *label170;
  GtkWidget *hbuttonbox4;
  GtkWidget *b_cancel;
  GtkWidget *b_ok;
  TGENSETUP gensetup_t;
  char buff[1024];

  if (hwnd == NULL || !GTK_IS_WIDGET (hwnd))
    return (LPSTR) attrs;

  dgensetup = gtk_dialog_new ();
  gtk_widget_set_name (dgensetup, "dgensetup");
  gtk_widget_set_size_request (dgensetup, 356, 445);
  sprintf (buff, "Setup of DSN %s ...", (dsn) ? dsn : "Unknown");
  gtk_window_set_title (GTK_WINDOW (dgensetup), buff);
  gtk_window_set_position (GTK_WINDOW (dgensetup), GTK_WIN_POS_CENTER);
  gtk_window_set_modal (GTK_WINDOW (dgensetup), TRUE);
  gtk_window_set_default_size (GTK_WINDOW (dgensetup), 600, 450);
  gtk_window_set_type_hint (GTK_WINDOW (dgensetup), GDK_WINDOW_TYPE_HINT_DIALOG);

#if GTK_CHECK_VERSION(2,0,0)
  gtk_widget_show (dgensetup);
#endif

  vbox42 = GTK_DIALOG (dgensetup)->vbox;
  gtk_widget_set_name (vbox42, "vbox42");
  gtk_widget_show (vbox42);

  vbox43 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox43, "vbox43");
  gtk_widget_show (vbox43);
  gtk_box_pack_start (GTK_BOX (vbox42), vbox43, TRUE, TRUE, 0);

  frame100 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame100, "frame100");
  gtk_widget_show (frame100);
  gtk_box_pack_start (GTK_BOX (vbox43), frame100, FALSE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame100), GTK_SHADOW_NONE);

  alignment84 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment84, "alignment84");
  gtk_widget_show (alignment84);
  gtk_container_add (GTK_CONTAINER (frame100), alignment84);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment84), 4, 4, 0, 0);

  label166 = gtk_label_new (_("Generic ODBC driver Setup"));
  gtk_widget_set_name (label166, "label166");
  gtk_widget_show (label166);
  gtk_container_add (GTK_CONTAINER (alignment84), label166);

  frame103 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame103, "frame103");
  gtk_widget_show (frame103);
  gtk_box_pack_start (GTK_BOX (vbox43), frame103, FALSE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame103), GTK_SHADOW_NONE);

  alignment87 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment87, "alignment87");
  gtk_widget_show (alignment87);
  gtk_container_add (GTK_CONTAINER (frame103), alignment87);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment87), 6, 0, 0, 0);

  table11 = gtk_table_new (2, 2, FALSE);
  gtk_widget_set_name (table11, "table11");
  gtk_widget_show (table11);
  gtk_container_add (GTK_CONTAINER (alignment87), table11);
  gtk_table_set_row_spacings (GTK_TABLE (table11), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table11), 4);

  label171 = gtk_label_new (_("Data Source Name (DSN) :"));
  gtk_widget_set_name (label171, "label171");
  gtk_widget_show (label171);
  gtk_table_attach (GTK_TABLE (table11), label171, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label171), 0, 0.5);

  label172 = gtk_label_new (_("                        Comment :"));
  gtk_widget_set_name (label172, "label172");
  gtk_widget_show (label172);
  gtk_table_attach (GTK_TABLE (table11), label172, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label172), 0, 0.5);

  t_dsn = gtk_entry_new ();
  gtk_widget_set_name (t_dsn, "t_dsn");
  gtk_widget_show (t_dsn);
  gtk_table_attach (GTK_TABLE (table11), t_dsn, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  t_comment = gtk_entry_new ();
  gtk_widget_set_name (t_comment, "t_comment");
  gtk_widget_show (t_comment);
  gtk_table_attach (GTK_TABLE (table11), t_comment, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  frame101 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame101, "frame101");
  gtk_widget_show (frame101);
  gtk_box_pack_start (GTK_BOX (vbox43), frame101, TRUE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame101), GTK_SHADOW_NONE);

  alignment85 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment85, "alignment85");
  gtk_widget_show (alignment85);
  gtk_container_add (GTK_CONTAINER (frame101), alignment85);

  scrolledwindow22 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_name (scrolledwindow22, "scrolledwindow22");
  gtk_widget_show (scrolledwindow22);
  gtk_container_add (GTK_CONTAINER (alignment85), scrolledwindow22);

  clist1 = gtk_clist_new (2);
  gtk_widget_set_name (clist1, "clist1");
  gtk_widget_show (clist1);
  gtk_container_add (GTK_CONTAINER (scrolledwindow22), clist1);
  gtk_clist_set_column_width (GTK_CLIST (clist1), 0, 137);
  gtk_clist_set_column_width (GTK_CLIST (clist1), 1, 80);
  gtk_clist_column_titles_show (GTK_CLIST (clist1));

  label167 = gtk_label_new (_("Keyword"));
  gtk_widget_set_name (label167, "label167");
  gtk_widget_show (label167);
  gtk_clist_set_column_widget (GTK_CLIST (clist1), 0, label167);
  gtk_widget_set_size_request (label167, 137, -1);

  label168 = gtk_label_new (_("Value"));
  gtk_widget_set_name (label168, "label168");
  gtk_widget_show (label168);
  gtk_clist_set_column_widget (GTK_CLIST (clist1), 1, label168);

  frame102 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame102, "frame102");
  gtk_widget_show (frame102);
  gtk_box_pack_start (GTK_BOX (vbox43), frame102, FALSE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame102), GTK_SHADOW_NONE);

  alignment86 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment86, "alignment86");
  gtk_widget_show (alignment86);
  gtk_container_add (GTK_CONTAINER (frame102), alignment86);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment86), 3, 0, 0, 0);

  hbox58 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox58, "hbox58");
  gtk_widget_show (hbox58);
  gtk_container_add (GTK_CONTAINER (alignment86), hbox58);

  table10 = gtk_table_new (2, 3, FALSE);
  gtk_widget_set_name (table10, "table10");
  gtk_widget_show (table10);
  gtk_box_pack_start (GTK_BOX (hbox58), table10, TRUE, TRUE, 0);

  label169 = gtk_label_new (_("Value : "));
  gtk_widget_set_name (label169, "label169");
  gtk_widget_show (label169);
  gtk_table_attach (GTK_TABLE (table10), label169, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  t_value = gtk_entry_new ();
  gtk_widget_set_name (t_value, "t_value");
  gtk_widget_show (t_value);
  gtk_table_attach (GTK_TABLE (table10), t_value, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  b_update = gtk_button_new_with_mnemonic (_("    _Update    "));
  gtk_widget_set_name (b_update, "b_update");
  gtk_widget_show (b_update);
  gtk_table_attach (GTK_TABLE (table10), b_update, 2, 3, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (b_update), 6);

  b_add = gtk_button_new_from_stock ("gtk-add");
  gtk_widget_set_name (b_add, "b_add");
  gtk_widget_show (b_add);
  gtk_table_attach (GTK_TABLE (table10), b_add, 2, 3, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (b_add), 6);

  t_keyword = gtk_entry_new ();
  gtk_widget_set_name (t_keyword, "t_keyword");
  gtk_widget_show (t_keyword);
  gtk_table_attach (GTK_TABLE (table10), t_keyword, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  label170 = gtk_label_new (_("Keyword : "));
  gtk_widget_set_name (label170, "label170");
  gtk_widget_show (label170);
  gtk_table_attach (GTK_TABLE (table10), label170, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  hbuttonbox4 = GTK_DIALOG (dgensetup)->action_area;
  gtk_widget_set_name (hbuttonbox4, "hbuttonbox4");
  gtk_widget_show (hbuttonbox4);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox4), GTK_BUTTONBOX_END);

  b_cancel = gtk_button_new_from_stock ("gtk-cancel");
  gtk_widget_set_name (b_cancel, "b_cancel");
  gtk_widget_show (b_cancel);
  gtk_dialog_add_action_widget (GTK_DIALOG (dgensetup), b_cancel, GTK_RESPONSE_CANCEL);
  GTK_WIDGET_SET_FLAGS (b_cancel, GTK_CAN_DEFAULT);

  b_ok = gtk_button_new_from_stock ("gtk-ok");
  gtk_widget_set_name (b_ok, "b_ok");
  gtk_widget_show (b_ok);
  gtk_dialog_add_action_widget (GTK_DIALOG (dgensetup), b_ok, GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS (b_ok, GTK_CAN_DEFAULT);

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF (dgensetup, dgensetup, "dgensetup");
  GLADE_HOOKUP_OBJECT_NO_REF (dgensetup, vbox42, "vbox42");
  GLADE_HOOKUP_OBJECT (dgensetup, vbox43, "vbox43");
  GLADE_HOOKUP_OBJECT (dgensetup, frame100, "frame100");
  GLADE_HOOKUP_OBJECT (dgensetup, alignment84, "alignment84");
  GLADE_HOOKUP_OBJECT (dgensetup, label166, "label166");
  GLADE_HOOKUP_OBJECT (dgensetup, frame103, "frame103");
  GLADE_HOOKUP_OBJECT (dgensetup, alignment87, "alignment87");
  GLADE_HOOKUP_OBJECT (dgensetup, table11, "table11");
  GLADE_HOOKUP_OBJECT (dgensetup, label171, "label171");
  GLADE_HOOKUP_OBJECT (dgensetup, label172, "label172");
  GLADE_HOOKUP_OBJECT (dgensetup, t_dsn, "t_dsn");
  GLADE_HOOKUP_OBJECT (dgensetup, t_comment, "t_comment");
  GLADE_HOOKUP_OBJECT (dgensetup, frame101, "frame101");
  GLADE_HOOKUP_OBJECT (dgensetup, alignment85, "alignment85");
  GLADE_HOOKUP_OBJECT (dgensetup, scrolledwindow22, "scrolledwindow22");
  GLADE_HOOKUP_OBJECT (dgensetup, clist1, "clist1");
  GLADE_HOOKUP_OBJECT (dgensetup, label167, "label167");
  GLADE_HOOKUP_OBJECT (dgensetup, label168, "label168");
  GLADE_HOOKUP_OBJECT (dgensetup, frame102, "frame102");
  GLADE_HOOKUP_OBJECT (dgensetup, alignment86, "alignment86");
  GLADE_HOOKUP_OBJECT (dgensetup, hbox58, "hbox58");
  GLADE_HOOKUP_OBJECT (dgensetup, table10, "table10");
  GLADE_HOOKUP_OBJECT (dgensetup, label169, "label169");
  GLADE_HOOKUP_OBJECT (dgensetup, t_value, "t_value");
  GLADE_HOOKUP_OBJECT (dgensetup, b_update, "b_update");
  GLADE_HOOKUP_OBJECT (dgensetup, b_add, "b_add");
  GLADE_HOOKUP_OBJECT (dgensetup, t_keyword, "t_keyword");
  GLADE_HOOKUP_OBJECT (dgensetup, label170, "label170");
  GLADE_HOOKUP_OBJECT_NO_REF (dgensetup, hbuttonbox4, "hbuttonbox4");
  GLADE_HOOKUP_OBJECT (dgensetup, b_cancel, "b_cancel");
  GLADE_HOOKUP_OBJECT (dgensetup, b_ok, "b_ok");

  /* Ok button events */
  gtk_signal_connect (GTK_OBJECT (b_ok), "clicked",
      GTK_SIGNAL_FUNC (gensetup_ok_clicked), &gensetup_t);
  /* Cancel button events */
  gtk_signal_connect (GTK_OBJECT (b_cancel), "clicked",
      GTK_SIGNAL_FUNC (gensetup_cancel_clicked), &gensetup_t);
  /* Add button events */
  gtk_signal_connect (GTK_OBJECT (b_add), "clicked",
      GTK_SIGNAL_FUNC (gensetup_add_clicked), &gensetup_t);
  /* Update button events */
  gtk_signal_connect (GTK_OBJECT (b_update), "clicked",
      GTK_SIGNAL_FUNC (gensetup_update_clicked), &gensetup_t);
  /* Close window button events */
  gtk_signal_connect (GTK_OBJECT (dgensetup), "delete_event",
      GTK_SIGNAL_FUNC (delete_event), &gensetup_t);
  gtk_signal_connect (GTK_OBJECT (dgensetup), "destroy",
      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
  /* List events */
  gtk_signal_connect (GTK_OBJECT (clist1), "select_row",
      GTK_SIGNAL_FUNC (gensetup_list_select), &gensetup_t);
  gtk_signal_connect (GTK_OBJECT (clist1), "unselect_row",
      GTK_SIGNAL_FUNC (gensetup_list_unselect), &gensetup_t);

  gensetup_t.dsn_entry = t_dsn;
  gensetup_t.comment_entry = t_comment;
  gensetup_t.key_list = clist1;
  gensetup_t.bupdate = b_update;
  gensetup_t.key_entry = t_keyword;
  gensetup_t.value_entry = t_value;
  gensetup_t.mainwnd = dgensetup;

  /* Parse the attributes line */
  parse_attribute_line (&gensetup_t, dsn, attrs, add);

  gtk_widget_show_all (dgensetup);
  gtk_main ();

  return gensetup_t.connstr;
}
