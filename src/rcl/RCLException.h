/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.8 $  $Author: trey $  $Date: 2005/06/01 00:47:43 $
 *  
 * PROJECT: FIRE Architecture Project
 *
 * @file    RCLExceptions.h
 * @brief   No brief
 ***************************************************************************/

#ifndef INCRCLExceptions_h
#define INCRCLExceptions_h

#include <string>
#include <iostream>
#include <cstdlib>
#include "mrException.h"

namespace rcl {

#if 0
  struct exception : public MRException {
    exception(void) : MRException() {}
    exception(const std::string& _text,
	      bool _is_warning = false) :
      MRException(_text, _is_warning)
    {}
  };
#endif

  // by using a typedef, we ensure that "catch rcl::exception" will also
  // catch an MRException, which is necessary because system calls
  // within rcl functions sometimes throw MRException
  typedef MRException exception;

  typedef void (*error_handler)(const std::string& text,
				bool is_warning);
  
  inline void onErrorPrintTextAndThrowException(const std::string& text,
					 bool is_warning)
  {
    std::cerr << (is_warning ? "WARNING" : "ERROR")
	      << ": " << text << std::endl;
    throw exception(text,is_warning);
  }
  inline void onErrorPrintTextAndExit(const std::string& text,
				      bool is_warning)
  {
    std::cerr << (is_warning ? "WARNING" : "ERROR")
	      << ": " << text << std::endl;
    //exit(EXIT_FAILURE);
    //exit failure is not a thing, we'll EXIT 1
    //exit is not a thing?
    exit(1);
  }
  inline void onErrorThrowException(const std::string& text,
				    bool is_warning)
  {
    throw exception(text,is_warning);
  }
  
  extern error_handler handler;

  inline void setErrorHandler(error_handler _handler) {
    handler = _handler;
  }
  inline void callErrorHandler(const std::string& text,
			       bool is_warning = false)
  {
    (*handler)(std::string("RCL: ")+text, is_warning);
  }
  inline void callErrorHandler(const std::string& prefix,
			       const std::string& text,
			       bool is_warning = false)
  {
    (*handler)(prefix+": "+text, is_warning);
  }

};

#endif // INCRCLExceptions_h

/***************************************************************************
 * REVISION HISTORY:
 * $Log: RCLException.h,v $
 * Revision 1.8  2005/06/01 00:47:43  trey
 * added another version of rcl::callErrorHandler
 *
 * Revision 1.7  2004/07/17 18:32:43  trey
 * catching rcl::exception will now also catch MRException, which should avoid some confusion
 *
 * Revision 1.6  2004/07/01 00:26:51  trey
 * fixed stupid mixup of function names
 *
 * Revision 1.5  2004/06/10 22:46:43  trey
 * changed default behavior for RCL so that it prints an error before throwing an exception; error handling is now configurable using setErrorHandler()
 *
 * Revision 1.4  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.3  2004/04/09 20:31:33  trey
 * major enhancement to rcl interface
 *
 * Revision 1.2  2003/02/14 00:13:11  trey
 * added better support for checking the type of numeric constants
 *
 * Revision 1.1  2003/02/06 16:36:23  trey
 * initial atacama check-in
 *
 *
 ***************************************************************************/
