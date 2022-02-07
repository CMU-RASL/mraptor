/********** tell emacs we use -*- c++ -*- style comments *******************
 * $Revision: 1.16 $  $Author: brennan $  $Date: 2004/11/25 05:23:12 $
 *
 * PROJECT:      Distributed Robotic Agents
 * DESCRIPTION:  
 *
 * (c) Copyright 1999 CMU. All rights reserved.
 ***************************************************************************/

%{
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <limits.h>

#include "strxcpy.h"

#include "RCL.h"
#include "RCLInternal.h"
#include "RCLGrammar.tab.hh"

#define YY_NO_UNPUT /* Stop getting a warning about it not getting used */

namespace rcl {

  char lexCurrentFile[1024];
  int lexLineNo;

}; // namespace rcl

/* set this #if to be 1 if you want a trace from the lexer, very handy for
   debugging */
#if 0
static void
DB(const string& s) {
#ifndef NDEBUG
  if (s != "\n") {
    printf("[%s] ",s.c_str());
  } else {
    printf("\n");	/* Don't do square brackets for new lines. */
  }
#endif
}
#else
/* no op */
#define DB(s)
#endif

%}
/* %start */

    /* Handy regular expression substitutions. */
WS		([ \t])
LETTER		([a-zA-Z])
DIGIT		([0-9])
INTEGER		(("+"|"-")?{DIGIT}+)
SCALEFACTOR	(("E"|"e")("+"|"-")?{DIGIT}+)
REALOPT1        ("."{DIGIT}*{SCALEFACTOR}?)
REALOPT2        (("."{DIGIT}*)?{SCALEFACTOR})
REAL		(("+"|"-")?{DIGIT}+({REALOPT1}|{REALOPT2}))
WORD		({LETTER}({LETTER}|{DIGIT})*)

	/* Variable names/identifiers */
VARLETTER	([a-zA-Z_])
VARWORD		({VARLETTER}({VARLETTER}|{DIGIT})*)

  /* char and strings */
