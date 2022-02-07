/***** tell emacs we use -*- c++ -*- style comments *****
 * $Revision: 1.3 $ $Author: dom $ $Date: 2004/04/28 18:58:53 $
 *
 * COPYRIGHT 2004, Carnegie Mellon University 
 *
 * PROJECT: Exploration Robotics (Life in the Atacama)
 *
 * MODULE:
 *
 * FILE: microraptor/daemon/slashr.c
 *
 * DESCRIPTION:
 *
 ********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* This /should/ print out:

   Alas, poor Yorick.
   He was a slob
   Let me count his yuckinesses...
   ##...th line

   (## incrementing)

   Chris' off-by-one \r implementation whacks the yuckinesses line.
*/
int main(void) {
  int i;
  fprintf(stderr, "Alas, poor Yorick.\nHe was a slob.\n");
  fprintf(stderr, "Let me count his yuckinesses...\n");
  fprintf(stderr, "stealth line\r");
  for(i = 0; i > -1; i++) {
    if(i % 10 == 0) {
      fprintf(stderr, "%d...\n", i);
    } else {
      fprintf(stderr, "%d...\r", i);
    }
    sleep(1);
  }
  return 1;
}
/***************************************************************
 * Revision History:
 * $Log: slashr.c,v $
 * Revision 1.3  2004/04/28 18:58:53  dom
 * Appended log directive
 * 
 ***************************************************************/
