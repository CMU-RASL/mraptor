#!/usr/bin/perl -w

# Test script for icd.pl

# To add a test case, append the test string to @test_vals, the
# desired result string (w/o trailing slash) to @result_vals, and the
# appropriate number of tabs to @tabs and @tabs2 (can be "" if you
# don't care about output alignment :-)

# Test cases:
# /bar/src/blah/src          (should give /bar/src/blah/)
# /bar/src/blah/src/         (should give /bar/src/blah/)
# /bar/src/blah/src/src_foo  (should give /bar/src/blah/)
# /bar/src/blah/src/foo_src  (should give /bar/src/blah/)
# /bar/src/src_foo           (should give /bar/)
# /bar/src/foo_src           (should give /bar/)
# /bar/src/                  (should give /bar/)
# /bar/src                   (should give /bar/)

$icd_loc = `/bin/pwd`;
chop $icd_loc;

@test_vals = ("/tmp/bar/src/blah/src", "/tmp/bar/src/blah/src/", "/tmp/bar/src/blah/src/src_foo", "/tmp/bar/src/blah/src/foo_src", "/tmp/bar/src/src_foo", "/tmp/bar/src/foo_src", "/tmp/bar/src/", "/tmp/bar/src");

@result_vals = ("/tmp/bar/src/blah", "/tmp/bar/src/blah", "/tmp/bar/src/blah", "/tmp/bar/src/blah", "/tmp/bar", "/tmp/bar", "/tmp/bar", "/tmp/bar");

@tabs = ("\t", "\t", "", "", "\t", "\t", "\t\t", "\t\t");
@tabs2 = ("\t", "\t", "\t", "\t", "\t\t", "\t\t", "\t\t", "\t\t");

$home = `pwd`;

$i = 0;
foreach $tv (@test_vals) {
  system("mkdir -p $tv");
  chdir $tv;
  print "${tv}:\t" . $tabs[$i];
  $icdout = `${icd_loc}/icd.pl`;
  chop $icdout; # Remove newline
  print $icdout . "\t" . $tabs2[$i];
  if($icdout eq $result_vals[$i] || $icdout eq $result_vals[$i] . "/") {
    print " PASS";
  } else {
    print " FAIL (should be $result_vals[$i])";
  }
  print "\n";
  $i++;
}

chdir $home;
