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

DATEBUILD		= `date +%d/%m/%Y_%T`

if GTK
lib_LTLIBRARIES		= libiodbcadm-gtk.la
endif

INCLUDES		= \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/iodbc \
	-I$(top_srcdir)/iodbcadm \
	-I$(top_srcdir)/iodbcinst \
	@GTK_CFLAGS@

CFLAGS			= @CFLAGS@ -D__GTK__ -DDATEBUILD=\"$(DATEBUILD)\"

libiodbcadm_gtk_la_LDFLAGS	= -static
libiodbcadm_gtk_la_LIBADD	= @GTK_LIBS@
libiodbcadm_gtk_la_SOURCES	= \
	administrator.c \
	confirm.c \
	connectionpooling.c \
	driverchooser.c \
	driversetup.c \
	dsnchooser.c \
	errorbox.c \
	filedsn.c \
	loginbox.c \
	main.c \
	messagebox.c \
	translatorchooser.c 

confirm.c: $(top_srcdir)/drvproxy/gtk/confirm.c question.xpm
	${RM} confirm.c
	${LN_S} $(top_srcdir)/drvproxy/gtk/confirm.c confirm.c

question.xpm: $(top_srcdir)/drvproxy/gtk/question.xpm
	${RM} question.xpm
	${LN_S} $(top_srcdir)/drvproxy/gtk/question.xpm question.xpm

noinst_HEADERS	= error.xpm img.xpm odbc4.xpm gui.h
