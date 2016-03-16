#!/usr/bin/perl -w
#
#  uninstall-iodbc.pl
#
#  The iODBC driver manager.
#
#  Copyright (C) 1995 by Ke Jin <kejin@empress.com>
#  Copyright (C) 1996-2014 by OpenLink Software <iodbc@openlinksw.com>
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

use strict;
use File::Basename;

use vars qw ($no_change $receipts_dir $verbose $lsbom);

$receipts_dir = "/Library/Receipts";
$no_change = 0;
$verbose = 1;
$lsbom = "/usr/bin/lsbom";


#
#  Make sure we are running as root
#
$| = 1;
if ($no_change == 0 && $< != 0) {
    die "ERROR: Must be run with root permissions; hint: use 'sudo'.\n";
}


sub main
{
    my @pkglist = ('iODBCapps', 'iODBCsdk', 'iODBCframe', 'iODBCsamples');
    my $answer;

    print STDERR <<'EOT';

    WARNING:
        The following script will remove the iODBC SDK including
        the iODBC and iODBCinst frameworks and the iODBC Administrator.

        Installed Drivers and Settings will not be removed by this script.

        Removing this package could lead to other Applications on your
        system to stop functioning properly.

EOT

    $answer = ask_yn("Are you sure you want to continue?");
    die "\n\nUninstall aborted by user" unless ($answer == 1);

    #
    #  Remove packages
    #
    foreach (@pkglist) {
	remove_package ($_);
    }

    #
    #  Remove SDK copy
    #
    remove_dir_tree ("/usr/local/iODBC");

    #
    #  Remove deprecated frameworks
    #
    remove_dir_tree ("/Library/Frameworks/iODBCadm.framework");

    print "Finished uninstalling.\n";
}


sub ask_yn
{
    my $question = shift;
    my $answer;

    print STDERR "$question (y/N): ";
    $answer = <STDIN>;

    for ($answer) {
       /^[Yy]/		and do { return 1; };
	
       # Default
       return 0;
    }
}


sub remove_package
{
    my $pkgname = shift;

    return if (!defined ($pkgname) || $pkgname eq "");

    my $pkgname_fullpath = "$receipts_dir/$pkgname.pkg";
    my $pkgname_bom_fullpath = undef;

    VERBOSE ("REMOVING PACKAGE $pkgname.pkg\n");

    #
    #  Make sure package receipt is actually there
    #
    return if (! -d "$pkgname_fullpath" );


    #
    #  Locate BOM (Bill of Materials)
    #
    foreach my $f ( "$pkgname_fullpath/Contents/Resources/$pkgname.bom",
		    "$pkgname_fullpath/Contents/Archive.bom",
		    "$pkgname_fullpath/Contents/Resources/Archive.bom") {
	if (-f $f && -r $f) {
	    $pkgname_bom_fullpath = $f;
	    last;
	}
    }
    return if (!defined ($pkgname_bom_fullpath));


    #
    #  Remove files listed in the BOM
    #
    return unless open (LSBOM, "$lsbom -l -f -p f '$pkgname_bom_fullpath' |");
    while (<LSBOM>) {
	chomp;
	m#^\.(/.*)$#;
	my $filename = $1;

	remove_a_file ($filename);
    }
    close (LSBOM);


    #
    #  Create map of directories listed in BOM (depth first)
    #
    my $rooth = { };
    return unless open (LSBOM, "$lsbom -d -p f '$pkgname_bom_fullpath' |");
    while (<LSBOM>) {
	chomp;
	m#^\.(/.*)$#;
	my $directory = $1;
	next if (!defined ($directory) || $directory eq "");

	if (-d $directory) {
	    $rooth = add_directory_to_tree ($directory, $rooth);
	}
    }
    close (LSBOM);

    #
    #  Remove all directories
    #
    remove_empty_directories ($rooth, "/");


    #
    #  Finally we can remove the receipt
    #
    remove_package_receipts ("$pkgname.pkg");
}


sub add_directory_to_tree
{
    my $dir = shift;
    my $rooth = shift;
    my $p = $rooth;

    my @pathcomp = split /\//, $dir;

    foreach (@pathcomp) {
	my $cur_name = $_;

	if ($cur_name eq "" || !defined ($cur_name)) {
	    $cur_name = "/";
	  }

	if (!defined ($p->{"$cur_name"})) {
	    $p->{$cur_name} = { };
	  }

	$p = $p->{$cur_name};
      }

    return $rooth;
}


