# iODBC Driver Manager

    Copyright (C) 1995 Ke Jin <kejin@empress.com>
    Copyright (C) 1996-2021 OpenLink Software <iodbc@openlinksw.com>
    All Rights Reserved.

## License
Copyright 1996-2021 [OpenLink Software](http://www.openlinksw.com)

This software is released under either the GNU Library General Public License
(see [LICENSE.LGPL](./LICENSE.LGPL))
or the BSD License
(see [LICENSE.BSD](./LICENSE.BSD)).

**Note**: The only valid version of the GPL license as far as this project is
concerned is the original GNU General Public License Version 2, dated June 1991.

### Contributions

While not mandated by the BSD license, any patches you make to the iODBC project
may be contributed back into the project at your discretion. Contributions will
benefit the Open Source and Data Access community as a whole. Submissions may be
made via the [iODBC Github project](https://github.com/openlink/iODBC/) or via
email to [iodbc@openlinksw.com](mailto:iodbc@openlinksw.com).



## Introduction
Welcome to the iODBC driver manager maintained by [OpenLink Software](http://www.openlinksw.com/).

This kit will provide you with everything you need in order to develop
ODBC-compliant applications under Unix without having to pay royalties to other
parties.

This kit consists of a number of parts:

* **The iODBC driver manager.** This is a complete implementation of an ODBC
  driver manager, released under either the GNU Library General Public License or
  the BSD License. We fully comply with these licenses by giving you this product
  in source form (as well as the binary form). You can download the latest version
  of the driver manager from the [iODBC website](http://www.iodbc.org/).

* **A simple example application, `iodbctest.c`**, which gives you a command-line
  interface to SQL. You can fit this to your purposes, but at the very least this
  is useful for verification of your ODBC installation.

You can use either part stand-alone, if you wish.

An ODBC driver is still needed to affect your connection architecture. You may
build a driver with the iODBC components or obtain an ODBC driver from a commercial
vendor. OpenLink Software produces cross-platform commercial drivers as well as
maintaining the iODBC distribution: evaluation copies may be obtained via download
from the [OpenLink Software website](http://www.openlinksw.com/). Any ODBC-compliant
driver will work with the iODBC Driver Manager.

You can see the [iODBC website](http://www.iodbc.org/) for pointers to various
ODBC drivers.

## Installation of run-time distribution

You have probably already unpacked this distribution. The next step is to make sure
that your applications can find all the dynamic link libraries. Depending on your
system's implementation of dynamic link libraries, you have a number of options:

* Install the libraries in a directory that is searched by your linker by default.
  Typical locations are `/usr/lib` and `/usr/local/lib`.

* Install the libraries in some other place, and make sure that the environment
  variable your dynamic linker uses to find extra locations for dynamic link
  libraries. Most systems use the environment variable `LD_LIBRARY_PATH` to this
  end. Known exceptions include AIX which uses `LIBPATH`, and HP/UX which uses 
  `SHLIB_PATH` for 32-bit libraries.

If your system has a C compiler, you can verify the installation by compiling the
`iodbctest` program.  Otherwise, you may have ODBC applications installed on your
system which you can use.

## Configuration of run-time distribution

The iODBC driver manager looks for a file `~/.odbc.ini`, where the tilde stands
for the user's home directory. This file initially contains only a default section
where you can select which driver library to use. Copy the `odbc.ini` file from the
examples directory to `~/.odbc.ini` and make sure the right path and filename is
used for your installation.

A data source is a section (enclosed in square brackets), and the attributes for
a data source are given within this section.  The most important attribute to
iODBC for each datasource is the `Driver` attribute. This must point to the shared
library for the ODBC driver associated with the data source.

As example, the OpenLink Enterprise Edition (Multi-Tier) ODBC drivers have a number
of attributes which can be set for a data source. Here is a description:

| `odbc.ini` keyword | ODBC connect string keyword | Description |
|-----|-----|-----|
| `Host` | `HOST` | The hostname where the database resides |
| `ServerType` | `SVT` | The type of Database Agent (see `oplrqb.ini` on the server) | 
| `ServerOptions` | `SVO` | Server-specific extra options. See Enterprise Edition server-side documentation for Agents which can use this. |
| `Database` | `DATABASE` | The database to use |
| `Options` | `OPTIONS` | Connect options for the database |
| `UserName` | `UID` | The name of the database user |
| `Password` | `PWD` | The password of the database user |
| `ReadOnly` |  `READONLY` | A Yes/No value in order to make the connection read-only |
| `FetchBufferSize` | `FBS` |The number of records that are transferred in a single call to the server.  Default is `5`; maximum is `999`, minimum is `1` |
| `Protocol` | `PROTO` | The protocol to use. Set to TCP for Release 3.x and later. |


Apart from these data source-specific settings, you may add a section called
`[Communications]`, which you may use to tune the OpenLink Enterprise Edition
(Multi-Tier) driver further:

| `odbc.ini` keyword | Description |
|-----|-----|
| `ReceiveTimeout` | The time in seconds that the client application will wait for the Database Agent to start sending results. Default is `60`. |
| `BrokerTimeout` | The time in seconds that the client application will wait for the Request Broker to accept or reject a database connection request. Default is `30`. |
| `SendSize` | RPC send buffer size. A value of `0` (the default) will cause the application to use system-dependent defaults. |
| `ReceiveSize` | RPC receive buffer size. A value of `0` (the default) will cause the application to use system-dependent defaults. |
| `DebugFile` | If set, the name of a file to which debugging output from the driver should be directed. |

## iODBC driver manager platform availability

The iODBC driver manager has been ported to following platforms:

| OS               | Version       | Processor                   |
|------------------|---------------|-----------------------------|
| BSDi BSD/OS      | 2.x           | x86                         |
| DEC Unix (OSF/1) | 3.x - 5.x     | DEC Alpha                   |
| DG/UX            | 5.x           | Aviion                      |
| FreeBSD          | 2.x - 9.x     | x86                         |
| HP/UX            | 9.x - 11.x    | HP9000 s700/s800            |
| HP/UX            | 9.x           | HP9000 s300/s400            |
| IBM AIX          | 3.x - 5.x     | IBM RS6000, PowerPC         |
| Linux ELF        | 1.x, 2.x      | x86, x86_64, IA_64, PowerPC |
| Mac OS X         | 10.x          | PowerPC, x86, x86_64        |
| Max/OS SVR4      | 1.x           | Concurrent Maxion 9200 MP   |
| NCR SVR4         | 3.x           | NCR 3435                    |
| OpenVMS          | 6.x           | DEC Alpha                   |
| SCO OpenServer   | 5.x           | x86                         |
| SGI Irix SVR4    | 5.x, 6.x      | IP12 MIPS, IP22 MIPS        |
| SunOS            | 4.1.x         | Sun Sparc                   |
| Sun Solaris      | 2.x           | Sun Sparc, x86, x86_64      |
| UnixWare SVR4.2  | 1.x, 2.x      | x86                         |
| Windows NT       | 4.x           | x86                         |

As the iODBC driver manager uses `autoconf`/`automake`/`libtool`, it should be
portable to most modern UNIX-like OS out of the box. However, if you do need to
make changes to the code or the configuration files, we would appreciate it if
you would share your changes with the rest of the internet community by mailing
your patches to [iodbc@openlinksw.com](mailto:iodbc@openlinksw.com), so we can
include them for the next build.

Porting the iODBC driver manager to some non-UNIX-like operating systems such
as the Windows family (3.x, 95, NT, 200x, etc.), IBM OS/2,  and Mac Classic is 
supported, but has not been compiled or tested recently. Of course, you will need
to supply a `make`/`build` file and a short `LibMain` for creating the `iodbc.dll`.

## How to build iODBC driver manager:

Users of macOS should read the separate [README_MACOSX](./README_MACOSX.md) document for more detail
of porting to this platform.

Users of all other UNIX-like OS:

1. Run `configure` to adjust to target platform
2. Run `make`
3. Run `make install`

The `configure` program will examine your system for various compiler flags,
system options, etc. In some cases, extra flags need to be added for the `C`
compiler to work properly; for instance, on HP systems, you may need:

    $ CFLAGS="-Ae -O" ./configure --prefix=/usr/local ..........

### File Hierarchy

Note that the path of the system wide `odbc.ini` file is calculated as follows
(based on flags to `./configure`):

```
no --prefix                     default is /etc/odbc.ini
--prefix=/usr                   /etc/odbc.ini
--prefix=/xxx/yyy               /xxx/yyy/etc/odbc.ini
--sysconfdir=/xxx/yyy           /xxx/yyy/odbc.ini
--with-iodbc-inidir=/xxx/yyy    /xxx/yyy/odbc.ini
```

If the **`--with-layout=`** option is set, then the `prefix` and `sysconfdir`
parameters will be changed accordingly. Currently, this parameter understands
values of **`gentoo`**, **`redhat`**, **`gnu`**, **`debian`**, or **`opt`**
(with everything going into `/opt/iodbc/`). If both are specified, `--prefix`
argument will overrule `--with-layout`.

### Example

```
$ ./configure --prefix=/usr/local --with-iodbc-inidir=/etc
    ...
    ...
    ...
$ make
    ...
    ...
    ...
$ su
# make install
    ...
    ...
    ...
```

## `odbc.ini`

Driver manager and drivers use the `odbc.ini` file or connection string when
establishing a data source connection. On Windows, `odbc.ini` is located in
the Windows directory.

On UNIX-like OS, the iODBC driver manager looks for the `odbc.ini` file in the
following sequence:

  1. check environment variable `ODBCINI`

  2. check `$HOME/.odbc.ini`

  3. check home in `/etc/passwd` and try `.odbc.ini` in there

  4. system-wide `odbc.ini` (settable at configuration time)

Item 1 is the easiest, as most drivers will also look at this variable.

The format of `odbc.ini` (or `~/.odbc.ini`) is defined as:
```
odbc.ini            ::= data_source_list

data_source_list    ::= /* empty */
                     | data_source '\n' data_source_list

data_source         ::= '[' data_source_name ']' '\n' data_source_desc

data_source_name    ::= 'default' | [A-Za-z]*[A-Za-z0-9_]*

data_source_desc    ::= /* empty */
                     | attrib_desc '\n' data_source_desc

addrib_desc         ::= Attrib '=' attrib_value

Attrib              ::= 'Driver' | 'PID' | 'UID' | driver_def_attrib

driver_def_attrib   ::= [A-Za-z]*[A-Za-z0-9_]*
```

An example of an `odbc.ini` file:
```
;
;  odbc.ini
;
[ODBC Data Sources]
Myodbc          = Myodbc
Sample          = OpenLink Generic ODBC Driver
Virtuoso        = Virtuoso

[ODBC]
TraceFile       = /tmp/odbc.trace
Trace           = 0        ; set to 1 to enable tracing

[Sample]
Driver          = /usr/local/openlink/lib/oplodbc.so.1
Description     = Sample OpenLink DSN
Host            = localhost
UserName        = openlink
Password        = xxxx
ServerType      = Oracle 8.1.x
Database        =
FetchBufferSize = 99
ReadOnly        = no

[Virtuoso]
Driver          = /usr/local/virtuoso/lib/virtodbc.so.1
Address         = localhost:1112
Database        = Demo

[Myodbc]
Driver          = /usr/lib/libmyodbc.so
HOST            = localhost

[Default]
Driver          = /usr/local/openlink/lib/oplodbc.so.1
```

## Tracing

The iODBC driver manager traces driver's ODBC call invoked by the driver manager.
Default tracing file is `./odbc.log`. Tracing option (i.e., on/off or optional
tracing file name) can be set in `odbc.ini` file under the `[ODBC]` heading, as:

```
[ODBC]
TraceFile = <optional_trace_file>
Trace = ON | On | on | 1 | OFF | Off | off | 0
```

If `<optional_trace_file>` is `stderr` or `stdout`, i.e. --

    TraceFile = stderr

-- or --

    TraceFile = stdout

-- the tracing message will go to the terminal screen (if available).

## Further Information Sources:

* [iODBC Website](http://www.iodbc.org/) containing binaries, sources and documentation.

* [iODBC Project page on GitHub](https://github.com/openlink/iODBC/) containing source archives, GIT tree, issues forum.

* [iODBC Project page on Sourceforge](http://sourceforge.net/projects/iodbc) containing source archives, GIT tree, mailing lists, forums, bug reports.

* [OpenLink Software Website](http://www.openlinksw.com/) containing free trials and support for OpenLink's ODBC drivers.

* [Microsoft ODBC Documentation](https://msdn.microsoft.com/en-us/library/ms714177) containing the ODBC API Reference Guide.
