# How to build iODBC for Mac OS X

    Copyright (C) 1996-2021 OpenLink Software <iodbc@openlinksw.com>

## Method 1: Using Project builder

This build method only works for Xcode 8.0 or newer.

This version of Xcode was first supported on macOS 10.11 El Capitan, however the resulting binaries
are backward compatible with OS X 10.9 Mavericks and newer.

OpenLink uses build environments on macOS 10.14 Mojave and macOS 11 Big Sur to build suitable
notarized installers with graphical components.

You first need to install the latest Mac OS X Developer Packages, which can be found at:

    http://developer.apple.com/tools

Then execute the following commands from a terminal session to build all the frameworks and demo applications:

    $ cd mac 
    $ make


After building the iODBC libraries and applications, you have to install them on your system with the command:

    $ sudo make install

This installs the iODBCinst and iODBC frameworks into:

    /Library/Frameworks/iODBC.framework 
    /Library/Frameworks/iODBCinst.framework

and the test applications iodbctest and iodbctestw into:

    /Library/Application Support/iODBC/bin

and the iODBC Administrator and the iODBC Demo applications in:

    /Applications/iODBC


### iODBC Administrator
Now that you have installed the iODBC frameworks on your system, you are able to use ODBC applications or build your own
applications using the iODBC API.

Once you have installed an ODBC Driver, you can configure a new ODBC DSN using either the 32bit Cocoa UI which can configure
and test any ODBC driver that is build in universal mode:

  /Applications/iODBC/iODBC Administrator.app

or the 64bit version which can configure drivers that only support a 64bit Cocoa UI:

     /Applications/iODBC/iODBC Administrator64.app

### Test DSN connection 
Once you have configured a DSN, you will be able to make a connection using the iodbctest command which is located in:

    /Library/Application Support/iODBC/bin/iodbctest



## Method 2: Using configure and make
The iODBC package can also be build like any other Open Source package not using any frameworks.

This build method still works with older versions of Xcode.

On OS X 10.10 Yosemite and newer, Apple removed a number of programs from their Xcode.app commandline installation including
the autoconf, automake, libtool, and some other tools needed to build iODBC from a newly checked out GIT tree. We suggest
using the [HomeBrew package manager](http://brew.sh/) to install these tools.

To build the libraries, open up a terminal session and execute the following commands:

    $ sh autogen.sh
    $ ./configure
    $ make

To install the header files and libraries in /usr/local, you execute the following command as administrator:

    $ sudo make install

Note that this will build all code based that only runs on the CPU type you build it on, so PowerPC on older systems, 32bit
Intel on early CoreDuo machine and 64bit on current models.

However iODBC can also be configured to build a universal library that works with all 3 models embedded.


### Mac OS X 10.5 and 10.6
The following commands will build a release of iODBC that works on both Mac OS X 10.5 as well as Mac OS X 10.6:

    $ CFLAGS="-O -arch ppc -arch i386 -arch x86_64"
    $ CFLAGS="$CFLAGS -isysroot /Developer/SDKs/MacOSX10.5.sdk"
    $ CFLAGS="$CFLAGS -mmacosx-version-min=10.5"
    $ export CFLAGS

    $ sh autogen.sh
    $ ./configure \
	--disable-dependency-tracking \
	--prefix=/usr/local/iODBC.universal

    $ make
    $ sudo make install


### Mac OS X 10.7 Lion and Mac OS X 10.8 Mountain Lion
The following commands will build a release of iODBC that works on Mac OS X 10.7 and 10.8:

    $ CFLAGS="-O -arch i386 -arch x86_64"
    $ CFLAGS="$CFLAGS -mmacosx-version-min=10.7"
    $ export CFLAGS

    $ sh autogen.sh
    $ ./configure \
	--disable-dependency-tracking \
	--prefix=/usr/local/iODBC.universal

    $ make
    $ sudo make install


### OS X 10.9 Maveriks through macOS 10.15 Catalina
The following commands will build a release of iODBC that works on OS X 10.9 and newer:

    $ CFLAGS="-O -arch i386 -arch x86_64"
    $ CFLAGS="$CFLAGS -mmacosx-version-min=10.9"
    $ export CFLAGS

    $ sh autogen.sh
    $ ./configure \
	--disable-dependency-tracking \
	--prefix=/usr/local/iODBC.universal

    $ make
    $ sudo make install


### macOS 11 Big Sur including Apple Silicon support
The following commands will build a release of iODBC that works on OS X 11 Big Sur:

    $ CFLAGS="-O -arch arm64 -arch x86_64"
    $ CFLAGS="$CFLAGS -mmacosx-version-min=10.9"
    $ export CFLAGS

    $ sh autogen.sh
    $ ./configure \
	--disable-dependency-tracking \
	--prefix=/usr/local/iODBC.universal

    $ make
    $ sudo make install


### Test DSN connection
Once you have installed an ODBC driver and configured a DSN, you will be able to make a connection using the iodbctest
command which is located in:

    /usr/local/iODBC.universal/bin/iodbctest
