/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.1 $  $Author: trey $  $Date: 2004/06/10 20:55:12 $
 *  
 * @file    safeSystem.cc
 * @brief   No brief
 ***************************************************************************/
#ifdef CYGWIN_HEADERS
#include <sys/signal.h>
#include <sys/ioctl.h>
#endif
#include <assert.h>
#include <cstring>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <iostream>
#include <sys/wait.h>
#include <fcntl.h>
#include <pty.h>

#include "mrSafeSystem.h"

using namespace std;

int safe_openpty(int *amaster, int *aslave, char *name,
		 struct termios *termp, struct winsize *winp)
{
  int ret = openpty(amaster, aslave, name, termp, winp);
  if (-1 == ret) {
    throw System_Exception(string("openpty failed: ") + strerror(errno));
  }
  return ret;
}

/***************************************************************************
 * REVISION HISTORY:
 * $Log: mrSafeSystemUtil.cc,v $
 * Revision 1.1  2004/06/10 20:55:12  trey
 * added build of libmrcommonclean.a that does not depend on -lutil
 *
 *
 ***************************************************************************/
