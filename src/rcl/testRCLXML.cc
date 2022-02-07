/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.8 $  $Author: dom $  $Date: 2004/04/28 17:15:21 $
 *  
 * PROJECT: Life in the Atacama
 *
 * @file    testRCLXML.cc
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

#include "RCLXML.h"

using namespace std;

void usage(void) {
  cerr <<
    "usage: testRCLXML <config_file>\n";
  exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
  string args;
  string config_file;
  for (int argi=1; argi < argc; argi++) {
    args = argv[argi];
    if (args[0] == '-') {
      if (args == "-h" || args == "--help") {
	usage();
      } else {
	cerr << "ERROR: unknown option " << args << endl << endl;
	usage();
      }
    } else {
      config_file = args;
    }
  }

  if (config_file == "") {
    cerr << "ERROR: not enough arguments" << endl << endl;
    usage();
  }

  try {
    rcl::exp expr = rcl::xml::readFromFile(config_file);
    cout << "----- pretty-printed parse tree" << endl;
    cout << expr << endl << endl;
    cout << "----- parse tree printed in xml form" << endl;
    string xmlString = rcl::xml::toString(expr);
    cout << xmlString << endl;
#if 0
    cout << "----- compact rcl form" << endl;
    cout << rcl::toStringCompact(expr) << endl << endl;
    cout << "----- verbose rcl reparsed from xml string" << endl;
    cout << rcl::xml::readFromString(xmlString) << endl;
#endif
  } catch (rcl::exception err) {
    cerr << "ERROR: " << err.text << endl;
    exit(EXIT_FAILURE);
  }

  return 0;
}

/***************************************************************************
 * REVISION HISTORY:
 * $Log: testRCLXML.cc,v $
 * Revision 1.8  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.7  2004/04/14 00:00:13  trey
 * updated documentation
 *
 * Revision 1.6  2004/04/13 21:12:38  trey
 * did some stress testing, fixed several obscure bugs
 *
 * Revision 1.5  2004/04/13 03:26:40  trey
 * added back some test code
 *
 * Revision 1.4  2004/04/13 03:21:11  trey
 * xml front end now in relatively stable form
 *
 * Revision 1.3  2004/04/12 19:25:47  trey
 * developed a more full-featured translation between xml and rcl
 *
 * Revision 1.2  2004/04/09 20:31:33  trey
 * major enhancement to rcl interface
 *
 * Revision 1.1  2004/04/09 00:49:03  trey
 * initial check-in
 *
 *
 ***************************************************************************/
