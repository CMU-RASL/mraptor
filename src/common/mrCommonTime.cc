/****************************************************************************
 * $Revision: 1.2 $  $Author: trey $  $Date: 2003/11/18 23:14:08 $
 *
 * PROJECT:      Autonomous Rover Technologies
 * FILE:         commonTime.c
 * DESCRIPTION:  common util functions for dealing with struct timeval,
 *               gettimeofday(), and select()
 *
 * (c) Copyright 2000 Carnegie Mellon University. All rights reserved.
 ***************************************************************************/

/***************************************************************************
 * INCLUDES
 ***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>

#include "mrCommonTime.h"
#define gettimeofdayPortable(x , y ) gettimeofday((x),(y))

namespace microraptor {

struct timeval
getTime(void) {
  struct timeval tv;
  gettimeofdayPortable(&tv,0);
  return tv;
}

double
timeDiff(struct timeval a, struct timeval b) {
  return (a.tv_sec - b.tv_sec) + 1e-6*(a.tv_usec - b.tv_usec);
}

struct timeval
timevalOf(double d) {
  struct timeval a;
  a.tv_sec = (int) d;
  a.tv_usec = (int) (1e+6 * (d-a.tv_sec));
  return a;
}

double
doubleOf(struct timeval tv) {
  return tv.tv_sec + 1e-6*tv.tv_usec;
}

const char *
timevalToString(struct timeval tv) {
  static char buf[256];
  snprintf(buf, sizeof(buf), "%ld.%06ld", tv.tv_sec, tv.tv_usec);
  return buf;
}

void
sleepPrecise(double seconds) {
  struct timeval tv;
  float fl;
  if (seconds < 0) return;
  fl = floor(seconds);
  tv.tv_sec = (int) fl;
  tv.tv_usec = (int) ((seconds - fl) * 1000000);
  select(0,0,0,0,&tv);
}

/**
   This will check to see if a read from stdin will not block
   @return TRUE if there is input waiting
   @return FALSE if there isn't
*/

int checkForInput(void) {
    fd_set input;
    struct timeval t0;
    t0.tv_sec = 0;
    t0.tv_usec=0;
    FD_ZERO(&input);
    FD_SET(fileno(stdin),&input);
    if (select(fileno(stdin)+1,&input,NULL,NULL,&t0)>0) {
        return 1;
    } else {
        return 0;
    }
}

} // namespace microraptor

/***************************************************************************
 * REVISION HISTORY:
 * $Log: mrCommonTime.cc,v $
 * Revision 1.2  2003/11/18 23:14:08  trey
 * fixed warning about printf format
 *
 * Revision 1.1  2003/11/15 17:53:40  brennan
 * Split up CYGWIN #define into more reasonable/understandable chunks.  Completed move to FIRE build system.
 *
 * Revision 1.5  2003/10/29 23:51:56  trey
 * added timevalToString()
 *
 * Revision 1.4  2003/10/04 21:56:48  trey
 * tweaked things to eliminate the need for the common/overlap directory and libmrcommonoverlap.a
 *
 * Revision 1.1  2003/03/28 17:38:15  trey
 * rearranged locations of files so that the atacama project can use rcl without causing namespace conflicts
 *
 * Revision 1.2  2003/02/06 16:35:44  trey
 * fixed include after name change
 *
 * Revision 1.1  2003/02/05 22:54:45  trey
 * made independent copy and renamed to avoid collisions
 *
 * Revision 1.1  2003/01/06 17:51:17  mwagner
 * Initial revision in atacama repository.
 *
 * Revision 1.3  2001/04/27 12:17:19  curmson
 * Added a function that allows you to check for keyboard input without blocking
 *
 * Revision 1.2  2001/03/05 19:15:06  curmson
 * wrapped PGMImage in ifndefs
 * added time.h to top of commonTime
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
 * Revision 1.3  2000/09/13 21:16:11  trey
 * made many minor changes to get the IPC version compiling under vxWorks
 *
 * Revision 1.2  2000/08/10 17:02:56  trey
 * handled the special case of waiting time less than 0 in sleepPrecise() by returning immediately instead of hanging forever
 *
 * Revision 1.1  2000/07/06 19:55:05  trey
 * initial check-in
 *
 *
 ***************************************************************************/
