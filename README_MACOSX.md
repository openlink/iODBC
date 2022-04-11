# How to build iODBC for macOS (formerly known as _OS X_ and _Mac OS X_)

    Copyright (C) 1996-2022 OpenLink Software <iodbc@openlinksw.com>

## Method 1: Using Project builder

This build method only works for Xcode 8.0 or newer.

This version of Xcode was first supported on macOS El Capitan (10.11), however the resulting binaries
are backward-compatible with OS X Mavericks (10.9) and newer.

OpenLink uses build environments on macOS Mojave (10.14) and macOS Big Sur (11.x) to produce suitable
notarized installers with graphical components.

To build iODBC components yourself, you will first need to install the latest Mac OS X Developer 
Packages, which can be found at —

    http://developer.apple.com/tools

Then, execute the following commands in a terminal session, to build all the frameworks and demo applications:

    $ cd mac 
    $ make


After building the iODBC libraries and applications, you have to install them on your system with the command:

    $ sudo make install

This installs the `iODBCinst` and `iODBC` frameworks into —

    /Library/Frameworks/iODBC.framework 
    /Library/Frameworks/iODBCinst.framework

— and the test applications `iodbctest` and `iodbctestw` into —

    /Library/Application Support/iODBC/bin

— and the iODBC Administrator and iODBC Demo applications in —

    /Applications/iODBC

### `iODBC Administrator.app`

Now that you have installed the iODBC frameworks on your system, you are able to use ODBC applications or build your own
applications using the iODBC API.

Once you have installed an ODBC Driver, you can configure and test a new ODBC DSN, using either the 32-bit Cocoa UI, which can configure
and test any ODBC driver that is built in Universal mode —

     /Applications/iODBC/iODBC Administrator.app

— or the 64-bit version, which can configure drivers that only support a 64-bit Cocoa UI —

     /Applications/iODBC/iODBC Administrator64.app

### Test DSN connection

Once you have configured a DSN, you will be able to make a connection using the `iodbctest` tool which is located at —

    /Library/Application Support/iODBC/bin/iodbctest

## Method 2: Using `configure` and `make`

The iODBC package can also be built like any other Open Source package not using any frameworks.

This build method still works with older versions of Xcode.

On OS X Yosemite (10.10) and newer, Apple removed a number of programs from their **`Xcode.app`** commandline installation, including
`autoconf`, `automake`, `libtool`, and some other tools needed to build iODBC from a newly checked out GIT tree. We suggest
using the [HomeBrew package manager](http://brew.sh/) to install these tools, according to their documentation.

To build the libraries, open up a terminal session in **`Terminal.app`** or similar, and execute the following commands:

    $ sh autogen.sh
    $ ./configure
    $ make

To install the header files and libraries in `/usr/local`, execute the following command as an administrator, 
and provide that user's password when prompted:

    $ sudo make install

Note that, by default, this will build components that only run on the CPU type you are building on, so `ppc` 
on very old systems, `x86` on early CoreDuo machines, `x86_64` on recent Intel models, and `arm64` on current 
Apple Silicon models.

You can also build iODBC components that support multiple CPU architectures — either Universal components
that support `ppc`, `x86`, and/or `x86_64`; or Universal2 components that support `x86_64` (including as 
emulated by Rosetta2) and `arm64`.

### Mac OS X Leopard (10.5) and Mac OS X Snow Leopard (10.6)

The following commands will build a release of iODBC that supports Mac OS X Leopard (10.5) as well as 
Mac OS X Snow Leopard (10.6), on `ppc` (including as emulated by Rosetta), `x86`, and `x86_64`:

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


### Mac OS X Lion (10.7) and OS X Mountain Lion (10.8)

The following commands will build a release of iODBC that works on Mac OS X Lion (10.7) 
and OS X Mountain Lion (10.8):

    $ CFLAGS="-O -arch i386 -arch x86_64"
    $ CFLAGS="$CFLAGS -mmacosx-version-min=10.7"
    $ export CFLAGS

    $ sh autogen.sh
    $ ./configure \
        --disable-dependency-tracking \
        --prefix=/usr/local/iODBC.universal

    $ make
    $ sudo make install

### OS X Mavericks (10.9) through macOS Big Sur (11.x) on `x86` or `x86_64` (including Rosetta2 emulation)

The following commands will build a release of iODBC that works on OS X Mavericks (10.9) through macOS Big Sur (11.x), supporting other components built for `x86` (through macOS Mojave (10.14), where Apple dropped support for 32-bit components) or `x86_64` (including Rosetta2 emulation):

    $ CFLAGS="-O -arch i386 -arch x86_64"
    $ CFLAGS="$CFLAGS -mmacosx-version-min=10.9"
    $ export CFLAGS

    $ sh autogen.sh
    $ ./configure \
        --disable-dependency-tracking \
        --prefix=/usr/local/iODBC.universal

    $ make
    $ sudo make install

### macOS Big Sur (11.x) including Apple Silicon support

The following commands will build a release of iODBC that works on macOS Big Sur (11.x), 
running on Intel (`x86_64`) or Apple Silicon (M1 a/k/a `arm64`):

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

Once you have installed an ODBC driver and configured a DSN, you will be able to 
make a connection using the `iodbctest` tool which is located at —

    /usr/local/iODBC.universal/bin/iodbctest
