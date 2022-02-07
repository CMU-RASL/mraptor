/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.4 $  $Author: dom $  $Date: 2004/04/28 17:15:21 $
 *  
 * @file    safeSystem.h
 * @brief   No brief
 ***************************************************************************/

#ifndef INCsafeSystem_h
#define INCsafeSystem_h

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "mrException.h"

// handy constants when using pipe()
#define READ_PIPE (0)
#define WRITE_PIPE (1)

// general exception for most problems in the Raptor_Daemon::execute() function
struct Execute_Exception : public MRException {
  Execute_Exception(void) : MRException() {}
  Execute_Exception(const string& _text,
	       bool _is_warning = false) :
    MRException(_text, _is_warning)
  {}
};

// thrown in case of errors in any of the functions defined here
struct System_Exception : public Execute_Exception {
  System_Exception(void) : Execute_Exception() {}
  System_Exception(const string& _text,
	       bool _is_warning = false) :
    Execute_Exception(_text, _is_warning)
  {}
};

pid_t safe_fork(void);
pid_t safe_wait3(int *status, int options, struct rusage *rusage);
int safe_dup2(int oldfd, int newfd);
int safe_pipe(int fd[2]);
FILE *safe_fdopen(int fd, const char *mode);
int safe_select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
		struct timeval *timeout);
int safe_execv(const char *path, char *const argv[]);
int safe_open(const char *pathname, int flags, mode_t mode);
int safe_openpty(int *amaster, int *aslave, char *name,
		 struct termios *termp, struct winsize *winp);
int safe_kill(pid_t pid, int sig);
int safe_ioctl(int d, int request, void *argp);
int safe_read(int fd, void *buf, size_t count);
int safe_close(int fd);
int safe_system(const string& command);
int safe_mkstemp(char *format);
int safe_chdir(const string& dir_name);

#endif // INCsafeSystem_h

/***************************************************************************
 * REVISION HISTORY:
 * $Log: safeSystem.h,v $
 * Revision 1.4  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.3  2003/03/17 02:36:37  trey
 * added safe_chdir()
 *
 * Revision 1.2  2003/02/24 22:09:33  trey
 * fixed reading of perl config files
 *
 * Revision 1.1  2003/02/24 16:29:52  trey
 * added "executing" config file ability
 *
 * Revision 1.8  2003/02/19 00:46:51  trey
 * implemented logging, updated format of stdout responses, improved cleanup
 *
 * Revision 1.7  2003/02/16 02:31:10  trey
 * full initial command set now implemented
 *
 * Revision 1.6  2003/02/15 05:45:40  trey
 * added stdoutBuffer
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
