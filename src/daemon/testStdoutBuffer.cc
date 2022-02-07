/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.2 $  $Author: dom $  $Date: 2004/04/28 17:15:21 $
 *  
 * @file    testStdoutBuffer.cc
 * @brief   No brief
 ***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <iostream>
using namespace std;

#include "stdoutBuffer.h"

int main(void) {
  Byte_Buffer bstdin(fileno(stdin), BUFSIZ);
  Line_Data ld;

  while (1) {
    if (bstdin.getline(&ld)) {
      cout << "got: " << ld.data << endl;
      cout << "line_ended = " << ld.line_ended << endl;
    }
    sleep(1);
  }

  return 0;
}

/***************************************************************************
 * REVISION HISTORY:
 * $Log: testStdoutBuffer.cc,v $
 * Revision 1.2  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.1  2003/03/09 22:50:01  trey
 * initial check-in of old test code, not sure if it still compiles, but possibly useful anyway
 *
 *
 ***************************************************************************/
