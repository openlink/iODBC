#
#  Makefile.am
#
#  The iODBC driver manager.
#
#  Copyright (C) 1995 by Ke Jin <kejin@empress.com>
#  Copyright (C) 1996-2015 by OpenLink Software <iodbc@openlinksw.com>
#  All Rights Reserved.
#
#  This software is released under the terms of either of the following
#  licenses:
#
#      - GNU Library General Public License (see LICENSE.LGPL)
#      - The BSD License (see LICENSE.BSD).
#
#  Note that the only valid version of the LGPL license as far as this
#  project is concerned is the original GNU Library General Public License
#  Version 2, dated June 1991.
#
#  While not mandated by the BSD license, any patches you make to the
#  iODBC source code may be contributed back into the iODBC project
#  at your discretion. Contributions will benefit the Open Source and
#  Data Access community as a whole. Submissions may be made at:
#
#      http://www.iodbc.org
#
#
#  GNU Library Generic Public License Version 2
#  ============================================
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Library General Public
#  License as published by the Free Software Foundation; only
#  Version 2 of the License dated June 1991.
#
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Library General Public License for more details.
#
#  You should have received a copy of the GNU Library General Public
#  License along with this library; if not, write to the Free
#  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#
#
#  The BSD License
#  ===============
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in
#     the documentation and/or other materials provided with the
#     distribution.
#  3. Neither the name of OpenLink Software Inc. nor the names of its
#     contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL OPENLINK OR
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

AUTOMAKE_OPTIONS	= gnu dist-zip 1.9.6

SUBDIRS 		= admin bin etc man include iodbcinst iodbc iodbcadm drvproxy samples

ACLOCAL_AMFLAGS		= -I admin

