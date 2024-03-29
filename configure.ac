AC_PREREQ(2.59)
AC_REVISION($Id$)
#
#  configure.ac
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


##########################################################################
##									##
##  Version info							##
##									##
##########################################################################
m4_define(V_iodbc_major,       [3])            # ODBC Major version
m4_define(V_iodbc_minor,       [52])           # ODBC Minor version
m4_define(V_iodbc_patch,       [17])           # Increase every release
m4_define(V_iodbc_devel,       [-dev])         # -dev, -rc1 or empty for stable

m4_define(V_iodbc_date,        [230502])       # YYMMDD of release
m4_define(V_iodbc_shlib,       [31])           # Increase every change


##########################################################################
##									##
##  Initialization							##
##									##
##########################################################################
AC_INIT([iODBC Driver Manager],
        V_iodbc_major.V_iodbc_minor.V_iodbc_patch[]V_iodbc_devel,
	[iodbc@openlinksw.com],
	[libiodbc])
AC_CONFIG_SRCDIR([include/isql.h])
AC_CONFIG_AUX_DIR(admin)
AC_CONFIG_MACRO_DIR(admin)
AC_CONFIG_HEADERS(include/config.h)
AM_INIT_AUTOMAKE
AC_CANONICAL_HOST
AM_MAINTAINER_MODE


##########################################################################
##									##
##  Version information							##
##									##
#########################################################################

#
#  Version information for SQLGetInfo(SQL_DM_VER) call
#
BUILD_MAJOR="V_iodbc_patch"		# last digit of version number
BUILD_MINOR="V_iodbc_date"		# year/month/day of release
BUILD_VER="${BUILD_MAJOR}${BUILD_MINOR}"

#
#  Library version numbers
#
lib3_version=3:V_iodbc_shlib:1	# ODBC 3.x mode
lib2_version=2:55:0		# ODBC 2.x mode (do not change)


##########################################################################
##									##
##  Check whether config.cache belongs to this machine			##
##									##
##########################################################################
AC_MSG_CHECKING(cached information)
hostcheck="$host"
AC_CACHE_VAL(ac_cv_hostcheck, [ ac_cv_hostcheck="$hostcheck" ])
if test "$ac_cv_hostcheck" != "$hostcheck"; then
  AC_MSG_RESULT(changed)

  AC_MSG_WARN([Running on a different architecture.])
  AC_MSG_WARN([Can't use cached values.])
  AC_MSG_ERROR([Please remove the invalid config.cache file, then try again.])
else
  AC_MSG_RESULT(ok)
fi


##########################################################################
##									##
##  Save configuration for later reuse					##
##									##
##########################################################################

OPL_CONFIG_NICE([config.nice])


##########################################################################
##									##
##  Set Layout for iODBC						##
##									##
##########################################################################
AC_PREFIX_DEFAULT(/usr/local)

AC_MSG_CHECKING(for iODBC installation layout)
AC_ARG_WITH(layout, [dnl
AS_HELP_STRING([--with-layout=LAYOUT], [Choose installation layout])dnl
],
[ case "${withval}" in
    yes|no)	IODBC_LAYOUT=unset ;;
    *)		IODBC_LAYOUT=${withval} ;;
  esac
], [IODBC_LAYOUT=unset]) 


#
#  Defaults for Layout
#
install_libodbc=true

