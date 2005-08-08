#!/usr/bin/perl 
#
# Copyright (C) 2004-2004 by John P. Weiss
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the Artistic License, included as the file
# "LICENSE" in the source code archive.
#
# This package is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# You should have received a copy of the file "LICENSE", containing
# the License John Weiss originally placed this program under.
#
# RCS $Id$
############


############
#
# Configuration Variables
#
############


############
#
# Precompilation Init
#
############
# Before anything else, split the script's name into directory/file.  We
# want to add the script's directory to the set of include-dirs.  This way, we
# get any packages living in the script's directory.
my $_MyName;
BEGIN {
    if ($0 =~ m|\A(.*)/([^/]+\Z)|) {
        if ($1 ne ".") { push @INC, $1; }
        $_MyName = $2;
    } else { $_MyName = $0; }  # No path; only the script name.
}


############
#
# Includes/Packages
#
############


require 5;
use strict;
use Getopt::Long;
use rpmUtils;
use jpwTools;


############
#
# Other Global Variables
#
############


my $_SkipModifiedPkgfiles_re
    ='/(?:dev|usr/(?:doc|man|s(?:hare/(?:doc|man)|rc)))';
my $_MasterIncludeDirs_re='/(?:etc|root)';


############
#
# Functions
#
############


sub usage {
    print "usage: $_MyName -", join(" -", @_), "\n";
    exit 1;
}


############
#
# Main
#
############


sub main {
    my $argc = scalar(@ARGV);
    my $help = 0;

    my %optmap=('help' => \$help,
                'h' => \$help);
    my @valid_opts=('get_rpm_pkglist',
                    'get_modified_pkgfiles',
                    'scan_modified_pkgfiles',
                    'get_changed_since_install',
                    'read_rpmset',
                    'write_rpmset');
    @valid_opts = sort(@valid_opts, keys(%optmap));

    unless ( ($argc > 0) &&
             (GetOptions(\%optmap, @valid_opts)) ) {
        print "Invalid commandline.\n";
        usage @valid_opts;
    }
    if ($help) {
        usage @valid_opts;
    }

    # Set the package-global vars
    $rpmUtils::_UnitTest = 1;
    $rpmUtils::_Verbose = 1;

    if (exists $optmap{'get_rpm_pkglist'}) {
        my %pkgs=();
        %pkgs = get_rpm_pkglist;
        print_hash("Packages", %pkgs);
        exit 0;
    }

    if (exists $optmap{'get_modified_pkgfiles'}) {
        my @diffrpm = ();
        my %pkgs=();
        my %dirs=( 'none' => 0 );
        %pkgs = get_rpm_pkglist;
        @diffrpm = get_modified_pkgfiles(%pkgs, %dirs, 
                                         $_SkipModifiedPkgfiles_re);
        print "( @diffrpm )\n";
        exit 0;
    }

    if (exists $optmap{'scan_modified_pkgfiles'}) {
        my %diffrpm = ();
        my %pkgs=();
        %pkgs = get_rpm_pkglist;
        %diffrpm = scan_modified_pkgfiles(%pkgs, $_SkipModifiedPkgfiles_re);
        print_hash("ModifiedPackages", %diffrpm);
        exit 0;
    }

    if (exists $optmap{'get_changed_since_install'}) {
        my %diffrpm = ();
        my %pkgs=();
        my %dirs=( 'none' => 0 );
        my %files = %dirs;
        %pkgs = get_rpm_pkglist;
        %diffrpm = get_changed_since_install(%pkgs, %files, %dirs,
                                             $_MasterIncludeDirs_re,
                                             $_SkipModifiedPkgfiles_re);
        print_hash("ModifiedPackages", %diffrpm);
        exit 0;
    }

    if (exists $optmap{'write_rpmset'}) {
        my %pkgs=();
        %pkgs = get_rpm_pkglist;
        write_rpmset "./test.rpms", %pkgs, $_MyName;
        exit 0;
    }

    if (exists $optmap{'read_rpmset'}) {
        my %pkgs=();
        read_rpmset "./test.rpms", %pkgs;
        print_hash("Packages", %pkgs);
        exit 0;
    }
}


main;
exit 0;

#################
#
#  End
