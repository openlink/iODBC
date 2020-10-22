# Using the iODBC GIT Tree

    Copyright (C) 1996-2020 OpenLink Software <iodbc@openlinksw.com>


## Introduction
This document describes how to checkout a copy of the git tree for development purposes. It also lists the packages that
need to be installed prior to generating the necessary scripts and Makefiles to build the project.

Git access is only needed for developers who want to actively track progress of the iODBC source code and contribute
bugfixes or enhancements to the project. It requires basic knowledge of git itself, the general layout of open source and
GNU projects, the use of autoconf and automake etc, which is beyond the scope of this document.

If you have any questions, please email us at [iodbc@openlinksw.com](mailto:iodbc@openlinksw.com).


## Git Archive Server Access
For main development OpenLink Software will publish the iODBC tree to GitHub and encourage everyone who is interested in
tracking the project, to make an account there.

Users who mainly just want to track the code can use the following command to get a copy of the tree:

    $ git clone git://github.com/openlink/iODBC

At this point you can create your own work branch based on any of the branches available, create bugfixes and commit them
to your own branch and then use the 'git format-patch' command to generate the appropriate diffs to send to
[iodbc@openlinksw.com](mailto:iodbc@openlinksw.com)

Developers are encouraged to fork the project using GitHub, create their own branches to make enhancements/bugfixes and
then send pull requests using the excellent GitHub interface for the OpenLink team to examine and incorporate the fixes
into the master tree for an upcoming release.

Github has [excellent documentation](http://help.github.com/) on how to fork a project, send pull requests, track the
project etc.

OpenLink Software will continue to use sourceforge.net for the source tarball releases and certain binary releases, and
for completeness will also provides read-only Git Archive access.

For more information check the [iODBC SourceForge pages](https://sourceforge.net/scm/?type=git&group_id=161622)


## Package Dependencies
To generate the configure script and all other build files necessary, please make sure the following packages and recommended
versions are installed on your system.

    | Package  | Version  | From                               |
    | -------- | -------- | ---------------------------------- |
    | autoconf | 2.59     | ftp://ftp.gnu.org/pub/gnu/autoconf |
    | automake | 1.9.6    | ftp://ftp.gnu.org/pub/gnu/automake |
    | libtool  | 1.5.22   | ftp://ftp.gnu.org/pub/gnu/libtool  |
    | make     | 3.79.1   | http://www.gnu.org/software/make   |
    | gtk+     | 1.2.10   | ftp://ftp.gtk.org/pub/gtk/v1.2     |

and any GNU packages required by these.

The autogen.sh and configure scripts check for the presence and right version of some of the required components.

The above version are the minimum recommended versions of these packages. Older version of these packages can sometimes
be used, but could cause build problems.

To check the version number of the tools installed on your system, use one of the following commands:

    $ autoconf --version
    $ automake --version
    $ libtoolize --version
    $ make --version
    $ gtk-config --version


## Generate build files
To generate the configure script and all related build files, use one
of the following commands:

    $ autoreconf --install

or use the supplied script in your checkout directory:

    $ ./autogen.sh

If the above commands succeed without any error messages, please use the following command to check out all the options
you can use:

    $ ./configure --help

Certain build targets are only enabled when the --enable-maintainer-mode flag is added to configure.

Please read the files INSTALL and README in this directory for further information on how to configure the package and
install it on your system.