case ${IODBC_LAYOUT} in
  unset)					# No Layout set
	iodbc_layout="default"
	inidir=/etc
	;;

  [[Gg]][[Nn]][[Uu]])				# GNU
	iodbc_layout="GNU"
	test "x$prefix" = xNONE && prefix=/usr/local
	inidir=${prefix}/etc/iodbc
	;;

  [[Dd]][[Ee]][[Bb]][[Ii]][[Aa]][[Nn]])		# Debian
	iodbc_layout="Debian"
	test "x$prefix" = xNONE && prefix=/usr
	inidir=/etc
	install_libodbc=false
	;;

  [[Gg]][[Ee]][[Nn]][[Tt]][[Oo]][[Oo]])		# Gentoo
	iodbc_layout="Gentoo"
	test "x$prefix" = xNONE && prefix=/usr
	includedir='${prefix}/include/iodbc'
	inidir=/etc/iodbc
	install_libodbc=false
	;;

  [[Rr]][[Ee]][[Dd]][[Hh]][[Aa]][[Tt]])		# RedHat
	iodbc_layout=RedHat
	test "x$prefix" = xNONE && prefix=/usr
	inidir=/etc
	;;

  [[Ff]][[Rr]][[Ee]][[Ee]][[Bb]][[Ss]][[Dd]])	# FreeBSD
	iodbc_layout=FreeBSD
	test "x$prefix" = xNONE && prefix=/usr
	inidir=/etc
	;;

  [[Oo]][[Pp]][[Tt]])				# /opt
	iodbc_layout=opt
	test "x$prefix" = xNONE && prefix=/opt/iodbc
	inidir=/etc
	;;

 [[Oo]][[Pp]][[Ee]][[Nn]][[Ll]][[Ii]][[Nn]][[Kk]])	# OpenLink
 	iodbc_layout=openlink
	prefix=/usr/local/iODBC
	inidir=/etc
	;;

  *)						# UNKNOWN
  	AC_MSG_RESULT(unknown)
	echo ""
	echo "Supported layouts: Debian FreeBSD GNU OpenLink opt RedHat"
	echo ""
	AC_MSG_ERROR([unable to continue])
	;;
esac
AC_MSG_RESULT([${iodbc_layout}])


##########################################################################
##									##
##  Check for C compiler						##
##									##
##########################################################################
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CC_STDC
AC_PROG_CPP
AC_LANG([C])
AC_C_CONST


##########################################################################
##									##
##  Check for standard programs						##
##									##
##########################################################################
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

LT_INIT


##########################################################################
##									##
##  Check for standard header files					##
##									##
##########################################################################
AC_HEADER_STDC
AC_HEADER_TIME
AC_CHECK_HEADERS(wchar.h)


##########################################################################
##									##
##  Check for specific library functions				##
##									##
##########################################################################
AC_CHECK_FUNCS(strerror setenv asprintf snprintf gettimeofday localtime_r)

dnl Unicode
AC_CHECK_FUNCS(wcslen wcscpy wcsncpy wcschr wcscat wcscmp towlower wcsncasecmp)


##########################################################################
##									##
##  Check if we want to build the GUI applications and libraries	##
##									##
##########################################################################
AC_ARG_ENABLE(gui, [dnl
AS_HELP_STRING([--enable-gui], [build GUI applications (default)])
AS_HELP_STRING([--disable-gui], [build GUI applications])dnl
],
[ case "${enableval}" in
    yes) USE_GUI=true ;;
    no)  USE_GUI=false ;;
    *) AC_MSG_ERROR(bad value ${enableval} for --enable-gui) ;;
  esac
],[USE_GUI=true]) 


##########################################################################
##									##
##  Check for GTK library functions					##
##									##
##########################################################################
USE_GTK=false
if test x$USE_GUI = xtrue
then
  no_gtk=""
  # Check for GTK+ 2.x
  AM_PATH_GTK_2_0(2.0.0)
  if test x$no_gtk != x
  then
      # Try GTK 1.2.x instead
      no_gtk=""
      AM_PATH_GTK(1.2.3)
  fi
  if test x$no_gtk = x
  then
    USE_GTK=true
  fi
fi

##########################################################################
##									##
##  Now make the final checks whether we can use GUI extensions		##
##									##
##########################################################################
if test x$USE_GTK = xfalse
then
  USE_GUI=false
fi

if test x$USE_GUI = xtrue
then
  AC_DEFINE(GUI, 1, [Define if we detected a GUI library we can use])
fi

AM_CONDITIONAL(GUI, [test x$USE_GUI = xtrue])
AM_CONDITIONAL(GTK, [test x$USE_GTK = xtrue])


##########################################################################
##									##
##  Check for dynamic load module					##
##									##
##########################################################################

AC_CHECK_HEADERS(dlfcn.h dl.h dld.h)

save_LIBS="$LIBS"
LIBADD_DL=""

AC_CHECK_LIB(dl, dlopen,
  [
    AC_DEFINE(HAVE_LIBDL, 1, [Define if you have the -ldl library])
    LIBADD_DL="-ldl"
    LIBS="$LIBS -ldl"
  ]
)

AC_CHECK_FUNCS(dlopen _dlopen dlsym dladdr)

