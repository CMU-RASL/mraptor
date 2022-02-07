/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.24 $  $Author: trey $  $Date: 2006/06/21 22:14:14 $
 *  
 * PROJECT: FIRE Architecture Project
 *
 * @file    exp.cc
 * @brief   No brief
 ***************************************************************************/

/***************************************************************************
 * INCLUDES
 ***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include <iostream>
#include <fstream>
#include <cstring>

#include "RCL.h"

using namespace std;
using namespace microraptor;

namespace rcl {

  /********** functions that act as casting operators */

  double exp::getDouble(void) const { return getValue<double>(); }
  long exp::getLong(void) const { return getValue<long>(); }
  bool exp::getBool(void) const { return getValue<bool>(); }
  string exp::getString(void) const { return getValue<string>(); }

  vector& exp::getVector(void) {
    checkType<vector>();
    return getValueRef<vector>();
  }
  map& exp::getMap(void) {
    checkType<map>();
    return getValueRef<map>();
  }
  
  const vector& exp::getVector(void) const { return getValueConstRef<vector>(); }
  const map& exp::getMap(void) const { return getValueConstRef<map>(); }
  
  /********** assignment operators (and corresponding constructors) */

#define RCL_EXP_ASSIGN_CAST(type, castToType) \
  exp& exp::operator=(type data) { \
    setValue((castToType) data); \
    return *this; \
  } \
  exp::exp(type data) { \
    setValue((castToType) data); \
  }

#define RCL_EXP_ASSIGN(type) RCL_EXP_ASSIGN_CAST(type,type)

  RCL_EXP_ASSIGN(double);
  RCL_EXP_ASSIGN(long);
  RCL_EXP_ASSIGN(bool);
  RCL_EXP_ASSIGN(const string&);
  RCL_EXP_ASSIGN(const vector&);
  RCL_EXP_ASSIGN(const map&);

  RCL_EXP_ASSIGN_CAST(int, long);
  RCL_EXP_ASSIGN_CAST(unsigned int, long);
  RCL_EXP_ASSIGN_CAST(float, double);

  // special case for const char* : avoid dumping core on null
  exp& exp::operator=(const char* data) {
    if (0 == data) {
      setValue((string) "__NULL__");
    } else {
      setValue((string) data);
    }
    return *this;
  }
  exp::exp(const char* data) {
    operator=(data);
  }

  /********** pass-through functions */
  
  /*-------------- vector */
  
  exp& exp::operator[](int index) { return getVector()[index]; }
  const exp exp::operator[](int index) const { return getVector()[index]; }
  void exp::push_back(const exp& elt) { getVector().push_back(elt); }

  /*-------------- map */
  
  exp& exp::operator()(const string& name) { return getMap()(name); }
  const exp exp::operator()(const string& name) const { return getMap()(name); }

  /********** information functions */

  bool exp::defined(void) const {
    return (getType() != typeName<undefinedExp>());
  }
  string exp::getType(void) const { return impl->getType(); }
  string exp::getDebugTrace(void) const { return debugInfo.getDebugTrace(); }
  string exp::getUndefDebugTrace(void) const { return debugInfo.getUndefDebugTrace(); }

  /********** misc functions */
  
  void exp::writeToFile(const string& fileName) {
    ofstream out(fileName.c_str());
    if (!out) {
      string s = "exp writeToFile: couldn't open `"
	+ fileName + "' for writing: "
	+ strerror(errno);
      callErrorHandler(s);
    }
    writeConfig(out, 0);
    out << endl;
    out.close();
  }
  void exp::addFrom(const exp& expr) {
    // if expr is undefined, no-op
    if (expr.getType() == typeName<undefinedExp>()) return;

    // if this expression is undefined, just does assignment
    if (!defined()) {
      (*this) = expr;
      return;
    }

    if (getType() == typeName<map>() && expr.getType() == typeName<map>()) {
      getMap().addFrom(expr.getMap());
    } else if (getType() == typeName<vector>() && expr.getType() == typeName<vector>()) {
      getVector().addFrom(expr.getVector());
    } else {
      callErrorHandler("rcl::exp addFrom: operands must be either both vectors or both maps");
    }
  }
  exp exp::copy(void) {
    // this is a bit of a hack
    return readFromString(toStringCompact(*this))[0];
  }

  /********** functions to aid debugging (mostly used by the parser) */
  
  void exp::setObjectNameRecurse(const string& name) {
    debugInfo.objectName = name;
    if (getType() == typeName<vector>()) {
      vector& self = getValueRef<vector>();
      self.debugInfo = debugInfo;
      std::vector<exp>& vec = self.body;
      FOR (i, vec.size()) {
	vec[i].setObjectNameRecurse(name + "[" + toString((int)i) + "]");
      }
    } else if (getType() == typeName<map>()) {
      map& self = getValueRef<map>();
      self.debugInfo = debugInfo;
      std::vector<map::RCLMapPair>& mp = self.body;
      FOR_EACH (pr, mp) {
	pr->second.setObjectNameRecurse(name + "(\"" + pr->first + "\")");
      }
    }
  }
  
  void exp::setFileLineNumber(const std::string& _inputFile, int _lineNumber)
  {
    debugInfo.setFileLineNumber(_inputFile, _lineNumber);
  }
  void exp::setFileRecurse(const std::string& _inputFile)
  {
    debugInfo.inputFile = _inputFile;
    if (getType() == typeName<vector>()) {
      rcl::vector& self_vec = getVector();
      self_vec.debugInfo.inputFile = _inputFile;
      FOR (i, self_vec.size()) {
	self_vec[i].setFileRecurse(_inputFile);
      }
    } else if (getType() == typeName<map>()) {
      rcl::map& self_map = getMap();
      self_map.debugInfo.inputFile = _inputFile;
      FOR_EACH (pr, self_map) {
	pr->second.setFileRecurse(_inputFile);
      }
    }
  }
  exp::exp(const string& _inputFile, int _lineNumber) {
    init();
    debugInfo.setFileLineNumber(_inputFile, _lineNumber);
  }
  
  /********** internal -- you shouldn't need these */

  void exp::init(void) {
#if USE_MEM_TRACKER
    MTR_NEW(impl, RCLExpImplT<undefinedExp>(undefinedExp()));
#else
    impl = new RCLExpImplT<undefinedExp>(undefinedExp());
#endif
  }
  
  exp::exp(void)
  {
    init();
  }
  
