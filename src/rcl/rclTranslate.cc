/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.4 $  $Author: trey $  $Date: 2005/05/09 21:27:06 $
 *  
 * PROJECT: Life in the Atacama
 *
 * @file    rclTranslate.cc
 * @brief   No brief
 ***************************************************************************/

/***************************************************************************
 * INCLUDES
 ***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <errno.h>

#include <iostream>
#include <fstream>
#include <cstring>

#include "RCL.h"
#include "RCLXML.h"
#include "RCLObject.h"

using namespace std;

struct RCLTranslateOptions {
  const char* in_file_name;
  const char* out_file_name;
  bool force_output_is_xml;
  bool force_output_is_rcl;

  RCLTranslateOptions(void) :
    in_file_name(NULL),
    out_file_name(NULL),
    force_output_is_xml(false),
    force_output_is_rcl(false)
  {}
};

bool ends_with(const std::string& s,
	       const std::string& suffix)
{
  if (s.size() < suffix.size()) return false;
  return (s.substr(s.size() - suffix.size()) == suffix);
}

static string replace_suffix(const string& s,
			     const string& suffix,
			     const string& replacement)
{
  if (s.size() < suffix.size()) return s;
  if (s.substr(s.size() - suffix.size()) != suffix) return s;

  string ret = s;
  ret.replace(ret.size() - suffix.size(), suffix.size(), replacement);
  return ret;
}

void doit(RCLTranslateOptions& options)
{
  bool input_is_xml = ends_with(options.in_file_name,".xml");
  bool write_to_stdout =
    ((NULL != options.out_file_name)
     && (0 == strcmp( options.out_file_name, "-" )));

  string outs;
  if (NULL == options.out_file_name) {
    if (input_is_xml) {
      outs = replace_suffix(options.in_file_name,".xml",".rcl");
    } else {
      outs = replace_suffix(options.in_file_name,".rcl",".xml");
    }
    if (outs == options.in_file_name) {
      cerr << "ERROR: can't infer output file name" << endl;
      exit(EXIT_FAILURE);
    }
    options.out_file_name = outs.c_str();
  }

  rcl::exp expr;
  if (input_is_xml) {
    expr = rcl::xml::readFromFile(options.in_file_name);
  } else {
    expr = rcl::loadConfigFile(options.in_file_name);
  }
  if (options.force_output_is_rcl
      || (input_is_xml && !options.force_output_is_xml)) {
    if (write_to_stdout) {
      rcl::writeConfig( cout, expr, 0 );
      cout << endl;
    } else {
      expr.writeToFile( options.out_file_name );
    }
  } else {
    if (write_to_stdout) {
      rcl::xml::writeXML( cout, expr );
    } else {
      ofstream out(options.out_file_name);
      if (!out) {
	rcl::callErrorHandler
	  (string("couldn't open ") + options.out_file_name
	   + "for writing: " + strerror(errno));
      }
      rcl::xml::writeXML(out, expr);
      out.close();
    }
  }
}

void usage(void) {
  cerr <<
    "\n"
    "usage: rclTranslate OPTIONS <infile> [<outfile>]\n"
    "  -h or --help         Print this help\n"
    "  -x or --xml          Force output to be formatted in XML\n"
    "  -r or --rcl          Force output to be formatted in RCL\n"
    "\n"
    ;
  exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
  static char short_options[] = "hxr";
  static struct option long_options[]={
    {"help",0,NULL,'h'},
    {"xml",0,NULL,'x'},
    {"rcl",0,NULL,'r'},
    {NULL,0,0,0}
  };

  RCLTranslateOptions options;

  while (1) {
    char optchar = getopt_long(argc,argv,short_options,long_options,NULL);
    if (optchar == -1) break;

    switch (optchar) {
    case 'h': // help
      usage();
      break;
    case 'x':
      options.force_output_is_xml = true;
      break;
    case 'r':
      options.force_output_is_rcl = true;
      break;
    case '?': // unknown option
      // getopt() prints an error
      cerr << endl;
      usage();
      break;
    default:
      abort(); // never reach this point
    }
  }
  if (argc - optind > 2) {
    cerr << argv[0] << ": too many arguments" << endl;
    usage();
  }
  if (optind < argc) {
    options.in_file_name = argv[optind++];
    if (optind < argc) {
      options.out_file_name = argv[optind++];
    }
  } else {
    cerr << argv[0] << ": not enough arguments" << endl;
    usage();
  }

  try {
    doit(options);
  } catch (rcl::exception err) {
    // rcl prints an error before throwing the exception
    exit(EXIT_FAILURE);
  }
}

/***************************************************************************
 * REVISION HISTORY:
 * $Log: rclTranslate.cc,v $
 * Revision 1.4  2005/05/09 21:27:06  trey
 * changed interpretation of -x and -r flags, added ability to write output on console
 *
 * Revision 1.3  2005/03/28 16:50:15  dom
 * Added missing include for errno
 *
 * Revision 1.2  2004/08/02 21:47:29  trey
 * removed .esl as a synonym for .xml
 *
 * Revision 1.1  2004/08/02 03:36:24  trey
 * moved eslTranslate to rcl directory and renamed it to rclTranslate
 *
 * Revision 1.2  2004/08/02 02:49:58  trey
 * made things more robust to corner cases
 *
 * Revision 1.1  2004/08/02 02:10:05  trey
 * initial check-in
 *
 *
 ***************************************************************************/
