# COPYRIGHT 2004, Carnegie Mellon University
#!gmake 	# Tell emacs about this file: -*- Makefile -*-  
# $Id: Makefile,v 1.21 2004/12/01 06:59:24 brennan Exp $
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

# ADD ANY SUBDIRS THAT NEED TO BE VISITED DURING A BUILD TO THE SUBDIRS LIST
SUBDIRS :=

ifneq (,$(USE_MEM_TRACKER))
  CFLAGS += -DUSE_MEM_TRACKER
  EXTRA_LIBS += -lcommon
endif

CFLAGS += -I$(EXTERNAL_DIR)/ipc/include -I$(EXTERNAL_DIR)/include \
	  -I$(EXTERNAL_DIR)/expat/include
LDFLAGS += -L$(EXTERNAL_DIR)/ipc/lib/linux2 -L$(EXTERNAL_DIR)/lib/Linux-2.4

# this prefix setting overrides the default specified in
# Platforms/generic.mak (which is 'yy').  here we set it to 'rclyy' to
# avoid colliding with other libraries that use the default prefix (such
# as the EUROPA planner).
LEX_PREFIX := rclyy

BUILDLIB_TARGET := librcl.a
BUILDLIB_SRCS := RCLParser.cc RCLTypes.cc RCLExp.cc RCLMap.cc RCLVector.cc \
   RCLConfigFile.cc RCLGrammar.tab.cc RCLGrammar.rclyy.cc strxcpy.cc \
   RCLDebugInfo.cc RCLException.cc
include $(BUILD_DIR)/buildlib.mak

INSTALLHEADERS_HEADERS := \
    RCLParser.h RCL.h RCLTypes.h RCLException.h RCLExp.h RCLMap.h RCLVector.h \
    RCLConfigFile.h RCLDebugInfo.h RCLObject.h RCLXML.h
include $(BUILD_DIR)/installheaders.mak

ifneq (,$(TEST))

BUILDBIN_TARGET := testRCL
BUILDBIN_SRCS := testRCL.cc
BUILDBIN_DEP_LIBS := -lrcl -lmrcommonclean $(EXTRA_LIBS)
BUILDBIN_INDEP_LIBS :=
include $(BUILD_DIR)/buildbin.mak

BUILDBIN_TARGET := testRCLObject
BUILDBIN_SRCS := testRCLObject.cc
BUILDBIN_DEP_LIBS := -lrcl -lmrcommonclean $(EXTRA_LIBS)
BUILDBIN_INDEP_LIBS :=
include $(BUILD_DIR)/buildbin.mak

endif

ifeq (,$(NO_XML))

BUILDLIB_TARGET := librclxml.a
BUILDLIB_SRCS := RCLXML.cc
include $(BUILD_DIR)/buildlib.mak

BUILDBIN_TARGET := rclTranslate
BUILDBIN_SRCS := rclTranslate.cc
BUILDBIN_DEP_LIBS := -lrclxml -lrcl -lmrcommonclean $(EXTRA_LIBS)
BUILDBIN_INDEP_LIBS := -lexpat
include $(BUILD_DIR)/buildbin.mak

ifneq (,$(TEST))

BUILDBIN_TARGET := testRCLXML
BUILDBIN_SRCS := testRCLXML.cc
BUILDBIN_DEP_LIBS := -lrclxml -lrcl
BUILDBIN_INDEP_LIBS := -lexpat
include $(BUILD_DIR)/buildbin.mak

endif # TEST

endif # RCLXML

######################################################################
# DO NOT MODIFY BELOW THIS POINT

include $(BUILD_DIR)/footer.mak

######################################################################
# $Log: Makefile,v $
# Revision 1.21  2004/12/01 06:59:24  brennan
# Modified CHECKOUT_DIR inference to handle paths with multiple "src"'s.
#
# Revision 1.20  2004/08/04 20:37:48  reids
# Added a '-I' flag for expat
#
# Revision 1.19  2004/08/02 03:36:24  trey
# moved eslTranslate to rcl directory and renamed it to rclTranslate
#
# Revision 1.18  2004/08/02 02:54:25  trey
# turned on expat compilation by default
#
# Revision 1.17  2004/07/06 15:35:12  trey
# added optional memory tracking and plugged some memory leaks
#
# Revision 1.16  2004/06/28 15:46:18  trey
# fixed to properly find ipc.h and libipc.a in new atacama external area
#
# Revision 1.15  2004/06/10 22:46:43  trey
# changed default behavior for RCL so that it prints an error before throwing an exception; error handling is now configurable using setErrorHandler()
#
# Revision 1.14  2004/06/10 20:55:49  trey
# switched to use libmrcommonclean.a that does not depend on -lutil
#
# Revision 1.13  2004/06/09 22:50:08  trey
# made it possible to switch the name prefix from the default "yy" when running flex
#
# Revision 1.12  2004/05/12 19:26:20  trey
# added RCLObject.h to installed headers
#
# Revision 1.11  2004/05/04 17:45:27  trey
# added build for testRCLObjectc
#
# Revision 1.10  2004/04/28 17:15:21  dom
# Added header with copyright information to source and Makefiles.
#
# Revision 1.9  2004/04/09 20:31:33  trey
# major enhancement to rcl interface
#
# Revision 1.8  2004/04/09 00:48:34  trey
# made changes to support XML front end
#
# Revision 1.7  2004/04/08 16:42:56  trey
# overhauled RCLExp
#
# Revision 1.6  2003/11/18 23:16:27  trey
# checkout dir inference is now simpler and has better error checking
#
# Revision 1.5  2003/11/15 17:53:40  brennan
# Split up CYGWIN #define into more reasonable/understandable chunks.  Completed move to FIRE build system.
#
# Revision 1.17  2003/10/29 17:59:41  trey
# removed redundant -L link flag for the external directory
#
# Revision 1.16  2003/10/05 02:55:36  trey
# switched to variant of FIRE makefile system
#
#
