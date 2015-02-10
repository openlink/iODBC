#
#  Makefile.mac
#
#  The iODBC driver manager.
#
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


#
#  Determine build model (Deployment/Development)
#
ifeq (,$(MODEL))
MODEL	:= Deployment
endif


#
#  Determine which version of Mac OS X we are running
#
macosx_version	:= $(shell sw_vers -productVersion)


#
#  Mac OS X 10.2
#
ifeq (10.2, $(findstring 10.2, $(macosx_version)))
    PROJBUILD := /usr/bin/pbxbuild -buildstyle $(MODEL)
endif


#
#  Mac OS X 10.3
#
ifeq (10.3, $(findstring 10.3, $(macosx_version)))
    PROJBUILD := /usr/bin/xcodebuild -buildstyle $(MODEL)
endif


#
#  Mac OS X 10.4
#
ifeq (10.4, $(findstring 10.4, $(macosx_version)))
    PROJBUILD := /usr/bin/xcodebuild -configuration $(MODEL)
    IODBC_TARGET=10.4
    IODBC_SDK=/Developer/SDKs/MacOSX10.4u.sdk/
endif


#
#  Mac OS X 10.5
#
ifeq (10.5, $(findstring 10.5, $(macosx_version)))
    PROJBUILD := /usr/bin/xcodebuild -configuration $(MODEL)
    IODBC_TARGET=10.5
    IODBC_SDK=/Developer/SDKs/MacOSX10.5.sdk/
endif


#
#  Mac OS X 10.6
#
ifeq (10.6, $(findstring 10.6, $(macosx_version)))
    PROJBUILD := /usr/bin/xcodebuild -configuration $(MODEL)
    IODBC_TARGET=10.6
    IODBC_SDK=/Developer/SDKs/MacOSX10.6.sdk/
endif


#
#  Mac OS X 10.7
#
ifeq (10.7, $(findstring 10.7, $(macosx_version)))
    PROJBUILD := /usr/bin/xcodebuild -configuration $(MODEL) -sdk macosx10.7
    IODBC_TARGET=10.7
endif


#
#  Mac OS X 10.8
#
ifeq (10.8, $(findstring 10.8, $(macosx_version)))
    PROJBUILD := /usr/bin/xcodebuild -configuration $(MODEL) -sdk macosx10.8
    IODBC_TARGET=10.8
endif


#
#  Mac OS X 10.9
#
ifeq (10.9, $(findstring 10.9, $(macosx_version)))
    PROJBUILD := /usr/bin/xcodebuild -configuration $(MODEL) -sdk macosx10.9
    IODBC_TARGET=10.9
endif


#
#  Mac OS X 10.10
#
ifeq (10.10, $(findstring 10.10, $(macosx_version)))
    PROJBUILD := /usr/bin/xcodebuild -configuration $(MODEL) -sdk macosx10.10
    IODBC_TARGET=10.10
endif


#
#  Pass settings to xcodebuild
#
export IODBC_TARGET IODBC_SDK


#
#  Build the libraries
#
all: include
	@echo "Building on Mac OS X $(macosx_version)"
	@echo "Projbuild is [$(PROJBUILD)]"
	cd iODBCinst;		$(PROJBUILD) build
	cd iODBC;		$(PROJBUILD) build
	cd iODBCtest;		$(PROJBUILD) build
	cd iODBCtestw;		$(PROJBUILD) build
	cd iODBCdrvproxy;	$(PROJBUILD) build
	cd iODBCadm;		$(PROJBUILD) build
	cd iODBCadministrator;	$(PROJBUILD) build
	cd iODBCdemo;		$(PROJBUILD) build -alltargets


#
#  Generate iODBC framework style include files
#
include: framework-include.sh ../include/sql.h ../include/sqlext.h ../include/sqltypes.h ../include/iodbcext.h ../include/iodbcunix.h ../include/odbcinst.h
	sh framework-include.sh


#
#  Install libraries in /Library/Frameworks
#
install:
	#
	#  Install Frameworks
	#
	-rm $(DESTDIR)/Library/Frameworks/iODBC.framework/Versions/Current
	-rm $(DESTDIR)/Library/Frameworks/iODBCinst.framework/Versions/Current
	cd iODBCinst;		$(PROJBUILD) install DSTROOT=$(DESTDIR)/
	cd iODBC;		$(PROJBUILD) install DSTROOT=$(DESTDIR)/
	#
	#  Install /usr/local/iODBC
	#
	sh link-inclibs.sh
	#
	#  Install iODBC Administrator
	#
	cd iODBCadministrator;	$(PROJBUILD) install DSTROOT=$(DESTDIR)/
	#  Next ones are a bit special as they need to be installed twice
	cd iODBCadm;		$(PROJBUILD) install DSTROOT=$(DESTDIR)/Library/Frameworks/iODBC.framework/Resources/
	cd iODBCadm;		$(PROJBUILD) install DSTROOT=$(DESTDIR)/Library/Frameworks/iODBCinst.framework/Resources/
	cd iODBCdrvproxy;	$(PROJBUILD) install DSTROOT=$(DESTDIR)/Library/Frameworks/iODBC.framework/Resources/
	cd iODBCdrvproxy;	$(PROJBUILD) install DSTROOT=$(DESTDIR)/Library/Frameworks/iODBCinst.framework/Resources/
	#
	#  Install application
	#
	cd iODBCdemo;		$(PROJBUILD) install -alltargets DSTROOT=$(DESTDIR)/
	cd iODBCtest;		$(PROJBUILD) install DSTROOT=$(DESTDIR)/
	cd iODBCtestw;		$(PROJBUILD) install DSTROOT=$(DESTDIR)/
	#
	#  Install CFM bridge for Microsoft Office
	#
	-mkdir -p "$(DESTDIR)/Library/CFMSupport/iODBC CFM Bridge"
	install -m755 "iODBCcfmbridge/iODBC_CFM_Bridge" "$(DESTDIR)/Library/CFMSupport/iODBC CFM Bridge"
	install -m755 "iODBCcfmbridge/iODBC_CFM_Bridge.res" "$(DESTDIR)/Library/CFMSupport/._iODBC CFM Bridge"
	/System/Library/CoreServices/FixupResourceForks /Library/CFMSupport


#
#  Clean out the targets
#
clean:
	rm -rf include
	cd iODBC;		$(PROJBUILD) clean
	cd iODBCinst;		$(PROJBUILD) clean
	cd iODBCtest;		$(PROJBUILD) clean
	cd iODBCtestw;		$(PROJBUILD) clean
	cd iODBCdrvproxy;	$(PROJBUILD) clean
	cd iODBCadm;		$(PROJBUILD) clean
	cd iODBCadministrator;	$(PROJBUILD) clean
	cd iODBCdemo;		$(PROJBUILD) clean


realclean: clean
	rm -rf iODBC/build
	rm -rf iODBCadm/build
	rm -rf iODBCadministrator/build
	rm -rf iODBCdemo/build
	rm -rf iODBCdrvproxy/build
	rm -rf iODBCinst/build
	rm -rf iODBCtest/build
	rm -rf iODBCtestw/build
