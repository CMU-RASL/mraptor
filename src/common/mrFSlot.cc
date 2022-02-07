/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.3 $  $Author: dom $  $Date: 2004/04/28 17:15:21 $
 *  
 * PROJECT: FIRE Architecture Project
 *
 * @file    FSlot.cc
 * @brief   No brief
 ***************************************************************************/

/***************************************************************************
 * INCLUDES
 ***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <iostream>

#include "mrFSlot.h"

using namespace std;

/***************************************************************************
 * FUNCTIONS
 ***************************************************************************/

namespace microraptor {

FSlot0 fslot(void (*h)(void)) {
  return FSlot0(new FSlot0_Hnd(h));
}

FSlot1<Unit&> unitArg(FSlot0& sl) {
  return FSlot1<Unit&>(new FSlot1_Unit(sl));
}

} // namespace microraptor

/***************************************************************************
 * REVISION HISTORY:
 * $Log: mrFSlot.cc,v $
 * Revision 1.3  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.2  2003/10/04 21:56:48  trey
 * tweaked things to eliminate the need for the common/overlap directory and libmrcommonoverlap.a
 *
 * Revision 1.1  2003/02/14 05:58:05  trey
 * initial atacama check-in
 *
 * Revision 1.2  2001/10/31 02:36:02  trey
 * did a little code cleanup
 *
 * Revision 1.1  2001/10/30 05:34:27  trey
 * initial check-in
 *
 *
 ***************************************************************************/
