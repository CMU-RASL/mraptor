/* $Revision: 1.5 $

   $Date: 2003/03/12 15:53:05 $

   $Author: trey $*/
/** @file test_runner.cc

  (c) Copyright 2003 CMU. All rights reserved.
*/
#include "raptorDaemon.h"

int main(int argc, char * argv[]) {
  Raptor_Daemon run;
  run.run(argc, argv);
};

/*REVISION HISTORY

  $Log: mraptord.cc,v $
  Revision 1.5  2003/03/12 15:53:05  trey
  moved argv parsing to raptorDaemon.cc

  Revision 1.4  2003/03/10 05:35:11  trey
  added get_clock command and ability of daemons to connect to each other

  Revision 1.3  2003/02/20 15:21:57  trey
  switched to get_config/set_config commands, added exit_time and last_stdout_time fields in status, also cleaned out some old stuff

  Revision 1.2  2003/02/16 06:20:38  trey
  minor fixes, added -p command-line option, prompting now off by default

  Revision 1.1  2003/02/13 22:41:30  trey
  renamed files

  Revision 1.2  2003/02/05 21:41:36  trey
  added makefile, a bit of cleanup

  Revision 1.1  2003/02/05 03:57:54  trey
  initial atacama check-in
*/
