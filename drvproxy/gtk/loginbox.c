/*
 *  loginbox.c
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


static void
login_ok_clicked (GtkWidget *widget, TLOGIN *log_t)
{
  if (log_t)
    {
      log_t->user = (char *) malloc (sizeof (char) *
	  (STRLEN (gtk_entry_get_text (GTK_ENTRY (log_t->username))) + 1));
      log_t->pwd = (char *) malloc (sizeof (char) *
	  (STRLEN (gtk_entry_get_text (GTK_ENTRY (log_t->password))) + 1));

      if (log_t->user)
	strcpy (log_t->user,
	    gtk_entry_get_text (GTK_ENTRY (log_t->username)));
      if (log_t->pwd)
	strcpy (log_t->pwd, gtk_entry_get_text (GTK_ENTRY (log_t->password)));

      log_t->username = log_t->password = NULL;
      log_t->ok = TRUE;

      gtk_signal_disconnect_by_func (GTK_OBJECT (log_t->mainwnd),
	  GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
      gtk_main_quit ();
      gtk_widget_destroy (log_t->mainwnd);
    }
}


static void
login_cancel_clicked (GtkWidget *widget, TLOGIN *log_t)
{
  if (log_t)
    {
      log_t->user = log_t->pwd = NULL;
      log_t->username = log_t->password = NULL;
      log_t->ok = FALSE;

      gtk_signal_disconnect_by_func (GTK_OBJECT (log_t->mainwnd),
	  GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
      gtk_main_quit ();
      gtk_widget_destroy (log_t->mainwnd);
    }
}


static gint
delete_event (GtkWidget *widget, GdkEvent *event, TLOGIN *log_t)
{
  login_cancel_clicked (widget, log_t);

  return FALSE;
}


void
create_login (HWND hwnd, LPCSTR username, LPCSTR password, LPCSTR dsn,
    TLOGIN *log_t)
{
  GtkWidget *login;
  GtkWidget *dialog_vbox8;
  GtkWidget *frame99;
  GtkWidget *alignment83;
  GtkWidget *table9;
  GtkWidget *label165;
  GtkWidget *t_user;
  GtkWidget *t_password;
  GtkWidget *label164;
  GtkWidget *dialog_action_area8;
  GtkWidget *b_ok;
  GtkWidget *b_cancel;
  char buff[1024];

  if (hwnd == (HWND) - 1L)
    {
      gtk_init (0, NULL);
      hwnd = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    }

  if (hwnd == NULL || !GTK_IS_WIDGET (hwnd))
    return;

  login = gtk_dialog_new ();
  gtk_widget_set_name (login, "login");
  gtk_widget_set_size_request (login, 400, 150);
  sprintf (buff, "Login for DSN %s ...", (dsn) ? dsn : "Unknown");
  gtk_window_set_title (GTK_WINDOW (login), buff);
  gtk_window_set_position (GTK_WINDOW (login), GTK_WIN_POS_CENTER);
  gtk_window_set_modal (GTK_WINDOW (login), TRUE);
  gtk_window_set_default_size (GTK_WINDOW (login), 400, 150);
  gtk_window_set_type_hint (GTK_WINDOW (login), GDK_WINDOW_TYPE_HINT_DIALOG);

#if GTK_CHECK_VERSION(2,0,0)
  gtk_widget_show (login);
#endif

  dialog_vbox8 = GTK_DIALOG (login)->vbox;
  gtk_widget_set_name (dialog_vbox8, "dialog_vbox8");
  gtk_widget_show (dialog_vbox8);

  frame99 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame99, "frame99");
  gtk_widget_show (frame99);
  gtk_box_pack_start (GTK_BOX (dialog_vbox8), frame99, TRUE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame99), GTK_SHADOW_NONE);

  alignment83 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment83, "alignment83");
  gtk_widget_show (alignment83);
  gtk_container_add (GTK_CONTAINER (frame99), alignment83);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment83), 4, 0, 6, 7);

  table9 = gtk_table_new (2, 2, FALSE);
  gtk_widget_set_name (table9, "table9");
  gtk_widget_show (table9);
  gtk_container_add (GTK_CONTAINER (alignment83), table9);
  gtk_table_set_row_spacings (GTK_TABLE (table9), 10);
  gtk_table_set_col_spacings (GTK_TABLE (table9), 6);

  label165 = gtk_label_new (_("Password :"));
  gtk_widget_set_name (label165, "label165");
  gtk_widget_show (label165);
  gtk_table_attach (GTK_TABLE (table9), label165, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label165), 0, 0.5);

  t_user = gtk_entry_new ();
  gtk_widget_set_name (t_user, "t_user");
  gtk_widget_show (t_user);
  gtk_table_attach (GTK_TABLE (table9), t_user, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  if (username && STRLEN (username))
    gtk_entry_set_text (GTK_ENTRY (t_user), username);

  t_password = gtk_entry_new ();
  gtk_widget_set_name (t_password, "t_password");
  gtk_widget_show (t_password);
  gtk_table_attach (GTK_TABLE (table9), t_password, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_entry_set_visibility (GTK_ENTRY (t_password), FALSE);
  if (password && STRLEN (password))
    gtk_entry_set_text (GTK_ENTRY (t_password), password);

  label164 = gtk_label_new (_("Username :"));
  gtk_widget_set_name (label164, "label164");
  gtk_widget_show (label164);
  gtk_table_attach (GTK_TABLE (table9), label164, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label164), 0, 0.5);

  dialog_action_area8 = GTK_DIALOG (login)->action_area;
  gtk_widget_set_name (dialog_action_area8, "dialog_action_area8");
  gtk_widget_show (dialog_action_area8);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area8), GTK_BUTTONBOX_END);

  b_cancel = gtk_button_new_from_stock ("gtk-cancel");
  gtk_widget_set_name (b_cancel, "b_cancel");
  gtk_widget_show (b_cancel);
  gtk_dialog_add_action_widget (GTK_DIALOG (login), b_cancel, GTK_RESPONSE_CANCEL);
  GTK_WIDGET_SET_FLAGS (b_cancel, GTK_CAN_DEFAULT);

  b_ok = gtk_button_new_from_stock ("gtk-ok");
  gtk_widget_set_name (b_ok, "b_ok");
  gtk_widget_show (b_ok);
  gtk_dialog_add_action_widget (GTK_DIALOG (login), b_ok, GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS (b_ok, GTK_CAN_DEFAULT);

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF (login, login, "login");
  GLADE_HOOKUP_OBJECT_NO_REF (login, dialog_vbox8, "dialog_vbox8");
  GLADE_HOOKUP_OBJECT (login, frame99, "frame99");
  GLADE_HOOKUP_OBJECT (login, alignment83, "alignment83");
  GLADE_HOOKUP_OBJECT (login, table9, "table9");
  GLADE_HOOKUP_OBJECT (login, label165, "label165");
  GLADE_HOOKUP_OBJECT (login, t_user, "t_user");
  GLADE_HOOKUP_OBJECT (login, t_password, "t_password");
  GLADE_HOOKUP_OBJECT (login, label164, "label164");
  GLADE_HOOKUP_OBJECT_NO_REF (login, dialog_action_area8, "dialog_action_area8");
  GLADE_HOOKUP_OBJECT (login, b_ok, "b_ok");
  GLADE_HOOKUP_OBJECT (login, b_cancel, "b_cancel");

  /* Ok button events */
  gtk_signal_connect (GTK_OBJECT (b_ok), "clicked",
      GTK_SIGNAL_FUNC (login_ok_clicked), log_t);
  /* Cancel button events */
  gtk_signal_connect (GTK_OBJECT (b_cancel), "clicked",
      GTK_SIGNAL_FUNC (login_cancel_clicked), log_t);
  /* Close window button events */
  gtk_signal_connect (GTK_OBJECT (login), "delete_event",
      GTK_SIGNAL_FUNC (delete_event), log_t);
  gtk_signal_connect (GTK_OBJECT (login), "destroy",
      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);

  log_t->username = t_user;
  log_t->password = t_password;
  log_t->user = log_t->pwd = NULL;
  log_t->mainwnd = login;

  gtk_widget_show_all (login);
  gtk_main ();
}
