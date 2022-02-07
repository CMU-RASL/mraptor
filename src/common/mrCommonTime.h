/********** tell emacs we use -*- c++ -*- style comments *******************
 * $Revision: 1.6 $  $Author: brennan $  $Date: 2006/11/16 18:56:05 $
 *
 * PROJECT:      Autonomous Rover Technologies
 * FILE:         mrCommonTime.h
 * DESCRIPTION:  common util functions for dealing with struct timeval,
 *               gettimeofday(), and select()
 *
 * (c) Copyright 2000 Carnegie Mellon University. All rights reserved.
 ***************************************************************************/

/** @file mrCommonTime.h
 */

#ifndef INCmrCommonTime_h
#define INCmrCommonTime_h

/***************************************************************************
 * INCLUDES
 ***************************************************************************/

#ifdef __VXWORKS__
#  include <selectLib.h>
#else
#  include <sys/time.h>
#endif
#include <stdio.h>

/***************************************************************************
 * GLOBALS AND FUNCTION PROTOTYPES
 ***************************************************************************/

namespace microraptor {

/**
   This macro will time the code passed as the code argument, and output the
   time the code took to execute along with msg to stderr.
   @param msg the message to output before the timing information
   @param code the code to time
*/
#define TIME_IT(msg,code) { \
  struct timeval start, end; gettimeofday(&start, NULL);\
  code;\
  gettimeofday(&end, NULL);\
  fprintf(stderr, "%s: %.3f\n", msg,\
	  end.tv_sec - start.tv_sec + (end.tv_usec - start.tv_usec)/1e6);}

/**
   Gets the current time.
   @return a timeval contains the system time (seconds, milliseconds)
   @see timeDiff timevalOf doubleOf 
*/
    struct timeval getTime(void);

/**
   Returns the time differance between two timevals (in seconds).
   @param a the most recent time
   @param b the timeval to be subtracted from a
   @return the difference between a and b in seconds (can be fractional)
   @see doubleOf timevalOf
*/
    double timeDiff(struct timeval a, struct timeval b);

/**
   Converts a double that holds seconds into a timeval.
   @param d the double to be converted into a timeval
   @return a timeval representation of d seconds
   @see doubleOf timeDiff
*/
    struct timeval timevalOf(double d);
/**
   Converts a timeval into seconds.
   @param tv the timeval to convert
   @return a double representation of the timeval
   @see timevalOf timeDiff
*/
    double doubleOf(struct timeval tv);

/**
   Converts a timeval to a string showing a decimal number of seconds.
   @param tv the timeval to convert
   @return a string showing the decimal number of seconds
*/
  const char *timevalToString(struct timeval tv);

/**
   Uses select to sleep for a precise amount of time.
   @param seconds the number of seconds to sleep for
*/
    void sleepPrecise(double seconds);


/**
   This will check to see if a read from stdin will not block
   @return 1 if there is input waiting
   @return 0 if there isn't
*/
    int checkForInput(void);

} // namespace microraptor

#endif /* INCmrCommonTime_h */

/***************************************************************************
 * REVISION HISTORY:
 * $Log: mrCommonTime.h,v $
 * Revision 1.6  2006/11/16 18:56:05  brennan
 * Added support for callbacks during a reliableConnect.
 *
 * Revision 1.5  2004/07/23 19:49:16  trey
 * fixed collision with libcommon.a
 *
 * Revision 1.4  2003/10/29 23:51:55  trey
 * added timevalToString()
 *
 * Revision 1.3  2003/10/04 21:56:48  trey
 * tweaked things to eliminate the need for the common/overlap directory and libmrcommonoverlap.a
 *
 * Revision 1.1  2003/03/28 17:38:15  trey
 * rearranged locations of files so that the atacama project can use rcl without causing namespace conflicts
 *
 * Revision 1.1  2003/02/05 22:54:45  trey
 * made independent copy and renamed to avoid collisions
 *
 * Revision 1.1  2003/01/06 17:51:17  mwagner
 * Initial revision in atacama repository.
 *
 * Revision 1.6  2001/04/27 12:17:19  curmson
 * Added a function that allows you to check for keyboard input without blocking
 *
 * Revision 1.5  2001/03/05 19:15:06  curmson
 * wrapped PGMImage in ifndefs
 * added time.h to top of mrCommonTime
 *
 * Revision 1.4  2001/01/13 21:24:50  curmson
 *
 * Rewrote the output operators for common types to use streams.
 * statusPublisher now compiles cleanly.
 * added TIMEIT macro to commmon time- use this to time and output how long parts of code take to run.
 *
 * Revision 1.3  2000/12/13 23:32:30  curmson
 *
 * Touched up commenting in several files.
 * Added new class base coordinate transforms.
 * Overhauled hyperionDimensions.
 * Added ability to read a line upto a maximum number of characters from a serial port.
 *
 * Revision 1.2  2000/12/11 15:46:04  curmson
 *
 * Revised versions with autodocumenting comments
 *
 * Revision 1.1  2000/11/25 23:31:31  curmson
 *
 * This is an initial import of the JPL makefile system. Used atleast in the common directory.  Potentially very useful
 * This is a cleaned up version of the common directory from the mars autonomy software.  Significant changes have been made to roverTransforms.c/h and roverDimensions.h and hyperionDimensions.h has been created
 *
 * hyperionDimensions.h contains all of the physical parameters of the robot and should be the only source of physical constants used in code.
 *
 * roverTransforms provides transformation matricies between the laser, stereo, front axle robot and global frames.  These routines should be used for all transformations.  They can be used by linking against libcommon.  To generate libcommon run 'make install'
 *
 * Revision 1.1  2000/07/06 19:55:06  trey
 * initial check-in
 *
 *
 ***************************************************************************/
