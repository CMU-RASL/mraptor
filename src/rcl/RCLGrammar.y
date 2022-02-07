/****************************************************************************
 * $Revision: 1.14 $  $Author: trey $  $Date: 2005/06/16 00:25:01 $
 *
 * PROJECT:      Distributed Robotic Agents
 * DESCRIPTION:  Bison(yacc) source grammar for parsing config files.
 *               Builds a parse tree.
 *
 * (c) Copyright 1999 CMU. All rights reserved.
 ***************************************************************************/

%{ /* C code section */

#define YYERROR_VERBOSE 1
#define YYDEBUG 1

#include <stdlib.h>	/* For malloc */
#include <stdio.h>
#include <string.h>
#include <assert.h>
/* #include <ctype.h> */

#include <vector>

#include "RCL.h"
#include "RCLInternal.h"

#if 0
  //#ifndef IRIX
extern void __yy_memcpy (char *to, char *from, int count);
#endif

using namespace std;

/*** EXPORTED DATA STRUCTURE ***/

rcl::exp *RCL_YY_PREFIX(Tree);

%} /* End C code section */

%start command

%union
                {
		  long			longVal;
		  bool			boolVal;
		  double		doubleVal;
		  char* 		stringVal;
	          rcl::exp*             expVal;
	          rcl::map*             mapVal;
	          rcl::vector*          vectorVal;
		}

/* Reserved words */
%token	STRING
%token	DOUBLE FLOAT
%token	BOOL UNSIGNED LONG INT SHORT CHAR
%token  NAMESPACE ARROW
%token  UNDEF

%token	<stringVal> IDENTIFIER STRINGVAL ENVIDENT
%token	<boolVal> BOOLVAL
%token  <doubleVal> DOUBLEVAL
%token  <longVal> LONGVAL

%type           <expVal>                command exp primExp
%type		<stringVal>		ident strExpr string
%type           <mapVal>                map mapBody
%type           <vectorVal>             vector vectorBody spaceVector

/* Tokens to use as composite types.  Created for internal use */
%token	ULONG USHORT UINT UCHAR

%%  /* -------------------------------------------------------------------- */
    /* ------------  Begin the rule section of the yacc/bison file -------- */
    /* -------------------------------------------------------------------- */

command 	:	spaceVector
		        {
			  RCL_YY_PREFIX(Tree) = new rcl::exp(rcl::lexCurrentFile, rcl::lexLineNo);
			  RCL_YY_PREFIX(Tree)->setValue(*$1);
			  delete $1;
			  RCL_YY_PREFIX(Tree)->setObjectNameRecurse("input");
			}
		;

exp             :       map
                        {
                          $$ = new rcl::exp(rcl::lexCurrentFile, rcl::lexLineNo);
			  $$->setValue(*$1);
			  delete $1;
			}
                |       vector
                        {
			  $$ = new rcl::exp(rcl::lexCurrentFile, rcl::lexLineNo);
			  $$->setValue(*$1);
			  delete $1;
			}
                |       primExp { $$ = $1; }
                ;

map             :       '{' mapBody '}' { $$ = $2; }
                |       '{' '}'
                        {
			  $$ = new rcl::map;
			}
                ;

mapBody         :       mapBody ',' ident ARROW exp
                        {
	                  $$ = $1;
                          $$->push_back($3,*$5);
			  delete [] $3;
			  delete $5;
                        }
                |       mapBody ',' ident '=' exp
                        {
	                  $$ = $1;
                          $$->push_back($3,*$5);
                          delete [] $3;
			  delete $5;
                        }
                |       ident ARROW exp
                        {
                          $$ = new rcl::map;
	                  $$->push_back($1,*$3);
			  delete [] $1;
			  delete $3;
                        }
               |       ident '=' exp /* can replace => with = */
                        {
                          $$ = new rcl::map;
	                  $$->push_back($1,*$3);
                          delete [] $1;
			  delete $3;
                        }
                ;

/* space-separated vector like "a b c" instead of comma-separated like
   "[a, b, c]".  a command is a spaceVector, to mimic shell syntax. */
spaceVector     :       spaceVector exp
                        {
	                  $$ = $1;
                          $$->push_back(*$2);
			  delete $2;
                        }
                |       /* NULL */
                        {
                          $$ = new rcl::vector;
                        }
                ;


vector          :       '[' vectorBody ']' { $$ = $2; }
                |       '[' ']'
                         {
			   $$ = new rcl::vector;
			 }
                ;