sub remove_empty_directories
{
    my $rooth = shift;
    my $path = shift;
    my $children = (scalar (keys %{$rooth}));
    my $dirs_remain = 0;

    if ($children > 0) {
	foreach my $dirname (sort keys %{$rooth}) {
	    my $printpath;
	    $printpath = "$path/$dirname";
	    $printpath =~ s#^/*#/#;
	    remove_empty_directories ($rooth->{$dirname}, "$printpath");
	    $dirs_remain = 1 if (-d "$printpath");
	}
    }

    # If we've removed all the children, and there's a .DS_Store file, remove
    # it if it's last file sitting around.
    if ($dirs_remain == 0) {
	maybe_remove_ds_store ("$path");
    }

    remove_a_dir ("$path");
}


sub remove_a_file
{
    my $fn = shift;

    return if (!defined ($fn) || $fn eq "");
    return if (! -f $fn && ! -l $fn);

    my $dirname = dirname ($fn);
    my $basename = basename ($fn);
    my $ufs_rsrc_file = "$dirname/._$basename";

    if ($no_change == 1) {
	ECHO ("rm $fn\n");
	ECHO ("rm $ufs_rsrc_file\n") if ( -f $ufs_rsrc_file);
    } else {
	if ((unlink $fn) != 1) {
	    ERROR ("Encountered error while trying to remove \"$fn\"\n");
	}
	if (-f $ufs_rsrc_file && ((unlink ($ufs_rsrc_file)) != 1)) {
	    ERROR ("Encountered error while trying to remove \"$ufs_rsrc_file\"\n");
	}
    }
}


sub remove_a_dir
{
    my $dir = shift;

    return if (!defined ($dir) || $dir eq "");
    return if ($dir eq "/" || $dir eq "/usr");
    return if (! -d $dir);

    if ($no_change == 1) {
	ECHO ("rmdir $dir\n");
    } else {
	rmdir $dir;
    }
}


sub remove_dir_tree
{
    my $dir = shift;

    return if (!defined ($dir) || $dir eq "");
    return if ($dir eq "/" || $dir eq "/usr");
    return if (! -d $dir);

    my @rmcmd = ('/bin/rm', '-rf', "$dir");
    if ($no_change == 1) {
	ECHO ("rm -rf $dir\n");
    } else {
    	system @rmcmd;
	my $retcode = $? >> 8;
	if ($retcode != 0)
	  {
	    WARNING ("There may have been a problem removing \"$dir\"\n");
	  }
    }
}

sub remove_package_receipts
{
    my $pkgname = shift;
    $pkgname =~ s#/##g;  # There shouldn't be any path seps in the pkg name...
    return if (!defined ($pkgname) || $pkgname eq "");
    return if ($pkgname eq "." || $pkgname eq "..");

    my $pkgdir = "$receipts_dir/$pkgname";
    return if (!defined ($pkgdir) || $pkgdir eq "" || ! -d $pkgdir);

    my @rmcmd = ('/bin/rm', '-rf', "$pkgdir");
    if ($no_change == 1) {
	ECHO ("rm -rf $pkgdir\n");
    } else {
	system @rmcmd;
	my $retcode = $? >> 8;
	if ($retcode != 0) {
	    WARNING ("There may have been a problem removing \"$pkgdir\"\n");
	}
    }
}


sub maybe_remove_ds_store
{
    my $path = shift;
    my $filecount = 0;

    return if (!defined ($path) || $path eq "");
    return if ($path eq "/" || $path eq "/usr");
    return if (! -f "$path/.DS_Store");

    open (LS, "/bin/ls -a '$path' |");
    while (<LS>) {
	chomp;
	next if (m#^\.$# || m#^\.\.$#);
	$filecount++;
    }
    close (LS);

    if ($filecount == 1 && -f "$path/.DS_Store") {
	remove_a_file ("$path/.DS_Store");
    }
}


sub ECHO
{
    my $msg = shift;
    print $msg;
}

sub VERBOSE
{
    my $msg = shift;
    return if ($verbose != 1);
    print $msg;
}

sub WARNING
{
    my $msg = shift;
    print STDERR "WARNING: " . $msg;
}

sub ERROR
{
    my $msg = shift;
    print STDERR "ERROR: " . $msg;
}


main ();
