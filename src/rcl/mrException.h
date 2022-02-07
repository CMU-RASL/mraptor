/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.5 $  $Author: dom $  $Date: 2004/04/28 17:15:21 $
 *  
 * @file    mrexception.h
 * @brief   No brief
 ***************************************************************************/

#ifndef INCmrexception_h
#define INCmrexception_h

#include <string>

// to get an error: throw Execute_Exception("text");
// to get a warning: throw Execute_Exception("text", MR_WARNING);
#define MR_WARNING (true)
struct MRException {
  std::string text;
  bool is_warning;

  MRException(void) : text(""), is_warning(false) {}
  MRException(const std::string& _text,
	      bool _is_warning = false) :
    text(_text),
    is_warning(_is_warning)
  {}
};

#endif // INCmrexception_h

/***************************************************************************
 * REVISION HISTORY:
 * $Log: mrException.h,v $
 * Revision 1.5  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.4  2003/10/04 21:56:48  trey
 * tweaked things to eliminate the need for the common/overlap directory and libmrcommonoverlap.a
 *
 * Revision 1.3  2003/08/08 15:29:27  brennan
 * Attempted to make microraptor compile:
 *   (1) on Cygwin and
 *   (2) under gcc 3.2
 * This attempt has not yet succeeded
 *
 * Revision 1.2  2003/02/14 00:13:54  trey
 * fixed include issues
 *
 * Revision 1.1  2003/02/13 22:29:02  trey
 * added mrException
 *
 *
 ***************************************************************************/
