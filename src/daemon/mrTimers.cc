/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.4 $  $Author: trey $  $Date: 2006/06/14 01:40:34 $
 *  
 * PROJECT: Life in the Atacama
 *
 * @file    mrTimers.cc
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

#include "mrTimers.h"
#include "mrCommonDefs.h"

using namespace std;

namespace microraptor {

  TimerEntry::TimerEntry(const std::string& _name,
			 unsigned int _periodTicks,
			 unsigned int _lastTickFired,
			 TimerHandler _handler,
			 void* _clientData) :
    name(_name),
    periodTicks(_periodTicks),
    lastTickFired(_lastTickFired),
    handler(_handler),
    clientData(_clientData),
    numTimesFired(0)
  {}

  void TimerEntry::maybeFire(unsigned int currentTick, int debugLevel)
  {
    if (( currentTick - lastTickFired ) >= periodTicks ) {
      if (debugLevel >= 1) {
	cout << "DEBUG_TIMERS: firing " << name << endl;
      }
      (*handler)(clientData);
      lastTickFired = currentTick;
      numTimesFired++;
    }
  }

  void TimerEntry::debugPrint(void)
  {
    cout << name
	 << " periodTicks=" << periodTicks
	 << " lastTickFired=" << lastTickFired
	 << " numTimesFired=" << numTimesFired;
  }

  TimerQueue::TimerQueue(void) :
    currentTick(0),
    modified(false),
    debugLevel(0)
  {}

  void TimerQueue::addPeriodicTimer(const std::string& name,
				    double periodSeconds,
				    TimerHandler handler,
				    void *clientData)
  {
    unsigned int periodTicks = (unsigned int) ((periodSeconds / MR_TIMER_PERIOD_SECONDS) + 0.5);
    TimerEntry* newTimer = new TimerEntry( name, periodTicks, currentTick, handler, clientData );
    periodicTimers.push_back( newTimer );
    modified = true;

    if (debugLevel >= 1) {
      cout << "DEBUG_TIMERS: addPeriodicTimer: ";
      newTimer->debugPrint();
      cout << endl;
    }
  }

  void TimerQueue::addOneShotTimer(const std::string& name,
				   double periodSeconds,
				   TimerHandler handler,
				   void *clientData)
  {
    unsigned int periodTicks = (unsigned int) ((periodSeconds / MR_TIMER_PERIOD_SECONDS) + 0.5);
    TimerEntry* newTimer = new TimerEntry( name, periodTicks, currentTick, handler, clientData );
    oneShotTimers.push_back( newTimer );
    modified = true;

    if (debugLevel >= 1) {
      cout << "DEBUG_TIMERS: addOneShotTimer: ";
      newTimer->debugPrint();
      cout << endl;
    }
  }

  void TimerQueue::tick(void)
  {
    currentTick++;

    while (1) {
      modified = false;
      checkTimers();
      if (!modified) break;
    }

    if ( !oneShotTimers.empty() ) {
      std::vector< SmartRef<TimerEntry> > remainingOneShotTimers;
      FOR_EACH ( tp, oneShotTimers ) {
	if ( 0 == (*tp)->numTimesFired ) {
	  remainingOneShotTimers.push_back( *tp );
	}
      }
      oneShotTimers = remainingOneShotTimers;
    }
  }

  void TimerQueue::debugPrint(void)
  {
    cout << "DEBUG_TIMERS: currentTick=" << currentTick << endl;
    FOR_EACH ( tp, periodicTimers ) {
      cout << "  (per) ";
      (*tp)->debugPrint();
      cout << endl;
    }
    FOR_EACH ( tp, oneShotTimers ) {
      cout << "  (one) ";
      (*tp)->debugPrint();
      cout << endl;
    }
  }

  void TimerQueue::checkTimers(void)
  {
    FOR_EACH ( tp, periodicTimers ) {
      (*tp)->maybeFire(currentTick, debugLevel);
      if (modified) return;
    }
    FOR_EACH ( tp, oneShotTimers ) {
      (*tp)->maybeFire(currentTick, debugLevel);
      if (modified) return;
    }
  }

}; // namespace microraptor

/***************************************************************************
 * REVISION HISTORY:
 * $Log: mrTimers.cc,v $
 * Revision 1.4  2006/06/14 01:40:34  trey
 * turned off verbose timer logging by default
 *
 * Revision 1.3  2005/08/09 00:46:39  trey
 * added more debug output
 *
 * Revision 1.2  2005/06/27 19:27:11  trey
 * added optional debugging output
 *
 * Revision 1.1  2005/06/27 16:03:13  trey
 * initial check-in
 *
 *
 ***************************************************************************/
