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

if GUI
SUBDIRS			= gtk

bin_PROGRAMS		= iodbcadm-gtk
lib_LTLIBRARIES		= libiodbcadm.la
endif

if GTK
INCLUDES		= \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/iodbc \
	-I$(top_srcdir)/iodbcinst \
	@GTK_CFLAGS@

CFLAGS			= @CFLAGS@ -D__GTK__
else
INCLUDES		= \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/iodbc \
	-I$(top_srcdir)/iodbcinst
endif

libiodbcadm_la_LDFLAGS	= \
	-version-info @lib_version@ \
	-export-symbols $(srcdir)/iodbcadm.exp

if GTK
libiodbcadm_la_LIBADD	= \
	$(LIBADD_DL) \
	../iodbc/dlf.lo \
	../iodbcadm/gtk/administrator.lo \
	../iodbcadm/gtk/confirm.lo \
	../iodbcadm/gtk/connectionpooling.lo\
	../iodbcadm/gtk/driverchooser.lo \
	../iodbcadm/gtk/driversetup.lo \
	../iodbcadm/gtk/dsnchooser.lo \
	../iodbcadm/gtk/errorbox.lo \
	../iodbcadm/gtk/filedsn.lo \
	../iodbcadm/gtk/loginbox.lo \
	../iodbcadm/gtk/messagebox.lo \
	../iodbcadm/gtk/translatorchooser.lo \
	../iodbcinst/libiodbcinst.la \
	../iodbc/libiodbc.la
else
libiodbcadm_la_LIBADD	= \
	$(LIBADD_DL) \
	../iodbc/dlf.lo \
	../iodbcinst/libiodbcinst.la \
	../iodbc/libiodbc.la
endif

libiodbcadm_la_SOURCES	= \
	Info.c \
	drvconn.c \
	error.c \
	login.c 

iodbcadm_gtk_SOURCES	=
iodbcadm_gtk_LDADD	= \
	../iodbcadm/libiodbcadm.la \
	../iodbcadm/gtk/main.lo \
	@GTK_LIBS@
iodbcadm_gtk_LDFLAGS	= # -static

noinst_HEADERS		= \
	gui.h \
	iodbcadm.h

EXTRA_DIST		= iodbcadm.exp
