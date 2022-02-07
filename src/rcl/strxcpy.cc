/****************************************************************************
 * $Revision: 1.5 $  $Author: trey $  $Date: 2004/07/06 15:35:12 $
 *
 * PROJECT:      Distributed Robotic Agents
 * DESCRIPTION:  Replace strcadd() and streadd() libgen functionality
 *               for non-IRIX platforms.
 *
 * (c) Copyright 1999 CMU. All rights reserved.
 ***************************************************************************/

/***************************************************************************
 * INCLUDES
 ***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "strxcpy.h"

/***************************************************************************
 * LOCAL MACROS and DEFINES
 ***************************************************************************/

typedef struct {
  char input;
  char output;
} escapePairType;

/***************************************************************************
 * GLOBAL VARIABLES
 ***************************************************************************/

static escapePairType pairsG[] = {
  {'n', '\n'},
  {'t', '\t'},
  {'v', '\v'},
  {'b', '\b'},
  {'r', '\r'},
  {'f', '\f'},
  {'a', '\a'},
  {'\'', '\''},
  {'\\', '\\'},
  {'"', '"'},
  {'\0', '\0'}
};

static int genMaps(void);

static char compressMapG[256];
static char expandMapG[256];
int dummyG = genMaps(); /* ensures maps are initialized */

/***************************************************************************
 * FUNCTIONS
 ***************************************************************************/

#if 0

static char
compressesTo(char input) {
  escapePairType *p;
  for (p = pairsG; p->input; p++) {
    if (input == p->input) return p->output;
  }
  return '\0';
}

static char
expandsTo(char output) {
  escapePairType *p;
  for (p = pairsG; p->input; p++) {
    if (output == p->output) return p->input;
  }
  return '\0';
}

#else

static int genMaps(void) {
  escapePairType *p;
  memset(compressMapG, 0, sizeof(compressMapG));
  memset(expandMapG, 0, sizeof(expandMapG));
  for (p = pairsG; p->input; p++) {
    compressMapG[(int)p->input] = p->output;
    expandMapG[(int)p->output] = p->input;
  }
  return 0;
}

#define compressesTo(input) (compressMapG[(int)(input)])
#define expandsTo(input) (expandMapG[(int)(input)])

#endif

/****************************************************************************
 * FUNCTION:	strcadd
 * DESCRIPTION:
 *  copies the input string, up to a null byte, to the output string,
 *  compressing the C-language escape sequences (for example, \n, \001) to
 *  the equivalent character.  A null byte is appended to the output.  The
 *  output argument must point to a space big enough to accommodate the
 *  result.  If it is as big as the space pointed to by input it is
 *  guaranteed to be big enough.
 * RETURN: the pointer to the null byte that terminates the output.
 ****************************************************************************/
char *
strcadd(char *output, const char *input) {
  const char *ip;
  char *op, c, cmp;
  unsigned int digits, cmpInt;
  char digBuf[4];

  for (ip=input, op=output; '\0' != (c = *ip); ip++, op++) {
    if ('\\' != c) {
      *op = *ip;
    } else {
      digits = 0;
      while (digits < 3 && isdigit(ip[digits+1])) digits++;
      if (digits > 0) {
	/* numeric escape sequence */
	strncpy(digBuf,ip+1,digits);
	digBuf[digits] = '\0';
	sscanf(digBuf,"%o",&cmpInt);
	*op = (char) cmpInt;
	ip += digits; /* we used digits+1 bytes instead of 1 */
      } else {
	/* escape sequence like \n or \r */
	cmp = compressesTo(c = ip[1]);
	if ('\0' == cmp) {
	  /* no match... just repeat the given character */
	  *op = c;
	} else {
	  /* plug in what we got back */
	  *op = cmp;
	}
	ip++; /* we used 2 bytes instead of 1 */
      }  
    }
  }
  *op = '\0';

  return op;
}

std::string compressEscapes(const std::string& s) {
#if USE_MEM_TRACKER
  char *buf;
  int n = s.size()+1;
  MTR_NEW_ARR(buf, char[n], n);
#else
  char* buf = new char[s.size()+1];
#endif
  strcadd(buf, s.c_str());
  std::string ret = buf;
#if USE_MEM_TRACKER
  MTR_DELETE(buf);
#else
  delete buf;
#endif
  return ret;
}


/****************************************************************************
 * FUNCTION:	streadd
 * DESCRIPTION:
 *  streadd copies the input string, up to a null byte, to the output string,
 *  expanding non-graphic characters to their equivalent C-language escape
 *  sequences (for example, \n, \001).  The output argument must point to a
 *  space big enough to accommodate the result; four times the space pointed
 *  to by input is guaranteed to be big enough (each character could become \
 *  and 3 digits).  Characters in the exceptions string are not expanded.
 *  The exceptions argument may be zero, meaning all non-graphic characters
 *  are expanded.
 * RETURN: the pointer to the null byte that terminates the output.
 ****************************************************************************/
char *
streadd (char *output, const char *input, const char *exceptions) {
  const char *ip;
  char *op, c, expr;
  char digBuf[5];

  for (ip=input, op=output; '\0' != (c = *ip); ip++, op++) {
    expr = expandsTo(c);
    if ((isprint(c) && ('\0' == expr))
	|| (exceptions && (0 != strchr(exceptions,c)))) {
      *op = *ip;
    } else {
      if ('\0' == expr) {
	/* no match... use numeric escape sequence */
	sprintf(digBuf, "\\%03o", (unsigned int) c);
	strncpy(op, digBuf, 4);
	op += 3; /* copied in 4 bytes instead of 1 */
      } else {
	/* escape sequence like \n or \r */
	op[0] = '\\';
	op[1] = expr;
	op++; /* copied in 2 bytes instead of 1 */
      }
    }
  }
  *op = '\0';

  return op;
}

/***************************************************************************
 * REVISION HISTORY:
 * $Log: strxcpy.cc,v $
 * Revision 1.5  2004/07/06 15:35:12  trey
 * added optional memory tracking and plugged some memory leaks
 *
 * Revision 1.4  2004/04/13 21:12:38  trey
 * did some stress testing, fixed several obscure bugs
 *
 * Revision 1.3  2004/04/09 20:31:33  trey
 * major enhancement to rcl interface
 *
 * Revision 1.2  2004/04/08 16:42:56  trey
 * overhauled exp
 *
 * Revision 1.1  2003/11/15 17:53:40  brennan
 * Split up CYGWIN #define into more reasonable/understandable chunks.  Completed move to FIRE build system.
 *
 * Revision 1.3  2003/02/18 22:40:17  trey
 * sped up string escaping and improved an error message
 *
 * Revision 1.2  2003/02/16 06:19:45  trey
 * fixed reading from file after reading from string
 *
 * Revision 1.1  2003/02/05 03:57:54  trey
 * initial atacama check-in
 *
 * Revision 1.1  2002/07/24 21:22:03  trey
 * initial FIRE checkin
 *
 * Revision 1.2  1999/11/08 15:35:04  trey
 * major overhaul
 *
 * Revision 1.1  1999/11/03 19:31:38  trey
 * initial check-in
 *
 ***************************************************************************/
