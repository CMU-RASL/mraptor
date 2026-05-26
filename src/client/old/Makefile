# COPYRIGHT 2004, Carnegie Mellon University
#!gmake 	# Tell emacs about this file: -*- Makefile -*-  
# $Id: Makefile,v 1.28 2007/04/18 17:13:28 brennan Exp $
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
CFLAGS += -Wno-deprecated -Wno-deprecated-declarations -Wno-unused-result
LDFLAGS += -L$(EXTERNAL_DIR)/ipc/lib

CFLAGS += -g

# Avoid having to rename all the .c's to .cc's
CC = $(C++)

# Comment out to use ClawText instead of ZVT terminal widget
#USE_ZVT_TERM = 0

# Attempt to control which version of GTK we're using
GTK_VERSION = 2.0

# build a binary (you can use this multiple times)
ifneq (,$(STATIC))
BUILDBIN_TARGET := claw_static
LDFLAGS += -static
else
BUILDBIN_TARGET := claw
endif
BUILDBIN_SRCS := claw.c callbacks.c interface.c processWidget.c support.c clientIO.c ClawText.cc imageHeaders.c
BUILDBIN_DEP_LIBS := -lrcl -lmrclient -lmrcommon
#BUILDBIN_INDEP_LIBS := -lipc -lpthread -lzvt -lutil -lgdk_imlib
BUILDBIN_INDEP_LIBS := -lipc -lpthread -lutil -lX11 -lXaw
BUILDBIN_INDEP_LIBS += -lXmu $(shell pkg-config --libs gtk+-$(GTK_VERSION))
BUILDBIN_WHOLE_DEP_LIBS :=
BUILDBIN_WHOLE_INDEP_LIBS :=
include $(BUILD_DIR)/buildbin.mak

# for gtk
CFLAGS += $(shell pkg-config --cflags gtk+-$(GTK_VERSION))

ifdef USE_ZVT_TERM
CFLAGS += -DUSE_ZVT_TERM -I/usr/include/gnome-1.0
endif

######################################################################
# DO NOT MODIFY BELOW THIS POINT

include $(BUILD_DIR)/footer.mak

######################################################################
# $Log: Makefile,v $
# Revision 1.28  2007/04/18 17:13:28  brennan
# Fixed occasional crash in claw.
#
# Revision 1.27  2004/12/01 06:59:24  brennan
# Modified CHECKOUT_DIR inference to handle paths with multiple "src"'s.
#
# Revision 1.26  2004/08/12 13:27:59  trey
# fixed compile/link flags
#
# Revision 1.25  2004/06/28 15:46:18  trey
# fixed to properly find ipc.h and libipc.a in new atacama external area
#
# Revision 1.24  2004/04/28 17:15:20  dom
# Added header with copyright information to source and Makefiles.
#
# Revision 1.23  2004/03/10 16:12:46  trey
# fixed stupid binary name error in Makefile
#
# Revision 1.22  2004/03/05 23:24:26  trey
# finally fixed Makefile so that static compilation works
#
# Revision 1.21  2004/03/05 20:25:36  trey
# fixed weird missing login_tty function during static linking
#
# Revision 1.20  2004/03/05 20:06:03  trey
# added option to statically link
#
# Revision 1.19  2003/11/18 23:16:27  trey
# checkout dir inference is now simpler and has better error checking
#
# Revision 1.18  2003/11/15 17:53:39  brennan
# Split up CYGWIN #define into more reasonable/understandable chunks.  Completed move to FIRE build system.
#
# Revision 1.17  2003/10/29 17:59:41  trey
# removed redundant -L link flag for the external directory
#
# Revision 1.16  2003/10/05 02:55:36  trey
# switched to variant of FIRE makefile system
#
#
