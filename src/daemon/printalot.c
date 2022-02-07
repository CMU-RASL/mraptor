/***** tell emacs we use -*- c++ -*- style comments *****
 * $Revision: 1.4 $ $Author: dom $ $Date: 2004/04/28 18:58:53 $
 *
 * COPYRIGHT 2004, Carnegie Mellon University 
 *
 * PROJECT: Exploration Robotics (Life in the Atacama)
 *
 * MODULE:
 *
 * FILE: microraptor/daemon/printalot.c
 *
 * DESCRIPTION:
 *
 ********************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

void usage(void) {
  fprintf(stderr, "usage: printalot <delay_in_seconds>\n");
  exit(1);
}

int main(int argc, char **argv) {
  int i = 0;
  int argi;
  long delay_in_usecs = -1;
  
  for (argi=1; argi < argc; argi++) {
    if ('-' == argv[argi][0]) {
      if (0 == strcmp("-h", argv[argi])
	  || 0 == strcmp("--help", argv[argi])) {
	usage();
      } else {
	fprintf(stderr, "unknown option %s\n\n", argv[argi]);
	usage();
      }
    } else if (-1 == delay_in_usecs) {
      delay_in_usecs = (long) (1.0e+6 * atof(argv[argi]));
    } else {
      fprintf(stderr, "too many arguments\n\n");
      usage();
    }
  }
  if (-1 == delay_in_usecs) {
    fprintf(stderr, "not enough arguments\n\n");
    usage();
  }

  while (1) {
    printf("printalot: %d -------------------------------------\n", i++);
    usleep(delay_in_usecs);
  }
}
/***************************************************************
 * Revision History:
 * $Log: printalot.c,v $
 * Revision 1.4  2004/04/28 18:58:53  dom
 * Appended log directive
 * 
 ***************************************************************/
