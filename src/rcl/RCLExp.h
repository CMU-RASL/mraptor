/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.24 $  $Author: trey $  $Date: 2005/12/07 18:15:34 $
 *  
 * PROJECT: FIRE Architecture Project
 *
 * @file    RCLExp.h
 * @brief   No brief
 ***************************************************************************/

#ifndef INCRCLExp_h
#define INCRCLExp_h

#include <iostream>

#include "RCLTypes.h"
#include "mrSmartRef.h"
#include "RCLException.h"
#include "RCLDebugInfo.h"

namespace rcl {
  struct exp {
    /********** functions that act as casting operators */

    double getDouble(void) const;
    long getLong(void) const;
    bool getBool(void) const;
    string getString(void) const;

    vector& getVector(void);
    map& getMap(void);

    const vector& getVector(void) const;
    const map& getMap(void) const;
    
    /********** assignment operators (and corresponding constructors) */

    exp& operator=(const exp& x);
    exp& operator=(double x);
    exp& operator=(long x);
    exp& operator=(bool x);
    exp& operator=(const string& x);
    exp& operator=(const vector& x);
    exp& operator=(const map& x);
    exp& operator=(int x);
    exp& operator=(unsigned int x);
    exp& operator=(float x);
    exp& operator=(const char *x);

    exp(const exp& x);
    exp(double x);
    exp(long x);
    exp(bool x);
    exp(const string& x);
    exp(const vector& x);
    exp(const map& x);
    exp(int x);
    exp(unsigned int x);
    exp(float x);
    exp(const char *x);

    /********** pass-through functions */

    /*-------------- vector */
    
    exp& operator[](int index);
    const exp operator[](int index) const;
    void push_back(const exp& elt);
    
    /*-------------- map */

    exp& operator()(const string& name);
    const exp operator()(const string& name) const;

    /********** information functions */

    bool defined(void) const;
    string getType(void) const;
    string getDebugTrace(void) const;
    string getUndefDebugTrace(void) const;

    /********** misc functions */

    void writeToFile(const string& fileName);
    void addFrom(const exp& expr);
    exp copy(void);

    /********** functions to aid debugging (mostly used by the parser) */

    void setObjectNameRecurse(const string& name);
    void setFileLineNumber(const string& _inputFile,
			   int _lineNumber);
    void setFileRecurse(const std::string& _inputFile);
    exp(const string& _inputFile, int _lineNumber);

    /********** internal -- you shouldn't need these */

    struct Impl {
      virtual ~Impl(void) { }
      virtual string getType(void) const = 0;
      virtual void writeConfig(std::ostream& out, int indent) const = 0;
      virtual void writeCompact(std::ostream& out) const = 0;
    };
    struct InitData {
      virtual void commit(exp& thisExp) = 0;
      virtual ~InitData(void) {}
    };
    microraptor::SmartRef< Impl > impl;
    DebugInfo debugInfo;
    microraptor::SmartRef< InitData > initData;
    
    exp(void);
    exp(microraptor::SmartRef< Impl > _impl) : impl(_impl) { }

    void init(void);
    void copyFrom(const exp& expr);
    void commit(void);

    void writeConfig(std::ostream& out, int indent) const {
      impl->writeConfig(out, indent);
    }
    void writeCompact(std::ostream& out) const {
      impl->writeCompact(out);
    }
    void writeCommand(std::ostream& out) const;

    // getValue<T> looks for internal of type T
    //   except: when T is numeric, cast internal double to T on the way out
    // getValueRef<T> should only be used for map and vector
    // setValue<T> looks for internal of type T
    //   except: when T is numeric, cast T to internal double on the way in

    template <class T>
    const T& getValueInternal(void) const;

    template <class T>
    T getValueNumeric(void) const;
    
    // we need this silly internal struct so that we can do explicit
    // specialization
    template <class T>
    struct GV {
      static T getValue(const exp& expr) {
	return expr.getValueInternal<T>();
      }
    };

    template <class T>
    T getValue(void) const {
      return GV<T>::getValue(*this);
    }
    
    template <class T>
    T& getValueRef(void);
    
    template <class T>
    const T& getValueConstRef(void) const;
    
    template <class T>
    void setValueInternal(const T& _val) {
      getValueRef<T>() = _val;
    }

    // we need this silly internal struct so that we can do explicit
    // specialization
    template <class T>
    struct SV {
      static void setValue(exp& expr, const T& data) {
	expr.init();
	expr.setValueInternal(data);
	expr.commit();
      }
    };
        
    template <class T>
    void setValue(const T& data) {
      SV<T>::setValue(*this, data);
    }
    
    template <class AccessType>
    void checkType(void) const;
    
    bool hasComplexType(void) const;
  };

  /* Dom: Moved these explicit specializations out of struct exp
     to avoid compiler errors under gcc 3.4 */

#define GV_NUMERIC(TYPE)                       \
    template <> struct exp::GV< TYPE > {       \
      static TYPE getValue(const exp& expr) {  \
        return expr.getValueNumeric<TYPE>();   \
      }                                        \
    }

