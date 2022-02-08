# COPYRIGHT 2004, Carnegie Mellon University
#!gmake 	# Tell emacs about this file: -*- Makefile -*-  
# $Id: Makefile,v 1.29 2006/06/23 16:39:40 trey Exp $
#
# PROJECT:      FIRE Architecture Project
# DESCRIPTION:  

######################################################################
# DO NOT MODIFY THIS HEADER SECTION

CHECKOUT_DIR := $(shell perl -e '$$_ = `pwd`; if (s:(.*)/(src|src/|src/(.*?))$$:$$1:) { print $$_; } else { print "<error>: $$_"; die "*** could not calculate CHECKOUT_DIR ***\n"; }')
#CHECKOUT_DIR := $(shell perl -e '$$_ = `pwd`; chop; $$_ = scalar reverse("$$_/"); if (s:.*/crs/::) { print scalar reverse($$_); } else { print "<error>"; die "*** could not calculate CHECKOUT_DIR ***\n"; }')
include $(CHECKOUT_DIR)/src/Build/header.mak

######################################################################
# PLACE MODIFICATIONS BELOW THIS LINE

ATACAMA_MEMBERS_DIR := $(HOME)/mraptor-dev/software/mraptor/
ifndef EXTERNAL_DIR
  EXTERNAL_DIR := $(HOME)/mraptor-dev/software/external/
endif

# ADD ANY SUBDIRS THAT NEED TO BE VISITED DURING A BUILD TO THE SUBDIRS LIST
SUBDIRS :=

CFLAGS += -I$(EXTERNAL_DIR)/ipc/include -I$(EXTERNAL_DIR)/include
LDFLAGS += -L$(EXTERNAL_DIR)/ipc/lib

#Add this Library directory to attempt linking to -lmrclient and -lrcl
# EXTERNAL CORRESPONDS TO ./mraptor-dev/software/mraptor/external/
# ./mraptor/src/rcl/lib/librcl.a
LDFLAGS += -L$(EXTERNAL_DIR)/../src/rcl/lib/

# build a binary (you can use this multiple times)
ifneq (,$(STATIC))
BUILDBIN_TARGET := mraptord_static
LDFLAGS += -static
else
BUILDBIN_TARGET := mraptord
endif
BUILDBIN_SRCS := mraptord.cc raptorDaemon.cc processRecord.cc stdoutBuffer.cc mrTimers.cc mrSignals.cc
BUILDBIN_DEP_LIBS := -lmrclient -lmrcommon -lrcl
BUILDBIN_INDEP_LIBS := -lipc -lpthread -lutil
BUILDBIN_WHOLE_DEP_LIBS :=
BUILDBIN_WHOLE_INDEP_LIBS :=
include $(BUILD_DIR)/buildbin.mak

# install data files to a directory (you can use this multiple times)
#INSTALLDATA_FILES := example.config
INSTALLDATA_DIR := /tmp
include $(BUILD_DIR)/installdata.mak

# install script files (you can only use this once)
INSTALLSCRIPT_FILES := printConfigFile.perl
include $(BUILD_DIR)/installscript.mak

ifneq (,$(TEST))

BUILDBIN_TARGET := testPrintToStderr
BUILDBIN_SRCS := testPrintToStderr.cc
BUILDBIN_DEP_LIBS :=
BUILDBIN_INDEP_LIBS :=
include $(BUILD_DIR)/buildbin.mak

BUILDBIN_TARGET := testTimers
BUILDBIN_SRCS := testTimers.cc mrTimers.cc
BUILDBIN_DEP_LIBS :=
BUILDBIN_INDEP_LIBS :=
include $(BUILD_DIR)/buildbin.mak

endif

######################################################################
# DO NOT MODIFY BELOW THIS POINT

include $(BUILD_DIR)/footer.mak

######################################################################
# $Log: Makefile,v $
# Revision 1.29  2006/06/23 16:39:40  trey
# added mrSignals
#
# Revision 1.28  2005/06/27 19:27:25  trey
# added mrTimers
#
# Revision 1.27  2004/12/01 06:59:24  brennan
# Modified CHECKOUT_DIR inference to handle paths with multiple "src"'s.
#
# Revision 1.26  2004/07/29 22:45:24  trey
# fixed flaws in stderr buffering
#
# Revision 1.25  2004/06/28 15:46:18  trey
# fixed to properly find ipc.h and libipc.a in new atacama external area
#
# Revision 1.24  2004/04/28 17:15:21  dom
# Added header with copyright information to source and Makefiles.
#
# Revision 1.23  2004/04/19 14:49:25  trey
# split raptorDaemon.cc into two files
#
# Revision 1.22  2004/04/02 21:27:43  mwagner
# Removed example.config softlink command
#
# Revision 1.21  2004/03/05 20:05:53  trey
# added option to statically link
#
# Revision 1.20  2003/11/18 23:16:27  trey
# checkout dir inference is now simpler and has better error checking
#
# Revision 1.19  2003/11/18 21:36:31  trey
# added install of printConfigFile.perl
#
# Revision 1.18  2003/11/15 17:53:40  brennan
# Split up CYGWIN #define into more reasonable/understandable chunks.  Completed move to FIRE build system.
#
# Revision 1.17  2003/10/29 17:59:41  trey
# removed redundant -L link flag for the external directory
#
# Revision 1.16  2003/10/05 02:55:36  trey
# switched to variant of FIRE makefile system
#
#
