/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.7 $  $Author: dom $  $Date: 2004/04/28 17:15:21 $
 *  
 * @file    safeSystem.cc
 * @brief   No brief
 ***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <iostream>
#include <sys/wait.h>
#include <fcntl.h>
#include <pty.h>

#ifdef CYGWIN_HEADERS
// This is overkill, but I'm in a hurry.
// Probably only need ioctl and signal for kill() and ioctl()
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#endif

#include "safeSystem.h"

using namespace std;

pid_t safe_fork(void) {
  int ret = fork();
  if (-1 == ret) {
    throw System_Exception(string("fork failed: ") + strerror(errno));
  }
  return ret;
}

pid_t safe_wait3(int *status, int options, struct rusage *rusage) {
  int ret = wait3(status, options, rusage);
  if (-1 == ret
      && !( (options & WNOHANG) && ECHILD == errno)) {
    throw System_Exception(string("wait3 failed: ") + strerror(errno));
  }
  return ret;
}

int safe_dup2(int oldfd, int newfd) {
  int ret = dup2(oldfd, newfd);
  if (-1 == ret) {
    throw System_Exception(string("dup2 failed: ") + strerror(errno));
  }
  return ret;
}

int safe_pipe(int fd[2]) {
  int ret = pipe(fd);
  if (-1 == ret) {
    throw System_Exception(string("pipe failed: ") + strerror(errno));
  }
  return ret;
}

FILE *safe_fdopen(int fd, const char *mode) {
  FILE *ret = fdopen(fd, mode);
  if (NULL == ret) {
    throw System_Exception(string("fdopen failed: ") + strerror(errno));
  }
  return ret;
}

int safe_select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
		struct timeval *timeout)
{
  int ret = select(n, readfds, writefds, exceptfds, timeout);
  if (-1 == ret) {
    throw System_Exception(string("select failed: ") + strerror(errno));
  }
  return ret;
}

int safe_execv(const char *path, char *const argv[]) {
  int ret = execv(path, argv);
  if (-1 == ret) {
    throw System_Exception
      (string("execv(\"") + path + "\") failed: " + strerror(errno));
  }
  return ret;
}

int safe_open(const char *pathname, int flags, mode_t mode) {
  int ret = open(pathname, flags, mode);
  if (-1 == ret) {
    throw System_Exception(string("open(\"") + pathname
			   + "\", ...) failed: " + strerror(errno));
  }
  return ret;
}

int safe_openpty(int *amaster, int *aslave, char *name,
		 struct termios *termp, struct winsize *winp)
{
  int ret = openpty(amaster, aslave, name, termp, winp);
  if (-1 == ret) {
    throw System_Exception(string("openpty failed: ") + strerror(errno));
  }
  return ret;
}

int safe_kill(pid_t pid, int sig) {
  int ret = kill(pid, sig);
  if (-1 == ret) {
    throw System_Exception(string("kill failed: ") + strerror(errno));
  }
  return ret;
}

int safe_ioctl(int d, int request, void *argp) {
  int ret = ioctl(d, request, argp);
  string verbErr = "ioctl (ss) call: args: (";
  char foo[800];
  sprintf(foo, "%d, %d, %s)\n", d, request, argp);
  verbErr += foo;
  cerr << verbErr << endl;
  if (-1 == ret) {
    throw System_Exception(string("ioctl failed: ") + strerror(errno));
  }
  return ret;
}

int safe_read(int fd, void *buf, size_t count) {
  int ret = read(fd, buf, count);
  if (-1 == ret) {
    throw System_Exception(string("read failed: ") + strerror(errno));
  }
  return ret;
}

int safe_close(int fd) {
  int ret = close(fd);
  if (-1 == ret) {
    throw System_Exception(string("close failed: ") + strerror(errno));
  }
  return ret;
}

int safe_system(const string& command_in) {
  bool ignore_return_val = (command_in.substr(0,1) == "-");
  string command = ignore_return_val ? command_in.substr(1) : command_in;
  int ret = system(command.c_str());
  if (0 != ret && !ignore_return_val) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", ret);
    throw System_Exception
      (string("system(\"") + command + "\") failed with exit value "
       + buf);
  }
  return ret;
}

int safe_mkstemp(char *format) {
  int ret = mkstemp(format);
  if (-1 == ret) {
    throw System_Exception
      (string("mkstemp(\"") + format + "\") failed:" + strerror(errno));
  }
  return ret;
}

int safe_chdir(const string& dir_name) {
  int ret = chdir(dir_name.c_str());
  if (-1 == ret) {
    throw System_Exception
      (string("chdir(\"") + dir_name + "\") failed:" + strerror(errno));
  }
  return ret;
}

/***************************************************************************
 * REVISION HISTORY:
 * $Log: safeSystem.cc,v $
 * Revision 1.7  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.6  2003/11/15 17:53:40  brennan
 * Split up CYGWIN #define into more reasonable/understandable chunks.  Completed move to FIRE build system.
 *
 * Revision 1.5  2003/08/08 15:29:27  brennan
 * Attempted to make microraptor compile:
 *   (1) on Cygwin and
 *   (2) under gcc 3.2
 * This attempt has not yet succeeded
 *
 * Revision 1.4  2003/03/17 02:36:37  trey
 * added safe_chdir()
 *
 * Revision 1.3  2003/03/14 23:20:13  mwagner
 * added ability to ignore errors with safe_system by prefacing a command with -
 *
 * Revision 1.2  2003/02/24 22:09:33  trey
 * fixed reading of perl config files
 *
 * Revision 1.1  2003/02/24 16:29:52  trey
 * added "executing" config file ability
 *
 * Revision 1.9  2003/02/19 00:46:51  trey
 * implemented logging, updated format of stdout responses, improved cleanup
 *
 * Revision 1.8  2003/02/16 02:31:10  trey
 * full initial command set now implemented
 *
 * Revision 1.7  2003/02/15 05:45:40  trey
 * added stdoutBuffer
 *
 * Revision 1.6  2003/02/14 21:54:10  trey
 * made status reporting mostly work
 *
 * Revision 1.5  2003/02/14 18:30:33  trey
 * many fixes, started implementation of kill, ipc, get commands
 *
 * Revision 1.4  2003/02/14 05:57:13  trey
 * child stdin/stdout now have correct buffering properties
 *
 * Revision 1.3  2003/02/14 03:28:12  trey
 * fixed some pipe issues
 *
 * Revision 1.2  2003/02/14 00:12:33  trey
 * added safe_wait3
 *
 * Revision 1.1  2003/02/13 23:21:38  trey
 * moved Execute_Exception to safeSystem.h
 *
 *
 ***************************************************************************/
