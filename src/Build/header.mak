#!gmake 	# Tell emacs about this file: -*- Makefile -*-  
# $Id: header.mak,v 1.10 2008/07/29 17:26:16 brennan Exp $
#
# PROJECT:      FIRE Architecture Project
# DESCRIPTION:  Included at the header of a Makefile.  Mostly defines
#               variables, including OS- and site-specific stuff.

######################################################################
# USER HELP

.PHONY: defaultbuildrule
defaultbuildrule: all

# avoid using multiple @echo lines (it keeps invoking the shell. slow...)
.PHONY: help advanced
help:
	@echo -e "\nUse one of the following:" \
		 "\n" \
		 "\ngmake all		   Build all files (default)" \
		 "\ngmake install          Build/install all files" \
		 "\ngmake clean            Remove object, dependency files" \
                 "\ngmake uninstall        Remove installed libs, bins, data files" \
		 "\ngmake realclean        realclean = clean + uninstall" \
		 "\ngmake doc              Generate documentation using doxygen (only at top level)\n" \
		 "\ngmake advanced         Get more usage help" \
		 "\n"

advanced:
	@echo -e "\nAdvanced targets:" \
		 "\n" \
		 "\ngmake foo-local        'gmake foo' without recursion" \
		 "\ngmake <filename>       Builds file according to make rules" \
		 "\ngmake localdoc         Build docs for this dir only in gen/html" \
		 "\n" \
		 "\nVariables:" \
		 "\n" \
		 "\ngmake OS=VxWorks       Cross-compile (not currently set up)" \
		 "\ngmake NO_DEPEND=1      Do not update dependencies" \
		 "\ngmake TEST=1           Many individual makefiles build extra" \
		 "\n                         test binaries if TEST=1" \
		 "\ngmake echo-foo         Echo value of the make variable 'foo'" \
		 "\ngmake vars             Echo value of all make variables" \
		 "\n"

######################################################################
# INITIAL DEFINITIONS

# these variables are declared here so they can be modified
#   by the OS-specific section, and added to later in this file
CFLAGS :=
LDFLAGS :=

######################################################################
# PICK UP OS-SPECIFIC STUFF

# set up default names of binaries and flags for compilers
SRC_DIR := $(CHECKOUT_DIR)/src
BUILD_DIR := $(SRC_DIR)/Build
include $(BUILD_DIR)/Platforms/generic.mak

# Determine what operating system we are trying to build for
# Override this on the command line for cross compilation (e.g. OS=m68kVx5.1)
SHELL = /bin/sh
#OS_SYSNAME := $(shell uname -s)
#OS_RELEASE := $(shell uname -r)

# Assume that we are not building 64 bit binaries.
#ifeq ($(OS_SYSNAME),IRIX64)
#OS_SYSNAME := IRIX
#endif

#ifeq ($(OS_SYSNAME),Linux)
#LINUX_MAJOR_VERSION_CMD := $(PERL) -e '$$_ = shift @ARGV; ($$major,$$minor,$$patchlevel) = split(/\./,$$_,3); print "$$major.$$minor\n";'
## cut off patch level, keep only major and minor OS version numbers.
#OS_RELEASE := $(shell $(LINUX_MAJOR_VERSION_CMD) $(OS_RELEASE))
#endif

CC_VER := $(shell gcc --version)
ifeq ($(CC_VER),2.96)
  CFLAGS += -DGCC2_96
endif

#OS := $(OS_SYSNAME)-$(OS_RELEASE)
## we want to go by the gcc version
#ifeq ($(OS),Linux-2.4)
#  ifeq ($(CC_VER),2.95.3)
#     OS := Linux-2.2
#  endif
#endif

# put detection back in later if needed
OS_SYSNAME := linux
OS_RELEASE := 2
DIST_ID := $(shell if [ -e /etc/redhat-release ] ; then echo 'Redhat'; elif [ -e /etc/lsb-release ] ; then echo 'Ubuntu'; else echo 'Unknown'; fi)

ifeq (Redhat,$(DIST_ID))
# If we're Redhat 7.1, substitute for OS_RELEASE; we need to
# differentiate from 9, and both 9 and 7.1 have 2.4 kernels.
# Whee-ha!
REDHAT_RELEASE_VERSION = $(shell if [ -e /etc/redhat-release ] ; then cat /etc/redhat-release | cut --delimiter=" " -f5 ; else echo 'NonRedhat'; fi)
ifeq ($(REDHAT_RELEASE_VERSION),7.1)
OS_RELEASE := rh71
endif
ifeq ($(REDHAT_RELEASE_VERSION),6.2)
OS_RELEASE := rh62
endif

