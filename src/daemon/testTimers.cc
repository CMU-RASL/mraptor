/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.1 $  $Author: trey $  $Date: 2005/06/27 16:03:13 $
 *  
 * PROJECT: Life in the Atacama
 *
 * @file    testTimers.cc
 * @brief   No brief
 ***************************************************************************/

/***************************************************************************
 * INCLUDES
 ***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>

#include <iostream>

#include "mrTimers.h"

using namespace std;
using namespace microraptor;

TimerQueue tqueueG;

void handler1(void* clientData);
void handler2(void* clientData);
void handler3(void* clientData);

void handler1(void* clientData) {
  cout << "--- handler1 fired" << endl;
  tqueueG.addOneShotTimer( "handler2", 0.5, &handler2, NULL );
}

void handler2(void* clientData) {
  cout << "--- handler2 fired" << endl;
  tqueueG.addOneShotTimer( "handler1", 0.5, &handler1, NULL );
}

void handler3(void* clientData) {
  cout << "--- handler3 fired" << endl;
}

void doit(void)
{
  tqueueG.addOneShotTimer( "handler1", 0.5, &handler1, NULL );
  tqueueG.addPeriodicTimer( "handler3", 0.6, &handler3, NULL );
  while (1) {
    usleep( (unsigned int) (MR_TIMER_PERIOD_SECONDS * 1e+6) );
    cout << endl << endl << "*** TICK ***" << endl;
    tqueueG.tick();
    tqueueG.debugPrint();
  }
}

static void usage(void) {
  cerr <<
    "usage: testTimers OPTIONS\n"
    "  -h or --help                  Print this help\n";
  exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
  static char shortOptions[] = "h";
  static struct option longOptions[]={
    {"help",0,NULL,'h'},
    {NULL,0,0,0}
  };

  while (1) {
    char optchar = getopt_long(argc,argv,shortOptions,longOptions,NULL);
    if (optchar == -1) break;

    switch (optchar) {
    case 'h': // help
      usage();
      break;
    case '?':
      // getopt() prints an error
      cerr << endl;
      usage();
      break;
    default:
      abort(); // never reach this point
    }
  }
  if (argc-optind != 0) {
    cerr << argv[0] << ": wrong number of arguments" << endl << endl;
    usage();
  }

  doit();

  return EXIT_SUCCESS;
}

/***************************************************************************
 * REVISION HISTORY:
 * $Log: testTimers.cc,v $
 * Revision 1.1  2005/06/27 16:03:13  trey
 * initial check-in
 *
 *
 *
 ***************************************************************************/
