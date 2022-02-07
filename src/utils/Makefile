# COPYRIGHT 2004, Carnegie Mellon University
#!gmake 	# Tell emacs about this file: -*- Makefile -*-  
# $Id: Makefile,v 1.4 2004/12/02 18:26:12 brennan Exp $
#
# PROJECT:      Life in the Atacama
# DESCRIPTION:  

######################################################################
# DO NOT MODIFY THIS HEADER SECTION

CHECKOUT_DIR := $(shell perl -e '$$_ = `pwd`; if (s:(.*)/(src|src/|src/(.*?))$$:$$1:) { print $$_; } else { print "<error>: $$_"; die "*** could not calculate CHECKOUT_DIR ***\n"; }')
include $(CHECKOUT_DIR)/src/Build/header.mak

######################################################################
# PLACE MODIFICATIONS BELOW THIS LINE

ATACAMA_MEMBERS_DIR := $(HOME)/mraptor-dev/software/mraptor/
ifndef EXTERNAL_DIR
  EXTERNAL_DIR := $(HOME)/mraptor-dev/software/external
endif

# ADD ANY SUBDIRS THAT NEED TO BE VISITED DURING A BUILD TO THE SUBDIRS LIST
SUBDIRS := 

CPPFLAGS += -I$(ATACAMA_MEMBERS_DIR)/software/external/include
CFLAGS += -I$(EXTERNAL_DIR)/ipc/include -I$(EXTERNAL_DIR)/include
LDFLAGS += -L$(EXTERNAL_DIR)/ipc/lib/Linux

# add extra files to clean up with 'gmake clean' here
LOCAL_CLEAN_FILES := 

# compile with 'gmake MR_COMM_PROFILING=1 install' to get profiling output
ifneq (,$(MR_COMM_PROFILING))
 CPPFLAGS += -DMR_COMM_PROFILING
endif

ifeq (,$(NO_TELEMETRY_MANAGER))
 CPPFLAGS += -DMR_USE_ATACAMA_TELEMETRY_MANAGER
endif

# build a binary (you can use this multiple times)
BUILDBIN_TARGET := mrmonitor
BUILDBIN_SRCS := mrapMonitor.cc
BUILDBIN_DEP_LIBS := -lmrclient -lrcl -lmrcommon
BUILDBIN_INDEP_LIBS := -lipc -lpthread -lutil
BUILDBIN_WHOLE_DEP_LIBS :=
BUILDBIN_WHOLE_INDEP_LIBS :=
include $(BUILD_DIR)/buildbin.mak

ifeq (,$(TEST))

# any targets defined here will only be built with 'gmake TEST=1 install'

endif


######################################################################
# DO NOT MODIFY BELOW THIS POINT

include $(BUILD_DIR)/footer.mak

######################################################################
# $Log: Makefile,v $
# Revision 1.4  2004/12/02 18:26:12  brennan
# Fixed mrmonitor to compile with the new RCL syntax, and added a
# BUILD_UTILS make option to build it.  mrmonitor is currently only used
# by the Valerie project.
#
# Revision 1.3  2004/12/01 06:59:24  brennan
# Modified CHECKOUT_DIR inference to handle paths with multiple "src"'s.
#
# Revision 1.2  2004/04/28 17:15:21  dom
# Added header with copyright information to source and Makefiles.
#
# Revision 1.1  2003/11/24 02:07:33  brennan
# Initial commit, mrmonitor - monitors process status and does arbitrary
# stuff on status changes.  No restart functionality yet.
#
# Revision 1.14  2003/10/29 17:59:35  trey
# removed redundant -L link flag for the external directory
#
# Revision 1.13  2003/10/05 02:56:09  trey
# switched to variant of FIRE makefile system
#
#
