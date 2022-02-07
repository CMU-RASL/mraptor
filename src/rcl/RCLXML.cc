/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.12 $  $Author: trey $  $Date: 2004/09/13 16:47:21 $
 *  
 * PROJECT: Life in the Atacama
 *
 * @file    RCLXML.cc
 * @brief   No brief
 ***************************************************************************/

/***************************************************************************
 * INCLUDES
 ***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <expat.h>
#include <regex.h>

#include <iostream>
#include <algorithm>

#include "strxcpy.h"
#include "RCLXML.h"

using namespace std;

namespace rcl {

  namespace xml {

    struct regex_subexp_type {
      regex_t regex; // the regexp
      int subexp; // the key sub-expression
    };
    struct regexes_type {
      regex_subexp_type double_regex;
      regex_subexp_type long_regex;
      regex_subexp_type bool_regex;
      regex_subexp_type string_regex;
    };
    static regexes_type* regexesG;

    static void initializeRegexps(void) {

#define LEADING_SPACES "^[[:space:]]*("
#define TRAILING_SPACES ")[[:space:]]*$"

#define REGCOMP_CHECK_ERROR(RETVAL,REGEX) \
  retval = (RETVAL); \
  if (retval != 0) { \
    regerror(retval,REGEX,errbuf,sizeof(errbuf)); \
    cerr << "ERROR: regcomp: (" << #REGEX << "): " << errbuf << endl; \
    exit(EXIT_FAILURE); \
  }

      if (regexesG != NULL) return;
      regexesG = new regexes_type;
      int retval;
      char errbuf[1024];
      
      REGCOMP_CHECK_ERROR
	(regcomp(&regexesG->double_regex.regex,
		 LEADING_SPACES
		 "(\\+|-)?[[:digit:]]+\\.[[:digit:]]*" // mantissa
		 "((E|e)(\\+|-)?[[:digit:]]+)?" // exponent
		 TRAILING_SPACES,
		 REG_EXTENDED),
	 &regexesG->double_regex.regex);
      regexesG->double_regex.subexp = 1;

      REGCOMP_CHECK_ERROR
	(regcomp(&regexesG->long_regex.regex,
		 LEADING_SPACES
		 "(\\+|-)?[[:digit:]]+"
		 TRAILING_SPACES,
		 REG_EXTENDED),
	 &regexesG->long_regex.regex);
      regexesG->long_regex.subexp = 1;

      REGCOMP_CHECK_ERROR
	(regcomp(&regexesG->bool_regex.regex,
		 LEADING_SPACES
		 "(true)|(false)"
		 TRAILING_SPACES,
		 REG_EXTENDED),
	 &regexesG->bool_regex.regex);
      regexesG->bool_regex.subexp = 1;
      
      REGCOMP_CHECK_ERROR
	(regcomp(&regexesG->string_regex.regex,
		 LEADING_SPACES
		 "\"(("
		 "\\\\[ntvbrfa\'\"]"      // escapes like \n
		 "|"
		 "\\\\[[:digit:]]{3}" // escapes like \001
		 "|"
		 "[^\"\\\\]"              // any character other than '"' or '\'
		 ")*)\"" 
		 TRAILING_SPACES,
		 REG_EXTENDED),
	 &regexesG->string_regex.regex);
      regexesG->string_regex.subexp = 2;
    }

    static string getSubstring(const string& s, regmatch_t& m) {
      return s.substr(m.rm_so, m.rm_eo - m.rm_so);
    }

    static exp readAtomFromString(const string& s) {
#define ATOM_MAX (20)
#define REGMATCH(TYPE,STRING,MATCH) \
  (0 == regexec(&regexesG->TYPE##_regex.regex, STRING.c_str(), ATOM_MAX, MATCH, 0))
#define REGSUBEXP(TYPE) regexesG->TYPE##_regex.subexp

      if (regexesG == NULL) initializeRegexps();
      regmatch_t match[ATOM_MAX];

      if (REGMATCH( double, s, match )) {
	//cout << "  double" << endl;
	return atof( getSubstring( s, match[REGSUBEXP(double)] ).c_str() );
      } else if (REGMATCH( long, s, match )) {
	//cout << "  long" << endl;
	return atol( getSubstring( s, match[REGSUBEXP(long)] ).c_str() );
      } else if (REGMATCH( bool, s, match )) {
	//cout << "  bool" << endl;
	return ( ( getSubstring( s, match[REGSUBEXP(bool)] ) == "true" )
		 ? true : false );
      } else if (REGMATCH( string, s, match )) {
	//cout << "  string" << endl;
	return compressEscapes( getSubstring( s, match[REGSUBEXP(string)] ) );
      } else {
	// didn't match the regex, just return the string
	return s;
      }
    }

#if 0
    static void testReadAtomFromString(void) {
      cout << "exp = " << readAtomFromString("3.5") << endl;
      cout << "exp = " << readAtomFromString("-3.5e+20") << endl;
      cout << "exp = " << readAtomFromString("347") << endl;
      cout << "exp = " << readAtomFromString("+2") << endl;
      cout << "exp = " << readAtomFromString("true") << endl;
      cout << "exp = " << readAtomFromString("false") << endl;
      cout << "exp = " << readAtomFromString("\"hello\"") << endl;
      cout << "exp = " << readAtomFromString("\"hello\\n\\004\"") << endl;
    }
#endif

    struct RCLXMLStack {
      XML_Parser parser;
      string filename;
      std::vector<exp*> expStack;
      ostringstream *text;
      exp rootExp;

      RCLXMLStack(void) :
	text(0)
      {}
    };
    
#ifndef DELETE_AND_NULL
#  define DELETE_AND_NULL(x) { delete (x); (x) = NULL; }
#endif

    static void outputText(RCLXMLStack& stk)
    {
      if (stk.text) {
	string text = stk.text->str();
	// ignore whitespace-only text
	if (string::npos != text.find_first_not_of("\n\t ")) {
	  exp expr( stk.filename, XML_GetCurrentLineNumber(stk.parser) );
	  expr = text; 
	  stk.expStack.back()->getMap().push_back(string("__text"),expr);
	}
	DELETE_AND_NULL( stk.text );
      }
    }
  
    static void handleElementStart(void *data, const char *el,
				   const char **attr)
    {
      RCLXMLStack& stk = *((RCLXMLStack *) data);
      outputText(stk);
      
      int lineNumber = XML_GetCurrentLineNumber(stk.parser);
      
      exp expr = map();
      expr.setFileLineNumber( stk.filename, lineNumber );
    
#if 0
      expr("__name") = el;
      expr("__name").setFileLineNumber( stk.filename, lineNumber );
#endif
      
      if (attr[0]) {
	expr("__attributes") = map();
	expr("__attributes").setFileLineNumber( stk.filename, lineNumber );
      }
      for (int i = 0; attr[i]; i += 2) {
	expr("__attributes")(attr[i]) = attr[i + 1];
      }
      
      exp& stackBack = *stk.expStack.back();
      stackBack.getMap().push_back(el, expr);
      stk.expStack.push_back(&stackBack(el));
    }
    
    static void handleElementEnd(void *data, const char *el)
    {
      RCLXMLStack& stk = *((RCLXMLStack *) data);
      outputText(stk);

      // map certain kinds of elements into rcl datatypes other than map
      exp& expr = *stk.expStack.back();
      if (!expr("__attributes").defined()) {

	// check if we should map this element to an atomic type
	if (expr.getMap().size() == 1 && expr("__text").defined()) {
#if 1
	  expr = readAtomFromString( expr("__text").getString() );
#else
	  string textString = expr("__text").getString();
	  exp textExp;
	  bool readSuccess = true;
	  try {
	    textExp = rcl::readFromString(textString);
	    cout << "textExp = " << textExp << endl;
	  } catch (exception err) {
	    readSuccess = false;
	  }
	  if (readSuccess && !textExp[0].hasComplexType()) {
	    expr = textExp[0];
	  } else {
	    expr = textString;
	  }
#endif
	}
	// check if we should map it to a vector type
	else {

	  if (expr.getMap().size() == 0) {
	    // an empty tag is a map
	  } else if (expr.getMap().size() == 1
		     && expr("empty_vector").defined()) {
	    // a tag that only contains <empty_vector/> is an
	    // empty vector
	    expr = rcl::vector();
	  } else {
	    // check if there are only <item> tags inside
	    bool allItems = true;
	    FOR_EACH ( pr, expr.getMap() ) {
	      if (pr->second.defined() && "item" != pr->first) {
		allItems = false;
		break;
	      }
	    }
	    if (allItems) {
	      rcl::vector v;
	      FOR_EACH ( pr, expr.getMap() ) {
		v.push_back( pr->second );
	      }
	      expr = v;
	    }
	  }

	}
      }

      stk.expStack.pop_back();
    }
    
    static void handleCharacterData(void *data, const char *s, int len)
    {
      RCLXMLStack& stk = *((RCLXMLStack *) data);
      if (!stk.text) {
	stk.text = new ostringstream;
      }
      (*stk.text).write(s,len);
    }
    
    static XML_Parser initializeParser(RCLXMLStack& stk) {
      XML_Parser p = XML_ParserCreate(NULL);
      if (! p) {
	callErrorHandler
	  ("rcl::xml::parseInternal: couldn't allocate memory for parser");
      }

      stk.rootExp = map();
      stk.expStack.push_back(&stk.rootExp);
      stk.parser = p;
      
      XML_SetElementHandler(p, handleElementStart, handleElementEnd);
      XML_SetCharacterDataHandler(p, handleCharacterData);
      XML_SetUserData(p, &stk);

      return p;
    }

    static void parseChunk(XML_Parser parser, const char *inbuf,
			   int len, int done)
    {
      if (XML_Parse(parser, inbuf, len, done) == XML_STATUS_ERROR) {
	char errbuf[1024];
	RCLXMLStack& stk = *((RCLXMLStack *) XML_GetUserData(parser));
	snprintf(errbuf, sizeof(errbuf),
		 "%s:%ld: XML parse error: %s",
		 stk.filename.c_str(),
		 XML_GetCurrentLineNumber(parser),
		 XML_ErrorString(XML_GetErrorCode(parser)));
	callErrorHandler(errbuf);
      }
    }

    static void finishParsing(XML_Parser parser) {
      RCLXMLStack& stk = *((RCLXMLStack *) XML_GetUserData(parser));
      stk.rootExp.setObjectNameRecurse("input");
      XML_ParserFree(parser);
    }

    exp readFromFile(const string& filename, FILE* configFile) {
      RCLXMLStack stk;
      stk.filename = filename;

      XML_Parser parser = initializeParser(stk);
      
      char inputChunkBuf[8192];
      for (;;) {
	int done;
	int len;
	
	len = fread(inputChunkBuf, 1, sizeof(inputChunkBuf), configFile);
	if (ferror(configFile)) {
	  callErrorHandler("xmlParser::parseInternal: read error");
	}
	done = feof(configFile);
	
	parseChunk(parser, inputChunkBuf, len, done);

	if (done) break;
      }
      
      finishParsing(parser);

      return stk.rootExp("rcl");
    }
    
    exp readFromFile(const string& configFile)
    {
      FILE* in;
      
#if 0
      testReadAtomFromString();
#endif

      if (0 == (in = fopen(configFile.c_str(),"r"))) {
	string s = "rcl::xml::readFromFile: couldn't open `" + configFile
	  + "' for reading";
	callErrorHandler(s);
      }
      exp e = readFromFile(configFile, in);
      fclose(in);
      
      return e;
    }
  
    exp readFromFD(const string& fileName, int fd)
    {
      FILE* in;
      
      if (0 == (in = fdopen(fd, "r"))) {
	string s = "rcl::xml::readFromFD: couldn't fdopen `" + fileName
	  + "' for reading";
	callErrorHandler(s);
      }
      exp e = readFromFile(fileName, in);
      
      fclose(in);
      
      return e;
    }
    
    exp readFromString(const string& s) {
      RCLXMLStack stk;
      stk.filename = "<string>";

      XML_Parser parser = initializeParser(stk);
      parseChunk(parser, s.c_str(), s.size(), true);
      finishParsing(parser);

      return stk.rootExp("rcl");
    }

    static void writeExpandXMLEscapes(ostream& out, const string& s) {
      out << "\"";

      // ugly, slow, works
      FOR ( i, s.size() ) {
	char c = s[i];

	// ignore one leading cr
	if (i == 0 && c == '\n') continue;

	if (c == '\"') {
	  out << "&quot;";
	} else if (c == '\'') {
	  out << "&apos;";
	} else if (c == '&') {
	  out << "&amp;";
	} else if (c == '<') {
	  out << "&lt;";
	} else if (c == '>') {
	  out << "&gt;";
	} else {
	  out << c;
	}
      }
      out << "\"";
    }

    static void writeAtomXML(ostream& out, const exp& expr)
    {
      if (expr.getType() == typeName<string>()) {
	writeExpandXMLEscapes(out, expr.getString());
      } else {
	out << expr;
      }
    }

    static void writeElementXML(ostream& out,
				const string& elementName,
				const exp& expr,
				int indentChars)
    {
      if (!expr.defined()) return;

      if (expr.hasComplexType()) {
	out << indent(indentChars) << "<" << elementName;
	if (expr.getType() == typeName<map>()
	    && expr("__attributes").defined()) {
	  const map& attributes = expr("__attributes").getMap();
	  FOR_EACH (pr, attributes) {
	    out << " " << pr->first << "=\"" << pr->second.getString() << "\"";
	  }
	}
	out << ">" << endl;
	writeXMLRecurse(out, expr, indentChars+2);
	out << indent(indentChars) << "</" << elementName << ">" << endl;
      } else {
	out << indent(indentChars) << "<" << elementName << ">";
	writeAtomXML(out, expr);
	out << "</" << elementName << ">" << endl;
      }
    }

    void writeXMLRecurse(ostream& out, const exp& expr, int indentChars) {
      if (expr.getType() == typeName<map>()) {
	const map& m = expr.getMap();
	bool lastWasText = false;
	FOR_EACH ( pr, m ) {
	  if (pr->first == "__attributes") {
	    // already handled in writeElementXML()
	    continue;
	  } else if (pr->first == "__text") {
	    writeExpandXMLEscapes(out, pr->second.getString());
	    lastWasText = true;
	  } else {
	    writeElementXML(out, pr->first, pr->second,
			    lastWasText ? 0 : indentChars);
	    lastWasText = false;
	  }
	}
      } else if (expr.getType() == typeName<vector>()) {
	const vector& v = expr.getVector();
	if (v.size() == 0) {
	  // write an <empty_vector/> tag to distinguish the vector from
	  // an empty map
	  out << indent(indentChars) << "<empty_vector/>" << endl;
	} else {
	  FOR_EACH ( i, v ) {
	    writeElementXML(out, "item", *i, indentChars);
	  }
	}
      } else {
	writeCompact(out, expr);
      }
    }

    void writeXML(ostream& out, const exp& expr) {
      out << "<rcl>" << endl;
      writeXMLRecurse(out, expr, 2);
      out << "</rcl>" << endl;
    }

    string toString(const exp& expr) {
      ostringstream out;
      writeXML(out, expr);
      return out.str();
    }

  }; // namespace xml
  
}; // namespace rcl

/***************************************************************************
 * REVISION HISTORY:
 * $Log: RCLXML.cc,v $
 * Revision 1.12  2004/09/13 16:47:21  trey
 * fixed inability to distinguish between empty maps and empty vectors in XML parser
 *
 * Revision 1.11  2004/08/18 15:47:46  trey
 * fixed regexps to work under cygwin
 *
 * Revision 1.10  2004/08/05 23:37:05  trey
 * parser now accepts standard Java scientific notation format (1E7 instead of 1E+7)
 *
 * Revision 1.9  2004/08/02 02:56:24  trey
 * strings are now quoted more often
 *
 * Revision 1.8  2004/07/06 15:35:12  trey
 * added optional memory tracking and plugged some memory leaks
 *
 * Revision 1.7  2004/06/10 22:46:43  trey
 * changed default behavior for RCL so that it prints an error before throwing an exception; error handling is now configurable using setErrorHandler()
 *
 * Revision 1.6  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.5  2004/04/13 21:12:38  trey
 * did some stress testing, fixed several obscure bugs
 *
 * Revision 1.4  2004/04/13 03:21:11  trey
 * xml front end now in relatively stable form
 *
 * Revision 1.3  2004/04/12 19:25:47  trey
 * developed a more full-featured translation between xml and rcl
 *
 * Revision 1.2  2004/04/09 20:31:33  trey
 * major enhancement to rcl interface
 *
 * Revision 1.1  2004/04/09 00:48:12  trey
 * initial check-in
 *
 *
 ***************************************************************************/
