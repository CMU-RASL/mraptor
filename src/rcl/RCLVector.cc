/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.20 $  $Author: trey $  $Date: 2005/05/16 21:10:42 $
 *  
 * PROJECT: FIRE Architecture Project
 *
 * @file    vector.cc
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
#include <iomanip>

#include "mrCommonDefs.h"
#include "RCL.h"

using namespace std;

namespace rcl {
  /********** accessor functions */

  exp& vector::operator[](int index) {
    return getExpressionRef(index);
  }
  const exp& vector::operator[](int index) const {
    return getExpression(index);
  }

  /********** information functions */
  
  size_t vector::size(void) const { return body.size(); }
  bool vector::empty(void) const { return body.empty(); }
  string vector::getDebugTrace(void) const { return debugInfo.getDebugTrace(); }

  /********** iteration functions */
  
  vector::iterator vector::begin(void) { return body.begin(); }
  vector::iterator vector::end(void) { return body.end(); }
  
  vector::const_iterator vector::begin(void) const { return body.begin(); }
  vector::const_iterator vector::end(void) const { return body.end(); }

  /********** misc functions */
  
  void vector::clear(void) {
    body.clear();
  }
  void vector::deleteElement(unsigned index) {
    body.erase(body.begin() + index, body.begin() + (index+1));
  }
  void vector::addFrom(const vector& rhs) {
    FOR_EACH(pr, rhs.body) {
      add(*pr);
    }    
  }
  void vector::push_back(const exp& expr) {
    setExpression( body.size(), expr );
  }
  bool vector::isNumeric(void) const {
    FOR_EACH(elt, body) {
      std::string s = elt->getType();
      if ( !(("long" == s) || ("double" == s)) ) {
	return false;
      }
    }
    return true;
  }
  
  /********** functions to aid debugging (mostly used by the parser) */
  
  void vector::setFileLineNumber(const string& _inputFile,
			      int _lineNumber)
  {
    debugInfo.setFileLineNumber(_inputFile, _lineNumber);
  }
  vector::vector(const string& _inputFile, int _lineNumber) {
    debugInfo.setFileLineNumber(_inputFile, _lineNumber);
  }

  /********** internal -- you shouldn't need these */

  static void setCreationTrace(exp& expr,
			       unsigned index,
			       const DebugInfo& debugInfo)
  {
    expr.debugInfo = debugInfo;
    expr.debugInfo.objectName = debugInfo.objectName
      + "[" + toString((int)index) + "]";
    // FIX we probably don't need to keep undefCreationTrace at all
    expr.debugInfo.undefCreationTrace =
      string("trying to access undefined ")
      + expr.debugInfo.getUndefDebugTrace();
  }

  void vector::add(const exp& expr) {
    body.push_back(expr);
  }
  
  struct VectorInitData : public exp::InitData {
    vector& parent;
    int index;
    VectorInitData(vector& _parent, int _index) :
      parent(_parent),
      index(_index)
    {}
    void commit(exp& thisExp) {
      parent.setExpression(index, thisExp);
      thisExp.initData = NULL;
    }
  };

  const exp& vector::getExpression(unsigned index) const {
    static exp expr;
    // if the key is missing, return an undefined exp
    if (index >= body.size() || !body[index].defined()) {
      setCreationTrace(expr, index, debugInfo);
      return expr;
    }
    else return body[index];
  }
  
  exp& vector::getExpressionRef(unsigned index) {
    static exp expr;
    if (index >= body.size() || !body[index].defined()) {
      expr.init();
      setCreationTrace(expr, index, debugInfo);
#if USE_MEM_TRACKER
      MTR_NEW(expr.initData, VectorInitData(*this, index));
#else
      expr.initData = new VectorInitData(*this, index);
#endif
      return expr;
    } else {
      return body[index];
    }
  }
  
  void vector::setExpression(unsigned index, const exp& expr) {
    // if the indexed location is missing, create it
    if (index >= body.size()) {
      for (unsigned i = body.size(); i < index+1; i++) {
	exp temp;
	setCreationTrace(temp, index, debugInfo);
	add(temp);
      }
    }
    body[index] = expr;
  }

