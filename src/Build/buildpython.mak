#!gmake 	# Tell emacs about this file: -*- Makefile -*-  
# $Id: buildpython.mak,v 1.3 2005/08/27 23:35:40 trey Exp $
#
# PROJECT:      FIRE Architecture Project
# DESCRIPTION:  Include to build a C++ extension to python using SWIG.
#

# Input variables:
#
# BUILDPYTHON_MODULE
#   Python module to produce (e.g. cmu_ipc)
# BUILDPYTHON_HEADER
#   Header to pass to swig (e.g. cmu_ipc.i)
# BUILDPYTHON_SRCS
#   Source files to link against (e.g. cmu_ipc.cc)
# BUILDPYTHON_DEP_LIBS
#    Libraries to link against: specifically, ones that we should
#    include in dependency checking in case they are rebuilt
#    (e.g. -lctr)
# BUILDPYTHON_INDEP_LIBS
#    Libraries to link against: external libraries that are unlikely
#    to be rebuilt, so skip dependency checking (e.g. -lipc)

ifndef BUILDPYTHON_INCLUDED

# rules inside this ifndef are defined only once, whether or not
# buildpython.mak is included multiple times

$(TARGET_LIB_DIR)/%.py: $(LOCAL_INC_DIR)/%.py
	$(CP) $< $@

$(TARGET_LIB_DIR)/_%.so: $(TARGET_LIB_DIR)/_%.so
	$(LN) $*cmodule.so $@

PYVER := $(shell python -V 2>&1 | perl -ple 's/Python (\d+\.\d+)(\.\d+)?/\1/;')

# SWIG sometimes auto-generates functions that are not used, skip the warning
CFLAGS += -I/usr/include/python$(PYVER) -I$(SWIG_LIB_DIR) -Wno-unused

# filter out compiler flags that SWIG doesn't understand
SWIG_CANT_TAKE_THESE_FLAGS := -Wall -Wno-unused -pipe -g -O2 -O3 -pg
SWIG_CFLAGS := $(filter-out $(SWIG_CANT_TAKE_THESE_FLAGS),$(CFLAGS))

BUILDPYTHON_INCLUDED := true

endif # BUILDPYTHON_INCLUDED

# process the input variables
#BUILDPYTHON_ALT_LIB := $(TARGET_LIB_DIR)/libpython_$(BUILDPYTHON_MODULE).so
BUILDPYTHON_TARGET_LIB := $(TARGET_LIB_DIR)/_$(BUILDPYTHON_MODULE).so
BUILDPYTHON_TARGET_PY := $(TARGET_LIB_DIR)/$(BUILDPYTHON_MODULE).py
BUILDPYTHON_TARGET_SWIG := $(TARGET_LIB_DIR)/swig.py
BUILDPYTHON_DEP_LIB_FILES := \
  $(shell $(PERL) $(BUILD_DIR)/findlibs.perl $(LDFLAGS) $(BUILDPYTHON_DEP_LIBS))

BUILDPYTHON_OBJS := $(BUILDPYTHON_SRCS)
BUILDPYTHON_OBJS := $(BUILDPYTHON_OBJS:%.c=$(OBJ_DIR)/%.o)
BUILDPYTHON_OBJS := $(BUILDPYTHON_OBJS:%.cc=$(OBJ_DIR)/%.o)
BUILDPYTHON_OBJS := $(BUILDPYTHON_OBJS:%.cpp=$(OBJ_DIR)/%.o)

_WR := $(LOCAL_INC_DIR)/$(BUILDPYTHON_MODULE)_wrap.cc
$(_WR)_MODULE := $(BUILDPYTHON_MODULE)

$(_WR) $(LOCAL_INC_DIR)/$(BUILDPYTHON_MODULE).py: $(BUILDPYTHON_HEADER)
	@[ -d $(LOCAL_INC_DIR) ] || mkdir -p $(LOCAL_INC_DIR)
	cd $(LOCAL_INC_DIR) && $(SWIG) $(SWIG_FLAGS) $(SWIG_CFLAGS) -I.. -module $($@_MODULE) -o $($@_MODULE)_wrap.cc ../$<

_BT := $(BUILDPYTHON_TARGET_LIB)
$(_BT)_OBJS := $(BUILDPYTHON_OBJS)
$(_BT)_DEP_LIBS := $(BUILDPYTHON_DEP_LIBS)
$(_BT)_INDEP_LIBS := $(BUILDPYTHON_INDEP_LIBS)

$(_BT): $(OBJ_DIR)/$(BUILDPYTHON_MODULE)_wrap.o $(BUILDPYTHON_OBJS) $(BUILDPYTHON_DEP_LIB_FILES)
	$(LD) $(LDFLAGS) -shared $< $($@_OBJS) $($@_DEP_LIBS) $($@_INDEP_LIBS) -o $@.tmp
	mv -f $@.tmp $@

# add to the same list of targets as buildlib.mak; these targets will be
#   built during the libinstall phase of 'gmake install'
INSTALLHEADERS_TARGETS += \
  $(LOCAL_INC_DIR)/$(BUILDPYTHON_MODULE)_wrap.cc \
  $(LOCAL_INC_DIR)/$(BUILDPYTHON_MODULE).py

# add to the same list of targets as buildbin.mak; these targets will be
#   build during the bininstall phase of 'gmake install'
BUILDBIN_TARGETS += \
  $(BUILDPYTHON_TARGET_LIB) \
  $(BUILDPYTHON_TARGET_PY) \
#  $(BUILDPYTHON_ALT_LIB)

# clear input variables
BUILDPYTHON_MODULE :=
BUILDPYTHON_HEADER :=
BUILDPYTHON_SRCS :=
BUILDPYTHON_DEP_LIBS :=
BUILDPYTHON_INDEP_LIBS :=

######################################################################
# $Log: buildpython.mak,v $
# Revision 1.3  2005/08/27 23:35:40  trey
# now infer python version instead of hard-coding 2.2
#
# Revision 1.2  2004/07/06 15:10:59  trey
# fixed problem which required running "make install" twice
#
# Revision 1.1  2004/05/13 20:25:50  trey
# synced up with atacama makefile system again
#
# Revision 1.2  2004/03/19 18:26:33  trey
# cleaned up some variables
#
# Revision 1.1  2003/12/01 04:01:01  trey
# initial check-in
#
#
