/***** tell emacs we use -*- c++ -*- style comments *****
 * $Revision: 1.7 $ $Author: trey $ $Date: 2005/06/27 19:27:50 $
 *
 * COPYRIGHT 2004, Carnegie Mellon University 
 *
 * PROJECT: Exploration Robotics (Life in the Atacama)
 *
 * MODULE:
 *
 * FILE: microraptor/common/mrCommonDefs.h
 *
 * DESCRIPTION:
 *
 ********************************************************/
/****************************************************************************
 *
 * PROJECT:      Intelligent Robotics
 *
 * MODULE:       Common
 *
 * FILE:         mrCommonDefs.h
 *
 * DESCRIPTION:  Contains common defines used by many modules.
 *
 * NOTES:        
 **********************************************************************/

#ifndef INCmrCommonDefs_h
#define INCmrCommonDefs_h
/** @file mrCommonDefs.h */

/***************************************************************************
 * INCLUDES - needed for defintions in this header
 ***************************************************************************/
 
#include <math.h>
#include <unistd.h>

/***************************************************************************
 * "CONSTANTS"
 ***************************************************************************/

#ifndef FOR_EACH
#  define FOR_EACH(elt,collection) \
    for (typeof((collection).begin()) elt=(collection).begin(); \
         elt != (collection).end(); elt++)
#endif

#ifndef FOR
#  define FOR(i,n) \
    for (int i=0; i<(n); i++)
#endif

#ifndef MIN
#  define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

#ifndef SEC_TO_MSEC
#  define SEC_TO_MSEC(sec) ((unsigned long) ((sec)*1000))
#endif

#ifndef SEC_TO_USEC
#  define SEC_TO_USEC(sec) ((unsigned long) ((sec)*1e+6))
#endif

#endif /* INCmrCommonDefs_h */

/**********************************************************************
 * $Log: mrCommonDefs.h,v $
 * Revision 1.7  2005/06/27 19:27:50  trey
 * added SEC_TO_USEC() macro
 *
 * Revision 1.6  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.5  2003/10/04 21:56:48  trey
 * tweaked things to eliminate the need for the common/overlap directory and libmrcommonoverlap.a
 *
 * Revision 1.1  2003/03/28 17:38:15  trey
 * rearranged locations of files so that the atacama project can use rcl without causing namespace conflicts
 *
 * Revision 1.3  2003/02/16 06:20:08  trey
 * moved some things from daemon to common
 *
 * Revision 1.2  2003/02/14 21:53:55  trey
 * made SEC_TO_MSEC work better with IPC
 *
 * Revision 1.1  2003/02/05 22:54:45  trey
 * made independent copy and renamed to avoid collisions
 *
 * (much other history no longer relevant, axed most of the file)
 ***************************************************************************/

