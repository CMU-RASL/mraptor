/********** tell emacs we use -*- c++ -*- style comments *******************
 * $Revision: 1.3 $  $Author: trey $  $Date: 2004/07/06 15:35:12 $
 *
 * PROJECT:      Distributed Robotic Agents
 * DESCRIPTION:  Replace strcadd() and streadd() libgen functionality
 *               for non-IRIX platforms.
 *
 * (c) Copyright 1999 CMU. All rights reserved.
 ***************************************************************************/

#ifndef INCstrxcpy_h
#define INCstrxcpy_h

/***************************************************************************
 * GLOBALS AND FUNCTION PROTOTYPES
 ***************************************************************************/

#ifdef __cplusplus

#include <string>
#if USE_MEM_TRACKER
#  include "memTracker.h"
#endif

  /* strcadd with C++ strings */
std::string compressEscapes(const std::string& s);

extern "C" {
#endif

/****************************************************************************
 * FUNCTION:	strcadd
 * DESCRIPTION:
 *  copies the input string, up to a null byte, to the output string,
 *  compressing the C-language escape sequences (for example, \n, \001) to
 *  the equivalent character.  A null byte is appended to the output.  The
 *  output argument must point to a space big enough to accommodate the
 *  result.  If it is as big as the space pointed to by input it is
 *  guaranteed to be big enough.
 * RETURN: the pointer to the null byte that terminates the output.
 ****************************************************************************/
char *strcadd(char *output, const char *input);

/****************************************************************************
 * FUNCTION:	streadd
 * DESCRIPTION:
 *  streadd copies the input string, up to a null byte, to the output string,
 *  expanding non-graphic characters to their equivalent C-language escape
 *  sequences (for example, \n, \001).  The output argument must point to a
 *  space big enough to accommodate the result; four times the space pointed
 *  to by input is guaranteed to be big enough (each character could become \
 *  and 3 digits).  Characters in the exceptions string are not expanded.
 *  The exceptions argument may be zero, meaning all non-graphic characters
 *  are expanded.
 * RETURN: the pointer to the null byte that terminates the output.
 ****************************************************************************/
char *streadd (char *output, const char *input, const char *exceptions);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif // INCstrxcpy_h

/***************************************************************************
 * REVISION HISTORY:
 * $Log: strxcpy.h,v $
 * Revision 1.3  2004/07/06 15:35:12  trey
 * added optional memory tracking and plugged some memory leaks
 *
 * Revision 1.2  2004/04/13 21:12:38  trey
 * did some stress testing, fixed several obscure bugs
 *
 * Revision 1.1  2003/02/05 03:57:54  trey
 * initial atacama check-in
 *
 * Revision 1.1  2002/07/24 21:22:03  trey
 * initial FIRE checkin
 *
 * Revision 1.1  1999/11/03 19:31:38  trey
 * initial check-in
 *
 * Revision 1.1  1999/11/01 17:30:15  trey
 * initial check-in
 *
 *
 ***************************************************************************/
