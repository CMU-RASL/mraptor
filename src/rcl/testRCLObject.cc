/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.7 $  $Author: dom $  $Date: 2004/06/18 19:35:32 $
 *  
 * PROJECT: Life in the Atacama
 *
 * @file    testRCLObject.cc
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

#include "RCL.h"
#include "RCLObject.h"

using namespace std;

/* The MyPos struct is an example of how to set up automatic
 * interconversion between a datatype and a map type rcl::exp.
 *
 *  1) Make your class derive from rcl::mapObject.
 *  2) Declare RCL_SYNC_FUNCTION().  Within the sync function,
 *     call RCL_SYNC_FIELD() on all of the variables you wish
 *     to be read from / written to an rcl::exp as part of
 *     conversions.
 *  3) Call RCL_DEFINE_OPERATORS() with your class as the argument.
 *     This defines:
 *     a) A constructor which takes const rcl::exp& as an argument,
 *        so that you can say, e.g., "MyPos pos = expr;".
 *     b) An assignment operator, similar to (a), that lets
 *        you write "pos = expr;"
 *     c) An operator that allows casting from your type to rcl::exp.
 *        This allows you to call any functions that expect an
 *        rcl::exp as an argument.  For example, you can make
 *        assignments "expr = pos;" or call various rcl functions such
 *        as "rcl::toStringCompact(pos)"
 */
struct MyPos {
  int x,y,z;
  
  RCL_SYNC_FUNCTION(MyPos) {
    RCL_SYNC_FIELD( x );
    RCL_SYNC_FIELD( y );
    RCL_SYNC_FIELD( z );
  }
  RCL_DEFINE_OPERATORS(MyPos);
};

void usage(void) {
  cerr <<
    "usage: testRCLObject OPTIONS\n"
    "  -h or --help       Print this help\n";
  exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
  static char shortOptions[] = "h";
  static struct option longOptions[]={
    {"help",0,NULL,'h'},
    //{"config",1,NULL,'c'},
    {NULL,0,0,0}
  };

  while (1) {
    char optchar = getopt_long(argc,argv,shortOptions,longOptions,NULL);
    if (optchar == -1) break;

    switch (optchar) {
    case 'h':
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
  if (optind < argc) {
    cerr << argv[0] << ": too many arguments" << endl << endl;
    usage();
  }

  /* Here is an example of using an rcl::object.  You can assign to an
   * rcl::object from an rcl::exp (here, the exp is parsed from a
   * string).  You can also pass it as an argument to any function that
   * expects an rcl::exp (here, we call rcl::toStringCompact() on a
   * MyPos object). */

  MyPos pos = rcl::readFromString("{x=1,y=2,z=3}")[0];
  cout << "1 pos = {x=" << pos.x  << ",y=" << pos.y << ",z="
       << pos.z << "}" << endl;
  cout << "2 pos = " << rcl::toStringCompact(pos) << endl;

  cout << endl
       << "pretty-printed form:" << endl
       << (rcl::exp) pos << endl;

}

/***************************************************************************
 * REVISION HISTORY:
 * $Log: testRCLObject.cc,v $
 * Revision 1.7  2004/06/18 19:35:32  dom
 * Removed new test-case now that it works properly again
 *
 * Revision 1.6  2004/06/18 19:24:39  trey
 * fixed a broken conversion
 *
 * Revision 1.5  2004/06/18 18:43:17  dom
 * Elaborated on the broken test to show how an explicit readFrom will succeed
 *
 * Revision 1.4  2004/06/18 17:43:12  dom
 * Added two more tests
 *
 * Revision 1.3  2004/05/12 22:20:14  trey
 * fixed for updates of RCLObject.h
 *
 * Revision 1.2  2004/05/12 19:26:41  trey
 * updated for changes in RCLObject.h
 *
 * Revision 1.1  2004/05/04 17:45:15  trey
 * initial check-in
 *
 *
 ***************************************************************************/
