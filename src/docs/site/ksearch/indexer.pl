#!/usr/bin/env perl

# KSearch Client Side Indexer v1.0
#
# Copyright (C) 2000 David Kim (kscripts.com)
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or (at
# your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
# USA

my $t0 = time();

my $configuration_file = 'configuration/configuration.pl';	#CONFIGURATION PATH#  
require $configuration_file;

if ($MAKE_LOG) {
	my $indexnumbers = $INDEX_NUMBERS ? "yes" : "no";
	my $removecommonterms = $REMOVE_COMMON_TERMS ? "yes [cutoff = $REMOVE_COMMON_TERMS percent]" : "no";
	open(LOG,">".$LOG_FILE) or (warn "Cannot open log file $LOG_FILE: $!");
	print LOG localtime()."\nConfiguration File: $configuration_file\n";
	print LOG "\nINDEXER SETTINGS:\n";
	print LOG "Minimum term length: $MIN_TERM_LENGTH\n";
	print LOG "Description length: $DESCRIPTION_LENGTH\n";
	print LOG "Index whole numbers: $indexnumbers\n";
	print LOG "Remove common terms from index: $removecommonterms\n";
	print LOG "Index files with extensions: ".(join " ", @FILE_EXTENSIONS)."\n";
}

$|=1;

my ( $allterms, $filesizetotal, $file_id );
my ( @terms, @file_ids, @titles, @descriptions, @ignore_files );
my ( %terms, %modification_date, %size, %file_name );

print "\nLoading files to ignore:\n";
print LOG "\nLoading files to ignore:\n" if $MAKE_LOG;

ignore_files();

print "\nUsing stop words file: $STOP_TERMS_FILE\n";
print LOG "\nUsing stop words file: $STOP_TERMS_FILE\n" if $MAKE_LOG;

my $stopwords_regex = ignore_terms();

print "\nStarting indexer at $INDEXER_START\n\n";
print LOG "\nStarting indexer at $INDEXER_START\n\n" if $MAKE_LOG;

indexer($INDEXER_START);

# remove COMMON TERMS from index
remove_common_terms() if $REMOVE_COMMON_TERMS;	

create_database();

print "\n\nFinished: Indexed ".keys (%terms).' terms from '.$file_id.' files ('.$filesizetotal.'KB) with '.$allterms." total terms.\n\n";
print LOG "\n\nFinished: Indexed ".keys (%terms).' terms from '.$file_id.' files ('.$filesizetotal.'KB) with '.$allterms." total terms.\n\n" if $MAKE_LOG;

set_configuration();

print "Saved index file: $DATABASE (".int((((stat($DATABASE))[7])/1024)+.5)." KB)\n\n";
print LOG "Saved index file: $DATABASE (".int((((stat($DATABASE))[7])/1024)+.5)." KB)\n\n" if $MAKE_LOG;

print "Saved indexing information in logfile:\n $LOG_FILE\n\n" if $MAKE_LOG;

my $timediff = time() - $t0;
my $seconds = $timediff % 60;
my $minutes = ($timediff - $seconds) / 60;
if ($minutes >= 1) { $minutes = ($minutes == 1 ? "$minutes minute" : "$minutes minutes"); } else { $minutes = ""; }
$seconds = ($seconds == 1 ? "$seconds second" : "$seconds seconds");
print "Total run time: $minutes $seconds\n";
print LOG "Total run time: $minutes $seconds\n" if $MAKE_LOG;
close (LOG) if $MAKE_LOG;


####sub routines###########################################################################
sub indexer {
  my $dir = $_[0];
  my ($file_ref, $file);
  chdir $dir or (warn "Cannot chdir $dir: $!\n" and next);
  opendir(DIR, $dir) or (warn "Cannot open $dir: $!\n" and next);
  my @dir_contents = readdir DIR;
  closedir(DIR);
  my @dirs  = grep {-d and not /^\.{1,2}$/} @dir_contents; 
  my @files = grep {-f and /^.+\.(.+)$/ and grep {/^\Q$1\E$/} @FILE_EXTENSIONS} @dir_contents;
  FILE: foreach my $file_name (@files) {
    $file = $dir."/".$file_name;
    $file =~ s/\/\//\//og;
    foreach my $skip (@ignore_files) {
      next FILE if $file =~ m/^$skip$/;
    }
    $file_ref = make_file_ref($file);
    index_file($file,$file_ref);
  }
  DIR: foreach my $dir_name (@dirs) {
    $file = $dir."/".$dir_name;
    $file =~ s/\/\//\//og;
    foreach my $skip (@ignore_files) {
      next DIR if $file =~ /^$skip$/;
    }
    indexer($file);
  }
}

sub make_file_ref {
  my $file = $_[0];
  ++$file_id;
  push @file_ids, $file_id;
  $size{$file_id} = int((((stat($file))[7])/1024)+.5);		# get size of file in kb
  $filesizetotal += $size{$file_id};				# get total size of all files
  my $update = (stat($file))[9];	 			# get date of last file modification
  $modification_date{$file_id} = get_date($update);

  print "Indexed $file \n Last Updated: $modification_date{$file_id} \n File Size: $size{$file_id} KB\n";
  print LOG "Indexed $file \n Last Updated: $modification_date{$file_id} \n File Size: $size{$file_id} KB\n" if $MAKE_LOG;

  $file =~ m/^$INDEXER_START(.*)$/;
  $file = $1;
  $file_name{$file_id} = $file;
  return $file_id;
}

sub index_file {
  my ($file, $file_id) = @_;
  my ($contents, $termcount);
  my %term_total;

  undef $/;
  open(FILE, $file) or (warn "Cannot open $file: $!" and next);
  $contents = <FILE>;
  close(FILE); 
  $/ = "\n";

  record_description($file_id, $file, $contents);	# record titles and descriptions
  $contents = clean($contents, $file_id); 		# isolate terms 
  
  print LOG "\nTerms:\n" if ($MAKE_LOG && $LOG_TERMS);

  foreach (split " ", $contents) {
    $termcount++;
    next if m/^$stopwords_regex$/io;			# skip stop words
    if (!$INDEX_NUMBERS) {next if m/^\d+$/;}		# skip whole numbers if configured to do so
    if (length $_ >= $MIN_TERM_LENGTH && length $_ <= $MAX_TERM_LENGTH) {	# count each term in file if valid
      ++$term_total{$_};
    }
  }
  foreach (keys %term_total) {
    $terms{$_} .= pack("ff", $file_id - 1, $term_total{$_});# pack term count with file reference
  }
  $allterms += $termcount;				# count all terms in all files
  print " Total terms in file: ".$termcount."\n Total terms indexed: ".keys (%term_total)."\n\n";
  print LOG " Total terms in file: ".$termcount."\n Total terms indexed: ".keys (%term_total)."\n\n" if $MAKE_LOG;
}

sub get_date {  # gets date of last modification
   my $updatetime = $_[0];
   my @month = ('Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec');
   my ($mday,$mon,$yr) = (localtime($updatetime))[3,4,5];
   $yr += 1900;
   my $date = "$month[$mon] $mday, $yr";  
   $date ||= "n/a";
   return $date;
}

sub clean {  # clean up terms to be indexed
  my ($contents, $file_ref) = @_;
  $contents =~ s/(<script[^>]*>.*?<\/script>)|(<style[^>]*>.*?<\/style>)/ /gsi;	# remove scripts and styles
  $contents =~ s/(<[^>]*>)|(&[^\s]+;)/ /gs;		# remove html
  $contents =~ tr/A-Za-z0-9'\.,\-\$\%/ /cs;		# remove invalid characters
  $contents =~ s/ [\.,'\-]+([A-Za-z])/ $1/gs;		# remove [.,'-] in front of words
  $contents =~ s/[\.,\-]+[']* / /gs;			# remove [.,-]' at end of words
  return lc $contents;
} 

sub record_description {  # record descriptions and titles
  my ($file_ref, $file, $contents) = @_;
  my ($description, $title);
  my @temparray;
  $contents =~ s/(<script[^>]*>.*?<\/script>)|(<style[^>]*>.*?<\/style>)/ /gsi;	# remove scripts and styles
  $contents =~ m/<META\s+name\s*=\s*[\"\']?description[\"\']?\s+content=[\"\']?(.*?)[\"\']?>/is;
  $description = $1;
  my $usedmeta = 1 if ($description && $description !~ /^\s*$/);
  unless ($description && $description !~ /^\s*$/) {
    $contents =~ m/<BODY.*?>(.*)<\/BODY>/si;
    $description = $1;	
    $description = $contents unless $description;
    $description =~ s/(<[^>]*>)|(&[^\s]+;)/ /gs; # remove html
  }
  $description =~ tr/A-Za-z0-9'\.,\-\$\%/ /cs;	 # remove invalid characters
  $description =~ s/\s+/ /gs;
  @temparray = split " ", $description;
  if ($usedmeta) {
          $description = join " ", @temparray[0..$DESCRIPTION_LENGTH];
  } else {
          my $start_desc = $DESCRIPTION_START;
          my $end_desc = $DESCRIPTION_START + $DESCRIPTION_LENGTH;
	  $start_desc = ($end_desc > scalar@temparray ? 0 : $DESCRIPTION_START);
          $description = join " ", @temparray[$start_desc..$end_desc];
          $description = "...".$description if $start_desc;
  }
  $description =~ s/\s+$//;
  $descriptions[$file_ref] = $description;

  $contents =~ m/<TITLE>\s*(.*?)\s*<\/TITLE>/is;
  $title = $1;
  $title =~ s/(<[^>]*>)|(&[^\s]+;)/ /gs; # remove html
  if(length($title) <= 0) {
    $contents =~ m/<H(\d)>\s*(.*?)\s*<\/H\1>/is;
    $title = $2;
  }
  $file =~ s/^.*\/([^\/]+)$/$1/g;
  $title ||= $file;
  $titles[$file_ref] = $title;
  print " Title: $title\n Description: $description\n";
  print LOG " Title: $title\n Description: $description\n" if $MAKE_LOG;
}

sub ignore_files {
  my @list;
  if (-e $IGNORE_FILES_FILE) {
    open (FILE, $IGNORE_FILES_FILE) or (warn "Cannot open $IGNORE_FILES_FILE: $!\n" and next);
    while (<FILE>) {
      chomp;
      $_ =~ s/\r//g;        
      $_ =~ s/\#.*$//g;
      $_ =~ s/[\/\s]*$//;
      next if /^\s*$/;
      push @list, "$_\n";
      $_ = quotemeta;       
      $_ =~ s/\\\*/\.\*/g;  
      push @ignore_files, $_;
    }
    close (FILE);
    if (scalar@list > 0) { print @list; print LOG @list; } else { print "List is empty\n\n"; print LOG "List is empty\n\n"; }
  } else {
    print STDERR "Warning: Can't find $IGNORE_FILES_FILE.\n";
  }
}

sub ignore_terms {
  my @stopwords;
  my $stopwords_regex;
  open (FILE, $STOP_TERMS_FILE) or (warn "Cannot open $STOP_TERMS_FILE: $!" and next);
  while (<FILE>) {
    chomp;
    last if /\#DO NOT EDIT/;
    $_ =~ s/\#.*$//g;
    $_ =~ s/\s//g;
    next if /^\s*$/;
    push @stopwords, $_;
  }
  close(FILE);
  $stopwords_regex = '(' . join('|', @stopwords) . ')';
  return $stopwords_regex;
}
           
sub remove_common_terms { # removes common terms from index if present above cutoff percentage
	my (@common_terms, @stop_terms, @stop_terms_copy);
	while (($term,$val) = each %terms) {
		my %p = unpack('f*', $val);
		if (scalar(keys %p) > ($REMOVE_COMMON_TERMS/100 * $file_id)) {
			push @common_terms, $term;
		}
	}
	if (@common_terms) {
		print "Removed common terms present in over $REMOVE_COMMON_TERMS percent of all files:\n";
		print LOG "Removed common terms present in over $REMOVE_COMMON_TERMS percent of all files:\n" if $MAKE_LOG;
		foreach $term (@common_terms) {
			delete ($terms{$term});	# remove common terms from index
			print "$term\n";
			print LOG "$term\n" if $MAKE_LOG;
		}
	} else { 
		print "\nNo common terms were present in over $REMOVE_COMMON_TERMS percent of all files";
		print LOG "\nNo common terms were present in over $REMOVE_COMMON_TERMS percent of all files" if $MAKE_LOG;
	}
}

sub create_database {
	open(NEWFILE,">$DATABASE") || die "Can't open $DATABASE\n";
	print NEWFILE 'var u = new Array();'."\n";
	foreach my $file_id (@file_ids) {
	        print NEWFILE 'u['.($file_id -1).'] = "'.$file_name{$file_id}.'|'.$titles[$file_id].'|'.$descriptions[$file_id].'|'.$size{$file_id}.'|'.$modification_date{$file_id}.'";'."\n";
	}
	@terms = sort(keys %terms);
	print NEWFILE 'var w = new Array();'."\n";
	foreach my $term (@terms) {
		my $filelist;
                my %p = unpack('f*', $terms{$term});
	        if ($term =~ /^[0-9]+$/) {
	                print NEWFILE 'w["#'.$term.'"] = "';
	        } else {
	                print NEWFILE 'w["'.$term.'"] = "';
	        }
		while (my ($key, $val) = each %p) {
                        $filelist .= $key.'='.$val.'|';
	        }
	        $filelist =~ s/\|$/\"/g;
	        print NEWFILE $filelist.';'."\n";
	}
	close (NEWFILE);
}

sub set_configuration () {
	my $skip;
	open (FILE,$TOP_FRAME) || warn "Cannot open $TOP_FRAME: $!";
	my @lines = <FILE>;
	close (FILE);
	open (FILE,">$TOP_FRAME") || warn "Cannot open $TOP_FRAME: $!";
	foreach $line (@lines) {
		if ($line =~ /^\/\/ START CONFIGURATION/) {
			print FILE $line;
			print FILE "var displaymatches = $RESULTS_PER_PAGE; \/\/ number of matches to display\n";
			print FILE "var termlength = $MIN_TERM_LENGTH; \/\/ minimum term length\n";
			print FILE "var displaylastupdated = $DISPLAY_UPDATE; \/\/ set to 0 if you don't want the last updated date to be shown in results\n";
			print FILE "var displayfilesize = $DISPLAY_SIZE; \/\/ set to 0 if you don't want the file size to be shown in results\n";
			print FILE "var urlprefix = \"$BASE_URL\";    \/\/ where the manual resides\n";
                        print FILE "var pageref = \"$PAGE_REF\"; \/\/ the string to append to urlprefix when viewing non-index.html pages\n";
			print FILE "var style_sheet = \"$STYLE_SHEET\"; \/\/ url of style sheet\n";
			$skip = 1;
		}
		$skip = 0 if $line =~ /^\/\/ END CONFIGURATION/;
		next if $skip;
		print FILE $line;
	}
	close (FILE);
}