else # Not Redhat
# Come up with a nifty moniker for Ubuntu builds.
ifeq (Ubuntu,$(DIST_ID))
UBUNTU_RELEASE_VERSION = $(shell cat /etc/lsb-release | grep -E "ID|CODE" | cut -f2 --delimiter='=' | tr '[A-Z]' '[a-z]' | cut -b1-2 | tr -d '\n')
ifeq ($(UBUNTU_RELEASE_VERSION),ubda) # Ubuntu Dapper Drake
OS_RELEASE := ubDa
endif
ifeq ($(UBUNTU_RELEASE_VERSION),ubha) # Ubuntu Hardy Heron
OS_RELEASE := ubHa
endif
endif # End Ubuntu
endif # End Not Redhat

OS := $(OS_SYSNAME)$(OS_RELEASE)
include $(BUILD_DIR)/Platforms/$(OS).mak

######################################################################
# PICK UP USER-SPECIFIC CONFIGURATION

PROJECT := microraptor

BUILDRC_FILE := $(HOME)/.$(PROJECT).buildrc
ifeq (,$(shell ls $(BUILDRC_FILE) 2> /dev/null))
  # use this as a fall-back if there is no buildrc in $HOME
  BUILDRC_FILE := $(BUILD_DIR)/example-$(PROJECT)-buildrc.mak
endif
# use '-include' instead of 'include' so we silently fail if the file
# doesn't exist
-include $(BUILDRC_FILE)

######################################################################
# DIRECTORY DEFINES

THIS_DIR := $(shell pwd)

OBJ_DIR := obj

LOCAL_INC_DIR := include
LOCAL_LIB_DIR := lib
LOCAL_BIN_DIR := bin

TARGET_DIR := $(CHECKOUT_DIR)
TARGET_INC_DIR := $(TARGET_DIR)/include
TARGET_LIB_DIR := $(TARGET_DIR)/lib
TARGET_BIN_DIR := $(TARGET_DIR)/bin
TARGET_COMMON_BIN_DIR := $(TARGET_DIR)/bin/common
TARGET_PYTHON_DIR := $(TARGET_DIR)/lib/python
TARGET_JAVA_DIR := $(TARGET_DIR)/java
TARGET_CLEAN_DIRS := $(TARGET_DIR)/bin $(TARGET_DIR)/lib \
   $(TARGET_DATA_DIR) $(TARGET_INC_DIR)

ifndef EXTERNAL_DIR
 EXTERNAL_DIR := $(CHECKOUT_DIR)/external
endif
EXTERNAL_LIB_DIR := $(EXTERNAL_DIR)/lib
EXTERNAL_BIN_DIR := $(EXTERNAL_DIR)/bin
EXTERNAL_INC_DIR := $(EXTERNAL_DIR)/include

ifdef RPM_BUILD_IN_PROGRESS
  CFLAGS += -I$(CHECKOUT_DIR)/../../../depSpace/usr/include/ipc -I/usr/include/ipc
  LDFLAGS += -L$(CHECKOUT_DIR)/../../../depSpace/usr/lib -I/usr/lib
#  CFLAGS += -I$(MRAPTOR_BUILD_ROOT)/depSpace/usr/include/ipc
#  LDFLAGS += -L$(MRAPTOR_BUILD_ROOT)/depSpace/usr/lib
endif
LDFLAGS += -L$(LOCAL_LIB_DIR) -L$(TARGET_LIB_DIR) -L$(EXTERNAL_LIB_DIR)
CFLAGS += -I. -I$(LOCAL_INC_DIR) -I$(TARGET_INC_DIR) -I$(EXTERNAL_INC_DIR)

ifdef DEBUG_DEFUNCT
  CFLAGS += -DDEBUG_DEFUNCT=1
endif

VPATH := .:$(LOCAL_INC_DIR)

######################################################################
# INITIALIZE SOME RULES/VARIABLES THAT WILL BE ADDED TO LATER