#if 0
  const exp exp::operator()(const string& name) const {
    checkType<map>();
    return getValue<map>().getExpression(name);
  }
  
  exp& exp::operator()(const string& name) {
    checkType<map>();
    return getValueRef<map>().getExpressionRef(name);
  }
#endif

#if 0
  const exp exp::operator[](int index) const {
    checkType<vector>();
    return getValue<vector>().getExpression(index);
  }
  
  exp& exp::operator[](int index) {
    checkType<vector>();
    return getValueRef<vector>().getExpressionRef(index);
  }
#endif

  void exp::writeCommand(ostream& out) const {
    if (getType() == typeName<vector>()) {
      getVector().writeCommand(out);
    } else {
      writeCompact(out);
    }
  }
  
  bool exp::hasComplexType(void) const {
    return ((getType() == typeName<vector>())
	    || (getType() == typeName<map>()));
  }
  
  void exp::commit(void) {
    if (!initData.null()) {
      initData->commit(*this);
    }
  }

  void exp::copyFrom(const exp& rhs) {
    // avoid ref-count problem in the special case that
    //   rhs is a sub-expresion of (*this)
    typeof(impl) tmpCopy = rhs.impl;

    debugInfo = rhs.debugInfo;
    impl = rhs.impl;
  }

  exp::exp(const exp& rhs) {
    copyFrom(rhs);
  }
  exp& exp::operator=(const exp& rhs) {
    copyFrom(rhs);
    commit();
    return *this;
  }

  void exp::SV<vector>::setValue(exp& expr, const vector& data) {
    expr.init();
    vector& h = expr.getValueRef<vector>();
    h = data;
    h.debugInfo = expr.debugInfo;
    expr.commit();
  };
  
  void exp::SV<map>::setValue(exp& expr, const map& data) {
    expr.init();
    map& h = expr.getValueRef<map>();
    h = data;
    h.debugInfo = expr.debugInfo;
    expr.commit();
  };
  
  ostream& operator<<(ostream& out, const exp& expr) {
    writeConfig(out, expr, 0);
    return out;
  }

}; // namespace rcl

