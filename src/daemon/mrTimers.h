/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.4 $  $Author: brennan $  $Date: 2006/06/04 05:27:03 $
 *  
 * PROJECT: Life in the Atacama
 *
 * @file    mrTimers.h
 * @brief   No brief
 ***************************************************************************/

#ifndef INCmrTimers_h
#define INCmrTimers_h

#define MR_TIMER_PERIOD_SECONDS (0.1)

#include <vector>
#include "mrSmartRef.h"

#ifdef GCC2_96
#include <string>
#endif

namespace microraptor {

  typedef void (*TimerHandler)(void* clientData);

  struct TimerEntry {
    // the period between successive calls to the handler, measured in
    // 0.1 second ticks
    std::string name;
    unsigned int periodTicks;
    unsigned int lastTickFired;
    TimerHandler handler;
    void *clientData;
    int numTimesFired;

    TimerEntry(const std::string& _name,
	       unsigned int _periodTicks,
	       unsigned int _lastTickFired,
	       TimerHandler _handler,
	       void* _clientData);
    void maybeFire(unsigned int currentTick, int debugLevel);
    void debugPrint(void);
  };
    
  struct TimerQueue {
    TimerQueue(void);
    void addPeriodicTimer(const std::string& name, double periodSeconds,
			  TimerHandler handler, void* clientData);
    void addOneShotTimer(const std::string& name, double periodSeconds,
			 TimerHandler handler, void *clientData);
    void tick(void);
    void debugPrint(void);
    void setDebugLevel(int _debugLevel) { debugLevel = _debugLevel; }

  protected:
    std::vector< SmartRef<TimerEntry> > periodicTimers;
    std::vector< SmartRef<TimerEntry> > oneShotTimers;
    unsigned int currentTick;
    bool modified;
    int debugLevel;
    
    void checkTimers(void);
  };

}; // namespace microraptor

#endif // INCmrTimers_h

/***************************************************************************
 * REVISION HISTORY:
 * $Log: mrTimers.h,v $
 * Revision 1.4  2006/06/04 05:27:03  brennan
 * First pass at modularized RPMs, and changes to build under gcc 2.96.
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
