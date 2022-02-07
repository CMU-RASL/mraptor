/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.2 $  $Author: trey $  $Date: 2004/08/02 18:57:44 $
 *  
 * PROJECT: Life in the Atacama
 *
 * @file    testPrintToStderr.cc
 * @brief   No brief
 ***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <iostream>

using namespace std;

int main(int argc, char** argv) {
  while (1) {
    cout << "> ";
    cout.flush();
    sleep(1);
    cerr << "printing to stderr" << endl;
  }
}

/***************************************************************************
 * REVISION HISTORY:
 * $Log: testPrintToStderr.cc,v $
 * Revision 1.2  2004/08/02 18:57:44  trey
 * sigh... tried to fix stderr output again
 *
 * Revision 1.1  2004/07/29 22:45:24  trey
 * fixed flaws in stderr buffering
 *
 *
 ***************************************************************************/
