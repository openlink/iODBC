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

AUTOMAKE_OPTIONS	= foreign no-dependencies 1.4

EXTRA_DIST		= odbctest.c

noinst_PROGRAMS		= odbctest
INCLUDES		= -I$(top_srcdir)

#
#  Build odbctest program
#
odbctest_SOURCES	= odbctest.c
odbctest_LDADD		= ../libiodbc.la
odbctest_LDFLAGS	= -static