AC_MSG_CHECKING([whether dlsym() requires a leading underscore in symbol names])
_LT_TRY_DLOPEN_SELF([
    AC_MSG_RESULT(no)
], [
    AC_MSG_RESULT(yes)
    AC_DEFINE(NEED_USCORE, 1, [Define if dlsym() requires a leading underscore in symbol names. ])
], [
  AC_MSG_RESULT(no)
], [])

##########################################################################
##  Check if the dl library has the Dl_info structure                   ##
##########################################################################
AC_MSG_CHECKING([for Dl_info])
AC_TRY_COMPILE([
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <dlfcn.h>
  ],[
Dl_info* info = 0;
dladdr(0, info);
  ],[
    AC_DEFINE(HAVE_DL_INFO, 1, [Define to 1 if you have the Dl_info structure])
    AC_MSG_RESULT(yes)
  ],[
    AC_MSG_RESULT(no)
])

##########################################################################
##  Check for dynamic load module (hpux)				##
##########################################################################
AC_CHECK_FUNC(shl_load,
  [AC_DEFINE(HAVE_SHL_LOAD, 1, [Define if you have the shl_load function])],
  [AC_CHECK_LIB(dld, shl_load,
    [AC_DEFINE(HAVE_SHL_LOAD, 1, [Define if you have the shl_load function]) LIBADD_DL="-ldld"])]
  )

##########################################################################
##  Check for dynamic load module (hpux)				##
##########################################################################
AC_CHECK_LIB(dld, dld_link,
   [AC_DEFINE(HAVE_DLD, 1, [Define if you have the -ldld library])dnl
    test "x$ac_cv_lib_dld_shl_load" = yes || LIBADD_DL="-ldld"])

##########################################################################
##  Check for dynamic load module (macos)				##
##########################################################################
AC_CHECK_FUNC(NSCreateObjectFileImageFromFile,
    [AC_DEFINE(HAVE_DYLD, 1, [Define if you have the DYLD library])])

AC_SUBST(LIBADD_DL)

LIBS="$save_LIBS"

##########################################################################
##									##
##  Set version information						## 
##									##
##########################################################################

AC_MSG_CHECKING(for iODBC mode)
AC_ARG_ENABLE(odbc3, [dnl
AS_HELP_STRING([--enable-odbc3], [build ODBC 3.x compatible driver manager (default)])
AS_HELP_STRING([--disable-odbc3], [build ODBC 2.x compatible driver manager])dnl
],
[
    case "${enableval}" in
      yes)	odbcmode=3 ;;
      no)	odbcmode=2 
      		AC_MSG_ERROR(--disable-odbc3 temporarily disabled)
		;;
      *)	AC_MSG_ERROR(bad value ${enableval} for --enable-odbc3) ;;
    esac
],
[
    odbcmode=3	  # set default to ODBC 3.x mode 
])

if test "x$odbcmode" = "x3"
then
	lib_version=$lib3_version
	CFLAGS="$CFLAGS -DODBCVER=0x0350 -DIODBC_BUILD=$BUILD_VER" 
	AC_MSG_RESULT(ODBC 3.x compatible mode)
else
	lib_version=$lib2_version
	CFLAGS="$CFLAGS -DODBCVER=0x0250 -DIODBC_BUILD=$BUILD_VER" 
	AC_MSG_RESULT(ODBC 2.x compatible mode)
fi
AC_SUBST(odbcmode)
AC_SUBST(lib_version)


##########################################################################
##									##
##  Where to find the system odbc.ini file				**
##									##
##########################################################################

#
#  First determine the systemwide default directory according 
#  to GNU specifications
#
AC_MSG_CHECKING(system config directory)
sysinidir=`eval echo $sysconfdir`
case "$sysinidir" in
NONE*) 
       sysinidir=$ac_default_prefix/etc
       ;;
/usr/etc)
       sysinidir=/etc
       ;;
*)
       sysinidir=`eval echo $sysconfdir`
       ;;
esac
AC_MSG_RESULT($sysinidir)

#
#  Now allow to overrule this directory with a custom setting
#
AC_MSG_CHECKING(for iODBC ini directory)
AC_ARG_WITH(iodbc-inidir, [dnl
AS_HELP_STRING([--with-iodbc-inidir=DIR], [where the system odbc.ini file should be located])
AS_HELP_STRING([ ], [(default is /etc)])dnl
],
[ 
    case "$withval" in
    yes|no)
        inidir=$sysinidir
	;;
    *)
    	inidir=$withval
	;;
    esac 
])
AC_MSG_RESULT($inidir)

