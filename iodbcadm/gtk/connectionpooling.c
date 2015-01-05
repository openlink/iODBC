/*
 *  connectionpooling.c
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
connectionpool_finish_clicked (GtkWidget *widget,
    TCONNECTIONPOOLING *connectionpool_t)
{
  if (connectionpool_t)
    {
      connectionpool_t->changed = TRUE;

      memset(connectionpool_t->timeout, 0, sizeof(connectionpool_t->timeout)); 
      memset(connectionpool_t->probe, 0, sizeof(connectionpool_t->probe)); 
      strncpy (connectionpool_t->timeout,
	    gtk_entry_get_text (GTK_ENTRY (connectionpool_t->timeout_entry)),
	    sizeof(connectionpool_t->timeout)-1);
      strncpy (connectionpool_t->probe,
	    gtk_entry_get_text (GTK_ENTRY (connectionpool_t->probe_entry)),
	    sizeof(connectionpool_t->probe)-1);

      gtk_signal_disconnect_by_func (GTK_OBJECT (connectionpool_t->mainwnd),
	  GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
      gtk_main_quit ();
      gtk_widget_destroy (connectionpool_t->mainwnd);
    }
}


static void
connectionpool_cancel_clicked (GtkWidget *widget,
    TCONNECTIONPOOLING *connectionpool_t)
{
  if (connectionpool_t)
    {
      connectionpool_t->changed = FALSE;

      gtk_signal_disconnect_by_func (GTK_OBJECT (connectionpool_t->mainwnd),
	  GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
      gtk_main_quit ();
      gtk_widget_destroy (connectionpool_t->mainwnd);
    }
}


static gint
delete_event (GtkWidget *widget,
    GdkEvent *event, TCONNECTIONPOOLING *connectionpool_t)
{
  connectionpool_cancel_clicked (widget, connectionpool_t);

  return FALSE;
}


BOOL
create_connectionpool (HWND hwnd, TCONNECTIONPOOLING *choose_t)
{
  GdkPixmap *pixmap;
  GdkBitmap *mask;
  GtkStyle *style;

  GtkWidget *connectionpool;
  GtkWidget *dialog_vbox7;
  GtkWidget *frame1;
  GtkWidget *alignment76;
  GtkWidget *vbox40;
  GtkWidget *frame93;
  GtkWidget *alignment77;
  GtkWidget *label154;
  GtkWidget *frame94;
  GtkWidget *alignment78;
  GtkWidget *hbox55;
  GtkWidget *label156;
  GtkWidget *t_cptimeout;
  GtkWidget *frame95;
  GtkWidget *alignment79;
  GtkWidget *label155;
  GtkWidget *frame96;
  GtkWidget *alignment80;
  GtkWidget *hbox56;
  GtkWidget *label157;
  GtkWidget *t_probe;
  GtkWidget *flabel1;
  GtkWidget *dialog_action_area7;
  GtkWidget *b_cancel;
  GtkWidget *b_finish;
  char msg[1024];

  if (hwnd == NULL || !GTK_IS_WIDGET (hwnd) || !choose_t)
    return FALSE;

  connectionpool = gtk_dialog_new ();
  gtk_widget_set_name (connectionpool, "connectionpool");
  gtk_widget_set_size_request (connectionpool, 433, 227);
  gtk_window_set_title (GTK_WINDOW (connectionpool), _("Connection pooling attributes"));
  gtk_window_set_position (GTK_WINDOW (connectionpool), GTK_WIN_POS_CENTER);
  gtk_window_set_modal (GTK_WINDOW (connectionpool), TRUE);
  gtk_window_set_default_size (GTK_WINDOW (connectionpool), 433, 227);
  gtk_window_set_type_hint (GTK_WINDOW (connectionpool), GDK_WINDOW_TYPE_HINT_DIALOG);

#if GTK_CHECK_VERSION(2,0,0)
  gtk_widget_show (connectionpool);
#endif

  dialog_vbox7 = GTK_DIALOG (connectionpool)->vbox;
  gtk_widget_set_name (dialog_vbox7, "dialog_vbox7");
  gtk_widget_show (dialog_vbox7);

  frame1 = gtk_frame_new (choose_t->driver);
  gtk_widget_set_name (frame1, "frame1");
  gtk_widget_show (frame1);
  gtk_box_pack_start (GTK_BOX (dialog_vbox7), frame1, TRUE, TRUE, 0);

  alignment76 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment76, "alignment76");
  gtk_widget_show (alignment76);
  gtk_container_add (GTK_CONTAINER (frame1), alignment76);

  vbox40 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox40, "vbox40");
  gtk_widget_show (vbox40);
  gtk_container_add (GTK_CONTAINER (alignment76), vbox40);

  frame93 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame93, "frame93");
  gtk_widget_show (frame93);
  gtk_box_pack_start (GTK_BOX (vbox40), frame93, FALSE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame93), GTK_SHADOW_NONE);

  alignment77 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment77, "alignment77");
  gtk_widget_show (alignment77);
  gtk_container_add (GTK_CONTAINER (frame93), alignment77);

  label154 = gtk_label_new (_("Enable connection pooling for this driver by specifying\na timeout in seconds"));
  gtk_widget_set_name (label154, "label154");
  gtk_widget_show (label154);
  gtk_container_add (GTK_CONTAINER (alignment77), label154);

  frame94 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame94, "frame94");
  gtk_widget_show (frame94);
  gtk_box_pack_start (GTK_BOX (vbox40), frame94, FALSE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame94), GTK_SHADOW_NONE);

  alignment78 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment78, "alignment78");
  gtk_widget_show (alignment78);
  gtk_container_add (GTK_CONTAINER (frame94), alignment78);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment78), 0, 0, 4, 10);

  hbox55 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox55, "hbox55");
  gtk_widget_show (hbox55);
  gtk_container_add (GTK_CONTAINER (alignment78), hbox55);

  label156 = gtk_label_new (_("Timeout :  "));
  gtk_widget_set_name (label156, "label156");
  gtk_widget_show (label156);
  gtk_box_pack_start (GTK_BOX (hbox55), label156, FALSE, FALSE, 0);

  t_cptimeout = gtk_entry_new ();
  gtk_widget_set_name (t_cptimeout, "t_cptimeout");
  gtk_widget_show (t_cptimeout);
  gtk_box_pack_start (GTK_BOX (hbox55), t_cptimeout, TRUE, TRUE, 0);

  if (choose_t)
    gtk_entry_set_text (GTK_ENTRY (t_cptimeout), choose_t->timeout);

  frame95 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame95, "frame95");
  gtk_widget_show (frame95);
  gtk_box_pack_start (GTK_BOX (vbox40), frame95, FALSE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame95), GTK_SHADOW_NONE);

  alignment79 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment79, "alignment79");
  gtk_widget_show (alignment79);
  gtk_container_add (GTK_CONTAINER (frame95), alignment79);

  label155 = gtk_label_new (_("Set an optional probe query, used for additional verification\nof the connection state"));
  gtk_widget_set_name (label155, "label155");
  gtk_widget_show (label155);
  gtk_container_add (GTK_CONTAINER (alignment79), label155);

  frame96 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame96, "frame96");
  gtk_widget_show (frame96);
  gtk_box_pack_start (GTK_BOX (vbox40), frame96, FALSE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame96), GTK_SHADOW_NONE);

  alignment80 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_set_name (alignment80, "alignment80");
  gtk_widget_show (alignment80);
  gtk_container_add (GTK_CONTAINER (frame96), alignment80);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment80), 0, 10, 4, 10);

  hbox56 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox56, "hbox56");
  gtk_widget_show (hbox56);
  gtk_container_add (GTK_CONTAINER (alignment80), hbox56);

  label157 = gtk_label_new (_("   Query :  "));
  gtk_widget_set_name (label157, "label157");
  gtk_widget_show (label157);
  gtk_box_pack_start (GTK_BOX (hbox56), label157, FALSE, FALSE, 0);

  t_probe = gtk_entry_new ();
  gtk_widget_set_name (t_probe, "t_probe");
  gtk_widget_show (t_probe);
  gtk_box_pack_start (GTK_BOX (hbox56), t_probe, TRUE, TRUE, 0);

  if (choose_t)
    gtk_entry_set_text (GTK_ENTRY (t_probe), choose_t->probe);

  dialog_action_area7 = GTK_DIALOG (connectionpool)->action_area;
  gtk_widget_set_name (dialog_action_area7, "dialog_action_area7");
  gtk_widget_show (dialog_action_area7);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area7), GTK_BUTTONBOX_END);

  b_cancel = gtk_button_new_from_stock ("gtk-cancel");
  gtk_widget_set_name (b_cancel, "b_cancel");
  gtk_widget_show (b_cancel);
  gtk_dialog_add_action_widget (GTK_DIALOG (connectionpool), b_cancel, GTK_RESPONSE_CANCEL);
  GTK_WIDGET_SET_FLAGS (b_cancel, GTK_CAN_DEFAULT);

  b_finish = gtk_button_new_from_stock ("gtk-ok");
  gtk_widget_set_name (b_finish, "b_finish");
  gtk_widget_show (b_finish);
  gtk_dialog_add_action_widget (GTK_DIALOG (connectionpool), b_finish, GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS (b_finish, GTK_CAN_DEFAULT);

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF (connectionpool, connectionpool, "connectionpool");
  GLADE_HOOKUP_OBJECT_NO_REF (connectionpool, dialog_vbox7, "dialog_vbox7");
  GLADE_HOOKUP_OBJECT (connectionpool, frame1, "frame1");
  GLADE_HOOKUP_OBJECT (connectionpool, alignment76, "alignment76");
  GLADE_HOOKUP_OBJECT (connectionpool, vbox40, "vbox40");
  GLADE_HOOKUP_OBJECT (connectionpool, frame93, "frame93");
  GLADE_HOOKUP_OBJECT (connectionpool, alignment77, "alignment77");
  GLADE_HOOKUP_OBJECT (connectionpool, label154, "label154");
  GLADE_HOOKUP_OBJECT (connectionpool, frame94, "frame94");
  GLADE_HOOKUP_OBJECT (connectionpool, alignment78, "alignment78");
  GLADE_HOOKUP_OBJECT (connectionpool, hbox55, "hbox55");
  GLADE_HOOKUP_OBJECT (connectionpool, label156, "label156");
  GLADE_HOOKUP_OBJECT (connectionpool, t_cptimeout, "t_cptimeout");
  GLADE_HOOKUP_OBJECT (connectionpool, frame95, "frame95");
  GLADE_HOOKUP_OBJECT (connectionpool, alignment79, "alignment79");
  GLADE_HOOKUP_OBJECT (connectionpool, label155, "label155");
  GLADE_HOOKUP_OBJECT (connectionpool, frame96, "frame96");
  GLADE_HOOKUP_OBJECT (connectionpool, alignment80, "alignment80");
  GLADE_HOOKUP_OBJECT (connectionpool, hbox56, "hbox56");
  GLADE_HOOKUP_OBJECT (connectionpool, label157, "label157");
  GLADE_HOOKUP_OBJECT (connectionpool, t_probe, "t_probe");
  GLADE_HOOKUP_OBJECT (connectionpool, flabel1, "flabel1");
  GLADE_HOOKUP_OBJECT_NO_REF (connectionpool, dialog_action_area7, "dialog_action_area7");
  GLADE_HOOKUP_OBJECT (connectionpool, b_cancel, "b_cancel");
  GLADE_HOOKUP_OBJECT (connectionpool, b_finish, "b_finish");


  /* Finish button events */
  gtk_signal_connect (GTK_OBJECT (b_finish), "clicked",
      GTK_SIGNAL_FUNC (connectionpool_finish_clicked), choose_t);
  /* Cancel button events */
  gtk_signal_connect (GTK_OBJECT (b_cancel), "clicked",
      GTK_SIGNAL_FUNC (connectionpool_cancel_clicked), choose_t);
  /* Close window button events */
  gtk_signal_connect (GTK_OBJECT (connectionpool), "delete_event",
      GTK_SIGNAL_FUNC (delete_event), choose_t);
  gtk_signal_connect (GTK_OBJECT (connectionpool), "destroy",
      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);

  choose_t->timeout_entry = t_cptimeout;
  choose_t->probe_entry = t_probe;
  choose_t->mainwnd = connectionpool;

  gtk_widget_show_all (connectionpool);
  gtk_main ();

  return choose_t->changed;
}