vectorBody      :       vectorBody ',' exp
                        {
	                  $$ = $1;
                          $$->push_back(*$3);
			  delete $3;
                        }
                |       exp
                        {
                          $$ = new rcl::vector;
                          $$->push_back(*$1);
			  delete $1;
                        }
                ;


primExp         :       DOUBLEVAL
                        {
			  $$ = new rcl::exp(rcl::lexCurrentFile, rcl::lexLineNo);
			  $$->setValue($1);
			}
	        |       LONGVAL
                        {
			  $$ = new rcl::exp(rcl::lexCurrentFile, rcl::lexLineNo);
			  $$->setValue($1);
			}
	        |       BOOLVAL
                        {
			  $$ = new rcl::exp(rcl::lexCurrentFile, rcl::lexLineNo);
			  $$->setValue($1);
			}
                |       UNDEF
                        {
			  $$ = new rcl::exp(rcl::lexCurrentFile, rcl::lexLineNo);
			}
                |       strExpr
                        {
			  $$ = new rcl::exp(rcl::lexCurrentFile, rcl::lexLineNo);
			  $$->setValue((string)$1);
			  delete [] $1;
			}
                ;

ident		:	IDENTIFIER { $$ = $1; }
		;

strExpr		:	strExpr '+' string
			{
			  int total_size = strlen($1)+strlen($3)+1;
			  $$ = new char[total_size];
			  snprintf($$,total_size,"%s%s",$1,$3);
			  free($1);
			  delete $3;
			}
		|	string { $$ = $1; }
		;

string		:	STRINGVAL	{ $$ = $1; }
                |       IDENTIFIER      { $$ = $1; }
		|	ENVIDENT
			{
			  char *val, errbuf[1024];
			  if (0 == (val = getenv($1))) {
			    snprintf(errbuf,sizeof(errbuf),
				     "unknown environment variable `%s'\n",
				    $1);
			    RCL_YY_PREFIX(error)(errbuf);
			    $$ = rcl::strdup_new("");
			  } else {
			    $$ = rcl::strdup_new(val);
			  }
                          free($1);
			}
                ;
%%
/* C code section */

;

/***************************************************************************
 * REVISION HISTORY:
 * $Log: RCLGrammar.y,v $
 * Revision 1.14  2005/06/16 00:25:01  trey
 * removed a bunch of obsolete USE_MEM_TRACKER ifdefs
 *
 * Revision 1.13  2004/11/25 05:23:12  brennan
 * Fixed some new/delete mismatches turned up by valgrind - needed delete []'s.
 *
 * Revision 1.12  2004/07/06 15:35:12  trey
 * added optional memory tracking and plugged some memory leaks
 *
 * Revision 1.11  2004/06/09 22:50:08  trey
 * made it possible to switch the name prefix from the default "yy" when running flex
 *
 * Revision 1.10  2004/04/12 19:25:10  trey
 * more api cleanup
 *
 * Revision 1.9  2004/04/09 20:31:33  trey
 * major enhancement to rcl interface
 *
 * Revision 1.8  2004/04/08 16:42:56  trey
 * overhauled RCLExp
 *
 * Revision 1.7  2003/08/19 16:03:15  brennan
 * Conflict resolution.
 *
 * Revision 1.6  2003/06/14 22:31:25  trey
 * added a new "undef" keyword which can be part of a vector/hash and is parsed into an undefined RCLExp
 *
 * Revision 1.5  2003/02/17 22:08:31  trey
 * incorporated bsellner fixes to RCL in such a way that it compiles under both bison 1.28 and 1.75
 *
 * Revision 1.4  2003/02/17 21:56:48  brennan
 * Made a few minor changes to allow compilation under Bison 1.75.
 *
 * Revision 1.3  2003/02/14 00:13:11  trey
 * added better support for checking the type of numeric constants
 *
 * Revision 1.2  2003/02/05 20:23:20  trey
 * implemented command syntax and switched error behavior to throw exceptions
 *
 * Revision 1.1  2003/02/05 03:57:54  trey
 * initial atacama check-in
 *
 * Revision 1.1  2002/09/24 21:50:23  trey
 * rebuilt RCL from scratch to support nested vectors and hashes
 *
 * Revision 1.3  2002/08/15 23:18:49  trey
 * added API functions for dealing with arrays
 *
 * Revision 1.2  2002/08/15 22:16:46  trey
 * added arrays and int type
 *
 * Revision 1.1  2002/07/24 21:22:00  trey
 * initial FIRE checkin
 *
 * Revision 1.2  1999/11/08 15:34:59  trey
 * major overhaul
 *
 * Revision 1.1  1999/11/03 19:31:34  trey
 * initial check-in
 *
 *
 ***************************************************************************/
