#!gmake 	# Tell emacs about this file: -*- Makefile -*-  
# $Id: installdata.mak,v 1.3 2004/06/25 20:22:32 trey Exp $
#
# PROJECT:      FIRE Architecture Project
# DESCRIPTION:  Include to install a file in the data directory.
#

# Input variables:
#
# INSTALLDATA_FILES
#   Files to install (e.g. foo.config)
# INSTALLDATA_DIR
#   Directory to put files in (e.g. $(HOME))

INSTALLDATA_SRCS := $(INSTALLDATA_FILES:%=$(THIS_DIR)/%)
INSTALLDATA_TARGETS := $(INSTALLDATA_FILES:%=$(INSTALLDATA_DIR)/%)

# the installdata target depends on $(INSTALL_DATA_TARGETS)
INSTALL_DATA_TARGETS += $(INSTALLDATA_TARGETS)

_LT := $(word 1,$(INSTALLDATA_SRCS))
$(_LT)_DIR := $(INSTALLDATA_DIR)

$(INSTALLDATA_TARGETS): $(INSTALLDATA_SRCS)
	@[ -d $($<_DIR) ] || mkdir -p $($<_DIR)
	$(INSTALLDATA_COPY) $? $($<_DIR)

# clear input variables
INSTALLDATA_FILES :=
INSTALLDATA_DIR :=

######################################################################
# $Log: installdata.mak,v $
# Revision 1.3  2004/06/25 20:22:32  trey
# propagated changes from atacama
#
# Revision 1.4  2004/06/25 20:16:20  trey
# fixed problem that cropped up when installing multiple data files
#
# Revision 1.3  2004/04/21 20:54:10  trey
# fixed behavior for multiple data files
#
# Revision 1.2  2004/03/19 18:26:33  trey
# cleaned up some variables
#
# Revision 1.1  2003/10/05 16:51:41  trey
# initial atacama check-in
#
# Revision 1.1  2003/10/05 02:59:35  trey
# initial microraptor check-in
#
# Revision 1.1.1.1  2001/08/27 15:46:51  trey
# initial check-in
#
# Revision 1.4  2000/02/14 20:34:36  trey
# fixed afs-related problems
#
# Revision 1.3  1999/11/11 15:08:23  trey
# now uses diraenv environment variables; using installdata is also streamlined
#
# Revision 1.2  1999/11/08 15:43:01  trey
# added ability to specify subdirectory of software/data to install to
#
# Revision 1.1  1999/10/27 18:41:08  trey
# Makefile system overhaul after talking to Reid
#
# Revision 1.1.1.1  1999/10/27 02:48:58  trey
# Imported sources
#
#
