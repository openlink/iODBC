#!/bin/sh
#
#  link-inclibs.sh
#
#  The iODBC driver manager.
#
#  Copyright (C) 1996-2021 OpenLink Software <iodbc@openlinksw.com>
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
#  Installation PATHS
#
PREFIX="$DESTDIR/usr/local/iODBC"
ODBC_FW="/Library/Frameworks/iODBC.framework"
INST_FW="/Library/Frameworks/iODBCinst.framework"


#
#  Remove old installation
#
if [ -d "$PREFIX" ] ; then
  rm -rf "$PREFIX"
fi


#
#  Create new directory structure
#
mkdir -p "$PREFIX/bin"
mkdir -p "$PREFIX/lib"
mkdir -p "$PREFIX/include"


#
#  Copy header files
#
cp -f "../include/iodbcext.h"	"$PREFIX/include/iodbcext.h"
cp -f "../include/iodbcunix.h"	"$PREFIX/include/iodbcunix.h"
cp -f "../include/isql.h"	"$PREFIX/include/isql.h"
cp -f "../include/isqlext.h"	"$PREFIX/include/isqlext.h"
cp -f "../include/isqltypes.h"	"$PREFIX/include/isqltypes.h"
cp -f "../include/sql.h"	"$PREFIX/include/sql.h"
cp -f "../include/sqlext.h"	"$PREFIX/include/sqlext.h"
cp -f "../include/sqltypes.h"	"$PREFIX/include/sqltypes.h"
cp -f "../include/sqlucode.h"	"$PREFIX/include/sqlucode.h"

cp -f "../include/iodbcinst.h"	"$PREFIX/include/iodbcinst.h"
cp -f "../include/odbcinst.h"	"$PREFIX/include/odbcinst.h"


#
#  Create symlinks for libraries
#
ln -s "$ODBC_FW/iODBC"			"$PREFIX/lib/libiodbc.dylib"
ln -s "$INST_FW/iODBCinst"		"$PREFIX/lib/libiodbcinst.dylib"


#
#  Add special macOS version of iodbc-config
#
cp iodbc-config.macos			"$PREFIX/bin/iodbc-config"
chmod 755				"$PREFIX/bin/iodbc-config"


#
# Fix basic permissions
#
chmod -R og+rX "$PREFIX"


#
#  Done
#
exit 0
