/***** tell emacs we use -*- c++ -*- style comments *****
 * $Revision: 1.21 $ $Author: trey $ $Date: 2005/06/15 23:47:54 $
 *
 * COPYRIGHT 2004, Carnegie Mellon University 
 *
 * PROJECT: Exploration Robotics (Life in the Atacama)
 *
 * MODULE:
 *
 * FILE: microraptor/rcl/testRCL.cc
 *
 * DESCRIPTION:
 *
 ********************************************************/

#include <getopt.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>

#include <fstream>

#if USE_MEM_TRACKER
#  include "memTracker.h"
#endif
#include "RCL.h"

using namespace std;

void demo(void) {
  /**********************************************************************/
  cout << "\n--- setting rcl::vector" << endl;
  rcl::vector v;
  // set the [0] entry
  v[0] = 1.5;
  // set the [1] entry
  v[1] = 3;
  // adds an entry to the vector, effectively sets the [2] entry
  v.push_back(false);
  // sets the [3] entry
  v.push_back("foo");

  cout << "[expected] v = 1.5 3 false foo" << endl;
  cout << "           v = " << rcl::toString(v) << endl;

  /**********************************************************************/
  cout << "\n--- accessing rcl::vector" << endl;
  double d = v[0].getDouble();
  long i = v[1].getLong();
  bool b = v[2].getBool();
  string s = v[3].getString();
  cout << "[expected] d = 1.5, i = 3, b = false, s = foo" << endl;
  cout << "           d = " << d
       << ", i = " << i
       << ", b = " << (b ? "true" : "false")
       << ", s = " << s
       << endl;

  /**********************************************************************/
  cout << "\n--- setting rcl::map" << endl;
  rcl::map m;
  m("a") = 1.5;
  m("b") = 3;
  m("c") = false;
  m("d") = "foo";

  cout << "[expected] m = {a=1.5,b=3,c=false,d=\"foo\"}" << endl;
  cout << "           m = " << rcl::toString(m) << endl;

  /**********************************************************************/
  cout << "\n--- accessing rcl::map" << endl;
  double d2 = m("a").getDouble();
  long i2 = m("b").getLong();
  bool b2 = m("c").getBool();
  string s2 = m("d").getString();
  cout << "[expected] d = 1.5, i = 3, b = false, s = foo" << endl;
  cout << "           d = " << d2
       << ", i = " << i2
       << ", b = " << (b2 ? "true" : "false")
       << ", s = " << s2
       << endl;

  /**********************************************************************/
  cout << "\n--- using rcl::exp to represent different types" << endl;
  rcl::exp expr;

  // set expr to internally be a vector
  expr = v;
  // can now treat expr as a vector
  expr[0] = 3.7;
  cout << "[expected] expr = 3.7 3 false foo" << endl;
  cout << "           expr = " << rcl::toString(expr) << endl;

  // set expr to internally be a map
  expr = m;
  // can now treat expr as a map
  expr("d") = "bar";
  cout << "[expected] expr = {a=1.5,b=3,c=false,d=\"bar\"}" << endl;
  cout << "           expr = " << rcl::toString(expr) << endl;

  // set expr to internally be an atomic type (in this case, string)
  expr = "hello!";
  // can access internal string value
  string s3 = expr.getString();
  cout << "[expected] expr = \"hello!\", s = hello!" << endl;
  cout << "           expr = " << expr << ", s = " << s3 << endl;

  /**********************************************************************/
  cout << "\n--- parsing from a string" << endl;

  // the parser accepts a space-separated list and produces a vector
  expr = rcl::readFromString("[1,2,3] {a=1.5,b=2.5,c=3.5} foo");
  cout << "[expected] expr = [1,2,3] {a=1.5,b=2.5,c=3.5} foo" << endl;
  cout << "           expr = " << rcl::toString(expr) << endl;

  // the [0] entry of the parser output is the first expression in the
  // space-separated list, which was a vector (because of the square
  // brackets).  we can access the [1] entry of the vector.
  long i3 = expr[0][1].getLong();
  cout << "[expected] i = 2" << endl;
  cout << "           i = " << i3 << endl;

  // the [1] entry was a map (because of the curly brackets).  we can
  // access its ("c") entry.
  double d3 = expr[1]("c").getDouble();
  cout << "[expected] d = 3.5" << endl;
  cout << "           d = " << d3 << endl;

  // the [2] entry was a string.  we can get its value.
  string s4 = expr[2].getString();
  cout << "[expected] s = foo" << endl;
  cout << "           s = " << s4 << endl;

  /**********************************************************************/
  cout << "\n--- loading a config file" << endl;

  // loading a config file performs the following steps:
  // - if the config file starts with the characters #!,
  //   execute it and parse the output
  // - otherwise, parse the file itself
  // - the parser is looking for just one expression rather than
  //   a space-separated list (unlike readFromString())
  // - the exp returned by loadConfigFile() has as its internal
  //   type whatever type the expression in the input file has

  cout << "[expected] (pretty-printed parse tree for example.config)" << endl;
  try {
    expr = rcl::loadConfigFile("example.config");
    cout << expr << endl;
  } catch (rcl::exception err) {
    //cout << "WARNING: " << err.text << endl;
  }

  /**********************************************************************/
  cout << "\n--- error handling" << endl;

  // NOTE: Explicitly checking whether an field is defined makes sense
  // if (according to your protocol) the field is optional.  But if
  // leaving out the field is an error, you can rely on the internal rcl
  // error-checking mechanism.
  
  // When accessing a complex nested rcl expression, rather than
  // checking at every step whether fields are defined, simply try to
  // access them.  If something is undefined, rcl will throw an
  // exception, which you can catch by wrapping the entire code block
  // with an exception handler.

  // If you want to pinpoint where the error occurred, disable your
  // exception handler and generate the error again.  The uncaught
  // exception will cause the program to dump core and you can examine
  // the stack trace in a debugger.

  expr = rcl::readFromString("[1,2,3] {a=1.5,b=2.5,c=3.5} foo");
  
  cout << "[expected] (exception)" << endl;
  try {
    // generate exception: trying to access expr[0]
    //   (which is a vector) as a map
    cout << expr[0]("a").getDouble() << endl;
    // never reach this
    cout << "FAILED TO GENERATE EXCEPTION!" << endl;
  } catch (rcl::exception err) {
    cout << "caught exception: " << err.text << endl;
  }

  cout << "[expected] (exception)" << endl;
  try {
    // generate exception: trying to access undefined field ("d") of
    //   expr[1]
    cout << expr[1]("d").getDouble() << endl;
    // never reach this
    cout << "FAILED TO GENERATE EXCEPTION!" << endl;
  } catch (rcl::exception err) {
    cout << "caught exception: " << err.text << endl;
  }

  /**********************************************************************/
  cout << "\n--- type checking and checking for undefined values" << endl;

  // NOTE: See earlier comment about when type checking is appropriate.

  expr = rcl::readFromString("[1,2,3] {a=1.5,b=2.5,c=3.5} foo");

  // the ("a") entry of expr[1] is defined
  bool b4 = expr[1]("a").defined();
  cout << "[expected] b = true" << endl;
  cout << "           b = " << (b4 ? "true" : "false") << endl;

  // but the ("d") entry of expr[1] is not
  b4 = expr[1]("d").defined();
  cout << "[expected] b = false" << endl;
  cout << "           b = " << (b4 ? "true" : "false") << endl;

  // the three expressions have types vector, map, and string
  cout << "[expected] types = vector map string" << endl;
  cout << "           types = " << expr[0].getType()
       << " " << expr[1].getType()
       << " " << expr[2].getType() << endl;

  // check that expr[1] has type map
  cout << "[expected] expr[1] is a map" << endl;
  if (expr[1].getType() == rcl::typeName<rcl::map>()) {
    cout << "           expr[1] is a map" << endl;
  }

  /**********************************************************************/
  cout << "\n--- unusual copy semantics" << endl;
  
  // the rcl::exp assignment operator performs copy-by-reference,
  // similar to java semantics.  this helps to improve efficiency, but
  // can sometimes be surprising!

  // make expr2 a copy of expr1
  rcl::exp expr1 = rcl::map();
  rcl::exp expr2 = expr1;
  // modifications to fields of expr2 now affect expr1!
  expr2("a") = "foo";

  cout << "[expected] expr1 = {a=\"foo\"}, expr2 = {a=\"foo\"}" << endl;
  cout << "           expr1 = " << rcl::toString(expr1)
       << ", expr2 = " << rcl::toString(expr2) << endl;

  // but assigning a new value to expr2 removes the connection and
  //   does not affect expr1
  expr2 = rcl::map();
  cout << "[expected] expr1 = {a=\"foo\"}, expr2 = {}" << endl;
  cout << "           expr1 = " << rcl::toString(expr1)
       << ", expr2 = " << rcl::toString(expr2) << endl;

  /**********************************************************************/
  cout << "\n--- iterating through rcl::vector" << endl;
  expr = rcl::readFromString("[1,2,3] {a=1.5,b=2.5,c=3.5} foo");

  // explicitly cast expr to an rcl::vector so that we get access to
  // the relevant iteration functions.
  rcl::vector& v2 = expr.getVector();

  // The iteration semantics for rcl::vector are like those for
  // the STL std::vector.  We present three styles you can use.

  // FOR_EACH is a useful macro defined in RCL.h.  It can be used to
  // iterate over any collection that supports STL-style iteration.
  cout << "[expected] elt=1 2 3 elt={a=1.5,b=2.5,c=3.5} elt=\"foo\"" << endl;
  cout << "           ";
  FOR_EACH ( elt, v2 ) {
    cout << "elt=" << rcl::toString(*elt) << " ";
  }
  cout << endl;

  // if you don't want to use the FOR_EACH macro, here is the longhand version
  cout << "[expected] elt=1 2 3 elt={a=1.5,b=2.5,c=3.5} elt=\"foo\"" << endl;
  cout << "           ";
  for (rcl::vector::iterator elt = v2.begin(); elt != v2.end(); elt++) {
    cout << "elt=" << rcl::toString(*elt) << " ";
  }
  cout << endl;

  // or you can just treat the vector as an array
  cout << "[expected] elt=1 2 3 elt={a=1.5,b=2.5,c=3.5} elt=\"foo\"" << endl;
  cout << "           ";
  for (unsigned int i = 0; i < v2.size(); i++) {
    cout << "elt=" << rcl::toString(v2[i]) << " ";
  }
  cout << endl;

  /**********************************************************************/
  cout << "\n--- iterating through rcl::map" << endl;
  expr = rcl::readFromString("[1,2,3] {a=1.5,b=2.5,c=3.5} foo");
  
  // explicitly cast expr[1] to an rcl::map so that we get access to
  // the relevant iteration functions.
  rcl::map& m2 = expr[1].getMap();

  // As demonstrated with rcl::vector above, you can use the FOR_EACH
  // macro to iterate over an rcl::map, or write the same loop out
  // longhand.  Array-style iteration is not supported.

  // Here we show the FOR_EACH style of iteration.
  cout << "[expected] key=a val=1.5 key=b val=2.5 key=c val=3.5" << endl;
  cout << "           ";
  FOR_EACH ( pr, m2 ) {
    cout << "key=" << pr->first << " val=" << pr->second << " ";
  }
  cout << endl;
  
  /**********************************************************************/
  cout << "\n--- multiple styles of output" << endl;
  expr = rcl::readFromString("[1,2,3] {a=1.5,b=2.5,c=3.5} foo");

  // There are three styles of output for RCL expressions.  All three
  // styles can be read interchangably by the parser.

  // The "config" style is pretty-printed with line breaks and indentation.
  // Config style is used if you say 'cout << expr'.
  cout << "[expected] (pretty-printed expression)" << endl;
  cout << rcl::toStringConfig(expr) << endl;

  // The "compact" style leaves out the whitespace and replaces the arrows
  // "=>" in maps with equal signs.
  cout << "[expected] [[1,2,3],{a=1.5,b=2.5,c=3.5},\"foo\"]" << endl;
  cout << "           " << rcl::toStringCompact(expr) << endl;

  // The "command" style is like the compact style, except that it
  // outputs a vector as a space-separated list "a b c" instead of a
  // comma-separated list "[a,b,c]".  Command style is used for
  // communication between mraptord and claw.
  cout << "[expected] [1,2,3] {a=1.5,b=2.5,c=3.5} foo" << endl;
  cout << "           " << rcl::toString(expr) << endl;

  /**********************************************************************/
  cout << "\n--- interactive" << endl;
  cout << "\n  You can type one-line rcl expressions below and see" << endl
       << "  the corresponding pretty-printed parse trees." << endl
       << "  Type 'q' to quit." << endl
       << endl;

  char userCommandLineBuf[1024];
  rcl::exp cmd;
  while (1) {
    cout << "> ";
    cout.flush();
    fgets(userCommandLineBuf, sizeof(userCommandLineBuf), stdin);
    try {
      cmd = rcl::readFromString(userCommandLineBuf);
      if (cmd[0].getType() == rcl::typeName<string>() 
	  && cmd[0].getString() == "q") {
	exit(0);
      }
      cout << "parse tree:" << endl << cmd << endl;
    } catch (rcl::exception err) {
      cout << "caught exception: " << err.text << endl;
    }
  }
}