/***************************************************************************
 * REVISION HISTORY:
 * $Log: RCLExp.cc,v $
 * Revision 1.24  2006/06/21 22:14:14  trey
 * addFrom() now accepts *either* operand being undefined
 *
 * Revision 1.23  2005/06/01 23:36:00  trey
 * added copy() function for rcl expressions
 *
 * Revision 1.22  2005/05/16 21:10:34  trey
 * fixed a ref-count problem that comes up when assigning a sub-expression of x to x; improved debug output
 *
 * Revision 1.21  2005/05/09 21:23:55  trey
 * added newline to end of rcl output files
 *
 * Revision 1.20  2005/04/21 18:58:56  trey
 * made error messages more readable, added setFileRecurse() function
 *
 * Revision 1.19  2004/08/09 02:24:54  trey
 * added using namespace microraptor to properly pick up mrSmartRef.h
 *
 * Revision 1.18  2004/07/06 15:35:12  trey
 * added optional memory tracking and plugged some memory leaks
 *
 * Revision 1.17  2004/06/18 14:33:14  dom
 * Added support for unsigned ints
 *
 * Revision 1.16  2004/06/10 22:46:43  trey
 * changed default behavior for RCL so that it prints an error before throwing an exception; error handling is now configurable using setErrorHandler()
 *
 * Revision 1.15  2004/05/12 19:25:50  trey
 * made "expr = (char*)NULL" not dump core, people will probably be less confused
 *
 * Revision 1.14  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.13  2004/04/13 21:12:38  trey
 * did some stress testing, fixed several obscure bugs
 *
 * Revision 1.12  2004/04/12 20:23:38  trey
 * fixed toString(vector) to act like toString(exp)
 *
 * Revision 1.11  2004/04/12 19:24:50  trey
 * more api cleanup
 *
 * Revision 1.10  2004/04/09 20:31:33  trey
 * major enhancement to rcl interface
 *
 * Revision 1.9  2004/04/09 00:48:34  trey
 * made changes to support XML front end
 *
 * Revision 1.8  2004/04/08 16:42:56  trey
 * overhauled exp
 *
 * Revision 1.7  2003/06/14 22:31:25  trey
 * added a new "undef" keyword which can be part of a vector/hash and is parsed into an undefined exp
 *
 * Revision 1.6  2003/06/06 14:38:19  trey
 * changed addFromHash to addFrom, made it also accept a vector
 *
 * Revision 1.5  2003/03/12 17:37:58  trey
 * made assignment of map and vector to expressions easier
 *
 * Revision 1.4  2003/02/14 00:13:11  trey
 * added better support for checking the type of numeric constants
 *
 * Revision 1.3  2003/02/05 22:50:51  trey
 * mostly ready for mraptor now
 *
 * Revision 1.2  2003/02/05 20:23:20  trey
 * implemented command syntax and switched error behavior to throw exceptions
 *
 * Revision 1.1  2003/02/05 03:57:54  trey
 * initial atacama check-in
 *
 * Revision 1.3  2002/09/25 04:26:04  trey
 * fixed defined() returning the exact opposite of what it should
 *
 * Revision 1.2  2002/09/24 22:18:20  trey
 * added writeToFile() function
 *
 * Revision 1.1  2002/09/24 21:50:23  trey
 * rebuilt RCL from scratch to support nested vectors and hashes
 *
 *
 ***************************************************************************/
