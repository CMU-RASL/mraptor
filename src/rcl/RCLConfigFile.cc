/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.15 $  $Author: trey $  $Date: 2005/06/24 18:01:32 $
 *  
 * @file    configFile.cc
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
#include <sys/wait.h>

#include <iostream>
#include <fstream>
#include <cstring>
#include <limits.h>

#include "RCLConfigFile.h"
#include "mrSafeSystem.h"
#include "RCLParser.h"

using namespace std;

namespace rcl {

  static exp exec_config_file(const string& file_name) {
    // clean out old temp files (the leading - indicates that we should
    // ignore an error in the rm call)
    safe_system("-rm -f /tmp/rclconfig*");
    
    char exec_stdout_file_name[PATH_MAX];
    char exec_stderr_file_name[PATH_MAX];
    strcpy(exec_stdout_file_name, "/tmp/rclconfigstdoutXXXXXX");
    strcpy(exec_stderr_file_name, "/tmp/rclconfigstderrXXXXXX");
    int exec_stdout_fd = safe_mkstemp(exec_stdout_file_name);
    safe_close( exec_stdout_fd );
    int exec_stderr_fd = safe_mkstemp(exec_stderr_file_name);
    safe_close( exec_stderr_fd );

    // go ahead and make these world-writable so that other users can
    // clean them up later (but don't hack me!)
    safe_system(string("chmod 666 ") + exec_stdout_file_name);
    safe_system(string("chmod 666 ") + exec_stderr_file_name);
    
    string prefix = (string::npos == file_name.find("/")) ? "./" : "";
    string command = prefix + file_name + " > " + exec_stdout_file_name
      + " 2> " + exec_stderr_file_name;
    int exit_value = system(command.c_str());
    if (0 != exit_value) {
      char errbuf[1024];
      // something went wrong: see if there is data on stderr
      ifstream exec_stderr_stream(exec_stderr_file_name);
      exec_stderr_stream.getline(errbuf, sizeof(errbuf));
      exec_stderr_stream.close();
      
      if (string(errbuf) != "") {
	callErrorHandler(string("exec of config file failed: ") + errbuf);
      } else {
	callErrorHandler("exec of config file failed");
      }
    }
    
    return readFromFile(exec_stdout_file_name);
  }
  
  exp loadConfigFile(const string& file_name) {
    // this is not very efficient, but it's nice and simple
    
    ifstream temp_in(file_name.c_str());
    if (!temp_in) {
      callErrorHandler(string("couldn't open ") + file_name + " for reading: "
		       + strerror(errno));
    }
    char shebangLineBuf[PATH_MAX];
    temp_in.getline(shebangLineBuf, sizeof(shebangLineBuf));
    temp_in.close();
    
    exp config_exp;
    if (shebangLineBuf[0] == '#' && shebangLineBuf[1] == '!') {
      config_exp = exec_config_file(file_name);
    } else {
      config_exp = readFromFile(file_name);
    }
    if (config_exp[1].defined()) {
      callErrorHandler("found extra junk in the config file after the first expression");
    }
    config_exp[0].setObjectNameRecurse("input");
    return config_exp[0];
  }

  // used to load a config file that is embedded in the binary as a string
  exp loadConfigFromString(const char* config_string, int num_bytes) {
    // clean out old temp files (the leading - indicates that we should
    // ignore an error in the rm call)
    safe_system("-rm -f /tmp/rclstringconfig*");

    // write the string to a temp file
    char config_string_file_name[PATH_MAX];
    strcpy(config_string_file_name, "/tmp/rclstringconfigXXXXXX");
    int config_string_fd = safe_mkstemp(config_string_file_name);

    // make it world-writable so that other users can clean it up later
    // (although this is a security hole).  also make it executable.
    safe_system(string("chmod 777 ") + config_string_file_name);

    if (0 == num_bytes) {
      num_bytes = strlen(config_string);
    }

    if (-1 == write(config_string_fd, config_string, num_bytes)) {
      callErrorHandler(string("write() call to temp file ")
		       + config_string_file_name
		       + " failed: " + strerror(errno));
    }
    close(config_string_fd);

    // load the temp file
    return loadConfigFile(config_string_file_name);
  }

}; // namespace rcl

/***************************************************************************
 * REVISION HISTORY:
 * $Log: RCLConfigFile.cc,v $
 * Revision 1.15  2005/06/24 18:01:32  trey
 * now properly close files opened by loadConfigFile()
 *
 * Revision 1.14  2004/07/17 18:31:16  trey
 * added loadConfigFromString() function
 *
 * Revision 1.13  2004/07/06 15:35:12  trey
 * added optional memory tracking and plugged some memory leaks
 *
 * Revision 1.12  2004/06/10 22:46:43  trey
 * changed default behavior for RCL so that it prints an error before throwing an exception; error handling is now configurable using setErrorHandler()
 *
 * Revision 1.11  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.10  2004/04/15 18:20:31  trey
 * removed debugging line
 *
 * Revision 1.9  2004/04/15 17:37:56  trey
 * made config temp files world-writable to avoid confusing warnings
 *
 * Revision 1.8  2004/04/15 14:11:04  trey
 * fixed to throw rcl::exception as opposed to MRException
 *
 * Revision 1.7  2004/04/09 20:31:33  trey
 * major enhancement to rcl interface
 *
 * Revision 1.6  2003/11/20 02:37:03  trey
 * obsoleted IFSTREAM_FROM_FD macro; we no longer need to generate an ifstream from an fd
 *
 * Revision 1.5  2003/11/15 17:53:40  brennan
 * Split up CYGWIN #define into more reasonable/understandable chunks.  Completed move to FIRE build system.
 *
 * Revision 1.4  2003/08/19 19:22:51  brennan
 * *** empty log message ***
 *
 * Revision 1.3  2003/08/19 16:03:15  brennan
 * Conflict resolution.
 *
 * Revision 1.2  2003/08/08 15:29:27  brennan
 * Attempted to make microraptor compile:
 *   (1) on Cygwin and
 *   (2) under gcc 3.2
 * This attempt has not yet succeeded
 *
 * Revision 1.1  2003/03/28 17:38:15  trey
 * rearranged locations of files so that the atacama project can use rcl without causing namespace conflicts
 *
 * Revision 1.6  2003/03/14 23:20:30  mwagner
 * added ability to ignore errors with safe_system by prefacing a command with -
 *
 * Revision 1.5  2003/03/12 18:04:15  trey
 * load_config_file no longer returns a vector; instead it returns an exp like you would expect
 *
 * Revision 1.4  2003/03/12 17:57:18  trey
 * we now correctly signal an error if there is extra data after the first rcl expression in the config file
 *
 * Revision 1.3  2003/02/25 23:44:50  trey
 * rewrote exec_config_file without fork() in order to avoid mysterious X crash
 *
 * Revision 1.2  2003/02/24 22:09:33  trey
 * fixed reading of perl config files
 *
 * Revision 1.1  2003/02/24 16:29:52  trey
 * added "executing" config file ability
 *
 *
 ***************************************************************************/
