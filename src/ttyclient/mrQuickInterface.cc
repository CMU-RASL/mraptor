/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.7 $  $Author: brennan $  $Date: 2006/06/04 05:27:03 $
 *  
 * PROJECT: Life in the Atacama
 *
 * @file    mrQuickInterface.cc
 * @brief   No brief
 ***************************************************************************/

/***************************************************************************
 * INCLUDES
 ***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include <iostream>
#include <fstream>
#include <cstring>

#if defined (REDHAT6_2) || defined(GCC2_96)
#define _USE_BSD
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>
#endif

#include "mrQuickInterface.h"
#include "RCLException.h"

using namespace std;

/***************************************************************************
 * FUNCTIONS
 ***************************************************************************/

namespace microraptor {

  static int quick_system(const string& command_in) {
#if 0
    bool ignore_return_val = (command_in.substr(0,1) == "-");
    string command;
    if (ignore_return_val) {
      command = command_in.substr(1);
    } else {
      command = command_in;
    }
#endif
    string command = command_in;
    bool ignore_return_val = true;

    int ret = system(command.c_str());
    // Hmm.  So this fixes the "mkdir failed" problem on systems where I 
    // saw it.  However, the manpages for system() vary on whether the return
    // status is in the format specified in man -S2 wait or not.  This new
    // code assumes that it is; we should verify that this works everywhere
    // (was it a bug in the manpage, or did the behavior of system() change?)
    //if (0 != ret && !ignore_return_val) {
    if(!ignore_return_val) {
      if (WIFEXITED(ret) && 0 != WEXITSTATUS(ret)) {
	char buf[32];
	snprintf(buf, sizeof(buf), "%d", ret);
	rcl::callErrorHandler
	  ("send_mraptord_command",
	   string("system(\"") + command
	   + "\") exited with non-zero exit status " + buf);
      } else if (WIFSIGNALED(ret)) {
	char buf[32];
	snprintf(buf, sizeof(buf), "%d", WSTOPSIG(ret));
	rcl::callErrorHandler
	  ("send_mraptord_command",
	   string("system(\"") + command
	   + "\") was killed by a signal " + buf);
      }
    }

    return ret;
  }

  void send_mraptord_command(const std::string& cmd,
			     int debug_level)
  {
    int uid = getuid();
    int euid = geteuid();
    if (uid != euid) {
      printf("WARNING: send_mraptord_command: uid=%d and euid=%d are not the same;\n"
	     "  redirect of mrterm output will probably fail (call seteuid() first)\n",
	     uid, euid);
    }

    char fname[] = "/tmp/mrquickXXXXXX";
    int fid = mkstemp(fname);
    if (-1 == fid) {
      rcl::callErrorHandler("send_mraptord_command","mkstemp() failed");
    }
    close(fid);

    string centralhost_arg = "";
    const char* mraptor_centralhost = getenv("MRAPTOR_CENTRALHOST");
    if (NULL != mraptor_centralhost) {
      centralhost_arg = string("--centralhost ") + mraptor_centralhost;
    }

    const char* mrterm_binary_name = getenv("MRTERM_BINARY");
    if (NULL == mrterm_binary_name) {
      mrterm_binary_name = "mrterm";
    }

    string run_string = string(mrterm_binary_name)
      + " " + centralhost_arg
      + " -e '" + cmd + "'"
      + " -w"
      + " >& " + fname;
    if (debug_level >= DEBUG_QUIET) {
      cout << "send_mraptord_command: executing [" << run_string << "]" << endl;
    }
    quick_system(run_string);

    char lbuf[1024];
    ifstream cmd_output(fname);
    if (!cmd_output) {
      rcl::callErrorHandler
	("send_mraptord_command",
	 string("couldn't open temp output file ") + fname
	 + " for reading: " + strerror(errno));
    }

    int num_lines = 0;
    while (!cmd_output.eof()) {
      cmd_output.getline(lbuf,sizeof(lbuf));
      if (0 == strlen(lbuf)) continue;
      if (0 != strcmp( lbuf, "response 0 ok" )) {
	rcl::callErrorHandler
	  ("send_mraptord_command",
	   string("while running [") + run_string
	   + "]: " + " got unexpected output [" + lbuf + "]");
      }
      num_lines++;
    }

    if (0 == num_lines) {
	rcl::callErrorHandler
	  ("send_mraptord_command",
	   string("while running [") + run_string
	   + "]: " + " didn't get expected 'response 0 ok'");
    }

    if (debug_level >= DEBUG_VERBOSE) {
      cout << "send_mraptord_command: mraptord returned 'ok'" << endl;
    }

    quick_system(string("rm -f ") + fname);
  }

}


/***************************************************************************
 * REVISION HISTORY:
 * $Log: mrQuickInterface.cc,v $
 * Revision 1.7  2006/06/04 05:27:03  brennan
 * First pass at modularized RPMs, and changes to build under gcc 2.96.
 *
 * Revision 1.6  2006/05/25 20:59:02  trey
 * added check for obscure problem when uid and euid are different
 *
 * Revision 1.5  2005/12/07 18:15:56  trey
 * committing dom's changes for gcc 3.4.0
 *
 * Revision 1.4  2005/06/01 00:46:34  trey
 * added error checking
 *
 * Revision 1.3  2004/12/01 06:58:48  brennan
 * Added platform support for RH 6.2; mostly, headers are in different places.
 * Note that this will only work on a RH 6.2 system hacked to use a newer
 * gcc than ships with 6.2 by default (tested on Valerie with gcc 3.2.3).
 *
 * Revision 1.2  2004/08/29 20:11:57  trey
 * introduced a way to not suppress all mrterm output when debugging
 *
 * Revision 1.1  2004/08/21 19:39:45  trey
 * initial check-in
 *
 *
 ***************************************************************************/
