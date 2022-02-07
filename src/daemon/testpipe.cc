/***** tell emacs we use -*- c++ -*- style comments *****
 * $Revision: 1.5 $ $Author: dom $ $Date: 2004/04/28 18:58:53 $
 *
 * COPYRIGHT 2004, Carnegie Mellon University 
 *
 * PROJECT: Exploration Robotics (Life in the Atacama)
 *
 * MODULE:
 *
 * FILE: microraptor/daemon/testpipe.cc
 *
 * DESCRIPTION:
 *
 ********************************************************/

/* this file is example code for forking and using pipes for child
   stdin and stdout */

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>
#include <fstream>

#include "mrSafeSystem.h"

using namespace std;

ifstream *child_stdout = NULL;
ofstream *child_stdin = NULL;

void proc_stdin(void) {
  char buffer[1024];
  fgets(buffer, sizeof(buffer), stdin);
  (*child_stdin) << buffer;
  //fputs(buffer, child_stdin);
  //fflush(child_stdin);

  // new prompt
  cout << "> ";
  cout.flush();
}

void proc_child_stdout(void) {
  char buffer[1024];
  //fgets(buffer, sizeof(buffer), child_stdout);
  child_stdout->getline(buffer, sizeof(buffer));
  cout << "parent got: " << buffer << endl;

  // new prompt
  cout << "> ";
  cout.flush();
}

void child_proc_stdin(void) {
  char buffer[1024];
  fgets(buffer, sizeof(buffer), stdin);
  sleep(1);
  cout << "child got: " << buffer;
  //cerr << "child got: " << buffer;
  // cout.flush(); // not needed b/c line-buffered

#if 0
  // test code for changing buffering of cerr and stderr
  cerr << "this is cerr";
  //fputs("this is stderr", stderr);
  static int i=0;
  if ((i++ % 2) == 0) {
    //fputs("\n", stderr);
    cerr << endl;
  }
#endif
}

#define MAX(x,y) ((x) > (y) ? (x) : (y))

void listen_one_event() {
  fd_set rfds;
  int maxfd;

  /* Watch stdin (fd 0) to see when it has input. */
  int child_stdout_fd = child_stdout->rdbuf()->fd();

  FD_ZERO(&rfds);
  FD_SET(fileno(stdin), &rfds);
  maxfd = fileno(stdin);
  FD_SET(child_stdout_fd, &rfds);
  maxfd = MAX(maxfd,child_stdout_fd);

  // last arg 0 => no timeout
  if (safe_select(maxfd+1, &rfds, NULL, NULL, 0)) {
    //cout << "incoming data in select()" << endl;
    if (FD_ISSET(fileno(stdin), &rfds)) {
      proc_stdin();
    }
    if (FD_ISSET(child_stdout_fd, &rfds)) {
      proc_child_stdout();
    }
  }
}

void dofork(char *child_cmd) {
  int child_stdin_pipe[2];
  int child_stdout_pipe[2];

  safe_pipe(child_stdin_pipe);
  safe_pipe(child_stdout_pipe);

  int pid = safe_fork();
  if (0 == pid) {
    // child
    safe_dup2(child_stdin_pipe[READ_PIPE], fileno(stdin));
    safe_dup2(child_stdout_pipe[WRITE_PIPE], fileno(stdout));
    safe_dup2(child_stdout_pipe[WRITE_PIPE], fileno(stderr));

    // make stderr line buffered instead of unbuffered because
    //   stdout processing in parent assumes complete lines (incomplete
    //   lines cause it to hang)
    setvbuf(stderr, 0, _IOLBF, 0);
    // same for cerr
    cerr.unsetf(ios::unitbuf);
    // for some reason stdout is sometimes switched to fully buffered
    //   by the dup2() call.  turn it back to the standard line buffered
    setvbuf(stdout, 0, _IOLBF, 0);

    while (1) child_proc_stdin();
  } else {
    // parent
#if 0
    child_stdin = safe_fdopen(child_stdin_pipe[WRITE_PIPE], "w");
    child_stdout = safe_fdopen(child_stdout_pipe[READ_PIPE], "r");

    // make child_stdin line buffered instead of fully buffered
    setvbuf(child_stdin, 0, _IOLBF, 0);
#endif    

    child_stdin = new ofstream(child_stdin_pipe[WRITE_PIPE]);
    child_stdout = new ifstream(child_stdout_pipe[READ_PIPE]);
    child_stdin->rdbuf()->linebuffered(true);

    cout << "> ";
    cout.flush();

    while (1) listen_one_event();
  }
}

int main(int argc, char **argv) {
  dofork(argv[1]);
  return 0;
}
/***************************************************************
 * Revision History:
 * $Log: testpipe.cc,v $
 * Revision 1.5  2004/04/28 18:58:53  dom
 * Appended log directive
 * 
 ***************************************************************/
