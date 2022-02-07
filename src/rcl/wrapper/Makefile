# COPYRIGHT 2004, Carnegie Mellon University
#!gmake 	# Tell emacs about this file: -*- Makefile -*-  
# $Id: Makefile,v 1.4 2004/12/01 06:59:24 brennan Exp $
#
# PROJECT:      Life in the Atacama
# DESCRIPTION:  

######################################################################
# DO NOT MODIFY THIS HEADER SECTION

CHECKOUT_DIR := $(shell perl -e '$$_ = `pwd`; if (s:(.*)/(src|src/|src/(.*?))$$:$$1:) { print $$_; } else { print "<error>: $$_"; die "*** could not calculate CHECKOUT_DIR ***\n"; }')
#CHECKOUT_DIR := $(shell perl -e '$$_ = `pwd`; chop; $$_ = scalar reverse("$$_/"); if (s:.*/crs/::) { print scalar reverse($$_); } else { print "<error>"; die "*** could not calculate CHECKOUT_DIR ***\n"; }')
include $(CHECKOUT_DIR)/src/microraptor/Build/header.mak

######################################################################
# PLACE MODIFICATIONS BELOW THIS LINE

# ADD ANY SUBDIRS THAT NEED TO BE VISITED DURING A BUILD TO THE SUBDIRS LIST
SUBDIRS := 

# add extra pre-processor flags here (e.g. -I, -D)
CPPFLAGS +=

# add extra compile flags here (e.g. -pg, -g, -O, -Wall)
CFLAGS += 

# add extra link flags here (e.g. -L, -l)
LDFLAGS += 

# add extra files to clean up with 'gmake clean' here
LOCAL_CLEAN_FILES := 

# build a C extension for python via SWIG (you can use this multiple times)
BUILDPYTHON_MODULE := rcl
BUILDPYTHON_HEADER := rcl.i
BUILDPYTHON_SRCS := RCLWrapper.cc
BUILDPYTHON_DEP_LIBS := -lrcl -lmrcommonclean
BUILDPYTHON_INDEP_LIBS :=
include $(BUILD_DIR)/buildpython.mak

# used below in the 'make java' rule
JAVA_MODULE_NAME := rcl_java
JAVA_OBJS := $(OBJ_DIR)/$(JAVA_MODULE_NAME)_wrap.o $(OBJ_DIR)/RCLWrapper.o

# add to the list of files rm'd when you make clean
LOCAL_CLEAN_FILES := TestRCL.class $(TARGET_LIB_DIR)/lib$(JAVA_MODULE_NAME).so

java:
	mkdir -p include
	cd include && swig -java -c++ -package $(JAVA_MODULE_NAME) -o $(JAVA_MODULE_NAME)_wrap.cc $(SWIG_CFLAGS) -I.. ../rcl.i
	cd include && javac *.java
	mkdir -p $(TARGET_LIB_DIR)/$(JAVA_MODULE_NAME)
	cp include/*.class $(TARGET_LIB_DIR)/$(JAVA_MODULE_NAME)
	$(MAKE) $(MFLAGS) $(JAVA_OBJS)
	g++ -shared $(JAVA_OBJS) -L$(TARGET_LIB_DIR) -lrcl -lmrcommonclean -o $(TARGET_LIB_DIR)/lib$(JAVA_MODULE_NAME).so
	javac TestRCL.java

ifneq (,$(TEST))

# any targets defined here will only be built with 'gmake TEST=1 install'

endif


######################################################################
# DO NOT MODIFY BELOW THIS POINT

include $(BUILD_DIR)/footer.mak

######################################################################
# $Log: Makefile,v $
# Revision 1.4  2004/12/01 06:59:24  brennan
# Modified CHECKOUT_DIR inference to handle paths with multiple "src"'s.
#
# Revision 1.3  2004/07/06 15:12:08  trey
# moved LOCAL_CLEAN_FILES define so it picks up variables correctly
#
# Revision 1.2  2004/07/01 18:04:01  trey
# changed so "make java" no longer depends on python
#
# Revision 1.1  2004/07/01 00:26:00  trey
# initial check-in
#
#
#