CCHAR		(\'(\\[ntvbrfa\'\"\\]|\\[0-9]{3}|[^\'\\])\')
CSCHR		(\\[ntvbrfa\'\"\\]|\\[0-9]{3}|[^\"\\])
CSTRING         (\"{CSCHR}*\")

  /* added these so things that don't need to be quoted don't
     need to be quoted */
PATHCHAR        ([A-Za-z0-9._\-/~])
PATHSTRING      ({PATHCHAR}*)

%%

%{ /* Can place C code here */
%}

"#"[^\n]*       { /* shell style comment... do nothing */ }

true			{
			  RCL_YY_PREFIX(lval).boolVal = true;
			  DB("BOOLVAL");
			  return (BOOLVAL);
			}

false			{
			  RCL_YY_PREFIX(lval).boolVal = false;
			  DB("BOOLVAL");
			  return (BOOLVAL);
			}

undef                   { DB("UNDEF"); return (UNDEF); }

"["		{DB("[");     return ((int)'[');}
"]"		{DB("]");     return ((int)']');}
"{"		{DB("{");     return ((int)'{');}
"}"		{DB("}");     return ((int)'}');}
","		{DB(",");     return ((int)',');}
";"		{DB(";");     return ((int)';');}
"="		{DB("=");     return ((int)'=');}
"+"		{DB("+");     return ((int)'+');}
"=>"            {DB("ARROW"); return (ARROW);}

{VARWORD}		{
#if USE_MEM_TRACKER
                            RCL_YY_PREFIX(lval).stringVal = MTR_STRDUP(RCL_YY_PREFIX(text));
#else
                            RCL_YY_PREFIX(lval).stringVal = rcl::strdup_new(RCL_YY_PREFIX(text));
#endif
			    DB("IDENTIFIER");
			    DB(RCL_YY_PREFIX(lval).stringVal);
                            return(IDENTIFIER);
                        }
\${VARWORD}		{
                            // the +1 skips past the initial $ sign in $VARNAME
#if USE_MEM_TRACKER
                            RCL_YY_PREFIX(lval).stringVal = MTR_STRDUP(RCL_YY_PREFIX(text)+1);
#else
                            RCL_YY_PREFIX(lval).stringVal = rcl::strdup_new(RCL_YY_PREFIX(text)+1);
#endif
			    DB("ENVIDENT");
			    DB(RCL_YY_PREFIX(lval).stringVal);
                            return(ENVIDENT);
                        }
\$\{{VARWORD}\}		{
                            // reading in ${VARNAME}: the +2 skips past the initial ${
#if USE_MEM_TRACKER
                            RCL_YY_PREFIX(lval).stringVal = MTR_STRDUP(RCL_YY_PREFIX(text)+2);
#else
                            RCL_YY_PREFIX(lval).stringVal = rcl::strdup_new(RCL_YY_PREFIX(text)+2);
#endif
                            int n = strlen(RCL_YY_PREFIX(lval).stringVal);
                            // chop the trailing }
                            RCL_YY_PREFIX(lval).stringVal[n-1] = '\0';
			    DB("ENVIDENT");
			    DB(RCL_YY_PREFIX(lval).stringVal);
                            return(ENVIDENT);
                        }
{INTEGER}		{
                          sscanf(RCL_YY_PREFIX(text), "%ld",&RCL_YY_PREFIX(lval).longVal);
			  DB("LONGVAL");
			  return (LONGVAL);
		        }
{REAL}			{
                          sscanf(RCL_YY_PREFIX(text), "%lf",&RCL_YY_PREFIX(lval).doubleVal);
			  DB("DOUBLEVAL");
			  return (DOUBLEVAL);
		        }

{CSTRING}		{
                            // reading in "STRING": the +1 skips past the initial "
#if USE_MEM_TRACKER
                            char* tmp = MTR_STRDUP(RCL_YY_PREFIX(text)+1);
#else
                            char* tmp = rcl::strdup_new(RCL_YY_PREFIX(text)+1);
#endif
                            // chop the trailing "
                            tmp[strlen(tmp)-1] = '\0';

#if USE_MEM_TRACKER
			    int n = strlen(tmp)+1;
			    MTR_NEW_ARR(RCL_YY_PREFIX(lval).stringVal, char[n], n);
#else
                            RCL_YY_PREFIX(lval).stringVal = new char[strlen(tmp)+1];
#endif
			    // strcadd handles C escape sequences like \n
                            strcadd(RCL_YY_PREFIX(lval).stringVal, tmp);

#if USE_MEM_TRACKER
                            MTR_DELETE(tmp);
#else
                            delete [] tmp;
#endif

			    DB("STRINGVAL");
                            return(STRINGVAL);
			}			  

{PATHSTRING}		{
#if USE_MEM_TRACKER
                            RCL_YY_PREFIX(lval).stringVal = MTR_STRDUP(RCL_YY_PREFIX(text));
#else
                            RCL_YY_PREFIX(lval).stringVal = rcl::strdup_new(RCL_YY_PREFIX(text));
#endif
			    DB("STRINGVAL");
                            return(STRINGVAL);
			}

"%"[^\n]*               {
                            /* this form of string avoids quoting issues */
                            // we are reading in %STRING: the +1 skips past the initial %
#if USE_MEM_TRACKER
                            RCL_YY_PREFIX(lval).stringVal = MTR_STRDUP(RCL_YY_PREFIX(text)+1);
#else
                            RCL_YY_PREFIX(lval).stringVal = rcl::strdup_new(RCL_YY_PREFIX(text)+1);
#endif
			    DB("STRINGVAL");
                            return(STRINGVAL);
			}

\n 		 	{ rcl::lexLineNo++; DB("\n"); }
{WS}		{ /* NOP -> Suck up white space */ }

<<EOF>>         	{ RCL_YY_PREFIX(terminate)(); }

.                       {
			  char buf[1024];
	                  if (isprint(RCL_YY_PREFIX(text)[0])) {
			    snprintf(buf, sizeof(buf),
				     "badly placed `%c'", RCL_YY_PREFIX(text)[0]);
                          } else {
			    snprintf(buf, sizeof(buf),
				     "badly placed [ASCII 0x%02x]",
				     RCL_YY_PREFIX(text)[0]);
                          }
                          RCL_YY_PREFIX(error)(buf);
                        }

%%  /* C code section */

YY_BUFFER_STATE RCL_YY_PREFIX(rcl_buf) = 0;

namespace rcl {

  char* strdup_new(const char* s) {
    char* ret = new char[strlen(s)+1];
    return strcpy(ret,s);
  }

  void lexSwitchToString(const string& s) {
    RCL_YY_PREFIX(rcl_buf) = yy_scan_string(s.c_str());
    RCL_YY_PREFIX(_switch_to_buffer)(RCL_YY_PREFIX(rcl_buf));
  }
  
  void lexBufferCleanup(void) {
    yy_delete_buffer(RCL_YY_PREFIX(rcl_buf));
  }
  
  void lexSwitchToFile(FILE *file) {
    RCL_YY_PREFIX(rcl_buf) = yy_create_buffer(file, YY_BUF_SIZE);
    RCL_YY_PREFIX(_switch_to_buffer)(RCL_YY_PREFIX(rcl_buf));
  }

}; // namespace rcl

void RCL_YY_PREFIX(error)(const char *s) { 
  char errbuf[1024];
  snprintf(errbuf, sizeof(errbuf), "%s:%d: before `%s': %s",
	   rcl::lexCurrentFile, rcl::lexLineNo, RCL_YY_PREFIX(text), s);
  rcl::callErrorHandler(errbuf);
}

/* Replacement for libfl -lfl, so that we don't have to link with it. */
/* This is exactly what is in libyywrap.c in flex-2.5.4a */
int yywrap(void)
{
    return (1);
}
