#
#  Makefile.am
#
#  The iODBC driver manager.
#
#  Copyright (C) 1995 Ke Jin <kejin@empress.com>
#  Copyright (C) 1996-2024 OpenLink Software <iodbc@openlinksw.com>
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
	$(srcdir)/README \
	$(srcdir)/README.md \
	$(srcdir)/README_GIT \
	$(srcdir)/README_GIT.md \
	$(srcdir)/README_MACOSX \
	$(srcdir)/README_MACOSX.md \
	$(srcdir)/autogen.sh \
	$(srcdir)/PORT.OpenLink \
	$(srcdir)/acinclude.m4 \
	$(srcdir)/drvproxy/*.exp \
	$(srcdir)/mac/drvproxy.exp \
	$(srcdir)/mac/framework-include.sh \
	$(srcdir)/mac/GNUmakefile \
	$(srcdir)/mac/iodbc-config.macos \
	$(srcdir)/mac/iodbc.exp \
	$(srcdir)/mac/iODBC/en.lproj/InfoPlist.strings \
	$(srcdir)/mac/iODBC/Info-iODBC.plist \
	$(srcdir)/mac/iODBC/iODBC.xcodeproj/project.pbxproj \
	$(srcdir)/mac/iODBC/iODBC.xcodeproj/project.xcworkspace/contents.xcworkspacedata \
	$(srcdir)/mac/iODBC/iODBC.xcodeproj/project.xcworkspace/xcshareddata/IDEWorkspaceChecks.plist \
	$(srcdir)/mac/iODBC/iODBC.xcodeproj/project.xcworkspace/xcshareddata/WorkspaceSettings.xcsettings \
	$(srcdir)/mac/iodbcadm.exp \
	$(srcdir)/mac/iODBCadm/en.lproj/InfoPlist.strings \
	$(srcdir)/mac/iODBCadm/Info-iODBCadm.plist \
	$(srcdir)/mac/iODBCadm/iODBCadm.xcodeproj/project.pbxproj \
	$(srcdir)/mac/iODBCadm/iODBCadm.xcodeproj/project.xcworkspace/contents.xcworkspacedata \
	$(srcdir)/mac/iODBCadm/iODBCadm.xcodeproj/project.xcworkspace/xcshareddata/IDEWorkspaceChecks.plist \
	$(srcdir)/mac/iODBCadm/iODBCadm.xcodeproj/project.xcworkspace/xcshareddata/WorkspaceSettings.xcsettings \
	$(srcdir)/mac/iODBCadministrator/*.h \
	$(srcdir)/mac/iODBCadministrator/*.m \
	$(srcdir)/mac/iODBCadministrator/Base.lproj/MainMenu.xib \
	$(srcdir)/mac/iODBCadministrator/en.lproj/InfoPlist.strings \
	$(srcdir)/mac/iODBCadministrator/en.lproj/iODBCAdministrator.icns \
	$(srcdir)/mac/iODBCadministrator/en.lproj/odbcman.tiff \
	$(srcdir)/mac/iODBCadministrator/Info-iODBC_Administrator.plist \
	$(srcdir)/mac/iODBCadministrator/iODBCadministrator.xcodeproj/project.pbxproj \
	$(srcdir)/mac/iODBCadministrator/iODBCadministrator.xcodeproj/project.xcworkspace/contents.xcworkspacedata \
	$(srcdir)/mac/iODBCadministrator/iODBCadministrator.xcodeproj/project.xcworkspace/xcshareddata/IDEWorkspaceChecks.plist \
	$(srcdir)/mac/iODBCadministrator/iODBCadministrator.xcodeproj/project.xcworkspace/xcshareddata/WorkspaceSettings.xcsettings \
	$(srcdir)/mac/iODBCadministrator64/*.h \
	$(srcdir)/mac/iODBCadministrator64/*.m \
	$(srcdir)/mac/iODBCadministrator64/Base.lproj/MainMenu.xib \
	$(srcdir)/mac/iODBCadministrator64/en.lproj/InfoPlist.strings \
	$(srcdir)/mac/iODBCadministrator64/en.lproj/iODBCAdministrator.icns \
	$(srcdir)/mac/iODBCadministrator64/en.lproj/odbcman.tiff \
	$(srcdir)/mac/iODBCadministrator64/Info-iODBC_Administrator.plist \
	$(srcdir)/mac/iODBCadministrator64/iODBCadministrator64.xcodeproj/project.pbxproj \
	$(srcdir)/mac/iODBCadministrator64/iODBCadministrator64.xcodeproj/project.xcworkspace/contents.xcworkspacedata \
	$(srcdir)/mac/iODBCadministrator64/iODBCadministrator64.xcodeproj/project.xcworkspace/xcshareddata/IDEWorkspaceChecks.plist \
	$(srcdir)/mac/iODBCadministrator64/iODBCadministrator64.xcodeproj/project.xcworkspace/xcshareddata/WorkspaceSettings.xcsettings \
	$(srcdir)/mac/iODBCdemo/*.h \
	$(srcdir)/mac/iODBCdemo/*.m \
	$(srcdir)/mac/iODBCdemo/Base.lproj/ExecSQL.xib \
	$(srcdir)/mac/iODBCdemo/Base.lproj/MainMenu.xib \
	$(srcdir)/mac/iODBCdemo/DemoAnsi/DemoAnsi.xcodeproj/project.pbxproj \
	$(srcdir)/mac/iODBCdemo/DemoAnsi/DemoAnsi.xcodeproj/project.xcworkspace/contents.xcworkspacedata \
	$(srcdir)/mac/iODBCdemo/DemoAnsi/DemoAnsi.xcodeproj/project.xcworkspace/xcshareddata/IDEWorkspaceChecks.plist \
	$(srcdir)/mac/iODBCdemo/DemoAnsi/DemoAnsi.xcodeproj/project.xcworkspace/xcshareddata/WorkspaceSettings.xcsettings \
	$(srcdir)/mac/iODBCdemo/DemoAnsi/en.lproj/InfoPlist.strings \
	$(srcdir)/mac/iODBCdemo/DemoAnsi/InfoAnsi.plist \
	$(srcdir)/mac/iODBCdemo/DemoUnicode/DemoUnicode.xcodeproj/project.pbxproj \
	$(srcdir)/mac/iODBCdemo/DemoUnicode/DemoUnicode.xcodeproj/project.xcworkspace/contents.xcworkspacedata \
	$(srcdir)/mac/iODBCdemo/DemoUnicode/DemoUnicode.xcodeproj/project.xcworkspace/xcshareddata/IDEWorkspaceChecks.plist \
	$(srcdir)/mac/iODBCdemo/DemoUnicode/DemoUnicode.xcodeproj/project.xcworkspace/xcshareddata/WorkspaceSettings.xcsettings \
	$(srcdir)/mac/iODBCdemo/DemoUnicode/en.lproj/InfoPlist.strings \
	$(srcdir)/mac/iODBCdemo/DemoUnicode/InfoUnicode.plist \
	$(srcdir)/mac/iODBCdemo/DSNchooserController.xib \
	$(srcdir)/mac/iODBCdemo/en.lproj/Credits.rtf \
	$(srcdir)/mac/iODBCdemo/en.lproj/iODBCdemo.icns \
	$(srcdir)/mac/iODBCdemo/iodbcdemo_Prefix.pch \
	$(srcdir)/mac/iODBCdrvproxy/en.lproj/InfoPlist.strings \
	$(srcdir)/mac/iODBCdrvproxy/Info-iODBCdrvproxy.plist \
	$(srcdir)/mac/iODBCdrvproxy/iODBCdrvproxy.xcodeproj/project.pbxproj \
	$(srcdir)/mac/iODBCdrvproxy/iODBCdrvproxy.xcodeproj/project.xcworkspace/contents.xcworkspacedata \
	$(srcdir)/mac/iODBCdrvproxy/iODBCdrvproxy.xcodeproj/project.xcworkspace/xcshareddata/IDEWorkspaceChecks.plist \
	$(srcdir)/mac/iODBCdrvproxy/iODBCdrvproxy.xcodeproj/project.xcworkspace/xcshareddata/WorkspaceSettings.xcsettings \
	$(srcdir)/mac/iodbcinst.exp \
	$(srcdir)/mac/iODBCinst/en.lproj/InfoPlist.strings \
	$(srcdir)/mac/iODBCinst/Info-iODBCinst.plist \
	$(srcdir)/mac/iODBCinst/iODBCinst.xcodeproj/project.pbxproj \
	$(srcdir)/mac/iODBCinst/iODBCinst.xcodeproj/project.xcworkspace/contents.xcworkspacedata \
	$(srcdir)/mac/iODBCinst/iODBCinst.xcodeproj/project.xcworkspace/xcshareddata/IDEWorkspaceChecks.plist \
	$(srcdir)/mac/iODBCinst/iODBCinst.xcodeproj/project.xcworkspace/xcshareddata/WorkspaceSettings.xcsettings \
	$(srcdir)/mac/iODBCtest/iODBCtest.xcodeproj/project.pbxproj \
	$(srcdir)/mac/iODBCtest/iODBCtest.xcodeproj/project.xcworkspace/contents.xcworkspacedata \
	$(srcdir)/mac/iODBCtest/iODBCtest.xcodeproj/project.xcworkspace/xcshareddata/IDEWorkspaceChecks.plist \
	$(srcdir)/mac/iODBCtest/iODBCtest.xcodeproj/project.xcworkspace/xcshareddata/WorkspaceSettings.xcsettings \
	$(srcdir)/mac/iODBCtestw/iODBCtestw.xcodeproj/project.pbxproj \
	$(srcdir)/mac/iODBCtestw/iODBCtestw.xcodeproj/project.xcworkspace/contents.xcworkspacedata \
	$(srcdir)/mac/iODBCtestw/iODBCtestw.xcodeproj/project.xcworkspace/xcshareddata/IDEWorkspaceChecks.plist \
	$(srcdir)/mac/iODBCtestw/iODBCtestw.xcodeproj/project.xcworkspace/xcshareddata/WorkspaceSettings.xcsettings \
	$(srcdir)/mac/link-inclibs.sh \
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
