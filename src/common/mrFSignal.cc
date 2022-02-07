/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.3 $  $Author: dom $  $Date: 2004/04/28 17:15:21 $
 *  
 * PROJECT: FIRE Architecture Project
 *
 * @file    FSignal.cc
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

#include "mrFSignal.h"

using namespace std;

/***************************************************************************
 * FUNCTIONS
 ***************************************************************************/

namespace microraptor {

FConnection
FSignal0::Impl::connect(FSlot0 slot) {
  slotList.push_back(slot);
#if 0
  cout << "FSignal0::connect: numConnections now = " << getNumConnections()
       << endl;
#endif
  numConnectionsChangeHandler(getNumConnections()-1, getNumConnections());
  return FConnection(new FConnectionList<FSignal0>
		     (this, --slotList.end()));
}

} // namespace microraptor

/***************************************************************************
 * REVISION HISTORY:
 * $Log: mrFSignal.cc,v $
 * Revision 1.3  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.2  2003/10/04 21:56:48  trey
 * tweaked things to eliminate the need for the common/overlap directory and libmrcommonoverlap.a
 *
 * Revision 1.1  2003/02/14 05:58:05  trey
 * initial atacama check-in
 *
 * Revision 1.4  2001/11/08 23:09:16  trey
 * removed debugging statement
 *
 * Revision 1.3  2001/11/02 21:06:07  trey
 * fixed problem with returning FSignal datatype from a function by adding a wrapper layer around everything
 *
 * Revision 1.2  2001/10/31 02:36:02  trey
 * did a little code cleanup
 *
 * Revision 1.1  2001/10/31 02:32:00  trey
 * initial check-in
 *
 *
 ***************************************************************************/
