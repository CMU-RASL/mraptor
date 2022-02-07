/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.12 $  $Author: trey $  $Date: 2005/06/24 17:58:57 $
 *  
 * @file    safeSystem.cc
 * @brief   No brief
 ***************************************************************************/
#ifdef DEBUG_DEFUNCT
#include <sys/types.h>
#include <sys/resource.h>
#endif
#include <sys/wait.h>
#ifdef REDHAT6_2
#include <signal.h>
#endif
#ifdef CYGWIN_HEADERS
#include <sys/signal.h>
#include <sys/ioctl.h>
#endif
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <pty.h>

#include "mrSafeSystem.h"

using namespace std;

static void mrss_error(const string& text)
{
  cerr << "ERROR: mrSafeSystem: " << text << endl;
  throw System_Exception(text);
}

pid_t safe_fork(void) {
  int ret = fork();
  if (-1 == ret) {
    mrss_error(string("fork failed: ") + strerror(errno));
  }
  return ret;
}

pid_t safe_wait3(int *status, int options, struct rusage *rusage) {

  int ret = wait3(status, options, rusage);

#ifdef DEBUG_DEFUNCT
  // We can't print every time, since that prints out a _lot_ of data,
  // and the bug is extremely intermittent.  Thus, only print when the
  // result is different than before.  Since ret is the PID of the
  // child that's been waited on, this shouldn't miss anything...
  static int oldRet = -2;
  static int oldStatus = 0;
  static int oldOptions = 0;

  if(oldRet != ret || (status != NULL && oldStatus != *status) || oldOptions != options) {
    fprintf(stderr, "wait3: ret=%d, options=(WNOHANG:%d, WUNTRACED:%d)", ret, options & WNOHANG, options & WUNTRACED);
    if(status != NULL)
      fprintf(stderr, ", status=(WIFEXITED:%d, WEXITSTATUS:0x%X, WIFSIGNALED:%d, WTERMSIG:%d, WIFSTOPPED:%d, WSTOPSIG:%d)",
              WIFEXITED(*status), WIFEXITED(*status) ? WEXITSTATUS(*status) : 0, WIFSIGNALED(*status), WIFSIGNALED(*status) ? WTERMSIG(*status) : 0, options & WUNTRACED ? WIFSTOPPED(*status) : -1, options & WUNTRACED && WIFSTOPPED(*status) ? WSTOPSIG(*status) : 0);
    if(-1 == ret || 0 == ret) {
      fprintf(stderr, ", errno: %d (ECHILD: %d, EINTR: %d)", errno, ECHILD, EINTR);
    }
    fprintf(stderr, "\n");
    // Let's not worry about rusage just yet. :-)
    oldRet = ret;
    oldStatus = status != NULL ? *status : oldStatus;
    oldOptions = options;
  } // End if something has changed
#endif

  if (-1 == ret
      && !( (options & WNOHANG) && ECHILD == errno)) {
    mrss_error(string("wait3 failed: ") + strerror(errno));
  }
  return ret;
}

int safe_dup2(int oldfd, int newfd) {
  int ret = dup2(oldfd, newfd);
  if (-1 == ret) {
    mrss_error(string("dup2 failed: ") + strerror(errno));
  }
  return ret;
}

int safe_pipe(int fd[2]) {
  int ret = pipe(fd);
  if (-1 == ret) {
    mrss_error(string("pipe failed: ") + strerror(errno));
  }
  return ret;
}

FILE *safe_fdopen(int fd, const char *mode) {
  FILE *ret = fdopen(fd, mode);
  if (NULL == ret) {
    mrss_error(string("fdopen failed: ") + strerror(errno));
  }
  return ret;
}

int safe_select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
		struct timeval *timeout)
{
  int ret = select(n, readfds, writefds, exceptfds, timeout);
  if (-1 == ret) {
    mrss_error(string("select failed: ") + strerror(errno));
  }
  return ret;
}

int safe_execv(const char *path, char *const argv[]) {
  int ret = execv(path, argv);
  if (-1 == ret) {
    mrss_error(string("execv(\"") + path + "\") failed: " + strerror(errno));
  }
  return ret;
}

int safe_open(const char *pathname, int flags, mode_t mode) {
  int ret = open(pathname, flags, mode);
  if (-1 == ret) {
    mrss_error(string("open(\"") + pathname
	       + "\", ...) failed: " + strerror(errno));
  }
  return ret;
}

