# COPYRIGHT 2004, Carnegie Mellon University
#!gmake 	# Tell emacs about this file: -*- Makefile -*-  
# $Id: Makefile,v 1.21 2005/06/01 00:47:15 trey Exp $
#
# PROJECT:      Life in the Atacama
# DESCRIPTION:  

######################################################################
# DO NOT MODIFY THIS HEADER SECTION

CHECKOUT_DIR := $(shell perl -e '$$_ = `pwd`; if (s:(.*)/(src|src/|src/(.*?))$$:$$1:) { print $$_; } else { print "<error>: $$_"; die "*** could not calculate CHECKOUT_DIR ***\n"; }')
#CHECKOUT_DIR := $(shell perl -e '$$_ = `pwd`; chop; $$_ = scalar reverse("$$_/"); if (s:.*/crs/::) { print scalar reverse($$_); } else { print "<error>"; die "*** could not calculate CHECKOUT_DIR ***\n"; }')
include $(CHECKOUT_DIR)/src/Build/header.mak

######################################################################
# PLACE MODIFICATIONS BELOW THIS LINE

ATACAMA_MEMBERS_DIR := $(CHECKOUT_DIR)/mraptor/
ifndef EXTERNAL_DIR
  EXTERNAL_DIR := $(CHECKOUT_DIR)/../external
endif

# ADD ANY SUBDIRS THAT NEED TO BE VISITED DURING A BUILD TO THE SUBDIRS LIST
SUBDIRS := 

CFLAGS += -I$(EXTERNAL_DIR)/ipc/include -I$(EXTERNAL_DIR)/include
LDFLAGS += -L$(EXTERNAL_DIR)/ipc/lib/Linux

CPPFLAGS += -DCYGWIN
#CFLAGS +=
#LDFLAGS += -L$(ATACAMA_MEMBERS_DIR)/software/external/lib/Linux-2.4

# add extra files to clean up with 'gmake clean' here
LOCAL_CLEAN_FILES := 

# compile with 'gmake MR_COMM_PROFILING=1 install' to get profiling output
ifneq (,$(MR_COMM_PROFILING))
 CPPFLAGS += -DMR_COMM_PROFILING
endif

ifeq (,$(NO_TELEMETRY_MANAGER))
 CPPFLAGS += -DMR_USE_ATACAMA_TELEMETRY_MANAGER
endif

# install header files (you can use this only once)
INSTALLHEADERS_HEADERS := clientComm.h processStatus.h mrQuickInterface.h
include $(BUILD_DIR)/installheaders.mak

BUILDLIB_TARGET := libmrquick.a
BUILDLIB_SRCS := mrQuickInterface.cc
include $(BUILD_DIR)/buildlib.mak

BUILDLIB_TARGET := libmrclient.a
BUILDLIB_SRCS := clientComm.cc processStatus.cc
include $(BUILD_DIR)/buildlib.mak

# build a binary (you can use this multiple times)
ifneq (,$(STATIC))
BUILDBIN_TARGET := mrterm_static
LDFLAGS += -static
else
BUILDBIN_TARGET := mrterm
endif

BUILDBIN_SRCS := mrterm.cc
BUILDBIN_DEP_LIBS := -lmrclient -lrcl -lmrcommon
BUILDBIN_INDEP_LIBS := -lipc -lpthread -lutil
BUILDBIN_WHOLE_DEP_LIBS :=
BUILDBIN_WHOLE_INDEP_LIBS :=
include $(BUILD_DIR)/buildbin.mak

ifneq (,$(TEST))

# any targets defined here will only be built with 'gmake TEST=1 install'
BUILDBIN_TARGET := testMRQuick
BUILDBIN_SRCS := testMRQuick.cc
BUILDBIN_DEP_LIBS := -lmrquick -lrcl -lmrcommonclean
BUILDBIN_INDEP_LIBS := -lipc -lpthread
BUILDBIN_WHOLE_DEP_LIBS :=
BUILDBIN_WHOLE_INDEP_LIBS :=
include $(BUILD_DIR)/buildbin.mak


endif


######################################################################
# DO NOT MODIFY BELOW THIS POINT

include $(BUILD_DIR)/footer.mak

######################################################################
# $Log: Makefile,v $
# Revision 1.21  2005/06/01 00:47:15  trey
# added testMRQuick
#
# Revision 1.20  2004/12/01 06:59:24  brennan
# Modified CHECKOUT_DIR inference to handle paths with multiple "src"'s.
#
# Revision 1.19  2004/08/21 19:39:56  trey
# added mrQuickInterface
#
# Revision 1.18  2004/06/28 15:46:18  trey
# fixed to properly find ipc.h and libipc.a in new atacama external area
#
# Revision 1.17  2004/04/28 17:15:21  dom
# Added header with copyright information to source and Makefiles.
#
# Revision 1.16  2004/03/05 20:27:35  trey
# added static linking option
#
# Revision 1.15  2003/11/18 23:16:27  trey
# checkout dir inference is now simpler and has better error checking
#
# Revision 1.14  2003/10/29 17:59:35  trey
# removed redundant -L link flag for the external directory
#
# Revision 1.13  2003/10/05 02:56:09  trey
# switched to variant of FIRE makefile system
#
#
