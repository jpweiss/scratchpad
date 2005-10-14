#!/usr/bin/perl 
#
# Copyright (C) 2002 by John P. Weiss
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


package jpwTools;
require 5;
use strict;

BEGIN {
    use Exporter ();
    our ($VERSION, @ISA, @EXPORT, @EXPORT_OK, %EXPORT_TAGS);

    # if using RCS/CVS, this may be preferred
    $VERSION = do { my @r = (q$Revision: 1401 $ =~ /\d+/g); sprintf "%d."."%02d" x $#r, @r }; # must be all one line, for MakeMaker

    @ISA         = qw(Exporter);

    # Default exports.
    @EXPORT = qw(check_syscmd_status do_error
                 datestamp datetime_now  
                 const_array uniq
                 invert_hash rename_keys transform_keys lc_keys uc_keys
                 asymm_diff circular_shift circular_pop
                 stats stats_gaussian 
                 set_seed random_indices randomize_array random_keys
                 print_hash print_array print_dump 
                 fprint_hash fprint_array
                 cmpVersionNumberLists
                 create_regexp_group non_overlapping 
                 not_empty set_array_if_nonempty set_scalar_if_nonempty 
                 read_options);
    # Permissable exports.
    # your exported package globals go here,
    # as well as any optionally exported functions
    @EXPORT_OK = qw($_Verbose $_UnitTest);

    # Tagged groups of exports.
    %EXPORT_TAGS = qw();
}
our @EXPORT_OK;

# Other Imported Packages/requirements.
use Data::Dumper;


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


my @_RecursiveRegexpGrouper_Stack = ();
$Data::Dumper::Sortkeys = 1;
$Data::Dumper::Indent = 1;
# None


############
#
# Exported Functions
#
############


sub check_syscmd_status {
    my $laststat=$?;
    my $lastErrmsg="$!";
    my $exitVal = ($laststat >> 8);
    my $signal = ($laststat & 0x7F);

    if (ref($_[0]) eq "ARRAY") {
        my %ignore = ();
        @ignore{ @{shift()} } = ();
        if (exists($ignore{$exitVal})) {
            print "WARNING: Command \"@_\" exited with status $exitVal.";
            unless ($lastErrmsg eq '') {
                print "  Reason: \"$lastErrmsg\".";
            }
            print "\n";
            return $exitVal;
        }
    }

    if ($laststat == 0) {
        # Everything's A-okay!
        return 0;
    }
    print "Command \"@_\" failed;\n";
    unless ($lastErrmsg eq '') {
        print "Reason: \"$lastErrmsg\".\n";
    }
    unless ($signal == 0) {
        print "Exited on signal $signal.  ";
    }
    unless ($exitVal == 0) {
        print "Exit status: $exitVal.";
    }
    print "\nAborting...\n";
    exit $laststat;
}


sub do_error($$$) {
    my $doFile = shift;
    my $doError = shift;
    my $parseError = shift;

    if (($parseError eq "") && ($doError eq "")) {
        return "Error:  Runtime error evaluating while executing \"".
            $doFile."\".\n";
    } #else
    my $mesg = "";
    if ($doError ne "") {
        $mesg = "Error: 'do \"$doFile\"': $doError\n";
    }
    if ($parseError ne "") {
        $mesg .= "Error while parsing file \"$doFile\": $parseError\n";
    }
    return $mesg;
}


sub datetime_now() {
    my @timeinfo = localtime;
    # month == $timeinfo[4] + 1
    ++$timeinfo[4];
    # year == $timeinfo[5] + 1900
    $timeinfo[5] += 1900;

    # Return in the order: YYYY, MM, DD, HH24, MI, SEC
    return ($timeinfo[5], $timeinfo[4], $timeinfo[3], 
            $timeinfo[2], $timeinfo[1], $timeinfo[0]);
}


sub datestamp() {
    return sprintf("%4d%02d%02d", datetime_now());
}


sub const_array($$) {
    my ($cval, $len) = @_;
    return ( $cval ) x $len;
}


# Using a hash as a set:
# 
#     my %set;
#     @set{@elements} = ();
#     
# ... Then use "exists" to test for membership (instead of using
# "defined").
#
# If, for some reason, you want or need to use "defined()" [i.e. check that
# the hash contains a key *and* that the key maps to something other than
# undef], use this idiom:
#
#    @set{@elements} = (1 .. scalar(@element));
#
# ...or:
#
#    @set{@elements} = grep { 1; } (1 .. scalar(@element));
#


sub uniq {
    my %seen;
    @seen{@_} = ();
    return keys %seen;
}


# How to do a set union, intersection, and difference.
# 
##
#sub set_ops {
#    @union = @intersection = @difference = ();
#    %count = ();
#    foreach $element (@array1, @array2) { $count{$element}++ }
#    foreach $element (keys %count) {
#        push @union, $element;
#        push @{ $count{$element} > 1 ? \@intersection : \@difference }, 
#             $element;
#    }
#}


sub invert_hash(\%;\%) {
    my $nArgs = scalar(@_);
    my $ref_h = shift;
    my $ref_inv;
    if ($nArgs > 1) {
        $ref_inv = shift;
    } else {
        $ref_inv = { };
    }
    while(my ($k, $v) = each(%$ref_h)) {
        next if(ref($v)); # Must be scalar
        $ref_inv->{$v} = $k;
    }
    unless ($nArgs > 1) {
        return %$ref_inv;
    }
}


sub transform_keys(\%\&) {
    my $ref_hash = shift;
    my $subref_transform = shift;

    foreach my $oldKey (keys(%$ref_hash)) {
        my $newKey = &$subref_transform($oldKey);
        next if ($newKey eq $oldKey); # Skip if identical
        $ref_hash->{$newKey} = $ref_hash->{$oldKey};
        delete $ref_hash->{$oldKey};
    }
}


sub lc_keys(\%) {
    my $ref_hash = shift;

    # Note: "lc" (and "uc") are not true "functions" in the "main::"
    # namespace.  They are actually treated as language tokens at compile
    # time.  So, we can't take a reference to them.
    # We must, instead, resort to this trick.
    sub lc_kludge { return lc $_[0]; };
    transform_keys(%$ref_hash, &lc_kludge);
}


sub uc_keys(\%) {
    my $ref_hash = shift;

    # Note: See comment in "lc_keys()" for why we must resort to this trick.
    sub uc_kludge{ return uc $_[0]; };
    transform_keys(%$ref_hash, &uc_kludge);
}


sub rename_keys(\%\%) {
    my $ref_hash = shift;
    my $ref_old2new_keys = shift;

    foreach my $oldKey (grep({defined($ref_hash->{$_})} 
                             keys(%$ref_old2new_keys)))
    {
        $ref_hash->{$ref_old2new_keys->{$oldKey}} = $ref_hash->{$oldKey};
        delete $ref_hash->{$oldKey};
    }
}


sub circular_shift(\@;$) {
    my $ref_array = shift;
    if (scalar(@_)) {
        my $count = shift;
        my @vals = splice @{$ref_array}, 0, $count;
        push @{$ref_array}, @vals;
        return @vals;
    }
    # else
    my $val = shift @{$ref_array};
    push @{$ref_array}, $val;
    return $val;
}


sub circular_pop(\@;$) {
    my $ref_array = shift;
    if (scalar(@_)) {
        my $count = shift;
        my @vals = splice @{$ref_array}, (scalar($#{$ref_array})-$count);
        unshift @{$ref_array}, @vals;
        return @vals;
    }
    # else
    my $val = pop @{$ref_array};
    unshift @{$ref_array}, $val;
    return $val;
}


sub asymm_diff {
    my ($ref_set1, 
        $ref_set2) = @_;
    my $set1_type=ref($ref_set1);
    my $set2_type=ref($ref_set2);
    unless ( (($set1_type eq "HASH") || 
              ($set1_type eq "ARRAY")) && 
             (($set2_type eq "HASH") || 
              ($set2_type eq "ARRAY")) ) {
        die "While calling jpwTools::asymm_diff()\n".
            "Syntax Error: First two args must be ref to hash or array.";
    }
    my $ref_delslice = ();
    my %tmphash = ();

    # "Convert" the second set to an array, if necessary.
    if ($set2_type eq "HASH") {
        $ref_delslice = [ keys %$ref_set2 ];
    } else {
        $ref_delslice = $ref_set2;
    }

    # 1. Copy the first set to a temporary hash.
    # 2. Delete the slice matching the second set.
    # 3. Return the results in the proper format.
    # Don't forget that this delete should be an array context.  Otherwise,
    # only the first element in the slice gets deleted.
    if ($set1_type eq "HASH") {
        %tmphash = %$ref_set1;
        delete @tmphash{@{$ref_delslice}};
        return %tmphash;
    } else {
        @tmphash{@{$ref_set1}} = map 1, @$ref_set1;
        delete @tmphash{@{$ref_delslice}};
        return sort(keys(%tmphash));
    }
}


sub not_empty($) {
    my $arg = shift;
    unless (defined($arg)) {
        return 0;
    }
    my $reftype=ref($arg);
    return ( (($reftype eq "ARRAY") && scalar(@$arg))
             || 
             (($reftype eq "HASH") && scalar(%$arg))
             ||
             (($reftype eq "SCALAR") && length($$arg))
             ||
             (($reftype eq "") && length($arg))
             ||
             0);
}


sub set_scalar_if_nonempty(\$\%$) {
    my ($svar_ref,
        $map_ref,
        $keyname) = @_;
    if (not_empty($map_ref->{$keyname})) 
    {
        $$svar_ref = $map_ref->{$keyname};
    }
}


sub set_array_if_nonempty(\@\%$) {
    my ($avar_ref,
        $map_ref,
        $keyname) = @_;
    if (not_empty($map_ref->{$keyname})) 
    {
        @{$avar_ref} = @{$map_ref->{$keyname}};
    }
}


sub stats(\@;$) {
    # According to Åß8.5 of "Numerical Recipies," a selection algorithm (find
    # the K^th largest value in an array) runs with O(N) time.  Thus,
    # computing the median, dispersion about the median, min, and max take
    # O(4N) (the min & max can be searched for together in a separate loop).
    # Since sorting takes O(N log N), the selection algorithm will perform
    # better when N > 10^4.
    #
    # Note, however that selection algorithms rearrange the original array.
    #
    # For the Perl versions of this method, I'll use sort.  For other
    # programming languages, especially C++, using a selection algorithm may
    # make more sense.

    my $ref_vals = shift;
    my $confidence = "";
    if (scalar(@_)) {
        $confidence = shift;
    }
    my @vals = sort {$a <=> $b;} (@{$ref_vals});
    my $N = scalar(@vals);

    # For small N, the median & dispersion are predetermined.  So, just return
    # those 
    if ($N < 4) {
        if ($N == 0) { return (0, 0, 0, 0, 0); }
        if ($N == 1) { return ($vals[0], 
                               $vals[0], $vals[0], $vals[0], 
                               $vals[0]); }
        if ($N == 2) { return ($vals[0], 
                               $vals[0], 0.5*($vals[0]+$vals[0]), $vals[1], 
                               $vals[1]); }
    }

    my $median = $vals[$N/2];
    my $k_disp_lo;
    my $k_disp_hi;

    # We will do as "Numerical Recipies" does (see comment at the start of
    # Åß8.5), and be pedantic for N <= 100.
    if ( ($N <= 100) && (($N % 2) == 0) ) {
        $median += $vals[int($N/2) + 1];
        $median /= 2.0;
    }

    if ($confidence =~ m/^[-+]?0\.\d+/) {
        my $half_interval = 0.5 * abs($confidence);
        $k_disp_lo = int( (0.5 - $half_interval) * $N );
        $k_disp_hi = int( (0.5 + $half_interval) * $N );
    }
    elsif ($confidence =~ m/^q/) {
        # Interquartile ranges.
        $k_disp_lo = int( 0.25 * $N );
        $k_disp_hi = int( 0.75 * $N );
    }
    elsif ($confidence =~ m/^3s/) {
        # 3sigma: 0.9973
        $k_disp_lo = int( 0.00135 * $N );
        $k_disp_hi = int( 0.99865 * $N );
    }
    elsif ($confidence =~ m/^2s/) {
        # 2sigma: 0.9545
        $k_disp_lo = int( 0.02275 * $N );
        $k_disp_hi = int( 0.97725 * $N );
    }
    else {
        # Default:
        # sigma: 0.6827
        $k_disp_lo = int( 0.15865 * $N );
        $k_disp_hi = int( 0.84135 * $N );
    }

    return ($vals[0], 
            $vals[$k_disp_lo], $median, $vals[$k_disp_hi], 
            $vals[$#vals]);
}


sub stats_gaussian(\@) {
    my $ref_vals = shift;
    my $n = scalar(@{$ref_vals});

    my $avg=0.0;
    foreach my $val (@{$ref_vals}) { $avg += $val; }
    $avg /= $n;

    my $avg_dev=0.0;
    #my $sum_anom=0.0;
    my $var=0.0;
    my $skew=0.0;
    my $kurt=0.0;
    foreach my $val (@{$ref_vals}) {
        my $anom = ($val - $avg);
        #$sum_anom += $anom;
        $avg_dev += abs($anom);

        my $anom_sq = $anom*$anom;
        $var += $anom_sq;
        $skew += $anom_sq*$anom;
        $kurt += $anom_sq*$anom_sq;
    }
    # Compute Avg. Deviation 
    $avg_dev /= $n;

    # Compute the variance using the "Corrected Two-Pass Algorithm," described
    # in "Numerical Recipies," Åß14.1.
    #$var -= (($sum_anom*$sum_anom)/$n);
    # Note: this doesn't work; you can generate small negative numbers with
    # this.  It looks, also, like the algorithm in NR wasn't entirely correct,
    # either.
    $var /= ($n - 1.0);
    my $stddev = sqrt($var);

    # Divide the skew by sigma^3
    $skew /= ($n * $var * $stddev);

    # Divide the kurtosis by sigma^4, then adjust so that a gaussian has 0
    # kurtosis. 
    $kurt /= ($n * $var * $var);
    $kurt -= 3.0;

    return ($avg, $var, $skew, $kurt, $stddev, $avg_dev);
}


sub set_seed(;$) {
    my $seed;
    if (scalar(@_)) {
        $seed = int(shift());
    } else {
        # Perl's "srand" function does not return the seed, making Monte Carlo
        # difficult.  So, to remedy that, we call srand explicitly without any
        # seed, then generate the first random integer as the new seed.
        #
        # So, it's not great, but it'll do for Monte Carlo simulation.
        srand();
        $seed = int(rand(0xFFFFFFFF));
    }
    srand($seed);
    return $seed;
}


sub randomize_array(\@) {
    my $ref_targArray = shift;
    my $n = scalar(@$ref_targArray);
    for (my $i=0; $i < $n; ++$i) {
        my $randIdx = int(rand($n));
        my $tmp = $ref_targArray->[$i];
        $ref_targArray->[$i] = $ref_targArray->[$randIdx];
        $ref_targArray->[$randIdx] = $tmp;
    }
}


sub random_indices($;\@) {
    my $n = shift;
    my $ref_idxs;
    my $returnArray = 1;
    if (scalar(@_)) {
        $ref_idxs = shift;
        $returnArray = 0;
    } else {
        $ref_idxs = [];
    }
    push @$ref_idxs, (0 .. ($n - 1));
    randomize_array(@$ref_idxs);
    if ($returnArray) {
        return @$ref_idxs;
    }
}


sub random_keys(\%) {
    my $ref_hash = shift;
    my @keyList = sort(keys(%$ref_hash));
    randomize_array(@keyList);
    return @keyList;
}


sub print_dump($;@) {
    my $ref_var = shift;
    my $fh = undef();
    if (ref($ref_var) eq "GLOB") {
        unless(scalar(@_)) {
            die("print_dump():  Must have 2 or more args when the first arg ".
                "is a GLOB.\n");
        }
        $fh = $ref_var;
        $ref_var = shift;
    }
    my $name = "";
    if (scalar(@_) && !ref($ref_var)) {
        # 1st Arg is scalar => it's the name of the variable.  Assume the
        # var-ref is in the 2nd Arg.
        $name = $ref_var;
        $ref_var = shift;
    }

    my $output;
    if ($name eq "") {
        $output = sprintf("%s", Dumper($ref_var));
    } else {
        $output = sprintf("%s", Data::Dumper->Dump([$ref_var], ["\*$name"]));
    }
    while (scalar(@_) > 1) {
        my $oldFmt = shift;
        my $newFmt = shift;
        eval "\$output =~ s/$oldFmt/$newFmt/mg";
    }
    if (defined($fh)) {
        print $fh ($output);
    } else {
        print $output;
    }
}


sub fprint_hash(\*$\%;@) {
    my $fh = shift;
    my $name = shift;
    my $map_ref=shift;
    if ($name eq "") {
        print_dump($fh, $map_ref, @_);
    } else {
        print_dump($fh, $name, $map_ref, @_);
    }
}


sub fprint_array(\*$\@;@) {
    my $fh = shift;
    my $name = shift;
    my $list_ref=shift;
    if ($name eq "") {
        print_dump($fh, $list_ref, @_);
    } else {
        print_dump($fh, $name, $list_ref, @_);
    }
}


sub print_hash($\%;@) {
    my $name = shift;
    my $map_ref=shift;
    if ($name eq "") {
        print_dump($map_ref, @_);
    } else {
        print_dump($name, $map_ref, @_);
    }
}


sub print_array($\@;@) {
    my $name = shift;
    my $list_ref=shift;
    if ($name eq "") {
        print_dump($list_ref, @_);
    } else {
        print_dump($name, $list_ref, @_);
    }
}


sub cmpVersionNumberLists(\@\@) {
    my $ref_xl = shift;
    my $ref_yl = shift;

    while (scalar(@$ref_xl) && scalar(@$ref_yl)) {
        my $x0 = shift @$ref_xl;
        my $y0 = shift @$ref_yl;
        my $cmpstat;
        # If either piece contains non-digit characters, do a string compare.
        # Otherwise, use the numeric compare.
        if ( ($x0 =~ m/\D/) || ($y0 =~ m/\D/) ) {
            $cmpstat = ($x0 cmp $y0);
        } else {
            $cmpstat = ($x0 <=> $y0);
        }
        if ($cmpstat) {
            # These two pieces aren't equal; we're done.
            return $cmpstat;
        }
    }

    # We only reach here if $x and $y don't have the same number of pieces in
    # their version numbers.  In this case, we choose the one with more.
    return (scalar(@$ref_xl) <=> scalar(@$ref_yl));
}


# TODO: This is now redundant and should be deprecated in the future.
sub old_print_hash(\%;$) {
    my $map_ref=shift;
    my $indent="";
    if (scalar(@_)) { $indent = shift; }

    print "(\n";
    foreach my $thekey (sort(keys(%{$map_ref}))) {
        my $reftype = ref $map_ref->{$thekey};
        if ($reftype) {
            if ($reftype eq "ARRAY") {
                print "$indent  \"$thekey\"\n".
                    "$indent    => [@{ $map_ref->{$thekey} }]\n";
            } elsif ($reftype eq "HASH") {
                print "$indent  \"$thekey\"\n$indent    => ";
                # WARNING:  Recursive function calls in Perl can go horribly
                # wrong.  Hence, we use an indirection function to prevent
                # Perl from mangling the stack.
                print_hash_recurse($map_ref->{$thekey}, "$indent\t");
            } elsif ($reftype eq "SCALAR") {
                print "$indent  \"$thekey\" =-> \"${$map_ref->{$thekey}}\"\n";
            } else {
                print "$indent  \"$thekey\" =-> $map_ref->{$thekey}\n";
            }
        } else {
            print "$indent  \"$thekey\" => \"$map_ref->{$thekey}\"\n";
        }
    }
    print "$indent)\n";
}


sub create_regexp_group {
    # Make sure the word list is sorted, unique, and doesn't contain the "".
    my %uniqifier;
    @uniqifier{@_} = ();
    delete $uniqifier{""};
    my @words = sort(keys %uniqifier);
    undef %uniqifier;

    # Using Perl extension: Regexp operator: "(?:)" is like the regular
    # grouping operator, but doesn't save anything into the \N registers.
    @_RecursiveRegexpGrouper_Stack = (0, '(?:', 0, @words);
    return recursive_regexp_grouper();
}


sub non_overlapping {
    my @wrk = sort @_;
    if (scalar(@wrk) == 0) {
        # Nothing to do; no prune expressions.
        return @wrk; 
    }
    # This isn't ideal, but it'll do for now.  Using create_regexp_group is
    # also somewhat overkill.
    my $overlap_re = '(?:';
    $overlap_re .= join "|", @wrk;
    $overlap_re .= ')';

    # Return non-overlapping element, i.e. elements that are not prefixes of
    # any other element, or those that are a minimum-sized prefix.
    return grep !m<^$overlap_re.>o, @wrk;
}


sub read_options($;\%) {
    my $filename = shift;
    my %options=();
    my $array_option="";
    my $validations_mapref = {};
    if (scalar(@_)) {
        $validations_mapref = shift;
    }

    open(IN_FS, "$filename")
        or die("Unable to open file for reading: \"$filename\"\n".
               "Reason: \"$!\"\n");

    while (<IN_FS>) {
        my $line = $_;
        chomp $line; # Remove newline

        # Trim whitespaces from either end of the line
        # (This is faster than the obvious single-regexp way.)
        for ($line) {
            s/^\s+//;
            s/\s+$//;
        }

        # Skip comment or blank lines (using optimizer-friendly Perl idiom).
        next if ($line eq "");
        next if ($line =~ m/^\#/);

        # Special handling:
        # This is the end of an array parameter.  Must come before the
        # array processing block.
        if ($line eq ")") {
            $array_option = "";
            next;
        }

        # Special handling:
        # We are in the middle of processing an array option.
        if ($array_option) {
            push @{$options{$array_option}}, $line;
            next;
        }

        # Get the option name and value, trimming whitespace on either
        # side of the delimiter characters.
        my ($optname, $val) = split /\s*[:=]\s*/, $line;

        # Special handling:
        # This is the start of an array parameter.
        if ($val eq "(") {
            $array_option=$optname;
            $options{$array_option} = [ ];
            next;
        }

        # Regular option processing
        $options{$optname} = $val;
    }
    close IN_FS;

    my $is_valid=1;
    while(my ($k, $v) = each(%$validations_mapref)) {
        if (defined($options{$k})) {
            if (ref($options{$k}) ne $v) {
                if ($is_valid) {
                    # Print out the header on the first error.
                    print "Error in file \"$filename\":\n";
                }
                print "\tParameter \"$k\" must be ";
                if ($v eq "") { 
                    print "scalar.\n"; 
                } else { 
                    print "an ", lc($v), ".\n";
                }
                $is_valid = 0;
            }
        }
    }

    unless($is_valid) { 
        print "Aborting.";
        exit 1; 
    }

    return %options;
}


# Unit-testing stub function.
sub ut {
    exit 0;
}


############
#
# Internal Functions
#
############


sub print_hash_recurse { &print_hash; }


sub make_char_regexp {
    my @charlist = map { ord $_; } sort(@_);
    my $charsets = "";
    my $dash = "";
    my $caret = "";
    my $rbracket = "";

    my $start=-1;
    my $end=-2;
    foreach my $current (@charlist) { 
        if ($current == ord("^")) { $caret = "^"; next; }
        if ($current == ord("-")) { $dash = "-"; next; }
        if ($current == ord("]")) { $rbracket = "]"; next;}

        # $range_end always equals the previous character in the list.
        if (($current-1) == $end) {
            $end = $current;
            next;
        }

        if ($end > (2+$start)) {
            $charsets .= chr($start);
            $charsets .= "-";
            $charsets .= chr($end);
        } else {
            while($end >= $start) {
                $charsets .= chr($start);
                ++$start;
            }
        }
        $start = $end = $current;
    }

    # Output final char or range.
    if($end >= $start) {
        if ($end > (2+$start)) {
            $charsets .= chr($start);
            $charsets .= "-";
            $charsets .= chr($end);
        } else {
            while($end >= $start) {
                $charsets .= chr($start);
                ++$start;
            }
        }
    }

    if (($charsets eq "") && ($rbracket eq "")) {
        return join("", "[", $dash, $caret, "]");
    }
    # else:
    return join("", "[", $rbracket, $charsets, $caret, $dash,  "]");
}


sub find_prefix {
    my @words = sort(@_);
    my $nwords = scalar(@words);
    unless ($nwords) {
        return "";
    }

    my $largest_prefix=$words[0];
    my $maxsz = length($largest_prefix);
    my $sz=1;
    my $tstprefix = "";
    my $prefix = "";
    while($sz <= $maxsz) {
        # Check the next possible match.
        $tstprefix = substr($largest_prefix, 0, $sz);
        ++$sz;
        unless ($nwords == scalar(grep(m<^$tstprefix>, @words))) {
            last;
        }
        # Set $prefix to the last successful match.
        $prefix = $tstprefix;
    }
    return $prefix;
}


sub find_suffix {
    my @words = sort(@_);
    my $nwords = scalar(@words);
    unless ($nwords) {
        return "";
    }

    my $largest_suffix=$words[0];
    my $maxsz = length($largest_suffix);
    my $sz=1;
    my $tstsuffix = "";
    my $suffix = "";
    while($sz <= $maxsz) {
        # Check the next possible match.
        $tstsuffix = substr($largest_suffix, -$sz);
        ++$sz;
        unless ($nwords == scalar(grep(m<$tstsuffix$>, @words))) {
            last;
        }
        # Set $suffix to the last successful match.
        $suffix = $tstsuffix;
    }
    return $suffix;
}


# The @_RecursiveRegexpGrouper_Stack global is a kludge around broken Perl
# recursion behavior.
sub recursive_regexp_grouper() { #($$$@) {
    my $recursionDepth = shift(@_RecursiveRegexpGrouper_Stack);
    my $group_open = shift(@_RecursiveRegexpGrouper_Stack);
    my $lax = shift(@_RecursiveRegexpGrouper_Stack);
    my @words = @_RecursiveRegexpGrouper_Stack;
    my $nwords = scalar(@words);

    if ($_Verbose) {
        print STDERR ("# recursive_regexp_grouper():  Called at depth==",
                      $recursionDepth, "\n");
    }

    unless ($nwords) {
        return "";
    }

    if ($_UnitTest) {
        print STDERR ("# recursive_regexp_grouper():  ", $nwords, 
                      " \"@words\"\n");
    }

    my $group_close = "";
    my $cgrp_open = "";
    my $cgrp_close = "";
    if ($group_open ne "") { $group_close = ")"; }
    unless ($lax) {
        $cgrp_open = $group_open;
        $cgrp_close = $group_close;
    }

    if ($nwords == 1) {
        if (length($words[0]) == 1) {
            return join("", $cgrp_open, $words[0], $cgrp_close);
        } # else:
        return join("", $group_open, $words[0], $group_close);
    }

    my $patstr = "";

    # A rather obtuse way of determining if there are 2 or more one-char
    # strings present.
    my @letters = grep((length($_) == 1), @words);
    if (scalar(@letters) > 1) {
        if ($_Verbose) {
            print STDERR ("# recursive_regexp_grouper():  ",
                          " Called with single-char \"words\".\n");
        }
        my @rest = grep((length($_) != 1), @words);
        unless ($nwords == (scalar(@letters) + scalar(@rest))) {
            die "Lost elements while dividing list on length";
        }
        undef @words;
        if (scalar(@rest)) {
            if ($_Verbose) {
                print STDERR ("# recursive_regexp_grouper():  ",
                              " Grouping remaining multichar-words.\n");
            }
            $patstr .= $group_open;
            @_RecursiveRegexpGrouper_Stack = ($recursionDepth+1,
                                              "", 0, reverse(@rest));
            $patstr .= recursive_regexp_grouper();
            $patstr .=  "|";
            $patstr .= make_char_regexp(@letters);
            $patstr .= $group_close;
        } else {
            $patstr .= $cgrp_open;
            $patstr .= make_char_regexp(@letters);
            $patstr .= $cgrp_close;
        }
        return $patstr;
    }

    ## Default Behavior: list of different-length strings. 

    my $prefix = find_prefix(@words);
    if ($prefix ne "") {
        # Common prefix => recurse on the suffixes.
        if ($_Verbose) {
            print STDERR ("# recursive_regexp_grouper():  ",
                          " Common prefix: \"",$prefix, 
                          "\".  Grouping on suffixes.\n");
        }
        my @suffixes = ();
        foreach my $newsuf (grep m<^$prefix>, @words) {
            $newsuf =~ s<^$prefix><>;
            push @suffixes, $newsuf;
        }
        undef @words;
        $patstr .= $group_open;
        $patstr .= $prefix;
        @_RecursiveRegexpGrouper_Stack = ($recursionDepth+1,
                                          "(?:", 1, @suffixes);
        $patstr .= recursive_regexp_grouper();
        $patstr .= $group_close;
        return $patstr;
    }

    my $suffix = find_suffix(@words);
    if ($suffix ne "") {
		# Common suffix => recurse on the prefixes.
        if ($_Verbose) {
            print STDERR ("# recursive_regexp_grouper():  ",
                          " Common suffix: \"",$suffix, 
                          "\".  Grouping on prefixes.\n");
        }
        my @prefixes = ();
        foreach my $newpre (grep m<$suffix$>, @words) {
            $newpre =~ s<$suffix$><>;
            push @prefixes, $newpre;
        }
        undef @words;
        $patstr .= $group_open;
        @_RecursiveRegexpGrouper_Stack = ($recursionDepth+1,
                                          "(?:", 1, @prefixes);
        $patstr .= recursive_regexp_grouper();
        $patstr .= $suffix;
        $patstr .= $group_close;
        return $patstr;
    }

    # ELSE:  Divide the list into two groups, based on common starting letter,
    # and recurse on them.
    my @half1 = ();
    my @half2 = ();
    if ($words[0] eq "") {
        # Special Case:  Divide on the empty string.  The group will start
        # with "(?:|...)"
        shift(@words);
        @half2 = @words;
    } else {
        my $markerchar = substr($words[0], 0, 1);
        foreach (@words) {
            if (m<^$markerchar>) { push @half1, $_; }
            else { push @half2, $_; }
        }
    }
    undef @words;
    if ($_Verbose) {
        print STDERR ("# recursive_regexp_grouper():  ",
                      " Divide & Conquer, left half.\n");
    }
    $patstr .= $group_open;
    @_RecursiveRegexpGrouper_Stack = ($recursionDepth+1, "", 0, @half1);
    $patstr .= recursive_regexp_grouper();
    if ($_Verbose) {
        print STDERR ("# recursive_regexp_grouper():  ",
                      " Divide & Conquer, right half.\n");
    }
    $patstr .= "|";
    @_RecursiveRegexpGrouper_Stack = ($recursionDepth+1, "", 0, @half2);
    $patstr .= recursive_regexp_grouper();
    $patstr .= $group_close;
    return $patstr;
}


1;  # don't forget to return a true value from the file
## POD STARTS HERE ##
__END__

=head1 NAME

jpwTools - Package containing John's Perl Tools.

=head1 SYNOPSIS

=over 0

=item datestamp

=item datetime_now 

=item check_syscmd_status I<args...>

=item do_error(I<filename>, $!, $@)

=item circular_shift I<@list> [, I<count>]

=item circular_pop I<@list> [, I<count>]

=item const_array I<value>, I<n_elements>

=item uniq I<@list>

=item invert_hash I<%hash> [, I<%invHash_out>]

=item rename_keys(I<%hash>, I<%oldKey2newKey>)

=item transform_keys(I<%hash>, I<&operator>)

=item lc_keys(I<%hash>)

=item uc_keys(I<%hash>)

=item asymm_diff I<\@list1, \@list2>

=item asymm_diff I<\@list, \%hash>

=item asymm_diff I<\@hash, \%list>

=item asymm_diff I<\%hash1, \%hash2>

=item stats I<@list> [, I<confidence>]

=item stats_gaussian I<@list>

=item print_hash I<name>, I<%hash> [, I<regexp, sub> ...]

=item print_array I<name>, I<%hash> [, I<regexp, sub> ...]

=item fprint_hash I<fh_ref>, I<name>, I<%hash> [, I<regexp, sub> ...]

=item fprint_array I<fh_ref>, I<name>, I<%hash> [, I<regexp, sub> ...]

=item print_dump [I<fh_ref>, ]  [I<name>, ] I<%hash> [, I<regexp, sub> ...]

=item create_regexp_group I<@words>

=item not_empty I<var>

=item set_array_if_nonempty I<$scalarvar>, I<%map>, I<key>

=item set_scalar_if_nonempty I<@listvar>, I<%map>, I<key>

=item non_overlapping I<@prefixes>

=item read_options I<option_filename>

=back

=head1 DESCRIPTION

=over 2

=item * 

datestamp

Returns string containing the current date, in the form 'YYYYMMDD'.  Useful
for creating date-stamped filenames.

=item * 

datetime_now

Returns 6 element array containing the date and time.  The array contents are
of the form: C<(year, month, day, hour24, min, sec)>.

=item * 

check_syscmd_status I<args...>

Checks \$\?, the status of the last system command run.  If the status is
nonzero, it prints out the specified args as part of an error message, then
aborts the program. 

=item *

do_error(I<filename>, $!, $@)

Use this to generate an error message after a failed do-call.  Use it in a
statement like this:

=over 4

do "file.pl" or die(do_error("file.pl", $!, $@));

=back


=item * 

circular_shift I<@list> [, I<count>]

Shifts I<count> elements off of I<@list>, or 1 if I<count> isn't specified.
Like the builtin C<shift> command, returns the shifted elements.  Unlike
C<shift>, it takes the shifted elements and immediately C<push>es them onto
the back of I<@list>.  Thus, I<@list> never loses elements; they merely change
location.

=item * 

circular_pop I<@list> [, I<count>]

Pops I<count> elements off of I<@list>, or 1 if I<count> isn't specified.
Like the builtin C<pop> command, returns the popped elements.  Unlike
C<popped>, it takes the popped elements and immediately C<unshift>s them onto
the front of I<@list>.  Thus, I<@list> never loses elements; they merely
change location. 

=item * 

const_array I<value>, I<n_elements>

Create an array, I<n_elements> long, with each element set to I<value>.
You can do this inline, without the overhead of the function call, with this
piece of Perl code:

=over 4

C<my @ca = ( I<value> ) x I<n_elements>;>

C<my @set{@members} = ( I<value> ) x I<@members>;>

=back

I.e.  the C<x> operator works on arrays as well as strings.  Note that the
thing following the C<x> operator is always evaluated in scalar context.

=item * 

uniq I<@list>

Like the Unix utility C<uniq>: Returns an array containing only the unique
members of I<@list>.  The input list does not need to be sorted.  The output
is in an arbitrary order (neither sorted, nor related to the ordering of
I<@list>).

Consider using a hash instead of an array if you truly need a set of unique
values.

=item *

invert_hash I<%hash> [, I<%invHash_out>]

Takes I<%hash> and inverts it, converting each scalar value in I<%hash> to a
key in I<%invHash_out> whose value is the corresponding key from I<%hash>.
Non-scalar values are ignored (i.e. they do not appear in I<%invHash_out>.

When called with only one arg, C<invert_hash()> returns the inverse hash.

=item *

rename_keys(I<%hash>, I<%oldKey2newKey>)

"Renames" a set of keys in I<%hash>, using the map I<%oldKey2newKey> to
determine which keys to rename and what to change them to.

In reality, it takes each (key, value) pair in I<%oldKey2newKey>, sets the
new key in I<%hash> to the value of the old key, then deletes the old key from
I<%hash>.  This is the only way to "rename" a key.  So, if the values of
I<%hash> are all large strings, this routine will be a performance hit.  For a
hash of references, however, it shouldn't be too bad.

=item *

transform_keys(I<%hash>, I<&operator>)

"Modifies" all of the keys in I<%hash> by applying I<&operator> to each one.

In reality, there is no way to "modify" a key.  What this function actually
does is call I<&operator> on each key to create the new key, sets the new
key to the old key's value, then deletes the old key.  So, if the values of
I<%hash> are all large strings, this routine will be a performance hit.  For a
hash of references, however, it shouldn't be too bad.

=item *

lc_keys(I<%hash>)

=item *

uc_keys(I<%hash>)

Both of these functions are akin to calling C<transform_keys(I<%hash>, &lc)>
and C<transform_keys(I<%hash>, &uc)>, respectively.  

They are not, however, I<equivalent> to the two calls above.  Those two calls
are not legal Perl.

The problem lies with Perl itself.  C<lc()> and C<uc()> are not in the
function symbol table of namespace C<main::> (if at all).  At best, they are
only bare symbols.  They may be tokenized immediately at compile time, for
that matter.  So, we can't take a reference to them.   We must, instead,
resort to the trick of creating a "wrapper" C<sub> around C<lc()> and
C<uc()>.

Bear this in mind whey trying to pass one of the built-in Perl functions to
C<transform_keys()>.

=item * 

asymm_diff I<set1>, I<set2>

Performs a "set subtraction", returning all of the elements from I<set1> that
are not also in I<set2>.  The arguments I<set1> and I<set2> must be references
to an array or to a hash.  When one or both arguments reference a hash,
C<asymm_diff()> operates on the hash's keys, ignoring (and preserving) the
values.  The subroutine returns either an array or hash, maching whatever data
type I<set1> has.  Note that neither I<set1> nor I<set2> are modified by this
subroutine.

=item *

stats I<@list> [, I<confidence>]

For the given I<@list>, computes general measures of central tendency ... in
this case, the median ...  and dispersion about the median.  Returns an array
containing:

C<(min, lowerDispersion, median, upperDispersion, max)>

C<min> and C<max> are self-explanatory, and are computed for free as a
by-product of determining the C<median>.  C<lowerDispersion> and
C<upperDispersion> are the dispersion about the C<median>, computed to the
specified interval or percentage. 

The term "central tendency" refers to the value in a set of data that is most
likely to occur.  The "dispersion" refers to a range of values in a set of
data which are most likely to occur.  The central tendency, therefore, always
is in the dispersion interval, hence the term "dispersion about the central
tendency".  "Percent dispersion" refers to the specific likelihood of
everything in the dispersion interval.  So, "50% dispersion" means that 50% of
the data in the set falls in the range defined by the dispersion.  For
data obeying a Gaussian distribution, the mean measures the central tendency
of the data, while the standard deviation measures the dispersion.

I<confidence> determines the percent dispersion about the median and can be
any of the following:

=over 4

=item any real < 1.0

If I<confidence> is a numeric real value less than 1.0, it is interpreted as
the percent dispersion about the median to compute.  (Any leading '-' on a
numeric value is ignored.)

=item q

Computes the "interquartile ranges," which corresponds to 50% dispersion about
the median.

=item 3s

Computes 99.73% dispersion about the median.  This is equivalent to 3 standard
deviations for a Gaussian distribution.

=item 2s

Computes 95.45% dispersion about the median.  This is equivalent to 2 standard
deviations for a Gaussian distribution.

=item default

For any other value of I<confidence>, or if it's omitted, computes 68.27%
dispersion about the median.  This is equivalent to 1 standard deviations for
a Gaussian distribution.

=back 

Note that C<stats()> will not work well for multi-modal distributions,
especially if two or more of the peaks in the distributions are of similar
height.  Multi-modal distributions require more sophisticated techniques for
computing central tendency and dispersion.

=item *

stats_gaussian I<@list>

Computes the "Gaussian statistics" of I<@list>, a la Åß14 of "Numerical
Recipies."  Returns an array containing:

C<(mean, variance, skew, kurtosis, StdDeviation, AvgDeviation)>

...where C<StdDeviation> is the square root of the C<variance>, and the
C<AvgDeviation> is the mean of abs(I<<@list>-mean).

These are referred to as "Gaussian statistics" because they work best for data
with a Gaussian or near-Gaussian distribution.  Only for these distributions
does the mean give a good measure of central tendency and the standard
deviation  give a good measure of dispersion about the central tendency.

If your data's distribution is asymmetric, has large, significant tails, or
outliers, consider using L<stats()|/"stats"> instead.  For these kinds of
distributions, the mean and variance will not be meaningful values.

=item *

fprint_hash I<fh_ref>, I<name>, I<%hash> [, I<regexp, sub> ...]

Convenience wrapper around 
C<print_dump(I<fh_ref>, [I<name>,] I<\%hash> [, ...])>.  I<name> can be the
empty string, in which case L<print_dump()|/"print_dump"> is called without it.

=item *

fprint_array I<fh_ref>, I<name>, I<%hash> [, I<regexp, sub> ...]

Convenience wrapper around 
C<print_dump(I<fh_ref>, [I<name>,] I<\@list> [, ...])>.  I<name> can be the
empty string, in which case L<print_dump()|/"print_dump"> is called without it.

=item *

print_hash I<name>, I<%hash> [, I<regexp, sub> ...]

Equivalent to C<fprint_hash(\*STDOUT, I<name>, I<\%hash> [, ...])>.

=item *

print_array I<name>, I<@list> [, I<regexp, sub> ...]

Equivalent to C<fprint_array(\*STDOUT, I<name>, I<\@list> [, ...])>.

=item * 

print_dump [I<fh_ref>, ] [I<name>, ] I<$ref> [, I<regexp, sub> ...]

Calls C<Data::Dumper::Dumper()> on I<$ref>, printing the results.  When called
with more than one arg, the first and/or second arg can be one of the
following:

=over 2

=item I<fh_ref>, a GLOB reference

I<fh_ref> is treated as the filehandle to print to.  If this is the only arg,
C<print_dump> dies.

=item I<name>, a scalar

I<name> is passed to C<Dumper()> as the variable name.

=item I<fh_ref>, I<name>, I<$ref>

Calls C<Dumper([I<$ref>], [I<name>])>, printing the results to I<fh_ref>.

If you call C<print_dump> with only two args, the first of which is I<fh_ref>,
the second arg is assumed to be the variable to dump, not the name.

=back

C<print_dump()> forces C<$Data::Dumper::Sortkeys==1> and
C<$Data::Dumper::Indent==1>.

Every additional pair of arguments, I<regexp, sub>, are treated as a
substitution operation to apply to the string returned by
C<Data::Dumper::Dumper()>.  Each pair is used to construct the expression:
C<s/I<regexp>/I<sub>/mg>, which will then be C<eval>'d.  Be sure, therefore,
to pass each I<regexp> and I<sub> single-quoted.

=item *

create_regexp_group I<@words>

Takes the array I<@words> and groups it into a pattern string for use in a
regular expression.  The pattern string is compact, with words grouped
together by common prefix & suffix.

=item *

non_overlapping I<@words>

Returns a list of "non-overlapping elements":  elements that are not
prefixes of any other element, or those that are a minimum-sized prefix.

=item *

not_empty I<var>

Returns C<true> if I<var> is:

=over 2

=item *

a scalar value with nonzero length;

=item *

a reference to a scalar variable whose value has nonzero length;

=item *

a reference to an array or hash with at least one element.

=back

For any other type of variable or reference, returns C<false>.

=item *

set_array_if_nonempty I<@array>, I<%map>, I<key>

If C<$I<map>{I<key>}> is non-empty (as determined by C<not_empty()>),
sets I<@array> to C<@{I<$map>{I<key>}}>.  

(Thus, the value of I<%map> corresponding to I<key> had better be an array
reference, or this function will return an error.)

=item *

set_scalar_if_nonempty I<@listvar>, I<%map>, I<key>

If C<$%I<map>{I<key>}> is non-empty (as determined by C<not_empty()>),
sets I<$scalarvar> to its value.


=item *

read_options I<option_filename>

Reads the file named I<option_filename> for options, returning them in a
hash.  This is a very powerful routine, permitting you to create B<safe>
configuration files for Perl scripts run by root.

The option file syntax:

=over 2

=item - 

Blank lines and whitespace at the beggining or end of a line are
ignored.

=item - 

Comments are lines starting with the '#' character.

=item -
 
The first non-whitespace character on a line starts an option name.

=item - 

Scalar options appear on the same line with their value, separated
by the one of delimiter characters ':' or '='.  

=item - 

Option names can contain any character except ':' and '='.

=item - 

Any whitespace surrounding the '=' or ':' delimiter is ignored.

=item - 

You cannot set a scalar option to the value "(".  [See below.]

=item - 

Options can contain an array value:

=over 2

=item * 

To start an array value, use a '(' character after the delimiter.

=item * 

The elements of the array are each subsequent line in the file.

=item * 

You can indent array elements.  [See below.]

=item * 

The array ends at the next line containing a lone ')' character.

=back

=item - 

Option names, scalar values, and array elements can neither begin
nor end with whitespace characters.  They are stripped off.

=item - 

The options are returned in a hash map, keyed by name.   Array
options are stored as references to anonymous Perl arrays.

=back

=back 

=cut

#################
#
#  End

