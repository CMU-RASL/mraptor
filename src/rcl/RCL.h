/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.7 $  $Author: trey $  $Date: 2004/07/06 15:35:12 $
 *  
 * PROJECT: FIRE Architecture Project
 *
 * @file    RCL.h
 * @brief   No brief
 ***************************************************************************/

#ifndef INCRCL_h
#define INCRCL_h

#if USE_MEM_TRACKER
#  include "memTracker.h"
#endif

#include "RCLTypes.h"
#include "RCLException.h"
#include "RCLExp.h"
#include "RCLMap.h"
#include "RCLVector.h"
#include "RCLParser.h"
#include "RCLConfigFile.h"

#ifndef FOR_EACH
#  define FOR_EACH(elt,collection) \
    for (typeof((collection).begin()) elt=(collection).begin(); \
         elt != (collection).end(); elt++)
#endif

#ifndef FOR
#  define FOR(i,n) \
    for (typeof(n) i=0; i<(n); i++)
#endif

#endif // INCRCL_h

/***************************************************************************
 * REVISION HISTORY:
 * $Log: RCL.h,v $
 * Revision 1.7  2004/07/06 15:35:12  trey
 * added optional memory tracking and plugged some memory leaks
 *
 * Revision 1.6  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.5  2004/04/12 19:25:16  trey
 * more api cleanup
 *
 * Revision 1.4  2004/04/09 20:31:33  trey
 * major enhancement to rcl interface
 *
 * Revision 1.3  2003/02/05 22:50:51  trey
 * mostly ready for mraptor now
 *
 * Revision 1.2  2003/02/05 20:23:20  trey
 * implemented command syntax and switched error behavior to throw exceptions
 *
 * Revision 1.1  2003/02/05 03:57:54  trey
 * initial atacama check-in
 *
 * Revision 1.1  2002/09/24 21:50:22  trey
 * rebuilt RCL from scratch to support nested vectors and hashes
 *
 *
 ***************************************************************************/
