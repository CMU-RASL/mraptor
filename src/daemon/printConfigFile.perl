# tell emacs that this is: -*- perl -*- source
# $Id: printConfigFile.perl,v 1.8 2004/07/19 21:44:11 trey Exp $

# this file is intended to be "require"d by a config file.  you
# pass the config hash to &print_main(), and it prints it out
# in a way that the rcl parser can understand.

%cmap = ( ( map { chr($_), sprintf("%03lo", $_) }  (0..255) ),
	  "\\"=>'\\', "\r"=>'r', "\n"=>'n', "\t"=>'t', "\""=>'"' );

$max_print_arrays_as_hashes_depth = 1;

sub print_main {
    my $config = shift;
    my $max_depth = shift;

    if (defined $max_depth) {
	$max_print_arrays_as_hashes_depth = $max_depth;
    }

    if (! $config) {
	die "config file $0 didn't specify $MR_PROCESSES\n";
    }

    &print_obj($config, 0, 0);
    print "\n";
}

sub print_obj {
    my $obj = shift;
    my $indent = shift;
    my $depth = shift;

    my $obj_type = ref $obj;
    if (! $obj_type) {
	&print_scalar($obj);
    } elsif ($obj_type eq "HASH") {
	&print_hash($obj, $indent, $depth);
    } elsif ($obj_type eq "ARRAY") {
	if ($depth <= $max_print_arrays_as_hashes_depth) {
	    &print_array_as_hash($obj, $indent, $depth);
	} else {
	    &print_array($obj, $indent, $depth);
	}
    }
}

sub print_scalar {
    my $obj = shift;

    # check if it's a number
    if ($obj =~ m/ ^[+-]?          # optional leading sign
	           \d+             # integer part
                   (\.\d*)?        # optional decimal part
                   ([eE][+-]\d*)?  # optional exponent
                   $               # match whole string  
	         /x) {
	print $obj;
    } elsif ($obj eq 'true' or $obj eq 'false') {
	# or a bool
	print $obj;
    } else {
	# it's a string, gotta escape and quote it
	$obj =~ s/([\r\n\t\"\\\x00-\x1f\x7F-\xFF])/\\$cmap{$1}/sg;
	print "\"$obj\"";
    }
}

sub print_hash {
    my $hash = shift;
    my $indent = shift;
    my $depth = shift;

    my $ind_space = " " x $indent;
    my $ind2_space = " " x ($indent+2);
    print "{\n";
    my @keys = keys %{$hash};
    for (0..$#keys) {
	my $key = $keys[$_];
	my $value = $hash->{$key};
	print "$ind2_space$key => ";
	if (ref $value) {
	    print "\n", (" " x ($indent+4));
	}
	&print_obj($value, $indent+4, $depth+1);
	if ($_ != $#keys) { print ","; }
	print "\n";
    }
    print "$ind_space}";
}

sub print_array {
    my $array = shift;
    my $indent = shift;
    my $depth = shift;

    my $ind_space = " " x $indent;
    my $ind2_space = " " x ($indent+2);
    print "[\n";
    for (0..$#{$array}) {
	my $value = $array->[$_];
	print "$ind2_space";
	if (ref $value) {
	    print (" " x 2);
	}
	&print_obj($value, $indent+4, $depth+1);
	if ($_ != $#{$array}) { print ","; }
	print "\n";
    }
    print "$ind_space]";
}

sub print_array_as_hash {
    my $array = shift;
    my $indent = shift;
    my $depth = shift;

    my $ind_space = " " x $indent;
    my $ind2_space = " " x ($indent+2);

    print "{\n";
    my $n = $#{$array}+1;
    if (($n % 2) != 0) {
	die "syntax error: found a key with no value in a hash\n";
    }
    my $num_keys = $n / 2;
    for (0..($num_keys-1)) {
	my $key = $array->[$_*2];
	my $value = $array->[$_*2+1];
	print "$ind2_space$key => ";
	if (ref $value) {
	    print "\n", (" " x ($indent+4));
	}
	&print_obj($value, $indent+4, $depth+1);
	if ($_ != $num_keys-1) { print ","; }
	print "\n";
    }
    print "$ind_space}";
}

1;

######################################################################
# $Log: printConfigFile.perl,v $
# Revision 1.8  2004/07/19 21:44:11  trey
# improved flexibility of remapping vectors to maps
#
# Revision 1.7  2004/06/25 15:54:09  trey
# made printConfigFile handle bool type correctly
#
# Revision 1.6  2003/11/18 21:40:53  trey
# undid earlier change, turned out to not be the problem
#
# Revision 1.5  2003/11/18 21:37:09  trey
# fixed regexp for newer versions of perl
#
# Revision 1.4  2003/03/15 18:49:53  trey
# changed config file reading so that processes appear in order in the claw display
#
# Revision 1.3  2003/02/26 15:48:35  trey
# fixed problem with printConfigFile.perl needing to be in the same dir as the config file
#
# Revision 1.2  2003/02/26 05:08:45  trey
# streamlined config file format
#
# Revision 1.1  2003/02/24 22:09:21  trey
# fixed reading of perl config files
#
#
