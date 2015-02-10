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
  if (dsn && gensetup_t->dsn_entry)
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
      if (gensetup_t->dsn_entry)
        {
          size +=
	      STRLEN (gtk_entry_get_text (GTK_ENTRY (gensetup_t->dsn_entry))) +
	      STRLEN ("DSN=") + 1;
          size += STRLEN ("Description=") + 1;
        }
      else
        {
          size = 1;
        }
      /* Malloc it (+1 for list-terminating NUL) */
      if ((gensetup_t->connstr = (char *) calloc (sizeof(char), ++size)))
	{
	  if (gensetup_t->dsn_entry)
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
		    sprintf (cour, curr, "");
		    cour += (STRLEN (cour) + 1);
		    break;
	          };
	    }
	  else
	    size = 1;

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
		  sprintf (gensetup_t->connstr + size - 1, "%s=%s", szKey, szValue);
		  free (cour);
		  size += STRLEN (szKey) + STRLEN (szValue) + 2;
		}
	      else
		gensetup_t->connstr = cour;
	    }

	  /* add list-terminating NUL */
	  gensetup_t->connstr[size - 1] = '\0';
	}

      gensetup_t->dsn_entry = NULL;
      gensetup_t->key_list = NULL;
      gensetup_t->verify_conn = gtk_toggle_button_get_active((GtkToggleButton*)gensetup_t->verify_conn_cb);

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
      gensetup_t->connstr = (LPSTR) -1L;

      gensetup_t->dsn_entry = NULL;
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
create_fgensetup (HWND hwnd, LPCSTR dsn, LPCSTR attrs, BOOL add, BOOL *verify_conn)
{
  GdkPixmap *pixmap;
  GdkBitmap *mask;
  GtkStyle *style;

  GtkWidget *gensetup;
  GtkWidget *dialog_vbox5;
  GtkWidget *vbox25;
  GtkWidget *frame56;
  GtkWidget *alignment48;
  GtkWidget *hbox35;
  GtkWidget *label93;
  GtkWidget *t_dsn;
  GtkWidget *frame55;
  GtkWidget *alignment47;
  GtkWidget *label92;
  GtkWidget *frame54;
  GtkWidget *alignment46;
  GtkWidget *scrolledwindow15;
  GtkWidget *clist1;
  GtkWidget *l_key;
  GtkWidget *l_value;
  GtkWidget *frame53;
  GtkWidget *alignment45;
  GtkWidget *hbox34;
  GtkWidget *table4;
  GtkWidget *label88;
  GtkWidget *t_value;
  GtkWidget *b_update;
  GtkWidget *b_add;
  GtkWidget *t_keyword;
  GtkWidget *label89;
  GtkWidget *frame52;
  GtkWidget *alignment44;
  GtkWidget *hbox33;
  GtkWidget *cb_verify;
  GtkWidget *dialog_action_area5;
  GtkWidget *b_cancel;
  GtkWidget *b_ok;
  TGENSETUP gensetup_t;
  char buff[1024];

  if (hwnd == NULL || !GTK_IS_WIDGET (hwnd))
    return (LPSTR) attrs;

  gensetup = gtk_dialog_new ();
  gtk_widget_set_name (gensetup, "gensetup");
  gtk_widget_set_size_request (gensetup, 354, 471);
  gtk_window_set_title (GTK_WINDOW (gensetup), _("File DSN Generic Setup"));
  gtk_window_set_position (GTK_WINDOW (gensetup), GTK_WIN_POS_CENTER);
  gtk_window_set_modal (GTK_WINDOW (gensetup), TRUE);
  gtk_window_set_type_hint (GTK_WINDOW (gensetup), GDK_WINDOW_TYPE_HINT_DIALOG);

#if GTK_CHECK_VERSION(2,0,0)
  gtk_widget_show (gensetup);
#endif
  
  dialog_vbox5 = GTK_DIALOG (gensetup)->vbox;
  gtk_widget_set_name (dialog_vbox5, "dialog_vbox5");
  gtk_widget_show (dialog_vbox5);

  vbox25 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox25, "vbox25");
  gtk_widget_show (vbox25);
  gtk_box_pack_start (GTK_BOX (dialog_vbox5), vbox25, TRUE, TRUE, 0);

  frame56 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame56, "frame56");
  gtk_widget_show (frame56);
  gtk_box_pack_start (GTK_BOX (vbox25), frame56, FALSE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame56), GTK_SHADOW_NONE);

  alignment48 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment48, "alignment48");
  gtk_widget_show (alignment48);
  gtk_container_add (GTK_CONTAINER (frame56), alignment48);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment48), 4, 0, 0, 0);

  hbox35 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox35, "hbox35");
  gtk_widget_show (hbox35);
  gtk_container_add (GTK_CONTAINER (alignment48), hbox35);

  label93 = gtk_label_new (_("File Data Source Name :    "));
  gtk_widget_set_name (label93, "label93");
  gtk_widget_show (label93);
  gtk_box_pack_start (GTK_BOX (hbox35), label93, FALSE, FALSE, 0);

  t_dsn = gtk_entry_new ();
  gtk_widget_set_name (t_dsn, "t_dsn");
  gtk_widget_show (t_dsn);
  gtk_box_pack_start (GTK_BOX (hbox35), t_dsn, TRUE, TRUE, 0);

  frame55 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame55, "frame55");
  gtk_widget_show (frame55);
  gtk_box_pack_start (GTK_BOX (vbox25), frame55, FALSE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame55), GTK_SHADOW_NONE);

  alignment47 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment47, "alignment47");
  gtk_widget_show (alignment47);
  gtk_container_add (GTK_CONTAINER (frame55), alignment47);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment47), 4, 4, 0, 0);

  label92 = gtk_label_new (_("If you know the driver-specific keywords for this data\nsource, you can type them and their values here. For\nmore information on driver-specific keywords, please\nconsult your ODBC driver documentation."));
  gtk_widget_set_name (label92, "label92");
  gtk_widget_show (label92);
  gtk_container_add (GTK_CONTAINER (alignment47), label92);

  frame54 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame54, "frame54");
  gtk_widget_show (frame54);
  gtk_box_pack_start (GTK_BOX (vbox25), frame54, TRUE, TRUE, 0);
  gtk_widget_set_size_request (frame54, -1, 180);
  gtk_frame_set_shadow_type (GTK_FRAME (frame54), GTK_SHADOW_NONE);

  alignment46 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment46, "alignment46");
  gtk_widget_show (alignment46);
  gtk_container_add (GTK_CONTAINER (frame54), alignment46);

  scrolledwindow15 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_name (scrolledwindow15, "scrolledwindow15");
  gtk_widget_show (scrolledwindow15);
  gtk_container_add (GTK_CONTAINER (alignment46), scrolledwindow15);

  clist1 = gtk_clist_new (2);
  gtk_widget_set_name (clist1, "clist1");
  gtk_widget_show (clist1);
  gtk_container_add (GTK_CONTAINER (scrolledwindow15), clist1);
  gtk_clist_set_column_width (GTK_CLIST (clist1), 0, 80);
  gtk_clist_set_column_width (GTK_CLIST (clist1), 1, 80);
  gtk_clist_column_titles_show (GTK_CLIST (clist1));

  l_key = gtk_label_new (_("Keyword"));
  gtk_widget_set_name (l_key, "l_key");
  gtk_widget_show (l_key);
  gtk_clist_set_column_widget (GTK_CLIST (clist1), 0, l_key);
  gtk_widget_set_size_request (l_key, 137, -1);

  l_value = gtk_label_new (_("Value"));
  gtk_widget_set_name (l_value, "l_value");
  gtk_widget_show (l_value);
  gtk_clist_set_column_widget (GTK_CLIST (clist1), 1, l_value);

  frame53 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame53, "frame53");
  gtk_widget_show (frame53);
  gtk_box_pack_start (GTK_BOX (vbox25), frame53, FALSE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame53), GTK_SHADOW_NONE);

  alignment45 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment45, "alignment45");
  gtk_widget_show (alignment45);
  gtk_container_add (GTK_CONTAINER (frame53), alignment45);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment45), 3, 0, 0, 0);

  hbox34 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox34, "hbox34");
  gtk_widget_show (hbox34);
  gtk_container_add (GTK_CONTAINER (alignment45), hbox34);

  table4 = gtk_table_new (2, 3, FALSE);
  gtk_widget_set_name (table4, "table4");
  gtk_widget_show (table4);
  gtk_box_pack_start (GTK_BOX (hbox34), table4, TRUE, TRUE, 0);

  label88 = gtk_label_new (_("Value : "));
  gtk_widget_set_name (label88, "label88");
  gtk_widget_show (label88);
  gtk_table_attach (GTK_TABLE (table4), label88, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  t_value = gtk_entry_new ();
  gtk_widget_set_name (t_value, "t_value");
  gtk_widget_show (t_value);
  gtk_table_attach (GTK_TABLE (table4), t_value, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  b_update = gtk_button_new_with_mnemonic (_("    _Update    "));
  gtk_widget_set_name (b_update, "b_update");
  gtk_widget_show (b_update);
  gtk_table_attach (GTK_TABLE (table4), b_update, 2, 3, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (b_update), 6);
  gtk_widget_set_sensitive(b_update, FALSE);

  b_add = gtk_button_new_from_stock ("gtk-add");
  gtk_widget_set_name (b_add, "b_add");
  gtk_widget_show (b_add);
  gtk_table_attach (GTK_TABLE (table4), b_add, 2, 3, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (b_add), 6);

  t_keyword = gtk_entry_new ();
  gtk_widget_set_name (t_keyword, "t_keyword");
  gtk_widget_show (t_keyword);
  gtk_table_attach (GTK_TABLE (table4), t_keyword, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  label89 = gtk_label_new (_("Keyword : "));
  gtk_widget_set_name (label89, "label89");
  gtk_widget_show (label89);
  gtk_table_attach (GTK_TABLE (table4), label89, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  frame52 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame52, "frame52");
  gtk_widget_show (frame52);
  gtk_box_pack_start (GTK_BOX (vbox25), frame52, FALSE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame52), GTK_SHADOW_NONE);

  alignment44 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment44, "alignment44");
  gtk_widget_show (alignment44);
  gtk_container_add (GTK_CONTAINER (frame52), alignment44);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment44), 0, 3, 12, 0);

  hbox33 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox33, "hbox33");
  gtk_widget_show (hbox33);
  gtk_container_add (GTK_CONTAINER (alignment44), hbox33);

  cb_verify = gtk_check_button_new_with_mnemonic (_("Verify this connection (recommended)"));
  gtk_widget_set_name (cb_verify, "cb_verify");
  gtk_widget_show (cb_verify);
  gtk_box_pack_start (GTK_BOX (hbox33), cb_verify, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb_verify), TRUE);

  dialog_action_area5 = GTK_DIALOG (gensetup)->action_area;
  gtk_widget_set_name (dialog_action_area5, "dialog_action_area5");
  gtk_widget_show (dialog_action_area5);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area5), GTK_BUTTONBOX_END);

  b_cancel = gtk_button_new_from_stock ("gtk-cancel");
  gtk_widget_set_name (b_cancel, "b_cancel");
  gtk_widget_show (b_cancel);
  gtk_dialog_add_action_widget (GTK_DIALOG (gensetup), b_cancel, GTK_RESPONSE_CANCEL);
  GTK_WIDGET_SET_FLAGS (b_cancel, GTK_CAN_DEFAULT);

  b_ok = gtk_button_new_from_stock ("gtk-ok");
  gtk_widget_set_name (b_ok, "b_ok");
  gtk_widget_show (b_ok);
  gtk_dialog_add_action_widget (GTK_DIALOG (gensetup), b_ok, GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS (b_ok, GTK_CAN_DEFAULT);

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF (gensetup, gensetup, "gensetup");
  GLADE_HOOKUP_OBJECT_NO_REF (gensetup, dialog_vbox5, "dialog_vbox5");
  GLADE_HOOKUP_OBJECT (gensetup, vbox25, "vbox25");
  GLADE_HOOKUP_OBJECT (gensetup, frame56, "frame56");
  GLADE_HOOKUP_OBJECT (gensetup, alignment48, "alignment48");
  GLADE_HOOKUP_OBJECT (gensetup, hbox35, "hbox35");
  GLADE_HOOKUP_OBJECT (gensetup, label93, "label93");
  GLADE_HOOKUP_OBJECT (gensetup, t_dsn, "t_dsn");
  GLADE_HOOKUP_OBJECT (gensetup, frame55, "frame55");
  GLADE_HOOKUP_OBJECT (gensetup, alignment47, "alignment47");
  GLADE_HOOKUP_OBJECT (gensetup, label92, "label92");
  GLADE_HOOKUP_OBJECT (gensetup, frame54, "frame54");
  GLADE_HOOKUP_OBJECT (gensetup, alignment46, "alignment46");
  GLADE_HOOKUP_OBJECT (gensetup, scrolledwindow15, "scrolledwindow15");
  GLADE_HOOKUP_OBJECT (gensetup, clist1, "clist1");
  GLADE_HOOKUP_OBJECT (gensetup, l_key, "l_key");
  GLADE_HOOKUP_OBJECT (gensetup, l_value, "l_value");
  GLADE_HOOKUP_OBJECT (gensetup, frame53, "frame53");
  GLADE_HOOKUP_OBJECT (gensetup, alignment45, "alignment45");
  GLADE_HOOKUP_OBJECT (gensetup, hbox34, "hbox34");
  GLADE_HOOKUP_OBJECT (gensetup, table4, "table4");
  GLADE_HOOKUP_OBJECT (gensetup, label88, "label88");
  GLADE_HOOKUP_OBJECT (gensetup, t_value, "t_value");
  GLADE_HOOKUP_OBJECT (gensetup, b_update, "b_update");
  GLADE_HOOKUP_OBJECT (gensetup, b_add, "b_add");
  GLADE_HOOKUP_OBJECT (gensetup, t_keyword, "t_keyword");
  GLADE_HOOKUP_OBJECT (gensetup, label89, "label89");
  GLADE_HOOKUP_OBJECT (gensetup, frame52, "frame52");
  GLADE_HOOKUP_OBJECT (gensetup, alignment44, "alignment44");
  GLADE_HOOKUP_OBJECT (gensetup, hbox33, "hbox33");
  GLADE_HOOKUP_OBJECT (gensetup, cb_verify, "cb_verify");
  GLADE_HOOKUP_OBJECT_NO_REF (gensetup, dialog_action_area5, "dialog_action_area5");
  GLADE_HOOKUP_OBJECT (gensetup, b_cancel, "b_cancel");
  GLADE_HOOKUP_OBJECT (gensetup, b_ok, "b_ok");

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
  gtk_signal_connect (GTK_OBJECT (gensetup), "delete_event",
      GTK_SIGNAL_FUNC (delete_event), &gensetup_t);
  gtk_signal_connect (GTK_OBJECT (gensetup), "destroy",
      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
  /* List events */
  gtk_signal_connect (GTK_OBJECT (clist1), "select_row",
      GTK_SIGNAL_FUNC (gensetup_list_select), &gensetup_t);
  gtk_signal_connect (GTK_OBJECT (clist1), "unselect_row",
      GTK_SIGNAL_FUNC (gensetup_list_unselect), &gensetup_t);

  gensetup_t.dsn_entry = t_dsn;
  gensetup_t.key_list = clist1;
  gensetup_t.bupdate = b_update;
  gensetup_t.key_entry = t_keyword;
  gensetup_t.value_entry = t_value;
  gensetup_t.mainwnd = gensetup;
  gensetup_t.verify_conn_cb = cb_verify;
  gensetup_t.verify_conn = *verify_conn;

  gtk_toggle_button_set_active((GtkToggleButton*)cb_verify, *verify_conn);

  /* Parse the attributes line */
  parse_attribute_line (&gensetup_t, dsn, attrs, add);

  gtk_widget_show_all (gensetup);
  gtk_main ();

  *verify_conn = gensetup_t.verify_conn;

  return gensetup_t.connstr;
}



LPSTR
create_keyval (HWND hwnd, LPCSTR attrs, BOOL *verify_conn)
{
  GdkPixmap *pixmap;
  GdkBitmap *mask;
  GtkStyle *style;

  GtkWidget *genkeyval;
  GtkWidget *dialog_vbox4;
  GtkWidget *vbox24;
  GtkWidget *frame48;
  GtkWidget *alignment40;
  GtkWidget *label81;
  GtkWidget *frame49;
  GtkWidget *alignment41;
  GtkWidget *scrolledwindow14;
  GtkWidget *clist1;
  GtkWidget *l_key;
  GtkWidget *l_value;
  GtkWidget *frame50;
  GtkWidget *alignment42;
  GtkWidget *hbox31;
  GtkWidget *table3;
  GtkWidget *label86;
  GtkWidget *t_value;
  GtkWidget *b_update;
  GtkWidget *b_add;
  GtkWidget *t_keyword;
  GtkWidget *label87;
  GtkWidget *frame51;
  GtkWidget *alignment43;
  GtkWidget *hbox32;
  GtkWidget *cb_verify;
  GtkWidget *dialog_action_area4;
  GtkWidget *cancelbutton2;
  GtkWidget *okbutton2;

  TGENSETUP gensetup_t;
  char buff[1024];

  if (hwnd == NULL || !GTK_IS_WIDGET (hwnd))
    return (LPSTR) attrs;

  genkeyval = gtk_dialog_new ();
  gtk_widget_set_name (genkeyval, "genkeyval");
  gtk_widget_set_size_request (genkeyval, 355, 430);
  gtk_window_set_title (GTK_WINDOW (genkeyval), _("Advanced File DSN Creation Settings"));
  gtk_window_set_position (GTK_WINDOW (genkeyval), GTK_WIN_POS_CENTER);
  gtk_window_set_modal (GTK_WINDOW (genkeyval), TRUE);
  gtk_window_set_default_size (GTK_WINDOW (genkeyval), 355, 430);
  gtk_window_set_type_hint (GTK_WINDOW (genkeyval), GDK_WINDOW_TYPE_HINT_DIALOG);

#if GTK_CHECK_VERSION(2,0,0)
  gtk_widget_show (genkeyval);
#endif

  dialog_vbox4 = GTK_DIALOG (genkeyval)->vbox;
  gtk_widget_set_name (dialog_vbox4, "dialog_vbox4");
  gtk_widget_show (dialog_vbox4);

  vbox24 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox24, "vbox24");
  gtk_widget_show (vbox24);
  gtk_box_pack_start (GTK_BOX (dialog_vbox4), vbox24, TRUE, TRUE, 0);

  frame48 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame48, "frame48");
  gtk_widget_show (frame48);
  gtk_box_pack_start (GTK_BOX (vbox24), frame48, FALSE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame48), GTK_SHADOW_NONE);

  alignment40 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment40, "alignment40");
  gtk_widget_show (alignment40);
  gtk_container_add (GTK_CONTAINER (frame48), alignment40);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment40), 4, 4, 0, 0);

  label81 = gtk_label_new (_("If you know the driver-specific keywords for this data\nsource, you can type them and their values here. For\nmore information on driver-specific keywords, please\nconsult your ODBC driver documentation."));
  gtk_widget_set_name (label81, "label81");
  gtk_widget_show (label81);
  gtk_container_add (GTK_CONTAINER (alignment40), label81);

  frame49 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame49, "frame49");
  gtk_widget_show (frame49);
  gtk_box_pack_start (GTK_BOX (vbox24), frame49, TRUE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame49), GTK_SHADOW_NONE);

  alignment41 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment41, "alignment41");
  gtk_widget_show (alignment41);
  gtk_container_add (GTK_CONTAINER (frame49), alignment41);

  scrolledwindow14 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_name (scrolledwindow14, "scrolledwindow14");
  gtk_widget_show (scrolledwindow14);
  gtk_container_add (GTK_CONTAINER (alignment41), scrolledwindow14);

  clist1 = gtk_clist_new (2);
  gtk_widget_set_name (clist1, "clist1");
  gtk_widget_show (clist1);
  gtk_container_add (GTK_CONTAINER (scrolledwindow14), clist1);
  gtk_clist_set_column_width (GTK_CLIST (clist1), 0, 80);
  gtk_clist_set_column_width (GTK_CLIST (clist1), 1, 80);
  gtk_clist_column_titles_show (GTK_CLIST (clist1));

  l_key = gtk_label_new (_("Keyword"));
  gtk_widget_set_name (l_key, "l_key");
  gtk_widget_show (l_key);
  gtk_clist_set_column_widget (GTK_CLIST (clist1), 0, l_key);
  gtk_widget_set_size_request (l_key, 137, -1);

  l_value = gtk_label_new (_("Value"));
  gtk_widget_set_name (l_value, "l_value");
  gtk_widget_show (l_value);
  gtk_clist_set_column_widget (GTK_CLIST (clist1), 1, l_value);

  frame50 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame50, "frame50");
  gtk_widget_show (frame50);
  gtk_box_pack_start (GTK_BOX (vbox24), frame50, FALSE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame50), GTK_SHADOW_NONE);

  alignment42 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment42, "alignment42");
  gtk_widget_show (alignment42);
  gtk_container_add (GTK_CONTAINER (frame50), alignment42);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment42), 3, 0, 0, 0);

  hbox31 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox31, "hbox31");
  gtk_widget_show (hbox31);
  gtk_container_add (GTK_CONTAINER (alignment42), hbox31);

  table3 = gtk_table_new (2, 3, FALSE);
  gtk_widget_set_name (table3, "table3");
  gtk_widget_show (table3);
  gtk_box_pack_start (GTK_BOX (hbox31), table3, TRUE, TRUE, 0);

  label86 = gtk_label_new (_("Value : "));
  gtk_widget_set_name (label86, "label86");
  gtk_widget_show (label86);
  gtk_table_attach (GTK_TABLE (table3), label86, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  t_value = gtk_entry_new ();
  gtk_widget_set_name (t_value, "t_value");
  gtk_widget_show (t_value);
  gtk_table_attach (GTK_TABLE (table3), t_value, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  b_update = gtk_button_new_with_mnemonic (_("    _Update    "));
  gtk_widget_set_name (b_update, "b_update");
  gtk_widget_show (b_update);
  gtk_table_attach (GTK_TABLE (table3), b_update, 2, 3, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (b_update), 6);
  gtk_widget_set_sensitive(b_update, FALSE);

  b_add = gtk_button_new_from_stock ("gtk-add");
  gtk_widget_set_name (b_add, "b_add");
  gtk_widget_show (b_add);
  gtk_table_attach (GTK_TABLE (table3), b_add, 2, 3, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (b_add), 6);

  t_keyword = gtk_entry_new ();
  gtk_widget_set_name (t_keyword, "t_keyword");
  gtk_widget_show (t_keyword);
  gtk_table_attach (GTK_TABLE (table3), t_keyword, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  label87 = gtk_label_new (_("Keyword : "));
  gtk_widget_set_name (label87, "label87");
  gtk_widget_show (label87);
  gtk_table_attach (GTK_TABLE (table3), label87, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  frame51 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame51, "frame51");
  gtk_widget_show (frame51);
  gtk_box_pack_start (GTK_BOX (vbox24), frame51, FALSE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame51), GTK_SHADOW_NONE);

  alignment43 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment43, "alignment43");
  gtk_widget_show (alignment43);
  gtk_container_add (GTK_CONTAINER (frame51), alignment43);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment43), 0, 3, 12, 0);

  hbox32 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox32, "hbox32");
  gtk_widget_show (hbox32);
  gtk_container_add (GTK_CONTAINER (alignment43), hbox32);

  cb_verify = gtk_check_button_new_with_mnemonic (_("Verify this connection (recommended)"));
  gtk_widget_set_name (cb_verify, "cb_verify");
  gtk_widget_show (cb_verify);
  gtk_box_pack_start (GTK_BOX (hbox32), cb_verify, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb_verify), TRUE);

  dialog_action_area4 = GTK_DIALOG (genkeyval)->action_area;
  gtk_widget_set_name (dialog_action_area4, "dialog_action_area4");
  gtk_widget_show (dialog_action_area4);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area4), GTK_BUTTONBOX_END);

  cancelbutton2 = gtk_button_new_from_stock ("gtk-cancel");
  gtk_widget_set_name (cancelbutton2, "cancelbutton2");
  gtk_widget_show (cancelbutton2);
  gtk_dialog_add_action_widget (GTK_DIALOG (genkeyval), cancelbutton2, GTK_RESPONSE_CANCEL);
  GTK_WIDGET_SET_FLAGS (cancelbutton2, GTK_CAN_DEFAULT);

  okbutton2 = gtk_button_new_from_stock ("gtk-ok");
  gtk_widget_set_name (okbutton2, "okbutton2");
  gtk_widget_show (okbutton2);
  gtk_dialog_add_action_widget (GTK_DIALOG (genkeyval), okbutton2, GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS (okbutton2, GTK_CAN_DEFAULT);

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF (genkeyval, genkeyval, "genkeyval");
  GLADE_HOOKUP_OBJECT_NO_REF (genkeyval, dialog_vbox4, "dialog_vbox4");
  GLADE_HOOKUP_OBJECT (genkeyval, vbox24, "vbox24");
  GLADE_HOOKUP_OBJECT (genkeyval, frame48, "frame48");
  GLADE_HOOKUP_OBJECT (genkeyval, alignment40, "alignment40");
  GLADE_HOOKUP_OBJECT (genkeyval, label81, "label81");
  GLADE_HOOKUP_OBJECT (genkeyval, frame49, "frame49");
  GLADE_HOOKUP_OBJECT (genkeyval, alignment41, "alignment41");
  GLADE_HOOKUP_OBJECT (genkeyval, scrolledwindow14, "scrolledwindow14");
  GLADE_HOOKUP_OBJECT (genkeyval, clist1, "clist1");
  GLADE_HOOKUP_OBJECT (genkeyval, l_key, "l_key");
  GLADE_HOOKUP_OBJECT (genkeyval, l_value, "l_value");
  GLADE_HOOKUP_OBJECT (genkeyval, frame50, "frame50");
  GLADE_HOOKUP_OBJECT (genkeyval, alignment42, "alignment42");
  GLADE_HOOKUP_OBJECT (genkeyval, hbox31, "hbox31");
  GLADE_HOOKUP_OBJECT (genkeyval, table3, "table3");
  GLADE_HOOKUP_OBJECT (genkeyval, label86, "label86");
  GLADE_HOOKUP_OBJECT (genkeyval, t_value, "t_value");
  GLADE_HOOKUP_OBJECT (genkeyval, b_update, "b_update");
  GLADE_HOOKUP_OBJECT (genkeyval, b_add, "b_add");
  GLADE_HOOKUP_OBJECT (genkeyval, t_keyword, "t_keyword");
  GLADE_HOOKUP_OBJECT (genkeyval, label87, "label87");
  GLADE_HOOKUP_OBJECT (genkeyval, frame51, "frame51");
  GLADE_HOOKUP_OBJECT (genkeyval, alignment43, "alignment43");
  GLADE_HOOKUP_OBJECT (genkeyval, hbox32, "hbox32");
  GLADE_HOOKUP_OBJECT (genkeyval, cb_verify, "cb_verify");
  GLADE_HOOKUP_OBJECT_NO_REF (genkeyval, dialog_action_area4, "dialog_action_area4");
  GLADE_HOOKUP_OBJECT (genkeyval, cancelbutton2, "cancelbutton2");
  GLADE_HOOKUP_OBJECT (genkeyval, okbutton2, "okbutton2");

  /* Ok button events */
  gtk_signal_connect (GTK_OBJECT (okbutton2), "clicked",
      GTK_SIGNAL_FUNC (gensetup_ok_clicked), &gensetup_t);
  /* Cancel button events */
  gtk_signal_connect (GTK_OBJECT (cancelbutton2), "clicked",
      GTK_SIGNAL_FUNC (gensetup_cancel_clicked), &gensetup_t);
  /* Add button events */
  gtk_signal_connect (GTK_OBJECT (b_add), "clicked",
      GTK_SIGNAL_FUNC (gensetup_add_clicked), &gensetup_t);
  /* Update button events */
  gtk_signal_connect (GTK_OBJECT (b_update), "clicked",
      GTK_SIGNAL_FUNC (gensetup_update_clicked), &gensetup_t);
  /* Close window button events */
  gtk_signal_connect (GTK_OBJECT (genkeyval), "delete_event",
      GTK_SIGNAL_FUNC (delete_event), &gensetup_t);
  gtk_signal_connect (GTK_OBJECT (genkeyval), "destroy",
      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
  /* List events */
  gtk_signal_connect (GTK_OBJECT (clist1), "select_row",
      GTK_SIGNAL_FUNC (gensetup_list_select), &gensetup_t);
  gtk_signal_connect (GTK_OBJECT (clist1), "unselect_row",
      GTK_SIGNAL_FUNC (gensetup_list_unselect), &gensetup_t);

//  gtk_window_add_accel_group (GTK_WINDOW (gensetup), accel_group);

  gensetup_t.dsn_entry = NULL;
  gensetup_t.key_list = clist1;
  gensetup_t.bupdate = b_update;
  gensetup_t.key_entry = t_keyword;
  gensetup_t.value_entry = t_value;
  gensetup_t.mainwnd = genkeyval;
  gensetup_t.verify_conn_cb = cb_verify;
  gensetup_t.verify_conn = *verify_conn;

  gtk_toggle_button_set_active((GtkToggleButton*)cb_verify, *verify_conn);

  /* Parse the attributes line */
  parse_attribute_line (&gensetup_t, NULL, attrs, TRUE);

  gtk_widget_show_all (genkeyval);
  gtk_main ();

  *verify_conn = gensetup_t.verify_conn;

  return gensetup_t.connstr;
}
