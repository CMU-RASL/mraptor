/********** tell emacs we use -*- c++ -*- style comments *******************
 * $Revision: 1.6 $  $Author: trey $  $Date: 2004/07/06 15:35:12 $
 *
 * PROJECT:      Distributed Robotic Agents
 * DESCRIPTION:  Make available some functions and globals defined in
 *               RCL.y, RCL.l
 *
 * (c) Copyright 1999 CMU. All rights reserved.
 ***************************************************************************/

#ifndef INCRCLInternal_h
#define INCRCLInternal_h

/***************************************************************************
 * INCLUDES
 ***************************************************************************/

#include "RCL.h"

namespace rcl {

  extern char lexCurrentFile[1024];
  extern int lexLineNo;

  // RCLGrammar.l
  void lexSwitchToString(const string& s);
  void lexSwitchToFile(FILE *file);
  void lexBufferCleanup(void);
  
  char* strdup_new(const char* s);

}; // namespace rcl

#define RCL_YY_PREFIX(NAME) rclyy##NAME

// set to 1 for a bison parser trace
extern int RCL_YY_PREFIX(debug);
// point this to the input file
extern FILE *RCL_YY_PREFIX(in);
extern rcl::exp *RCL_YY_PREFIX(Tree); // RCLGrammar.y - the output global

void RCL_YY_PREFIX(error) (const char *s);
int  RCL_YY_PREFIX(lex)   (void);
int  RCL_YY_PREFIX(parse) (void);

// flex's -P option to change the name prefix doesn't work perfectly: we
// need to explicitly define this alternate-name version of yyterminate
#ifndef rclyyterminate
#define rclyyterminate() return YY_NULL
#endif

#endif // INCRCLInternal_h

/***************************************************************************
 * REVISION HISTORY:
 * $Log: RCLInternal.h,v $
 * Revision 1.6  2004/07/06 15:35:12  trey
 * added optional memory tracking and plugged some memory leaks
 *
 * Revision 1.5  2004/06/09 22:50:08  trey
 * made it possible to switch the name prefix from the default "yy" when running flex
 *
 * Revision 1.4  2004/04/09 20:31:33  trey
 * major enhancement to rcl interface
 *
 * Revision 1.3  2003/02/16 06:19:45  trey
 * fixed reading from file after reading from string
 *
 * Revision 1.2  2003/02/05 20:23:20  trey
 * implemented command syntax and switched error behavior to throw exceptions
 *
 * Revision 1.1  2003/02/05 03:57:54  trey
 * initial atacama check-in
 *
 * Revision 1.2  2002/09/24 21:50:24  trey
 * rebuilt RCL from scratch to support nested vectors and hashes
 *
 * Revision 1.1  2002/07/24 21:22:00  trey
 * initial FIRE checkin
 *
 * Revision 1.1  1999/11/08 15:35:00  trey
 * major overhaul
 *
 * Revision 1.1  1999/11/03 19:31:30  trey
 * initial check-in
 *
 ***************************************************************************/
