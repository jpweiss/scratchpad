#!/usr/bin/perl 
#
# Copyright (C) 2003-2004 by John P. Weiss
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
# Std. Package Boilerplate
#
############


package debUtils;
require 5;
use strict;

BEGIN {
    use Exporter ();
    our ($VERSION, @ISA, @EXPORT, @EXPORT_OK, %EXPORT_TAGS);

    # if using RCS/CVS, this may be preferred
    $VERSION = do { my @r = (q$Revision$ =~ /\d+/g); sprintf "%d."."%02d" x $#r, @r }; # must be all one line, for MakeMaker

    @ISA         = qw(Exporter);

    # Default exports.
    @EXPORT = qw(get_pkglist
                 isChangeTypeAlias setChangeTypeAliases
                 isSupportedChangeType
                 get_changed_since_install
                 read_pkgset write_pkgset);
    # Permissable exports.
    # your exported package globals go here,
    # as well as any optionally exported functions
    @EXPORT_OK = qw($_Verbose $_UnitTest);

    # Tagged groups of exports.
    %EXPORT_TAGS = qw();
}
our @EXPORT_OK;

# Other Imported Packages/requirements.
use jpwTools;


############
#
# Global Variables
#
############


our $_Verbose;  $_Verbose = 0;
our $_UnitTest; $_UnitTest = 0;


############
#
# Internal Variables
#
############


my $_deb_bin="/bin/dpkg";
my %_UnsupportedChangeTypeSet = ("Unknown" => 1,
                                 #"Other" => 1,
                                 );


############
#
# Internal Functions
#
############


sub process_pkgspec(\%$;$) {
    my $ref_pkgset = shift;
    my $spec_txt = shift;

    # FIXME:
    # This should contain whatever it takes to split out the name, version,
    # release number, and installation timestamp of a *.deb from the lines
    # retrieved from get_pkglist().
    # 
    my @pkgspecs = split("[\t\n]", $spec_txt);
    if (scalar(@_)) {
        my $updatedSince = shift;
        return if (($updatedSince > 0) && ($updatedSince > $pkgspecs[4]));
    }

    # We use a hash of hashes of array-refs to handle multiple versions of the
    # same package installed at the same time.
    my $pkgName = $pkgspecs[0];
    my $pkgVer = $pkgspecs[1];
    $ref_pkgset->{$pkgName}->{$pkgVer} = [ @pkgspecs ];
}


sub set_if_older(\%$$$) {
    my $ref_fileset = shift;
    my $filename = shift;
    my $t_state = shift;
    my $pkgInstallTime = shift;

    if ($_UnitTest && $_Verbose) {
        print "'$filename'; $t_state; $pkgInstallTime\n";
    } 

    # If this was already set with a time, we'll override it.
    my $has_priorTime = defined($ref_fileset->{$filename});

    if ($t_state > $pkgInstallTime) {
        unless ( $has_priorTime && 
                 ($pkgInstallTime < $ref_fileset->{$filename}) )
        { # I.e. unless(curInstallTime < previousInstallTime)
            $ref_fileset->{$filename} = $pkgInstallTime;
        }
        return 1; #Something's been set.
    } # else

    # When ($t_state <= $pkgInstallTime), we want to leave
    # $ref_fileset->{$filename} undefined.
    #
    # If an earlier call to this function set $ref_filesetVal, compare our
    # present install time to the one stored earlier.
    if ( $has_priorTime && 
         ($pkgInstallTime > $ref_fileset->{$filename}) ) 
    {
        # This install time is later than both the previous one and the file's
        # $t_state.  Nuke the old setting.
        delete($ref_fileset->{$filename});
        # Note:  May want to return 0 here.  Yes, we've modified the fileset,
        # but we've also unset the entry.
        #return 1;
    }

    return 0; # Nothing set.
}


############
#
# Exported Functions
#
############


sub get_pkglist(;$) {
    my $updatedSince = 0;
    if (scalar(@_)) { 
        $updatedSince = shift; 
        --$updatedSince; # Make the cutoff 1 second before this date
    }

    my %pkgset = ();
    # FIXME:
    # This should contain whatever it takes to retrieve the name, version,
    # release number, and installation timestamp of a *.deb
    # 
    my $fmt='%{NAME}\t%{VERSION}\t%{RELEASE}\t%{SERIAL}\t%{INSTALLTIME}\n';
    if($_UnitTest || $_Verbose) {
        print "Retrieving DEB package list...";
    }

    open(DEB_IN, "$_deb_bin -l \"$fmt\" |");
    while (<DEB_IN>) {
        process_pkgspec(%pkgset, $_, $updatedSince);
    }
    close DEB_IN;
    check_syscmd_status "dpkg -l";

    if($_UnitTest || $_Verbose) {
        print "\t\tDone.\n";
    }
    return %pkgset;
}


# FIXME:
# If *.deb packages keep track of ownership & permsissions of their member
# files, you will not need this function.
sub isChangeTypeAlias($) {
    my $key = shift;
    return ($key eq "Ownership");
}


# FIXME:
# If *.deb packages keep track of ownership & permsissions of their member
# files, you will not need this function.
sub setChangeTypeAliases(\%) {
    my $ref_changeMap = shift;
    $ref_changeMap->{"Ownership"} = $ref_changeMap->{"Permissions"};
}


sub get_changed_since_install(\%\%\%$$$) {
    my $ref_pkgset = shift;
    my $ref_fileset = shift;
    my $ref_dirset = shift;
    my $alwaysIncludeDirs_re = shift;
    my $skipModifiedPkgfiles_re = shift;
    my $installTime_delta = shift;

    my @pkgset = sort(keys(%$ref_pkgset));
    my %modified_files = ();
    # FIXME:
    # This should contain whatever it takes to list the file installed by
    # dpkg.
    # 
    my $deb_cmd = "$_deb_bin -L";

    my ($n_dirs, $n_files);
    if ($_Verbose) { $n_dirs=keys(%$ref_dirset);
                     $n_files=keys(%$ref_fileset); }

    my $force_included_later_re = qr<^$alwaysIncludeDirs_re>o;
    # If there's no forced-includes, create a bogus regexp that will
    # never match.
    if ($alwaysIncludeDirs_re eq "") {
        $force_included_later_re = qr<^$>o;
    }

    my $skip_re;
    # If there's nothing to skip, create a bogus regexp that will
    # never match.
    if ($skipModifiedPkgfiles_re eq "") {
        $skip_re = qr<^$>o;
    } else {
        # Here, we only skip certain files that we know never truly change,
        # even though DEB thinks they have.
        $skip_re = qr<^$skipModifiedPkgfiles_re>o;
    }

    # Process one package at a time instead of 'deb -a'.
    if ($_UnitTest || $_Verbose) {
        print "Comparing package contents to actual disk files.\n";
    }

    # Flatten the package hash into one array of arrays.
    my @all_pkgspecs = ();
    foreach my $pkg (@pkgset) {
        my @vers = keys(%{$ref_pkgset->{$pkg}});
        push(@all_pkgspecs, @{$ref_pkgset->{$pkg}}{@vers});
    }

    # Use tmp hashes to ensure list uniqueness.
    my %mf_Permissions = ();
    my %mf_Contents = ();
    my %mf_Symlink = ();
    my %mf_Deleted = ();
    my %mf_Other = ();

    # *Now* iterate over all package names...
    foreach my $ref_pkgspecs (@all_pkgspecs) {
        my $pkg = @$ref_pkgspecs[0] . "-" . @$ref_pkgspecs[1];
        # Since we're using the package install time as a threshold, we can
        # get away with just adding the delta to it.
        my $pkgInstallTime = @$ref_pkgspecs[4] + $installTime_delta;
        if ($_UnitTest || $_Verbose) {
            print "\tchecking package: $pkg\n";
        }
        open(PKGL_IN, "$deb_cmd $pkg |");
        while (<PKGL_IN>) {
            my $fn = $_;
            chomp $fn; # Remove newline
            study $fn;

            # Trim whitespace from either side, using Perl idiom
            for ($fn) { s/^\s+//; s/\s+$//; }

            # Skip anything that isn't a filename with an absolute path.
            next if ( ($fn eq "") || 
                      ($fn !~ m<^/>) );

            # Flags used in processing:
            my $isSkippable = ($fn =~ m<$skip_re>);
            my $isIncludedLater = ($fn =~ m<$force_included_later_re>);

            # Skip any files that we are forcibly including into the
            # archive later on.
            unless ($isIncludedLater) {
                # Otherwise, prune it from the appropriate set.
                if (defined( $ref_dirset->{$fn})) {
                    delete $ref_dirset->{$fn};
                }
                elsif (defined($ref_fileset->{$fn})) {
                    delete $ref_fileset->{$fn};
                }
            }

            # At this point, if it's skippable or we're going to include it
            # later, do the next iter.
            next if ($isSkippable || $isIncludedLater);

            # Missing files: check for those first.
            if (! -e $fn) {
                if ($_UnitTest && $_Verbose) {
                    print "\t\tFile no longer exists: '$fn'\n";
                } 
                $mf_Deleted{$fn} = 1;
                next;
            }

            # Status flags:
            my $isFile = (-f $fn);
            # Dir flag masks out the "isFile" one.
            my $isDir = (-d $fn) && !$isFile;
            my $isSymLink = (-l $fn);
            # Symlink flag masks out the other two:
            $isDir = $isDir && !$isSymLink;
            $isFile = $isFile && !$isSymLink;

            # Examine non-skippable package members for age.  N.B.: The syntax
            # "stat(_)" gets the stats of the file last accessed with one of
            # the file operators (like "-e").
            my @fstats = stat(_);
            if (!scalar(@fstats)) {
                warn "\t\tCannot stat file: $fn.\n";
                next;
            }

            # Handle dirs first:
            if ($isDir) {
                # Only modifications we track for directories are permission
                # changes. 
                if ($_UnitTest && $_Verbose) { 
                    print "\t\tDir ctime: ";
                } 
                set_if_older(%mf_Permissions, $fn, 
                             $fstats[10], $pkgInstallTime);
                next;
            }

            # Contents modification trumps all others.
            if ($isFile) {
                if ($_UnitTest && $_Verbose) { 
                    print "\t\tFile ctime: ";
                } 
                unless (set_if_older(%mf_Contents, $fn, 
                                     $fstats[9], $pkgInstallTime)) {
                    if ($_UnitTest && $_Verbose) { 
                        print "\t\tFile mtime: ";
                    }
                    set_if_older(%mf_Permissions, $fn,
                                 $fstats[10], $pkgInstallTime);
                }
                next;
            }

            # Is this a symlink older than the install date?
            if ($isSymLink) {
                if ($_UnitTest && $_Verbose) { 
                    print "\t\tSymLink mtime: ";
                } 
                unless (set_if_older(%mf_Symlink, $fn, 
                                     $fstats[9], $pkgInstallTime)) {
                    if ($_UnitTest && $_Verbose) { 
                        print "\t\tSymLink ctime: ";
                    } 
                    set_if_older(%mf_Symlink, $fn,
                                 $fstats[10], $pkgInstallTime);
                }
                next;
            }

            # Are we some other type of file that's changed since the install
            # date? 
            if ($_UnitTest && $_Verbose) { 
                print "\t\tOther mtime: ";
            } 
            unless (set_if_older(%mf_Other, $fn, 
                                 $fstats[9], $pkgInstallTime)) {
                if ($_UnitTest && $_Verbose) { 
                    print "\t\tOther ctime: ";
                } 
                set_if_older(%mf_Other, $fn,
                             $fstats[10], $pkgInstallTime);
            }
        } #end PKGL_IN
        close PKGL_IN;
        my $exitStat = check_syscmd_status([1], "$deb_cmd $pkg");
        if ($exitStat) {
            print ("Skipping \"$pkg\".  Is it still installed?\n",
                   "Consider updating the package list and rerunning.\n");
        }
    } #end foreach $pkg

    if ($_UnitTest) {
        my $tmp=keys(%$ref_dirset); 
        print "Not part of a package: $tmp directories "; 
        $tmp=keys(%$ref_fileset); 
        print "and $tmp files.\n";
    }
    if ($_Verbose && $n_dirs && $n_files) { 
        print "Pruned ";
        if ($n_dirs) {
            my $dels = $n_dirs - scalar(keys(%$ref_dirset)); 
            print ("$dels directories (out of $n_dirs total, or ",
                   (100*($dels/$n_dirs)),"\%)");
            if ($n_files) {
                print "\nand ";
            }
        }
        if ($n_files) {
            my $dels = $n_files - scalar(keys(%$ref_fileset)); 
            print ("$dels files (out of $n_files total, or ",
                   (100*($dels/$n_files)), "\%)");
        }
        print ".\n";
    }

    # Sort before bundling up for return.
    $modified_files{"Permissions"} = [ sort(keys(%mf_Permissions)) ];
    $modified_files{"Contents"} = [ sort(keys(%mf_Contents)) ];
    $modified_files{"Unknown"} = [ ]; # Unused.
    $modified_files{"Symlink"} = [ sort(keys(%mf_Symlink)) ];
    $modified_files{"Deleted"} = [ sort(keys(%mf_Deleted)) ];
    $modified_files{"Other"} = [ sort(keys(%mf_Other)) ];
    setChangeTypeAliases(%modified_files);
    return %modified_files;
}


sub isSupportedChangeType($) {
    my $key = shift;
    return !exists($_UnsupportedChangeTypeSet{$key});
}


sub read_pkgset($\%) {
    my $filename = shift;
    my $ref_pkgset = shift;
    my %tmp_pkgset=();

    if ($_UnitTest || $_Verbose) {
        print "Reading package list...\n";
    }
    my %tmp_pkgset = read_options($filename);
    my $write_date = $tmp_pkgset{'date_written'};

    # This is the preferred list.
    if ( defined($tmp_pkgset{"pkgset"}) &&
         (ref($tmp_pkgset{"pkgset"}) eq "ARRAY") ) {
        foreach (@{$tmp_pkgset{"pkgset"}}) {
            process_pkgspec(%{$ref_pkgset}, $_);
        }
        return $write_date;
    }

    # This is the fallback list.
    unless ( defined($tmp_pkgset{"pkgname_set"}) &&
             (ref($tmp_pkgset{"pkgname_set"}) eq "ARRAY") ) {
        %$ref_pkgset = map({ $_, 1 } @{$tmp_pkgset{"pkgname_set"}});
    }
    return $write_date;
}


sub write_pkgset($\%;$) {
    my $filename = shift;
    my $ref_pkgset = shift;
    my $myName;

    if (scalar(@_)) {
        $myName = shift;
    } else {
        $myName = "rpmUtils::write_pkgset()";
    }

    chmod(0660, $filename);
    open(OFS, ">$filename")
        or die("Unable to open file for writing: \"$filename\"\n".
               "Reason: \"$!\"\n");
    # File header.
    print OFS ('#'x79, "\n#\n");
    print OFS ("# List of Installed DEB Packages.\n#\n");
    print OFS ("# Created by $myName.  DO NOT MODIFY.\n");
    print OFS ("#\n", '#'x79, "\n\n");

    # Print the date of the write, in seconds since epoch.
    print OFS ("date_written=", time(), "\n\n");

    # Print the DEB list.
    print OFS ("pkgname_set=(\n");
    foreach (sort(keys(%$ref_pkgset))) { print OFS ($_, "\n"); }
    print OFS (")\n\n", '#'x79, "\n#\n");

    print OFS ("# The previous list contained package names only.\n#\n");
    print OFS ("# It's provided as a convenience so that you know which ");
    print OFS ("packages\n# you need to reinstall.\n#\n");
    print OFS ("# What follows is the package list used by\n");
    print OFS ("# $myName.  Its format is as follows:\n#\n#\n");
    print OFS ("# NAME\\tVERSION\\tRELEASE\\tSERIAL\\tINSTALLTIME\n#");
    print OFS ("\n", '#'x79, "\n\n");

    # Print the entire hash.  Don't forget that a hash can only contain
    # scalars or references.  Hence the list dereference here.
    print OFS ("pkgset=(\n");
    foreach my $k1 (sort(keys(%$ref_pkgset))) { 
        foreach my $k2 (reverse(sort(keys(%{$ref_pkgset->{$k1}})))) {
            print OFS (join("\t", @{$ref_pkgset->{$k1}{$k2}}), "\n");
        }
    }
    print OFS (")\n");

    # File footer.
    print OFS ("\n", '#'x10, "\n# End\n#\n");
    close OFS;
    chmod(0440, $filename);
}


1;  # don't forget to return a true value from the file
## POD STARTS HERE ##
__END__

=head1 NAME

debUtils - Package for interacting with installed DEB packages.

=head1 SYNOPSIS

=over 0

=item $debUtils::_Verbose

=item $debUtils::_UnitTest

=item I<%packages> = get_pkglist()

=item I<$flag> = isChangeTypeAlias(I<$changeCategory>)

=item setChangeTypeAliases(I<%changeMap>)

=item %h = get_changed_since_install(I<%packages, %files, %directories, includedLaterRegexp, skipPkgRegexp, $installTime_delta>)

=item $date = read_pkgset(I<filename, %packages>)

=item write_pkgset(I<filename, %packages>)

=back

=head1 DESCRIPTION

=over 2


=item *

$debUtils::_Verbose

"Boolean" integer variable, set to 0 by default.  Set to 1 to enable verbose
messages from the functions in this package.

=item *

$debUtils::_UnitTest

"Boolean" integer variable, set to 0 by default.  Set to 1 when performing
unit tests on the functions in this package.

=item *

I<%packages> = get_pkglist()

Returns a hash containing information about all of the installed packages on
this system.  I<%packages> is keyed by the package name.  Each value is an
array ref containing the following information:

=over 2

=item ->[0]: The package name (again).

=item ->[1]: The installed package's version number.

=item ->[2]: The installed package's release number.

=item ->[3]: The installed package's serial number.  This will usually be 
the string, "(null)".

=item ->[4]: The installation date/time, in seconds since the Epoch.

=back

Note that the first four elements are strings, while the last one is an
integer.

The installation date/time should describe when the installation B<finished>.
Your implementation of this package should perform any necessary estimations
and adjustments to ensure this.

=item *

I<$flag> = isChangeTypeAlias(I<$changeCategory>)

Returns 1 if I<$changeCategory> is an "alias key", 0 otherwise.

C<get_changed_since_install()> returns a hash of arrayrefs.  Some of the keys
in that hash point to the same arrayref in memory.

=item *

setChangeTypeAliases(I<%changeMap>)

Creates the "alias keys" described in C<isChangeTypeAlias>, modifying
I<%changeMap>.  C<get_changed_since_install()> calls this function.

=item *

%h = get_changed_since_install(I<%packages, %files, %directories, includedLaterRegexp, skipPkgRegexp, $installTime_delta>)

This function uses a package's installation timestamp to find its modified
member files.  As it loops through each package, it ignores any filename
matching the regexp, I<skipPkgRegexp>.  It also ignores filenames matching
I<includedLaterRegexp>, assuming that you will add them or their parent
directory later.

This function specifically uses C<stat()> to directly determine how a package
member file has changed.

The returned hash contains references to arrays of filenames.  The key to each
file list identifies how it has been modified:

=over 4

=item "Contents"

The file's C<mtime> is later than the parent package's installation time.  The
file is a regular file (not a symlink, device, socket, etc.).

=item "Deleted"

The file no longer exists on disk.

=item "Symlink"

The file is a symlink with an C<mtime> I<or> C<ctime> later than the parent
package's installation time.

=item "Permissions"

=item "Ownership"

The file I<or> directory has a C<ctime> later than the parent
package's installation time.

These keys point to array refs with identical contents (they may even point to
the same array).

This is also the only array ref containing both files and directories.  All
others contain only files.

=item "Unknown"

Unused

=item "Other"

The file is neither a directory, symlink or regular file, and has an C<mtime>
I<or> C<ctime> later than the parent package's installation time.

=back

The documentation for both versions of C<stat> (C<perdoc -f stat>; C<man 2
stat>) describe what C<ctime> means.

The parameter I<$installTime_delta> is an upper limit on how long it takes a
package to install all of its files.  For RPM packages, the package's
installation time indicates when installation I<began>, so the member files'
C<mtime> may be later than the install time by several seconds.  DEB packages
may not suffer from this problem.  If that's the case, you can safely pass
C<0> as I<$installTime_delta>.

=item *

$date = read_pkgset(I<filename, %packages>)

Inverse operations to C<write_pkgset> (below).  Reads a package list from the
specified I<filename>, storing the results in I<%packages>.  The I<%packages>
hash has the same structure as the one returned by C<get_pkglist>.
Returns the date (in seconds since epoch) when the file was written.

=item *

write_pkgset(I<filename, %packages>)

Writes the package information in I<%packages> to the specified I<filename>.
The I<%packages> hash has the same structure as the one returned by
C<get_pkglist>.

=back 

=cut


################# 
#
#  End