BUILDLIB_TARGETS :=
BUILDLIB_LOCALS :=
BUILDBIN_TARGETS :=
BUILDBIN_LOCALS :=
INSTALL_DATA_TARGETS :=
INSTALLHEADERS_TARGETS :=
XDRGEN_TARGETS :=
BUILDBIN_SYMLINKS_TO_CLEAN :=

######################################################################
# DEPEND RULES

# i forget why, but these rules can't go in footer.mak

# need to add this rule explicitly in order to correctly infer that
# a tdl file with the line #include "foo.tdl.h" means that we need to
# generate foo.tdl.h.
$(OBJ_DIR)/%.tdl.d: $(LOCAL_INC_DIR)/%.tdl.cc
	@echo "Updating dependencies for $<..."
	@[ -d $(OBJ_DIR)/$(LOCAL_INC_DIR) ] || mkdir -p $(OBJ_DIR)/$(LOCAL_INC_DIR)
	@$(SHELL) -ec '$(MAKEDEP) $(CFLAGS) $< \
		| $(SED) '\''s|\($(*:%=%.tdl)\)\.o[ :]*|$(OBJ_DIR)/\1.o $@ : |'\'' > $@; \
		[ -s $@ ] || rm -f $@'

$(OBJ_DIR)/%.d: %.c
	@echo "Updating dependencies for $<..."
	@[ -d $(OBJ_DIR)/$(LOCAL_INC_DIR) ] || mkdir -p $(OBJ_DIR)/$(LOCAL_INC_DIR)
	@$(SHELL) -ec '$(MAKEDEP) $(CFLAGS) $< \
		| $(SED) '\''s|\($*\)\.o[ :]*|$(OBJ_DIR)/\1.o $@ : |'\'' > $@; \
		[ -s $@ ] || rm -f $@'

$(OBJ_DIR)/%.d: %.cc
	@echo "Updating dependencies for $<..."
	@[ -d $(OBJ_DIR)/$(LOCAL_INC_DIR) ] || mkdir -p $(OBJ_DIR)/$(LOCAL_INC_DIR)
	@$(SHELL) -ec '$(MAKEDEP) $(CFLAGS) $< \
		| $(SED) '\''s|\($*\)\.o[ :]*|$(OBJ_DIR)/\1.o $@ : |'\'' > $@; \
		[ -s $@ ] || rm -f $@'

$(OBJ_DIR)/%.d: %.cpp
	@echo "Updating dependencies for $<..."
	@[ -d $(OBJ_DIR)/$(LOCAL_INC_DIR) ] || mkdir -p $(OBJ_DIR)/$(LOCAL_INC_DIR)
	@$(SHELL) -ec '$(MAKEDEP) $(CFLAGS) $< \
		| $(SED) '\''s|\($*\)\.o[ :]*|$(OBJ_DIR)/\1.o $@ : |'\'' > $@; \
		[ -s $@ ] || rm -f $@'

