#
#  Makefile.am
#
#  $Id$
#
#  (C) 1999 OpenLink Software Inc.
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
SUBDIRS			= . samples

lib_LTLIBRARIES		= libiodbc.la

libiodbc_la_LDFLAGS	= -version-info @lib_version@
libiodbc_la_LIBADD	= $(LIBADD_DL)
libiodbc_la_SOURCES	= catalog.c connect.c dlf.c dlproc.c execute.c \
			  fetch.c hdbc.c henv.c herr.c hstmt.c info.c \
			  itrace.c misc.c prepare.c result.c


noinst_HEADERS		= config.h dlf.h dlproc.h hdbc.h henv.h herr.h \
			  hstmt.h itrace.h henv.ci herr.ci
include_HEADERS		= isql.h isqlext.h


noinst_SCRIPTS		= config.guess config.sub configure \
			  install-sh ltconfig ltmain.sh missing mkinstalldirs

EXTRA_DIST		= IAFA-PACKAGE odbc.ini.sample



#
#  Create Linux RPM's
#
linux-rpm:
	make dist
	cp libiodbc-$(VERSION).tar.gz /usr/src/redhat/SOURCES
	rpm -ba -v libiodbc.spec
