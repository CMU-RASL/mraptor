/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.1 $  $Author: trey $  $Date: 2004/06/10 22:46:43 $
 *  
 * PROJECT: Life in the Atacama
 *
 * @file    RCLException.cc
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

#include "RCLException.h"

using namespace std;

namespace rcl {
  error_handler handler = onErrorPrintTextAndThrowException;
}

/***************************************************************************
 * REVISION HISTORY:
 * $Log: RCLException.cc,v $
 * Revision 1.1  2004/06/10 22:46:43  trey
 * changed default behavior for RCL so that it prints an error before throwing an exception; error handling is now configurable using setErrorHandler()
 *
 *
 ***************************************************************************/
