/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.13 $  $Author: trey $  $Date: 2005/06/16 00:26:14 $
 *  
 * PROJECT: FIRE Architecture Project
 *
 * @file    map.cc
 * @brief   No brief
 ***************************************************************************/

/***************************************************************************
 * INCLUDES
 ***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <iostream>
#include <algorithm>

#include "RCL.h"

using namespace std;

namespace rcl {

  /********** accessor functions */

  exp& map::operator()(const string& name) {
    return getExpressionRef(name);
  }
  const exp& map::operator()(const string& name) const {
    return getExpression(name);
  }

  /********** information functions */
  
  size_t map::size(void) const {
    return body.size();
  }
  bool map::empty(void) const {
    return body.empty();
  }
  string map::getDebugTrace(void) const { return debugInfo.getDebugTrace(); }

  /********** iteration functions */
  
  map::iterator map::begin(void) { return body.begin(); }
  map::iterator map::end(void) { return body.end(); }
  
  map::const_iterator map::begin(void) const { return body.begin(); }
  map::const_iterator map::end(void) const { return body.end(); }

  /********** misc functions */
  
  void map::clear(void) {
    body.clear();
  }
  void map::push_back(const string& name, const exp& expr) {
    // was this ever the right thing to do?  i'm confused...
    //body.push_back(RCLMapPair(name,expr));
    setExpression(name,expr);
  }
  void map::addFrom(const map& rhs) {
    FOR_EACH(pr, rhs.body) {
      add(pr->first, pr->second);
    }    
  }
  /********** functions to aid debugging (mostly used by the parser) */
  
  void map::setFileLineNumber(const string& _inputFile,
			      int _lineNumber)
  {
    debugInfo.setFileLineNumber(_inputFile, _lineNumber);
  }
  map::map(const string& _inputFile, int _lineNumber) {
    debugInfo.setFileLineNumber(_inputFile, _lineNumber);
  }

  // this version implements the more expected overwrite syntax
  void map::add(const string& name, const exp& expr) {
    setExpression( name, expr );
  }
  
  static void setCreationTrace(exp& expr,
			       const string& name,
			       const DebugInfo& debugInfo)
  {
    expr.debugInfo = debugInfo;
    expr.debugInfo.objectName = debugInfo.objectName
      + "(\"" + name + "\")";
    // FIX we probably don't need to keep undefCreationTrace at all
    expr.debugInfo.undefCreationTrace =
      string("trying to access undefined ")
      + expr.debugInfo.getUndefDebugTrace();
  }
  
  struct MapInitData : public exp::InitData {
    map& parent;
    string name;
    MapInitData(map& _parent, string _name) :
      parent(_parent),
      name(_name)
    {}
    void commit(exp& thisExp) {
      parent.setExpression(name, thisExp);
      thisExp.initData = NULL;
    }
  };

  int map::find(const string& name) const {
    FOR (i, body.size()) {
      if (body[i].first == name) return i;
    }
    return -1;
  }

  void map::setExpression(const string& name, const exp& expr) {
    // if the key is missing, create it
    int index;
    if (-1 == (index = find(name))) {
      body.push_back(RCLMapPair(name, expr));
    } else {
      body[index].second = expr;
    }
  }
  
  exp& map::getExpressionRef(const string& name) {
    static exp expr;
    int index;
    if (-1 == (index = find(name))) {
      expr.init();
      setCreationTrace(expr, name, debugInfo);
#if USE_MEM_TRACKER
      MTR_NEW(expr.initData, MapInitData(*this, name));
#else
      expr.initData = new MapInitData(*this, name);
#endif
      return expr;
    } else {
      return body[index].second;
    }
  }

  const exp& map::getExpression(const string& name) const {
    static exp expr;
    int index;
    // if the key is missing, return an undefined exp
    if (-1 == (index = find(name))) {
      setCreationTrace(expr, name, debugInfo);
      return expr;
    } else {
      return body[index].second;
    }
  }
  
  int map::getLastDefined(void) const {
    for (int i=body.size()-1; i >= 0; i--) {
      if (body[i].second.defined()) {
	return i;
      }
    }
    return -1;
  }
  
  void map::writeCompact(ostream& out) const {
    int lastDefined = getLastDefined();
    out << "{";
    for (int i=0; i <= lastDefined; i++) {
      if (body[i].second.defined()) {
	out << body[i].first;
	//rcl::writeCompact(out, body[i].first);
	out << "=";
	rcl::writeCompact(out, body[i].second);
	if (i < lastDefined) out << ",";
      }
    }
    out << "}";
  }

  void map::writeConfig(ostream& out, int indentChars) const {
    int lastDefined = getLastDefined();
    out << "{\n";
    exp e;
    for (int i=0; i <= lastDefined; i++) {
      e = body[i].second;
      if (e.defined()) {
	out << indent(indentChars+2) << body[i].first << " => ";
	if (e.hasComplexType()) out << "\n" << indent(indentChars+4);
	e.writeConfig(out, indentChars+4);
	if (i < lastDefined) out << ",";
	out << "\n";
      }
    }
    out << indent(indentChars) << "}";
  }

  void map::erase(const string& key)
  {
    int index;
    if (-1 == (index = find(key))) {
      throw rcl::exception(string("tried to erase undefined key ")
			   + key);
    }
    body.erase(body.begin() + index, body.begin() + (index+1));
  }


}; // namespace rcl