int safe_kill(pid_t pid, int sig) {
  int ret = kill(pid, sig);
  if (-1 == ret) {
    mrss_error(string("kill failed: ") + strerror(errno));
  }
  return ret;
}

int safe_ioctl(int d, int request, void *argp) {
  int ret = ioctl(d, request, argp);
#if 0
  string verbErr = "ioctl (mss) call: args: (";
  char foo[800];
  sprintf(foo, "%d, 0x%X, %p)\n", d, request, argp);
  verbErr += foo;
  cerr << verbErr << endl;
#endif
  if (-1 == ret) {
    mrss_error(string("ioctl failed: ") + strerror(errno));
  }
  return ret;
}

int safe_read(int fd, void *buf, size_t count) {
  int ret = read(fd, buf, count);
  if (-1 == ret) {
    if(errno == EAGAIN) return ret;
    mrss_error(string("read failed: ") + strerror(errno));
  }
  return ret;
}

int safe_close(int fd) {
  int ret = close(fd);
  if (-1 == ret) {
    mrss_error(string("close failed: ") + strerror(errno));
  }
  return ret;
}

int safe_system(const string& command_in) {
  bool ignore_return_val = (command_in.substr(0,1) == "-");
  string command;
  if (ignore_return_val) {
    command = command_in.substr(1) + " 2> /dev/null";
  } else {
    command = command_in;
  }

  int ret = system(command.c_str());
  // Hmm.  So this fixes the "mkdir failed" problem on systems where I 
  // saw it.  However, the manpages for system() vary on whether the return
  // status is in the format specified in man -S2 wait or not.  This new
  // code assumes that it is; we should verify that this works everywhere
  // (was it a bug in the manpage, or did the behavior of system() change?)
  //if (0 != ret && !ignore_return_val) {
  if(!ignore_return_val
     && WIFEXITED(ret) 
     && 0 != WEXITSTATUS(ret)) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", ret);
    mrss_error(string("system(\"") + command + "\") failed with exit value "
	       + buf);
  }
  return ret;
}

int safe_mkstemp(char *format) {
  int ret = mkstemp(format);
  if (-1 == ret) {
    mrss_error(string("mkstemp(\"") + format + "\") failed: " + strerror(errno));
  }
  return ret;
}

int safe_chdir(const string& dir_name) {
  int ret = chdir(dir_name.c_str());
  if (-1 == ret) {
    mrss_error(string("chdir(\"") + dir_name + "\") failed: " + strerror(errno));
  }
  return ret;
}

int safe_fcntl(int fd, int cmd, long arg) {
  int ret = fcntl(fd, cmd, arg);
  if (-1 == ret) {
    mrss_error(string("fcntl failed: ") + strerror(errno));
  }
  return ret;
}

int safe_fclose(FILE* f) {
  int ret = fclose(f);
  if (-1 == ret) {
    mrss_error(string("fclose failed: ") + strerror(errno));
  }
  return ret;
}

/***************************************************************************
 * REVISION HISTORY:
 * $Log: mrSafeSystem.cc,v $
 * Revision 1.12  2005/06/24 17:58:57  trey
 * now print errors in addition to throwing an exception; added safe_fclose()
 *
 * Revision 1.11  2005/06/24 00:16:37  trey
 * modified mrSafeSystem calls to print an error message before printing an exception, makes debugging easier
 *
 * Revision 1.10  2004/12/01 06:58:47  brennan
 * Added platform support for RH 6.2; mostly, headers are in different places.
 * Note that this will only work on a RH 6.2 system hacked to use a newer
 * gcc than ships with 6.2 by default (tested on Valerie with gcc 3.2.3).
 *
 * Revision 1.9  2004/07/17 18:30:02  trey
 * changed to suppress warnings to stderr in safe_system() calls when ignore_return_val is true
 *
 * Revision 1.8  2004/06/10 20:55:12  trey
 * added build of libmrcommonclean.a that does not depend on -lutil
 *
 * Revision 1.7  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.6  2003/11/15 17:53:40  brennan
 * Split up CYGWIN #define into more reasonable/understandable chunks.  Completed move to FIRE build system.
 *
 * Revision 1.5  2003/10/28 18:07:03  brennan
 * Fixed mkdir failure bug; not tested on all systems - see comment in mrSafeSystem.cc:148
 *
 * Revision 1.4  2003/08/27 21:02:57  trey
 * added safe_fcntl()
 *
 * Revision 1.3  2003/08/08 19:19:58  trey
 * made changes to enable compilation under g++ 3.2
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
