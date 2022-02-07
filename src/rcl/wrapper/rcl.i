/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.3 $  $Author: trey $  $Date: 2004/07/06 15:12:37 $
 *  
 * PROJECT: Life in the Atacama
 *
 * @file    RCLWrapper.h
 * @brief   No brief
 ***************************************************************************/

#ifndef INCrcl_i
#define INCrcl_i

%module rcl

%{
#include "RCLWrapper.h"
%}

#ifdef SWIGJAVA

%exception {
  try {
    $function
  }
  catch (rcl::exception err) {
    jclass c = jenv->FindClass("java/lang/RuntimeException");
    jenv->ThrowNew(c, err.text.c_str());
    return $null;
  }
}

%typemap(javaout) Exp* {
  /* tell java that it owns and must deallocate Exp* return values */
  long cPtr = $jnicall;
  return (cPtr == 0) ? null : new $javaclassname(cPtr, /* thisown = */ true);
}

#endif // ifdef SWIGJAVA

#ifdef SWIGPYTHON

%exception {
  try {
    $function
  }
  catch (rcl::exception err) {
    std::string s = "RCL: " + err.text;
    PyErr_SetString(PyExc_IOError, s.c_str());
    return NULL;
  }
}

%typemap(out) Exp* {
  /* tell python that it owns and must deallocate Exp* return values */
  $result = SWIG_NewPointerObj((void *) $1, $1_descriptor,
			       /* thisown = */ true);
}

#endif // ifdef SWIGPYTHON

%include "RCLWrapper.h"

#endif // INCrcl_i

/***************************************************************************
 * REVISION HISTORY:
 * $Log: rcl.i,v $
 * Revision 1.3  2004/07/06 15:12:37  trey
 * fixed memory leak in python and java wrappers
 *
 * Revision 1.2  2004/07/01 01:23:51  trey
 * added java exception handling
 *
 * Revision 1.1  2004/07/01 00:26:00  trey
 * initial check-in
 *
 *
 ***************************************************************************/
