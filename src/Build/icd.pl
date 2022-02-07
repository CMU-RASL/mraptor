#!/usr/bin/perl -w

# this is the long-hand form of the script we use to calculate CHECKOUT_DIR
# in each Makefile

# And here's Brennan's revised form
# $(shell perl -e '$$_ = `pwd`; if (s:(.*)/(src|src/|src/(.*?))$$:$$1:) { print $$_; } else { print "<error>: $$_"; die "*** could not calculate CHECKOUT_DIR ***\n"; }')

$_ = `pwd`;

# Find the last src directory in our current directory, and truncate
# the directory to just before it
if (s:(.*)/(src|src/|src/(.*?))$:$1:) {
  print $_;
} else { 
  print "<error>: $_"; die "*** could not calculate CHECKOUT_DIR ***\n";
}

