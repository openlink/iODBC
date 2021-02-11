# Using the iODBC git tree

Copyright (C) 1996-2021 OpenLink Software <iodbc@openlinksw.com>

## Introduction

This document describes how to check out a copy of the git tree for development
purposes. It also lists the packages that need to be installed prior to generating
the necessary scripts and makefiles to build the project.

Git access is only needed by developers who want to actively track progress of the
iODBC source code and contribute bugfixes or enhancements to the project. It
requires basic knowledge of git itself, the general layout of open source and GNU
projects, the use of `autoconf` and `automake`, etc., which are beyond the scope
of this document.

If you have any questions, please email us at
[iodbc@openlinksw.com](mailto:iodbc@openlinksw.com).

## Git Archive Server Access

For main development, OpenLink Software publishes the iODBC tree to GitHub, and we
encourage everyone who is interested in tracking the project to make an account there.

Users who mainly just want to track the code can use the following command to get
a copy of the tree:

    $ git clone git://github.com/openlink/iODBC

At this point, you can create your own work branch based on any of the branches
available, create bugfixes and commit them to your own branch, and then use the
`git format-patch` command to generate the appropriate diffs to send to
[iodbc@openlinksw.com](mailto:iodbc@openlinksw.com).

Developers are encouraged to fork the project using GitHub, create their own branches
to make enhancements/bugfixes, and then send pull requests using the excellent GitHub
interface, for the OpenLink team to examine and incorporate into the master tree for
an upcoming release.

Github has [excellent documentation](http://help.github.com/) on how to fork a project,
send pull requests, track a project, etc.

OpenLink Software will continue to use [sourceforge.net](https://sourceforge.net/projects/iodbc/)
for the source tarball releases and certain binary releases, and for completeness will also
provide read-only Git Archive access there.

For more information, check the
[iODBC SourceForge pages](https://sourceforge.net/projects/iodbc/)

## Package Dependencies

To generate the `configure` script and all other build files necessary, please make
sure the following packages (and any GNU packages required by them) and recommended
versions are installed on your system.

*Note -- these are minimum versions; later versions should work just fine. Older 
versions of these packages can sometimes be used, but could cause build problems.*

| Package  | Minimum Version  | Source                             |
| -------- | ---------------- | ---------------------------------- |
| autoconf | 2.59             | ftp://ftp.gnu.org/pub/gnu/autoconf |
| automake | 1.9.6            | ftp://ftp.gnu.org/pub/gnu/automake |
| libtool  | 1.5.22           | ftp://ftp.gnu.org/pub/gnu/libtool  |
| make     | 3.79.1           | http://www.gnu.org/software/make   |
| gtk+     | 1.2.10           | ftp://ftp.gtk.org/pub/gtk/v1.2     |

The `autogen.sh` and `configure` scripts check for the presence and version of some
of the required components.

To check the version numbers of the tools installed on your system, you can use
the following commands:

    $ autoconf --version
    $ automake --version
    $ libtoolize --version
    $ make --version
    $ gtk-config --version

## Generate build files

To generate the `configure` script and all related build files, use the following
command:

    $ autoreconf --install

Alternatively, use the supplied script in your check-out directory:

    $ ./autogen.sh

If the above commands succeed without any error messages, you can use the following
command to see all the options you can use:

    $ ./configure --help

Certain build targets are only enabled when the `--enable-maintainer-mode` flag is
added to `configure`.

Please read the `INSTALL` and `README` files in this directory for further information
on how to configure the package and install it on your system.
