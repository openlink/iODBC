#
#  libiodbc.spec
#
#  $Id$
#
#  RPM specification file to build binary distribution set
#
#  (C)Copyright 1999 OpenLink Software.
#  All Rights Reserved.
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
#

# ----------------------------------------------------------------------
# BASE PACKAGE
# ----------------------------------------------------------------------
Summary: iODBC Driver Manager
name: libiodbc
version: 3.0.0
release: 1
Group: Libraries
Vendor: Ke Jin 
Packager: OpenLink Software <iodbc@openlinksw.com>
Copyright: LGPL
URL: http://www.iodbc.org/
Source: http://www.iodbc.org/dist/libiodbc-%{PACKAGE_VERSION}.tar.gz
#Prefix: /
BuildRoot:/tmp/libiodbc.root
AutoReqProv: yes

%description
The iODBC Driver Manager is a free implementation of the SAG CLI and
ODBC compliant driver manager which allows developers to write ODBC
compliant applications that can connect to various databases using
appropriate backend drivers.

The iODBC Driver Manager was originally created by Ke Jin and is 
currently maintained by OpenLink Software under an LGPL license.

%package devel
Summary: header files and libraries for iODBC development
Group: Development/Libraries
Requires: libiodbc

%description devel
The iODBC Driver Manager is a free implementation of the SAG CLI and
ODBC compliant driver manager which allows developers to write ODBC
compliant applications that can connect to various databases using
appropriate backend drivers.

This package contains the header files and libraries needed to develop
program that use the driver manager.

The iODBC Driver Manager was originally created by Ke Jin and is 
currently maintained by OpenLink Software under an LGPL license.

%prep
%setup
%build
./configure --prefix=/usr --enable-odbc3 --with-iodbc-inidir=/etc
make

%install
rm -rf $RPM_BUILD_ROOT
make install prefix=$RPM_BUILD_ROOT/usr
mkdir -p $RPM_BUILD_ROOT/etc
install -m644 odbc.ini.sample $RPM_BUILD_ROOT/etc/odbc.ini
# install -m644 odbcinst.ini.sample $RPM_BUILD_ROOT/etc/odbcinst.ini

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files 
%attr(0755, root, root) /usr/lib/libiodbc.so.2
%attr(0755, root, root) /usr/lib/libiodbc.so.2.1.0
%attr(0644, root, root) %config /etc/odbc.ini
# %attr(0644, root, root) %config /etc/odbcinst.ini

%files devel
%attr(0644, root, root) %doc AUTHORS COPYING ChangeLog NEWS README
%attr(0644, root, root) /usr/include/isql.h
%attr(0644, root, root) /usr/include/isqlext.h
%attr(0644, root, root) /usr/include/isqltypes.h
%attr(0644, root, root) /usr/include/sql.h
%attr(0644, root, root) /usr/include/sqlext.h
%attr(0644, root, root) /usr/include/sqltypes.h
%attr(0755, root, root) /usr/bin/iodbc-config
%attr(0755, root, root) /usr/lib/libiodbc.so
%attr(0644, root, root) /usr/lib/libiodbc.a
%attr(0644, root, root) /usr/lib/libiodbc.la
