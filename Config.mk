#
#	Copyright (C), 1995 by Ke Jin <kejin@empress.com>
#
#	This file now serves for reference purpose only.
#	Please use 'autoconfig' or 'build' included in this 
#	package to build iODBC driver manager by following 
#	the instruction in 'README'.
#
#	platform supporting include:
#
#	SunOS			4..1.x (sparc)
#	IBM AIX			3.x, 4.x
#	HP/UX(s300/s400)	9.x, 10.x
#	HP/UX(s700/s800)	9.x, 10.x
#	Sun Solaris		2.x (sparc and x86)
#	NCR SVR4		3.x
#	Novell UnixWare SVR4	1.x, 2.x
#	Concurrent MAX/OS SVR4	1.x
#	SCO OpenServer		5.x
#	DG/UX			5.x 
#	FreeBSD			2.x
#	BSDI BSD/OS		2.x
#	Linux ELF		1.2.x, 1.3.x 
#	SGI Irix		5.x, 6.x
#	DEC Unix(OSF/1)		3.x, 4.x


#============ Default for all system ==============
#	This section should always be uncommented.
#	The values in this section may be overridden 
#	by values in one of following section when
#	it is uncommented for a given system.
#
SHELL	=
SHELL	= /bin/sh
DLDAPI	= DLDAPI_SVR4_DLFCN
DLSUFFIX= so
OUTFILE = iodbc
OBJX    = 


#============ SunOS 4..1.x ========================
#
#PIC     = -pic
#CC      = acc
#LIBS    = -ldl


#=========== AIX 3.x 4.x ==========================
#
#DLDAPI  = DLDAPI_AIX_LOAD
#ANSI    = -langlvl=ansi
#LDFLAGS = -H512 -T512 -bE:shrsub.exp -bM:SRE 
#LIBS    = -lc 
#OBJX    = main.o
#DLSUFFIX= s.o
#CFLAGSX = -DCLI_NAME_PREFIX=\".SQL\"
#
# Do we need a -bglink:<...> option in the link line ??
#


#============ HP/UX (s300/s400) 9.x 10.x ==========
#
#DLDAPI  = DLDAPI_HP_SHL
#ANSI    = -Aa
#PIC     = +z
#LDFLAGS = -b
#DLSUFFIX= sl
#CFLAGSX = -D_INCLUDE_POSIX_SOURCE -DCLI_NAME_PREFIX=\"_SQL\"
#
## Note: On hp s300/s400, when linking an application with the
##       iODBC driver manager, one needs to link with 
##       /usr/lib/libdld.sl library with -ldld option on ld or 
##       cc command line.


#============ HP/UX (s700/s800) 9.x 10.x ==========
#
#DLDAPI  = DLDAPI_HP_SHL
#ANSI    = -Aa
#PIC     = +z
#LDFLAGS = -b
#LIBS    = -lc -ldld
#DLSUFFIX= sl
#CFLAGSX = -D_INCLUDE_POSIX_SOURCE 


#============ Solaris 2.x, SunOS 5.x (Sparc/x86) ==
#
#LDFLAGS = -G -z defs 
#LIBS    = -lc -ldl -lnsl


#============= NCR SVR4 3.x =======================
#
#PIC     = -KPIC
#LDFLAGS = -G -z defs
#LIBS    = -lc -ldl


#============= UnixWare SVR4 1.x, 2.x ==============
#
#PIC     = -KPIC
#LDFLAGS = -G -z defs
#LIBS    = -lc -ldl


#============ Concurrent Maxion MAX/OS 1.x ========
#
#PIC     = -KPIC
#LDFLAGS = -G -z defs 
#LIBS    = -lc -ldl


#============ SCO OpenServer 5.x ==================
#
#PIC     = -K PIC -b elf
#LDFLAGS = -G -z defs
#LIBS    = -lc -ldl


#============ DG/UX 5.x ===========================
#
#PIC     = -K PIC
#LDFLAGS = -G -z defs
#LIBS    = -lc -ldl


#============= FreeBSD 2.x, =======================
#
#PIC     = -fPIC
#CFLAGSX = -DCLI_NAME_PREFIX=\"_SQL\"
#LDFLAGS = -Bshareable
#LIBS    = -lc


#============= BSDI BSD/OS 2.x =====================
#
#	provided by Stuart Hayton <stuey@clic.co.uk>
#
#CC      = gcc
#DLSUFFIX= o
#LDFLAGS = -r
#LIBS    = -lc_s.2.0 -ldl


#============= Linux ELF 1.2.x ====================
# 
# Tested on: 
#      Slackware 2.x,(kernel 1.3.x) on i386
#      Red Hat   2.x (kernel 1.2.x) on i486
#
#ANSI    = -ansi
#CC      = gcc
#PIC     = -fPIC
#LDFLAGS = -shared 
#LIBS    = -ldl


#============= SGI IRIX 5.x, 6.x ===================
#
#LDFLAGS = -shared
#LIBS    = -lc


#============= DEC Unix(OSF/1) 3.x, 4.x ============
#
#LDFLAGS = -shared 
#LIBS    = -lc
