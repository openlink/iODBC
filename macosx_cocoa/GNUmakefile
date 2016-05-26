#
#  Makefile.mac
#
#  The iODBC driver manager.
#
#  Copyright (C) 1996-2011 by OpenLink Software <iodbc@openlinksw.com>
#  All Rights Reserved.
#
#  This software is released under the terms of either of the following
#  licenses:
#
#      - GNU Library General Public License (see LICENSE.LGPL)
#      - The BSD License (see LICENSE.BSD).
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
all:
	@echo "Building on Mac OS X $(macosx_version)"
	@echo "Projbuild is [$(PROJBUILD)]"
	cd iODBCimport;		$(PROJBUILD) build
	cd iODBCuninstall;	$(PROJBUILD) build


#
#  Install libraries in /Library/Frameworks
#
install:
	cd iODBCimport;		$(PROJBUILD) install DSTROOT=/
	cd iODBCuninstall;	$(PROJBUILD) install DSTROOT=/


#
#  Clean out the targets
#
clean:
	cd iODBCimport; 	$(PROJBUILD) clean
	cd iODBCuninstall;	$(PROJBUILD) clean

realclean: clean
	rm -rf iODBCimport/build iODBCuninstall/build

