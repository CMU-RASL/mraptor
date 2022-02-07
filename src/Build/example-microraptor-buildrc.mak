#!gmake 	# Tell emacs about this file: -*- Makefile -*-  
# $Id: example-microraptor-buildrc.mak,v 1.3 2004/08/19 22:17:41 trey Exp $
#
# PROJECT:      Life in the Atacama
# DESCRIPTION:  install this file as ~/.microraptor.buildrc to set up
#               your personal configuration
#

######################################################################
# This section is microraptor-specific configuration... tune for
#   your platform/compiler.
#
# NO_WORDEXP: If defined, don't use Gnome's wordexp expansion routines
# C_FILE_STREAMS: If defined, use FILE*'s; otherwise, use ofstreams
# NO_CHILD_ENV_SETTING: If defined, don't set child environment from config file
#                       No idea why this was part of CYGWIN...
# CYGWIN_HEADERS: If defined, use the Cygwin headers (there may be a more general
#		  way to describe the difference; not sure what it is yet).
# CYGWIN_IO: If defined, use Cygwin file IO oddities (see comments in code).
# ENABLE_FIFO: If defined, allow fifo connections.  Not yet implemented under
#              gcc 3.2.
# LIBSTDC_HAS_GETS: If not defined, use gets2 custom replacement for ifstream.gets()
# NO_BIG_XPMS: If defined, don't compile big xpm files (so we don't get cool images).
# On some gcc variants, compiling xpms takes a really... long... time.

# Default flags
#CFLAGS += -DC_FILE_STREAMS -DLIBSTDC_HAS_GETS

ifeq (,$(findstring CYGWIN,$(shell uname)))
  # linux flags
  CFLAGS += -DC_FILE_STREAMS
else
  # in cygwin, different default flags
  CFLAGS += -DCYGWIN_HEADERS -DCYGWIN_IO -DC_FILE_STREAMS -DNO_WORDEXP
endif

# This set of flags works on vostok (RH9 laptop, gcc 3.2.2)
CFLAGS += -DC_FILE_STREAMS

# This set of flags works to build common and rcl on corea.cfa.cmu.edu
# [CYGWIN_NT-5.1 1.5.9(0.112/4/2)]
# CFLAGS += -DCYGWIN_HEADERS -DCYGWIN_IO -DC_FILE_STREAMS -DNO_WORDEXP

# This set of flags works on valerie (iRobot-ized RH6.2 (B21R))
# CFLAGS += -DC_FILE_STREAMS -DCYGWIN_HEADERS

# This set works on bacall & rita (CS facilitized RH7.1)
#CFLAGS += -DC_FILE_STREAMS -DLIBSTDC_HAS_GETS

######################################################################
# This section is standard configuration variables for the makefile
# system.

# you might want to specify the location of gcc/g++ (the default
# is to use your path)

# COMPILER_DIRECTORY_WITH_SLASH := /usr/local/bin/

# uncomment to recover old behavior that files are directly built
# to e.g. software/bin/linux2 instead of ./bin/linux2 (faster, saves space)

# SKIP_LOCALS := 1

# uncomment to get the behavior Dom likes, that the binaries are built
# in the current directory (actually, this builds them in ./bin/linux2
# and symlinks them to .)  note: not compatible with SKIP_LOCALS.

# PUT_MY_BINARIES_RIGHT_HERE_THANK_YOU_VERY_MUCH := 1

######################################################################
# $Log: example-microraptor-buildrc.mak,v $
# Revision 1.3  2004/08/19 22:17:41  trey
# changed default build flags under cygwin
#
# Revision 1.2  2004/08/17 13:49:57  trey
# added flags for corea.cfa.cmu.edu
#
# Revision 1.1  2004/05/21 19:16:58  trey
# branched example-buildrc.mak into two files, one for atacama and one for microraptor; added COMPILER_DIRECTORY_WITH_SLASH configuration variable
#
# Revision 1.2  2004/05/13 20:52:50  trey
# fixed so that a build works even if you do not have a buildrc in $HOME
#
# Revision 1.1  2004/05/13 20:25:50  trey
# synced up with atacama makefile system again
#
#
#
