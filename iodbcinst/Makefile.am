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

lib_LTLIBRARIES		= libiodbcinst.la
INCLUDES		= \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/iodbc \
	-I$(top_srcdir)/iodbcinst

libiodbcinst_la_LDFLAGS	= -version-info @lib_version@ \
			  -export-symbols $(srcdir)/iodbcinst.exp

libiodbcinst_la_LIBADD	= $(LIBADD_DL) ../iodbc/dlf.lo

libiodbcinst_la_SOURCES	= \
	SQLConfigDataSource.c \
	SQLConfigDriver.c \
	SQLGetAvailableDrivers.c \
	SQLGetInstalledDrivers.c \
	SQLInstallDriver.c \
	SQLRemoveDSNFromIni.c \
	SQLRemoveDriver.c \
	SQLValidDSN.c \
	SQLWriteDSNToIni.c \
	SQLGetConfigMode.c \
	SQLSetConfigMode.c \
	SQLReadFileDSN.c \
	SQLWriteFileDSN.c \
	SQLInstallDriverEx.c \
	SQLInstallODBC.c \
	SQLInstallTranslator.c \
	SQLCreateDataSource.c \
	SQLManageDataSource.c \
	SQLRemoveTranslator.c \
	SQLRemoveDefaultDataSource.c \
	SQLInstallDriverManager.c \
	SQLRemoveDriverManager.c \
	SQLInstallTranslatorEx.c \
	SQLInstallerError.c \
	SQLPostInstallerError.c \
	SQLGetTranslator.c \
	SQLWritePrivateProfileString.c \
	SQLGetPrivateProfileString.c \
	inifile.c \
	misc.c \
	iodbc_error.c \
	Info.c

noinst_HEADERS		= \
	inifile.h \
	iodbc_error.h \
	misc.h 

EXTRA_DIST		= iodbcinst.exp
