#
#  Makefile.am
#
#  (C) 2001 OpenLink Software Inc.
#
#  The iODBC driver manager.
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

AUTOMAKE_OPTIONS	= gnu 1.4

INCLUDES		= -I$(top_srcdir)/include

lib_LTLIBRARIES		= libiodbc.la

libiodbc_la_LDFLAGS	= -version-info @lib_version@ \
	                  -export-symbols $(srcdir)/iodbc.exp
libiodbc_la_LIBADD	= $(LIBADD_DL)
libiodbc_la_SOURCES	= catalog.c connect.c dlf.c dlproc.c execute.c \
			  fetch.c hdbc.c henv.c herr.c hstmt.c info.c \
			  itrace.c misc.c prepare.c result.c odbc3.c 

noinst_HEADERS		= iodbc.h dlf.h dlproc.h hdbc.h henv.h herr.h \
			  hstmt.h itrace.h henv.ci herr.ci hdesc.h ithread.h

EXTRA_DIST		= iodbc.def iodbc.exp main.c vmscompile.com