AC_SUBST(inidir)
AC_DEFINE_UNQUOTED(SYS_ODBC_INI, "$inidir/odbc.ini",
	[Define path to systemwide odbc.ini file])
AC_DEFINE_UNQUOTED(SYS_ODBCINST_INI, "$inidir/odbcinst.ini",
	[Define path to systemwide odbcinst.ini file])

#
#  Now allow to override default FileDSN directory with a custom setting
#
filedsnpath=$inidir/ODBCDataSources
AC_MSG_CHECKING(for FILEDSN default directory)
AC_ARG_WITH(iodbc-filedsnpath, [dnl
AS_HELP_STRING([--with-iodbc-filedsnpath=DIR], [default location for FILEDSN data sources])
AS_HELP_STRING([ ], [(default is $inidir/ODBCDataSources)])dnl
],
[ 
    case "$withval" in
    yes|no)
        filedsnpath=$inidir/ODBCDataSources
	;;
    *)
    	filedsnpath=$withval
	;;
    esac 
])
AC_MSG_RESULT($filedsnpath)

AC_SUBST(filedsnpath)
AC_DEFINE_UNQUOTED(DEFAULT_FILEDSNPATH, "$filedsnpath",
	[Default location for FILEDSN data sources])


##########################################################################
##									##
##  Create libodbc.so if requested                     			##
##									##
##########################################################################

AC_MSG_CHECKING(for libodbc.so link)
AC_ARG_ENABLE(libodbc, [dnl
AS_HELP_STRING([--enable-libodbc], [install extra libodbc.so library (default)])
AS_HELP_STRING([--disable-libodbc], [do not install extra libodbc.so library ])dnl
],
[ case "${enableval}" in
    yes) install_libodbc=true ;;
    no)  install_libodbc=false ;;
    *) AC_MSG_ERROR(bad value ${enableval} for --enable-libodbc) ;;
  esac
],[]) 
AC_MSG_RESULT([$install_libodbc])

AM_CONDITIONAL(INSTALL_LIBODBC, [test x$install_libodbc = xtrue])


##########################################################################
##									##
##  Checkout pthread situation						##
##									##
##########################################################################

# for backward compatibility
AC_ARG_WITH(pthreads,[],
[ 
  AC_MSG_WARN([--with-pthreads flag is deprecated, use --enable-pthreads])
],[]) 


AC_MSG_CHECKING(for thread model)
AC_ARG_ENABLE(pthreads, [dnl
AS_HELP_STRING([--enable-pthreads], [build threaded libraries and applications (default)])
AS_HELP_STRING([--disable-pthreads], [build non-threaded libraries and applications])dnl
],
[ case "${enableval}" in
    yes) USE_PTHREADS=true ;;
    no)  USE_PTHREADS=false ;;
    *) AC_MSG_ERROR(bad value ${enableval} for --enable-pthreads) ;;
  esac
],[USE_PTHREADS=true]) 