std::string readFileAsString(const std::string& fileName)
{
  FILE *inFile = fopen(fileName.c_str(), "r");
  if (NULL == inFile) {
    cerr << "ERROR: couldn't open " << fileName << " for reading: "
	 << strerror(errno) << endl;
  }

  ostringstream out;
  char copyChunkBuf[8192];

  while (!feof(inFile)) {
    unsigned int bytes = fread(copyChunkBuf, 1, sizeof(copyChunkBuf), inFile);
    out.write(copyChunkBuf, bytes);
  }

  return out.str();
}

double getTime(void) {
  timeval t;
  gettimeofday(&t, 0);
  return t.tv_sec + 1e-6*t.tv_usec;
}

void usage(void) {
  cerr <<
    "usage: testRCL OPTIONS\n"
    "  -h or --help       Print this help\n"
    "  -c <filename>      Instead of the standard demo, load the given file\n"
    "                     and pretty-print its parse-tree\n"
    "  -s                 Stress-test (used with -c).  Enters a while loop\n"
    "                     that repeatedly parses the given file.\n";
  exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
  static char shortOptions[] = "hc:s";
  static struct option longOptions[]={
    {"help",0,NULL,'h'},
    {"config",1,NULL,'c'},
    {"stress-test",0,NULL,'s'},
    {NULL,0,0,0}
  };

  char *configFileName = NULL;
  bool stressTest = false;

  while (1) {
    char optchar = getopt_long(argc,argv,shortOptions,longOptions,NULL);
    if (optchar == -1) break;

    switch (optchar) {
    case 'h':
      usage();
      break;
    case 'c':
      configFileName = optarg;
      break;
    case 's':
      stressTest = true;
      break;
    case '?':
      // getopt() prints an error
      cerr << endl;
      usage();
      break;
    default:
      abort(); // never reach this point
    }
  }
  if (optind < argc) {
    cerr << argv[0] << ": too many arguments" << endl << endl;
    usage();
  }

  if (configFileName) {
    try {
      if (stressTest) {
	cout << "stress testing the rcl parser by repeatedly parsing " << configFileName << endl;
	const bool parseString = true;
	std::string fileString;
	if (parseString) {
	  cout << "(reading in the file once and repeatedly parsing the resulting string)" << endl;
	  fileString = readFileAsString(configFileName);
	} else {
	  cout << "(repeatedly reading in the file)" << endl;
	}
	
	int nextReport = 10;
	double startTime = getTime();
	for (int i=1; ; i++) {
	  {
	    rcl::exp expr;
	    if (parseString) {
	      expr = rcl::readFromString(fileString);
	    } else {
	      expr = rcl::readFromFile(configFileName);
	    }
	  }
	  if (i == nextReport) {
	    cout << "over the first " << i << " iterations: "
		 << (getTime() - startTime)/i << " seconds per parse" << endl;
#if USE_MEM_TRACKER
	    mTReport(MTR_REPORT_VERBOSE);
#endif
	    nextReport *= 10;
	  }
	}
      } else {
	rcl::exp expr = rcl::loadConfigFile(configFileName);
	cout << expr << endl;
      }
    } catch (rcl::exception err) {
      exit(EXIT_FAILURE);
    }
  } else {
    demo();
  }

  return 0;
}
/***************************************************************
 * Revision History:
 * $Log: testRCL.cc,v $
 * Revision 1.21  2005/06/15 23:47:54  trey
 * brought regression test up to date
 *
 * Revision 1.20  2004/07/06 15:35:12  trey
 * added optional memory tracking and plugged some memory leaks
 *
 * Revision 1.19  2004/05/03 17:51:25  trey
 * improved arg parsing
 *
 * Revision 1.18  2004/04/28 18:58:53  dom
 * Appended log directive
 * 
 ***************************************************************/