  GV_NUMERIC( float        );
  GV_NUMERIC( double       );
  GV_NUMERIC( int          );
  GV_NUMERIC( unsigned int );
  GV_NUMERIC( long         );

  template<> struct exp::SV<vector> {
    static void setValue(exp& expr, const vector& data);
  };

  template<> struct exp::SV<map> {
    static void setValue(exp& expr, const map& data);
  };


  template <class T>
  struct RCLExpImplT : public exp::Impl {
    T val;
    
    RCLExpImplT(T _val) : val(_val) { }
    
    virtual string getType(void) const {
      return typeName<T>();
    }
    virtual void writeCompact(std::ostream& out) const {
      return rcl::writeCompact(out, val);
    }
    virtual void writeConfig(std::ostream& out, int indent) const {
      return rcl::writeConfig(out, val, indent);
    }
  };

  template <class AccessType>
  void exp::checkType(void) const {
    if (getType() != typeName<AccessType>()) {
      if (getType() == typeName<undefinedExp>()
	  && debugInfo.undefCreationTrace != "") {
	callErrorHandler(debugInfo.undefCreationTrace);
      } else {
	string s = string("type mismatch: attempted access as type ")
	  + typeName<AccessType>()
	  + " but actual type is " + getType()
	  + ", from " + getDebugTrace();
	callErrorHandler(s);
      }
    }
  }

  template <class T>
  const T& exp::getValueInternal(void) const {
    checkType<T>();
    return ((RCLExpImplT<T> *) impl.pointer())->val;
  }

  template <class T>
  T exp::getValueNumeric(void) const {
    T val = -1;
    string type = impl->getType();
    
    if (type == "long") {
      val = (T) getValueInternal<long>();
    } else if (type == "double") {
      val = (T) getValueInternal<double>();
    } else {
      // this should throw an exception
      checkType<T>();
#if 0
      string s = "trying to extract numeric value from exp of type `"
	+ impl->getType() + "'";
      callErrorHandler(s);
#endif
    }
    
    return val;
  }

  template <class T>
  const T& exp::getValueConstRef(void) const {
    checkType<T>();
    return ((RCLExpImplT<T> *) impl.pointer())->val;
  }

  template <class T>
  T& exp::getValueRef(void) {
    // make sure this is an expression of the right type
    if (getType() != typeName<T>()) {
#if USE_MEM_TRACKER
      MTR_NEW(impl, RCLExpImplT<T>(T()));
#else
      impl = new RCLExpImplT<T>(T());
#endif
    }
    return ((RCLExpImplT<T> *) impl.pointer())->val;
  }

  std::ostream& operator<<(std::ostream& out, const exp& expr);

}; // namespace rcl

#endif // INCRCLExp_h

/***************************************************************************
 * REVISION HISTORY:
 * $Log: RCLExp.h,v $
 * Revision 1.24  2005/12/07 18:15:34  trey
 * committing dom's changes for gcc 3.4.0
 *
 * Revision 1.23  2005/06/01 23:36:00  trey
 * added copy() function for rcl expressions
 *
 * Revision 1.22  2005/05/16 21:10:34  trey
 * fixed a ref-count problem that comes up when assigning a sub-expression of x to x; improved debug output
 *
 * Revision 1.21  2005/04/21 18:58:56  trey
 * made error messages more readable, added setFileRecurse() function
 *
 * Revision 1.20  2004/08/09 02:25:23  trey
 * now explicitly use microraptor::SmartRef
 *
 * Revision 1.19  2004/07/06 15:35:12  trey
 * added optional memory tracking and plugged some memory leaks
 *
 * Revision 1.18  2004/06/18 17:14:18  dom
 * Added support for unsigned ints
 *
 * Revision 1.17  2004/06/18 14:33:14  dom
 * Added support for unsigned ints
 *
 * Revision 1.16  2004/06/10 22:46:43  trey
 * changed default behavior for RCL so that it prints an error before throwing an exception; error handling is now configurable using setErrorHandler()
 *
 * Revision 1.15  2004/05/04 17:54:44  trey
 * improved an error message
 *
 * Revision 1.14  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.13  2004/04/13 21:12:38  trey
 * did some stress testing, fixed several obscure bugs
 *
 * Revision 1.12  2004/04/12 19:53:42  trey
 * fixed some problems with push_back()
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
 * Revision 1.7  2003/11/21 19:50:41  trey
 * no longer an error to extract e.g. expr.getValue<double>() when expr has type long; instead, does casting properly
 *
 * Revision 1.6  2003/06/06 14:38:19  trey
 * changed addFromHash to addFrom, made it also accept a vector
 *
 * Revision 1.5  2003/03/12 17:37:59  trey
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
 * Revision 1.3  2002/09/25 04:25:27  trey
 * fixed bug casting double to integer type
 *
 * Revision 1.2  2002/09/24 22:18:21  trey
 * added writeToFile() function
 *
 * Revision 1.1  2002/09/24 21:50:23  trey
 * rebuilt RCL from scratch to support nested vectors and hashes
 *
 *
 ***************************************************************************/
