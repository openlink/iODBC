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
SUBDIRS			= admin bin etc include iodbc samples

EXTRA_DIST             = IAFA-PACKAGE 


# ----------------------------------------------------------------------
#
#  Maintainers only
# 
# ----------------------------------------------------------------------

MAINTAINERCLEANFILES	= Makefile.in aclocal.m4 configure


if MAINTAINER_MODE

#
#  Create Linux RPM's
#
linux-rpm:
	$(MAKE) dist
	cp libiodbc-$(VERSION).tar.gz /usr/src/redhat/SOURCES
	rpm -ba libiodbc.spec

#
#  Create a tar file containing the library and include files
#
binary-tar:
	-mkdir @build@
	$(MAKE) install prefix=`pwd`/@build@
	tar cvf @build@.tar @build@
	gzip -9v @build@.tar
	rm -rf @build@

#
#  Create a source snapshot package
#
snapshot:
	$(MAKE) dist distdir=$(PACKAGE)-`date +"%Y%m%d"`


#
#  Create an official release package
#
release:
	$(MAKE) distcheck

endif