EXTRA_DIST		= \
	$(srcdir)/IAFA-PACKAGE \
	$(srcdir)/LICENSE \
	$(srcdir)/LICENSE.LGPL \
	$(srcdir)/LICENSE.BSD \
	$(srcdir)/README.CVS \
	$(srcdir)/README.GIT \
	$(srcdir)/README.MACOSX \
	$(srcdir)/bootstrap.sh \
	$(srcdir)/PORT.OpenLink \
	$(srcdir)/acinclude.m4 \
	$(srcdir)/drvproxy/*.exp \
	$(srcdir)/mac/*.exp \
	$(srcdir)/mac/*.sh \
	$(srcdir)/mac/iodbc-config.macosx \
	$(srcdir)/mac/Makefile \
	$(srcdir)/mac/myodbc.configure.diff \
	$(srcdir)/mac/README.MYODBC \
	$(srcdir)/mac/iODBC/*.plist \
	$(srcdir)/mac/iODBC/English.lproj/*.strings \
	$(srcdir)/mac/iODBC/iODBC.pbproj/project.pbxproj \
	$(srcdir)/mac/iODBC/iODBC.xcodeproj/project.pbxproj \
	$(srcdir)/mac/iODBCadm/*.plist \
	$(srcdir)/mac/iODBCadm/*.strings \
	$(srcdir)/mac/iODBCadm/English.lproj/confirmation.xib \
	$(srcdir)/mac/iODBCadm/English.lproj/connectionpool.xib \
	$(srcdir)/mac/iODBCadm/English.lproj/driverremove.xib \
	$(srcdir)/mac/iODBCadm/English.lproj/driversetup.xib \
	$(srcdir)/mac/iODBCadm/English.lproj/drvchoose.tiff \
	$(srcdir)/mac/iODBCadm/English.lproj/dsnchooser.xib \
	$(srcdir)/mac/iODBCadm/English.lproj/gensetup.xib \
	$(srcdir)/mac/iODBCadm/English.lproj/keyval.xib \
	$(srcdir)/mac/iODBCadm/English.lproj/odbcadmin.xib \
	$(srcdir)/mac/iODBCadm/English.lproj/odbcdriver.xib \
	$(srcdir)/mac/iODBCadm/English.lproj/odbcdriverf.xib \
	$(srcdir)/mac/iODBCadm/iODBCadm.pbproj/project.pbxproj \
	$(srcdir)/mac/iODBCadm/iODBCadm.xcodeproj/project.pbxproj \
	$(srcdir)/mac/iODBCadministrator/*.plist \
	$(srcdir)/mac/iODBCadministrator/English.lproj/*.strings \
	$(srcdir)/mac/iODBCadministrator/English.lproj/about.xib \
	$(srcdir)/mac/iODBCadministrator/English.lproj/iODBCAdministrator.icns \
	$(srcdir)/mac/iODBCadministrator/English.lproj/main.xib \
	$(srcdir)/mac/iODBCadministrator/English.lproj/odbcman.tiff \
	$(srcdir)/mac/iODBCadministrator/iODBCadministrator.pbproj/project.pbxproj \
	$(srcdir)/mac/iODBCadministrator/iODBCadministrator.xcodeproj/project.pbxproj \
	$(srcdir)/mac/iODBCcfmbridge/*.exp \
	$(srcdir)/mac/iODBCcfmbridge/Info.c \
	$(srcdir)/mac/iODBCcfmbridge/Makefile \
	$(srcdir)/mac/iODBCcfmbridge/iODBC_CFM_Bridge \
	$(srcdir)/mac/iODBCcfmbridge/iODBC_CFM_Bridge.res \
	$(srcdir)/mac/iODBCcfmbridge/iodbccfm.c \
	$(srcdir)/mac/iODBCcfmbridge/iodbccfm.h \
	$(srcdir)/mac/iODBCcfmbridge/iodbccfm.r \
	$(srcdir)/mac/iODBCdemo/*.h \
	$(srcdir)/mac/iODBCdemo/*.m \
	$(srcdir)/mac/iODBCdemo/English.lproj/Credits.rtf \
	$(srcdir)/mac/iODBCdemo/English.lproj/ExecSQL.xib \
	$(srcdir)/mac/iODBCdemo/English.lproj/InfoPlist.strings \
	$(srcdir)/mac/iODBCdemo/English.lproj/iODBCdemo.icns \
	$(srcdir)/mac/iODBCdemo/English.lproj/MainMenu.xib \
	$(srcdir)/mac/iODBCdemo/InfoAnsi.plist \
	$(srcdir)/mac/iODBCdemo/InfoUnicode.plist \
	$(srcdir)/mac/iODBCdemo/iodbcdemo.xcodeproj/project.pbxproj \
	$(srcdir)/mac/iODBCdemo/iodbcdemo_Prefix.pch \
	$(srcdir)/mac/iODBCdrvproxy/*.plist \
	$(srcdir)/mac/iODBCdrvproxy/English.lproj/*.strings \
	$(srcdir)/mac/iODBCdrvproxy/English.lproj/confirmation.xib \
	$(srcdir)/mac/iODBCdrvproxy/English.lproj/gensetup.xib \
	$(srcdir)/mac/iODBCdrvproxy/English.lproj/login.xib \
	$(srcdir)/mac/iODBCdrvproxy/iODBCdrvproxy.pbproj/project.pbxproj \
	$(srcdir)/mac/iODBCdrvproxy/iODBCdrvproxy.xcodeproj/project.pbxproj \
	$(srcdir)/mac/iODBCinst/*.plist \
	$(srcdir)/mac/iODBCinst/English.lproj/*.strings \
	$(srcdir)/mac/iODBCinst/iODBCinst.pbproj/project.pbxproj \
	$(srcdir)/mac/iODBCinst/iODBCinst.xcodeproj/project.pbxproj \
	$(srcdir)/mac/iODBCtest/iODBCtest.pbproj/project.pbxproj \
	$(srcdir)/mac/iODBCtest/iODBCtest.xcodeproj/project.pbxproj \
	$(srcdir)/mac/iODBCtestw/iODBCtestw.pbproj/project.pbxproj \
	$(srcdir)/mac/iODBCtestw/iODBCtestw.xcodeproj/project.pbxproj \
	$(srcdir)/debian/changelog \
	$(srcdir)/debian/compat \
	$(srcdir)/debian/control \
	$(srcdir)/debian/copyright \
	$(srcdir)/debian/iodbc.files \
	$(srcdir)/debian/iodbc.install \
	$(srcdir)/debian/iodbc.lintian-overrides \
	$(srcdir)/debian/iodbc.undocumented \
	$(srcdir)/debian/libiodbc2-dev.examples \
	$(srcdir)/debian/libiodbc2-dev.files \
	$(srcdir)/debian/libiodbc2-dev.install \
	$(srcdir)/debian/libiodbc2.docs \
	$(srcdir)/debian/libiodbc2.examples \
	$(srcdir)/debian/libiodbc2.files \
	$(srcdir)/debian/libiodbc2.install \
	$(srcdir)/debian/libiodbc2.lintian-overrides \
	$(srcdir)/debian/libiodbc2.undocumented \
	$(srcdir)/debian/README.Debian \
	$(srcdir)/debian/rules \
	$(srcdir)/debian/watch 

# ----------------------------------------------------------------------
#
#  Maintainers only
# 
# ----------------------------------------------------------------------

MAINTAINERCLEANFILES	= Makefile.in aclocal.m4 configure
DISTCLEANFILES		= config.nice


if MAINTAINER_MODE

#
#  Create Linux RPM's
#
RPMFLAGS=--define="_topdir `pwd`/distrib"

linux-rpm:
	$(mkinstalldirs) distrib/SOURCES distrib/SRPMS distrib/SPECS
	$(mkinstalldirs) distrib/BUILD distrib/RPMS/i386
	$(MAKE) dist
	cp $(PACKAGE)-$(VERSION).tar.gz distrib/SOURCES
	rpmbuild $(RPMFLAGS) -ba admin/libiodbc.spec
	rpmbuild $(RPMFLAGS) --clean --rmsource admin/libiodbc.spec

#
#  Create a tar file containing the library and include files
#
binary-tar:
	-mkdir @build@
	$(MAKE) install prefix=`pwd`/@build@
	tar cvf @build@.tar @build@
	gzip -9vf @build@.tar
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

#
#  Generate ChangeLog
#
.PHONY: changelog
changelog:
	git2cl --format='%s%n%n%b%n' --strip-tab --strip-cherry-pick > ChangeLog

endif
