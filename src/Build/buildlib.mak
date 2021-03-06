#!gmake 	# Tell emacs about this file: -*- Makefile -*-  
# $Id: buildlib.mak,v 1.3 2004/05/13 20:25:50 trey Exp $
#
# PROJECT:      FIRE Architecture Project
# DESCRIPTION:  Used to build a library.
#
# You can include this sub-makefile makefile multiple times.

# Input variables:
#
# BUILDLIB_TARGET
#    The name of the library to build (e.g. libctr.a)
# BUILDLIB_SRCS
#    Source files to link into library (e.g. ctr.cc)

include $(BUILD_DIR)/fixvars.mak # backwards compatibility

ifneq (,$(SKIP_LOCALS))
  BUILDLIB_LIB_DIR := $(TARGET_LIB_DIR)
else
  BUILDLIB_LIB_DIR := $(LOCAL_LIB_DIR)
endif

_LT := $(BUILDLIB_LIB_DIR)/$(BUILDLIB_TARGET)

BUILDLIB_OBJS := $(BUILDLIB_SRCS)
BUILDLIB_OBJS := $(BUILDLIB_OBJS:%.c=$(OBJ_DIR)/%.o)
BUILDLIB_OBJS := $(BUILDLIB_OBJS:%.cc=$(OBJ_DIR)/%.o)
BUILDLIB_OBJS := $(BUILDLIB_OBJS:%.cpp=$(OBJ_DIR)/%.o)

$(_LT)_OBJS := $(BUILDLIB_OBJS)

$(_LT): $($(_LT)_OBJS) Makefile
	@[ -d $(BUILDLIB_LIB_DIR) ] || mkdir -p $(BUILDLIB_LIB_DIR)
	$(AR) $(ARFLAGS) $@.tmp $($@_OBJS)
ifneq ($(RANLIB),)
	$(RANLIB) $@.tmp
endif
	mv -f $@.tmp $@

BUILDLIB_DEPS := $(BUILDLIB_OBJS:$(OBJ_DIR)/%.o=$(OBJ_DIR)/%.d)
BUILDLIB_TARGETS += $(TARGET_LIB_DIR)/$(BUILDLIB_TARGET)


ifeq (,$(SKIP_LOCALS))

BUILDLIB_LOCALS += $(_LT)

$(TARGET_LIB_DIR)/$(BUILDLIB_TARGET): $(_LT)
	@[ -d $(TARGET_LIB_DIR) ] || mkdir -p $(TARGET_LIB_DIR)
	$(INSTALLLIBS_COPY) $< $@

endif # not SKIP_LOCALS

ifdef DEPEND
ifneq ($(BUILDLIB_DEPS),)
-include $(BUILDLIB_DEPS)
endif
endif

# clear input variables
BUILDLIB_TARGET :=
BUILDLIB_SRCS :=

######################################################################
# $Log: buildlib.mak,v $
# Revision 1.3  2004/05/13 20:25:50  trey
# synced up with atacama makefile system again
#
# Revision 1.5  2004/04/19 17:53:08  trey
# fixed dependency problem
#
# Revision 1.4  2004/03/19 18:26:33  trey
# cleaned up some variables
#
# Revision 1.3  2004/03/19 16:23:07  trey
# fixed some bugs i found with dom
#
# Revision 1.2  2004/03/19 14:50:40  trey
# major changes to support "gmake all" and other features atacama folks want
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
# Revision 1.6  2000/09/30 23:25:37  trey
# modified makefile system to put generated source and header files in the gen/ directory
#
# Revision 1.5  2000/02/14 20:34:35  trey
# fixed afs-related problems
#
# Revision 1.4  2000/02/02 18:35:28  trey
# fixed unfortunate tendency to remake dependencies on gmake clean, some other minor fixes
#
# Revision 1.3  1999/11/03 20:51:43  trey
# added support for flex/bison, tweaked other stuff
#
# Revision 1.2  1999/10/27 18:40:57  trey
# Makefile system overhaul after talking to Reid
#
# Revision 1.1.1.1  1999/10/27 02:48:58  trey
# Imported sources
#
#
