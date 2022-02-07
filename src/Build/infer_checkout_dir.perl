#!/usr/local/bin/perl -w

# This is deprecated; the new version is in icd.pl

# this is the long-hand form of the script we use to calculate CHECKOUT_DIR
# in each Makefile

$_ = `pwd`;
chop;
$_ .= "/";
# reversing twice is a kludge in order to match right to left
$_ = scalar reverse("$_");
if (s:.*/crs/::) {
    print scalar reverse($_);
} else {
    print "<error>";
    die "*** could not calculate CHECKOUT_DIR ***\n";
}

# and here is the short-hand form
# $(shell perl -e '$$_ = `pwd`; chop; $$_ = scalar reverse("$$_/"); if (s:.*/crs/::) { print scalar reverse($$_); } else { print "<error>"; die "*** could not calculate CHECKOUT_DIR ***\n"; }')
# And here's Brennan's revised form
# $(shell perl -e '$$_ = `pwd`; if (s:(.*)src(.*?)$$:$$1:) { print $$_; } else { print "<error>: $$_"; die "*** could not calculate CHECKOUT_DIR ***\n"; }')
