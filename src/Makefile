# COPYRIGHT 2004, Carnegie Mellon University
#!gmake 	# Tell emacs about this file: -*- Makefile -*-  
# $Id: Makefile,v 1.14 2004/12/02 18:26:12 brennan Exp $
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

# ADD ANY SUBDIRS THAT NEED TO BE VISITED DURING A BUILD TO THE SUBDIRS LIST
ifdef JUST_RCL
  SUBDIRS = common rcl
else
  ifdef NO_CLAW
    SUBDIRS = common rcl daemon ttyclient
  else
    SUBDIRS = common rcl daemon ttyclient client
  endif
endif

ifdef BUILD_UTILS
  SUBDIRS += utils
endif

noclaw:
	$(MAKE) $(MFLAGS) NO_CLAW=1 install

justrcl:
	$(MAKE) $(MFLAGS) JUST_RCL=1 install

include $(BUILD_DIR)/toplevel.mak

######################################################################
# DO NOT MODIFY BELOW THIS POINT

include $(BUILD_DIR)/footer.mak

######################################################################
# $Log: Makefile,v $
# Revision 1.14  2004/12/02 18:26:12  brennan
# Fixed mrmonitor to compile with the new RCL syntax, and added a
# BUILD_UTILS make option to build it.  mrmonitor is currently only used
# by the Valerie project.
#
# Revision 1.13  2004/12/01 06:59:24  brennan
# Modified CHECKOUT_DIR inference to handle paths with multiple "src"'s.
#
# Revision 1.12  2004/08/17 03:05:30  trey
# added limited build targets
#
# Revision 1.11  2004/04/28 17:15:20  dom
# Added header with copyright information to source and Makefiles.
#
# Revision 1.10  2003/11/18 23:16:27  trey
# checkout dir inference is now simpler and has better error checking
#
# Revision 1.9  2003/10/15 20:16:57  brennan
# Committing changes made during install on seamonkey.
#
# Revision 1.8  2003/10/05 02:56:47  trey
# switched to variant of FIRE makefile system
#
#