if test x$USE_PTHREADS = xtrue
then
    AC_MSG_RESULT(enabled pthread support)
    CPPFLAGS="$CPPFLAGS -DWITH_PTHREADS -D_REENTRANT"
    found_lib=false

    AC_MSG_CHECKING(for OS dependent thread flags)
    case $host in
      *-aix*)	
		#  Unless overruled by the user, we prefer this compiler
		if test "x$CC" = "xcc"
		then
		    CC=cc_r7
		fi
		AC_MSG_RESULT([using $CC for AIX])
		;;

      *UnixWare*)
      		CFLAGS="$CFLAGS -Kthread"
		AC_MSG_RESULT([use -Kthread for UnixWare])
		;;

      i[3456]86-dg-dgux*)
      		CFLAGS="$CFLAGS -D_POSIX4A_DRAFT6_SOURCE"
		LIBS="$LIBS -lthread"
		AC_MSG_RESULT([use -lthread for DG/UX])
		;;

      *-freebsd*)	
		CFLAGS="$CFLAGS -pthread"
		AC_MSG_RESULT([use -pthread for FreeBSD])
		;;

	*-hpux*)
		LIBS="$LIBS -lpthread -lcl"
		AC_MSG_RESULT([use -lpthread for HP/UX])
		;;

	*-osf3.*)
		CPPFLAGS="$CPPFLAGS -DOLD_PTHREADS"
		LIBS="$LIBS -lpthreads -lmach -lc_r"
		AC_MSG_RESULT([use -lpthreads for OSF 3.2])
		;;

	*-osf4.*)
	        LIBS="$LIBS -lpthread"
		AC_MSG_RESULT([use -lpthread for OSF 4.x])
	  	;;
		
      *)
		AC_MSG_RESULT([none])
		;;
    esac

    #
    #  Check headers
    #
    AC_CHECK_HEADER([pthread.h], , [AC_MSG_ERROR([pthreads header not found])])

    #
    #  If mutex functions are in C library, we should not need to link
    #  with -lpthread
    #
    AC_CHECK_FUNC(pthread_mutex_lock, [found_lib=true])

    #
    #  Check out the various libraries
    #
    if test x$found_lib = xfalse
    then
	IODBC_CHECK_PTHREAD_LIB(pthreads, pthread_mutex_lock,
	[
	    LIBS="$LIBS -lpthreads"
	    found_lib=true
	])
    fi
    if test x$found_lib = xfalse
    then
	IODBC_CHECK_PTHREAD_LIB(pthread, pthread_mutex_lock, 
	[
	    LIBS="$LIBS -lpthread"
	    found_lib=true
	])
    fi 
else
    AC_MSG_RESULT(no thread support)
fi


########################################################################## 
##									##
##  OS dependent libraries						##
##									##
##########################################################################
LIBADD_OS=""
AC_MSG_CHECKING(for OS dependent libraries and link flags)
AM_CONDITIONAL(DARWIN, false)	
case $host in
  *-hpux*)
	LDFLAGS="$LDFLAGS -Wl,+s"
	AC_MSG_RESULT([use -Wl,+s for HP/UX])
	;;
  *-darwin*)
	AM_CONDITIONAL(DARWIN, true)	
	CFLAGS="$CFLAGS -DNO_FRAMEWORKS"
	LIBADD_OS="-Wl,-framework,Carbon"
	AC_MSG_RESULT([$LIBADD_OS])
	;;
  *)
	AC_MSG_RESULT(none)
	;;
esac
AC_SUBST(LIBADD_OS)


########################################################################## 
##									##
##  Generate Makefiles etc.						##
##									##
##########################################################################
AC_CONFIG_FILES([
    Makefile 
    admin/Makefile
    admin/libiodbc.spec
    admin/libiodbc.pc
    bin/Makefile
    bin/iodbc-config
    etc/Makefile
    man/Makefile
    man/iodbc-config.1
    man/iodbctest.1
    man/iodbctestw.1
    man/iodbcadm-gtk.1
    include/Makefile
    iodbc/Makefile
    iodbc/trace/Makefile
    iodbcinst/Makefile
    iodbcadm/Makefile
    iodbcadm/gtk/Makefile
    drvproxy/Makefile
    drvproxy/gtk/Makefile
    samples/Makefile
])
AC_CONFIG_COMMANDS([default],[
    chmod 755 bin/iodbc-config
])
AC_OUTPUT


########################################################################## 
##									##
##  Display configuration information					##
##									##
##########################################################################

# ----------------------------------------------------------------------
#  Color settings
# ----------------------------------------------------------------------
B=`tput bold 2>/dev/null`
N=`tput sgr0 2>/dev/null`

cat <<EOF


${B}${PACKAGE_STRING} configuration summary${N}
`echo "${PACKAGE_STRING} configuration summary" | sed 's/./=/g'`

${B}Installation variables${N}
  layout		${iodbc_layout}
  prefix		${prefix}
  exec_prefix		${exec_prefix}

${B}Installation paths${N}
  programs		${bindir}
  include files		${includedir}
  libraries		${libdir}
  manual pages		${mandir}

${B}Configuration files${N}
  odbc.ini		${inidir}/odbc.ini
  odbcinst.ini		${inidir}/odbcinst.ini
  default FILEDSN path	${filedsnpath}

${B}Extensions${N}
  ODBC Version		${odbcmode}
  GUI Extensions	${USE_GUI}
  ThreadSafe		${USE_PTHREADS}
  Install libodbc.so	${install_libodbc}

EOF