######################################################################
# $Log: header.mak,v $
# Revision 1.10  2008/07/29 17:26:16  brennan
# Changes to Microraptor to support releasing on Ubuntu Hardy Heron.
#
# Revision 1.9  2006/09/29 03:43:05  brennan
# Added build support for Ubuntu Dapper Drake with gcc-3.3
#
# Revision 1.8  2006/06/06 19:31:19  brennan
# Moved to subpackaging the RPMs, extended the release script and header.mak
# to properly support automated release building.
#
# Revision 1.7  2006/06/04 05:27:03  brennan
# First pass at modularized RPMs, and changes to build under gcc 2.96.
#
# Revision 1.6  2004/12/15 08:22:27  brennan
# Moved DEBUG_DEFUNCT to header.mak
#
# Revision 1.5  2004/12/01 06:59:24  brennan
# Modified CHECKOUT_DIR inference to handle paths with multiple "src"'s.
#
# Revision 1.4  2004/05/21 19:16:59  trey
# branched example-buildrc.mak into two files, one for atacama and one for microraptor; added COMPILER_DIRECTORY_WITH_SLASH configuration variable
#
# Revision 1.3  2004/05/13 20:52:50  trey
# fixed so that a build works even if you do not have a buildrc in $HOME
#
# Revision 1.2  2004/05/13 20:25:50  trey
# synced up with atacama makefile system again
#
# Revision 1.6  2004/03/19 18:26:33  trey
# cleaned up some variables
#
# Revision 1.5  2004/03/19 16:23:07  trey
# fixed some bugs i found with dom
#
# Revision 1.4  2004/03/19 14:59:40  trey
# changed buildrc install location
#
# Revision 1.3  2004/03/19 14:50:40  trey
# major changes to support "gmake all" and other features atacama folks want
#
# Revision 1.2  2003/10/07 16:01:17  trey
# changed directory names to reflect move
#
# Revision 1.1  2003/10/05 16:51:41  trey
# initial atacama check-in
#
# Revision 1.1  2003/10/05 02:59:35  trey
# initial microraptor check-in
#
# Revision 1.11  2003/04/25 18:42:14  danig
# Modified to allow compilation on a system with linux kernel version
# 2.4 and gcc 2.95.3.
#
# Revision 1.10  2003/02/17 22:57:55  trey
# got rid of obsolete GRP setting
#
# Revision 1.9  2003/02/17 20:54:40  trey
# added a variable used for python/swig compilation
#
# Revision 1.8  2002/04/25 22:13:40  trey
# added java rules; removed xdrgen target now that the system can auto-detect when xdrgen needs to be run
#
# Revision 1.7  2002/04/25 01:37:17  trey
# turned back on suppression of echoing dependency generation commands
#
# Revision 1.6  2002/04/25 01:24:56  trey
# fixed bug that *.tdl.h files were not being automatically generated if another *.tdl file tried to include them
#
# Revision 1.5  2002/02/12 15:49:59  trey
# fixed *.c++ to be *.cpp
#
# Revision 1.4  2002/02/12 15:43:28  trey
# added a *.c++ dependency generation rule
#
# Revision 1.3  2001/08/27 19:12:41  trey
# fixed SRC_DIR problem
#
# Revision 1.2  2001/08/27 18:01:06  trey
# Makefile
#
# Revision 1.21  2001/03/21 19:25:02  trey
# fixed so that we have corresponding dependency-generating rules for .c and .cc files
#
# Revision 1.20  2001/03/06 17:48:22  trey
# set up auto-generation of hierarchical directory of docs
#
# Revision 1.19  2001/03/06 04:01:39  trey
# added rules for documentation generation, gmake doc
#
# Revision 1.18  2001/02/13 17:09:46  trey
# added to advanced help output
#
# Revision 1.17  2001/02/13 16:33:35  trey
# switched include of headers under src to use "dir/foo.h" instead of <dir/foo.h>; dependency checking is now easier. made some corresponding makefile changes
#
# Revision 1.16  2001/02/07 20:35:36  trey
# minor Build system updates to support cleaning up after xdrgen
#
# Revision 1.15  2001/02/06 02:03:12  trey
# added rules for generating header files from XDR specs
#
# Revision 1.14  2000/10/01 00:44:02  trey
# added separate CPPFLAGS variable for things that also need to be passed to makedepend; fixed help target to display properly under linux
#
# Revision 1.13  2000/09/30 23:25:37  trey
# modified makefile system to put generated source and header files in the gen/ directory
#
# Revision 1.12  2000/09/20 22:37:30  trey
# added . to the include path and moved generic.mak up in the file so that $(PERL) gets defined before we need it
#
# Revision 1.11  2000/09/20 18:27:07  trey
# fixed to avoid putting another file in Build just for the linux-major-version issue
#
# Revision 1.10  2000/09/17 19:26:41  hersh
# Changed OS_RELEASE to be only major and minor number, no patch numbers.
#
# Revision 1.9  2000/08/28 16:48:46  trey
# fixed help msg
#
# Revision 1.8  2000/02/25 22:11:35  trey
# added support for scripts and Logs directory
#
# Revision 1.7  2000/02/14 20:34:36  trey
# fixed afs-related problems
#
# Revision 1.6  2000/02/02 18:35:29  trey
# fixed unfortunate tendency to remake dependencies on gmake clean, some other minor fixes
#
# Revision 1.5  1999/11/11 15:08:23  trey
# now uses diraenv environment variables; using installdata is also streamlined
#
# Revision 1.4  1999/11/08 15:42:31  trey
# updated to reflect use of diraenv file
#
# Revision 1.3  1999/11/03 20:51:44  trey
# added support for flex/bison, tweaked other stuff
#
# Revision 1.2  1999/10/27 18:41:04  trey
# Makefile system overhaul after talking to Reid
#
# Revision 1.1.1.1  1999/10/27 02:48:58  trey
# Imported sources
#
#
