#
#  Makefile.am
#
#  $Id$
#
#  (C) 2001 OpenLink Software Inc.
#
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Library General Public
#  License as published by the Free Software Foundation; either
#  version 2 of the License, or (at your option) any later version.
#
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Library General Public License for more details.
#
#  You should have received a copy of the GNU Library General Public
#  License along with this library; if not, write to the Free
#  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

lib_LTLIBRARIES		= libdrvproxy-gtk.la
INCLUDES		= -I$(top_srcdir)/include -I$(top_srcdir)/iodbc \
	@GTK_CFLAGS@ -D __GTK__

libdrvproxy_gtk_la_LDFLAGS	= -static

libdrvproxy_gtk_la_LIBADD	= @GTK_LIBS@

libdrvproxy_gtk_la_SOURCES	= \
	confirm.c \
	gensetup.c

noinst_HEADERS		= \
	gui.h \
	question.xpm
