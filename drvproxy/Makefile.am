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

lib_LTLIBRARIES		= libdrvproxy.la
endif

if GTK
INCLUDES		= -I$(top_srcdir)/include -I$(top_srcdir)/iodbc -I$(top_srcdir)/iodbcinst \
	@GTK_CFLAGS@
CFLAGS			= @CFLAGS@ -D__GTK__
else
INCLUDES		= -I$(top_srcdir)/include -I$(top_srcdir)/iodbc -I$(top_srcdir)/iodbcinst
endif

libdrvproxy_la_LDFLAGS	= -version-info @lib_version@ \
			  -export-symbols $(srcdir)/drvproxy.exp

if GTK
libdrvproxy_la_LIBADD	= $(LIBADD_DL) ../iodbc/dlf.lo ../drvproxy/gtk/confirm.lo ../drvproxy/gtk/gensetup.lo ../iodbcinst/libiodbcinst.la ../iodbcadm/libiodbcadm.la @GTK_LIBS@
else
libdrvproxy_la_LIBADD	= $(LIBADD_DL) ../iodbc/dlf.lo ../iodbcinst/libiodbcinst.la ../iodbcadm/libiodbcadm.la
endif

libdrvproxy_la_SOURCES	= \
	ConfigDSN.c \
	ConfigDriver.c \
	Info.c \
	drvconn.c \
	login.c

noinst_HEADERS		= gui.h

EXTRA_DIST		= drvproxy.exp