std::ostream& operator<<(std::ostream& out, const rcl::map& m) {
  return out << ((rcl::exp) m);
}

/***************************************************************************
 * REVISION HISTORY:
 * $Log: RCLMap.cc,v $
 * Revision 1.13  2005/06/16 00:26:14  trey
 * removed old version of push_back() which should never have been used by code outside librcl
 *
 * Revision 1.12  2005/06/14 15:50:03  trey
 * removed use of std::map from rcl::map, now simpler but less efficient for really big maps
 *
 * Revision 1.11  2005/05/16 21:10:42  trey
 * improved debug output
 *
 * Revision 1.10  2005/05/12 00:17:01  trey
 * fixed bugs in error messages
 *
 * Revision 1.9  2005/04/21 18:59:16  trey
 * made error messages more readable
 *
 * Revision 1.8  2004/12/14 23:56:38  trey
 * improved printing of vectors that are entirely numeric
 *
 * Revision 1.7  2004/10/08 02:24:34  trey
 * added erase() function
 *
 * Revision 1.6  2004/08/02 03:25:54  trey
 * fixed problem with quoted map keys
 *
 * Revision 1.5  2004/07/06 15:35:12  trey
 * added optional memory tracking and plugged some memory leaks
 *
 * Revision 1.4  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.3  2004/04/13 21:12:38  trey
 * did some stress testing, fixed several obscure bugs
 *
 * Revision 1.2  2004/04/12 19:24:50  trey
 * more api cleanup
 *
 * Revision 1.1  2004/04/09 20:31:33  trey
 * major enhancement to rcl interface
 *
 * Revision 1.9  2004/04/08 16:42:56  trey
 * overhauled exp
 *
 * Revision 1.8  2003/06/06 16:11:40  trey
 * fixed addFrom() to overwrite as advertised
 *
 * Revision 1.7  2003/06/06 14:38:19  trey
 * changed addFromHash to addFrom, made it also accept a vector
 *
 * Revision 1.6  2003/03/14 23:19:08  mwagner
 * fixed nasty bug with assigning -1 to unsigned int, seg fault
 *
 * Revision 1.5  2003/03/14 02:24:12  trey
 * kludge fixed problem that checking whether an element of a hash or vector is defined() creates a new entry for it
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
 * Revision 1.1  2002/09/24 21:50:24  trey
 * rebuilt RCL from scratch to support nested vectors and hashes
 *
 *
 ***************************************************************************/