  size_t vector::getDefinedSize(void) const {
    for (int i=(int)body.size()-1; i >= 0; i--) {
      if (body[i].defined()) {
	return i+1;
      }
    }
    return 0;
  }
  
  void vector::writeCommand(ostream& out) const {
    int definedSize = getDefinedSize();
    for (int i=0; i < definedSize; i++) {
      if (body[i].getType() == typeName<string>()) {
	rcl::writeStringOptQuotes(out, body[i].getString());
      } else {
	rcl::writeCompact(out, body[i]);
      }
      if (i < definedSize-1) out << " ";
    }
  }
  
  // like writeConfig, but keep things compact and on one line
  void vector::writeCompact(ostream& out) const {
    int definedSize = getDefinedSize();
    out << "[";
    for (int i=0; i < definedSize; i++) {
      rcl::writeCompact(out, body[i]);
      if (i < definedSize-1) out << ",";
    }
    out << "]";
  }
  
  void vector::writeConfig(ostream& out, int indentChars) const {
    int definedSize = getDefinedSize();

    if (isNumeric()) {
      int i;
      int fieldsPerLine = (int) ((80+2-indentChars)/18);
      out << "[";
      for (i=0; i < definedSize; i++) {
	if (0 == (i % fieldsPerLine)) {
	  if (definedSize > fieldsPerLine) {
	    if (0 != i) out << ",";
	    out << endl << indent(indentChars+2);
	  } else {
	    out << " ";
	  }
	} else {
	  out << ", ";
	}
	if (definedSize > fieldsPerLine) out << setw(16);
	out << setprecision(10)
	    << body[i].getValueNumeric<double>();
      }
      if (definedSize > fieldsPerLine) {
	out << endl << indent(indentChars);
      } else {
	out << " ";
      }
      out << "]";
    } else {
      out << "[\n";
      for (int i=0; i < definedSize; i++) {
	out << indent(indentChars+2);
	body[i].writeConfig(out, indentChars+2);
	if (i < definedSize-1) out << ",";
	out << "\n";
      }
      out << indent(indentChars) << "]";
    }
  }

}; // namespace rcl

std::ostream& operator<<(std::ostream& out, const rcl::vector& m) {
  return out << ((rcl::exp) m);
}

/***************************************************************************
 * REVISION HISTORY:
 * $Log: RCLVector.cc,v $
 * Revision 1.20  2005/05/16 21:10:42  trey
 * improved debug output
 *
 * Revision 1.19  2005/05/12 00:17:01  trey
 * fixed bugs in error messages
 *
 * Revision 1.18  2005/04/21 18:59:41  trey
 * now store more information in debugInfo
 *
 * Revision 1.17  2004/12/15 20:00:10  trey
 * fixed compact vector printing
 *
 * Revision 1.16  2004/12/14 23:56:38  trey
 * improved printing of vectors that are entirely numeric
 *
 * Revision 1.15  2004/08/02 02:56:15  trey
 * strings are now quoted more often
 *
 * Revision 1.14  2004/07/06 15:35:12  trey
 * added optional memory tracking and plugged some memory leaks
 *
 * Revision 1.13  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.12  2004/04/13 21:12:38  trey
 * did some stress testing, fixed several obscure bugs
 *
 * Revision 1.11  2004/04/12 19:53:42  trey
 * fixed some problems with push_back()
 *
 * Revision 1.10  2004/04/12 19:24:50  trey
 * more api cleanup
 *
 * Revision 1.9  2004/04/09 20:31:33  trey
 * major enhancement to rcl interface
 *
 * Revision 1.8  2004/04/08 16:42:56  trey
 * overhauled exp
 *
 * Revision 1.7  2003/06/06 14:38:19  trey
 * changed addFromHash to addFrom, made it also accept a vector
 *
 * Revision 1.6  2003/03/14 23:19:09  mwagner
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
 * Revision 1.1  2002/09/24 21:50:25  trey
 * rebuilt RCL from scratch to support nested vectors and hashes
 *
 *
 ***************************************************************************/
