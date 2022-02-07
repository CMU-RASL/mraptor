# COPYRIGHT 2004, Carnegie Mellon University
#!gmake 	# Tell emacs about this file: -*- Makefile -*-  
# $Id: Makefile,v 1.17 2005/06/24 17:58:06 trey Exp $
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

ATACAMA_MEMBERS_DIR := $(CHECKOUT_DIR)/mraptor/
ifndef EXTERNAL_DIR
  EXTERNAL_DIR := $(CHECKOUT_DIR)/../external
endif

# ADD ANY SUBDIRS THAT NEED TO BE VISITED DURING A BUILD TO THE SUBDIRS LIST
SUBDIRS :=

CFLAGS += -I$(EXTERNAL_DIR)/ipc/include -I$(EXTERNAL_DIR)/include
LDFLAGS += -L$(EXTERNAL_DIR)/ipc/lib/Linux

CLEAN_SRCS := mrIPCFormatPrimTypes.cc mrFSignal.cc mrFSlot.cc mrIPCMessage.cc \
    mrSafeSystem.cc mrCommonTime.cc mrIPCHelper.cc

BUILDLIB_TARGET := libmrcommon.a
BUILDLIB_SRCS := $(CLEAN_SRCS) mrSafeSystemUtil.cc
include $(BUILD_DIR)/buildlib.mak

# the clean version does not depend on -lutil, which seems to cause problems
# in some cases.  some functions are missing
BUILDLIB_TARGET := libmrcommonclean.a
BUILDLIB_SRCS := $(CLEAN_SRCS)
include $(BUILD_DIR)/buildlib.mak

INSTALLHEADERS_HEADERS := mrSmartRef.h mrException.h mrIPCFormatPrimTypes.h mrFSignal.h \
    mrFSlot.h mrIPCMessage.h mrSafeSystem.h mrHashMap.h \
    mrCommonDefs.h mrCommonTime.h mrIPCHelper.h mrSets.h
include $(BUILD_DIR)/installheaders.mak

######################################################################
# DO NOT MODIFY BELOW THIS POINT

include $(BUILD_DIR)/footer.mak

######################################################################
# $Log: Makefile,v $
# Revision 1.17  2005/06/24 17:58:06  trey
# added mrSets.h
#
# Revision 1.16  2004/12/15 08:22:51  brennan
# Moved DEBUG_DEFUNCT to header.mak
#
# Revision 1.15  2004/12/01 06:59:24  brennan
# Modified CHECKOUT_DIR inference to handle paths with multiple "src"'s.
#
# Revision 1.14  2004/06/28 15:46:18  trey
# fixed to properly find ipc.h and libipc.a in new atacama external area
#
# Revision 1.13  2004/06/10 20:55:12  trey
# added build of libmrcommonclean.a that does not depend on -lutil
#
# Revision 1.12  2004/04/28 17:15:21  dom
# Added header with copyright information to source and Makefiles.
#
# Revision 1.11  2003/11/18 23:16:27  trey
# checkout dir inference is now simpler and has better error checking
#
# Revision 1.10  2003/11/15 17:53:40  brennan
# Split up CYGWIN #define into more reasonable/understandable chunks.  Completed move to FIRE build system.
#
# Revision 1.17  2003/10/29 17:59:41  trey
# removed redundant -L link flag for the external directory
#
# Revision 1.16  2003/10/05 02:55:36  trey
# switched to variant of FIRE makefile system
#
#
