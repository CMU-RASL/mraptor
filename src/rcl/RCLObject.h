/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.13 $  $Author: trey $  $Date: 2005/12/07 18:15:34 $
 *  
 * PROJECT: Life in the Atacama
 *
 * @file    RCLObject.h
 * @brief   No brief
 ***************************************************************************/

#ifndef INCRCLObject_h
#define INCRCLObject_h

#include "RCL.h"

namespace rcl {
  template <class T>
  struct ObjectGV {
    static T getValue(const exp& expr) {
      return (T) expr;
    }
  };

#define RCL_OBJECT_GV(TYPE)                       \
    template<> struct ObjectGV<TYPE> {            \
      static TYPE getValue(const exp& expr) {     \
        return expr.getValue<TYPE>();             \
      }                                           \
    }
  RCL_OBJECT_GV(bool);
  RCL_OBJECT_GV(string);

  RCL_OBJECT_GV(vector);
  RCL_OBJECT_GV(map);

  RCL_OBJECT_GV(float);
  RCL_OBJECT_GV(double);
  RCL_OBJECT_GV(int);
  RCL_OBJECT_GV(unsigned int);
  RCL_OBJECT_GV(long);

  template <class T>
  void syncField(const std::string& name, T& var,
		 rcl::exp& expr, bool readp)
  {
    if (readp) {
      var = ObjectGV<T>::getValue(expr(name));
      //var = expr(name).template getValue<T>();
    } else {
      if (expr.getType() != typeName<map>()) {
	expr = rcl::map();
      }
      expr(name) = var;
    }
  }

  template <class T>
  void syncFieldDefault(const std::string& name,
			T& var, const T& defaultValue,
			rcl::exp& expr, bool readp)
  {
    if (readp) {
      rcl::exp& field = expr(name);
      if (field.defined()) {
	var = ObjectGV<T>::getValue(field);
      } else {
	var = defaultValue;
      }
    } else {
      if (expr.getType() != typeName<map>()) {
	expr = rcl::map();
      }
#if 0
      // old behavior, tends to cause errors
      if (defaultValue == var) {
	// assigning rcl::exp() to an exp makes it undefined
	expr(name) = rcl::exp();
      } else {
	expr(name) = var;
      }
#else
      expr(name) = var;
#endif
    }
  }

  template <class T>
  void syncFieldIfDefined(const std::string& name,
			  T& var, bool& isDefined,
			  rcl::exp& expr, bool readp)
  {
    if (readp) {
      rcl::exp& field = expr(name);
      isDefined = field.defined();
      if (field.defined()) {
	var = ObjectGV<T>::getValue(field);
      }
    } else {
      if (expr.getType() != typeName<map>()) {
	expr = rcl::map();
      }
      if (isDefined) {
	expr(name) = var;
      } else {
	// assigning rcl::exp() to an exp makes it undefined
	expr(name) = rcl::exp();
      }
    }
  }

  template <class T>
  void syncFieldNew(const std::string& name, T& var,
		    rcl::exp& expr, bool readp)
  {
    typedef typename T::value_type value_type;
    if (readp) {
      rcl::exp& field = expr(name);
      if (field.defined()) {
	var.bind(new value_type(ObjectGV<value_type>::getValue(field)));
      } else {
	var.bind(NULL);
      }
    } else {
      if (expr.getType() != typeName<map>()) {
	expr = rcl::map();
      }
      if (var.defined()) {
	expr(name) = *var;
      } else {
	// assigning rcl::exp() to an exp makes it undefined
	expr(name) = rcl::exp();
      }
    }
  }

  template <class T>
  void syncVector(std::vector<T>& var, rcl::exp& expr, bool readp)
  {
    if (readp) {
      rcl::vector& vec = expr.getVector();
      var.clear();
      FOR_EACH ( elt_p, vec ) {
	var.push_back( ObjectGV<T>::getValue(*elt_p) );
      }
    } else {
      expr = rcl::vector();
      rcl::vector& vec = expr.getVector();
      FOR_EACH( elt_p, var ) {
	vec.push_back( *elt_p );
      }
    }
  }

#define RCL_SYNC_FUNCTION(CLASS_NAME) \
  void readFrom(const rcl::exp& expr) { \
    syncObject(const_cast<rcl::exp&>(expr), /* readp = */ true); \
  } \
  void writeTo(rcl::exp& expr) const { \
    CLASS_NAME& thisRef = const_cast<CLASS_NAME&>(*this); \
    thisRef.syncObject(expr, /* readp = */ false); \
  } \
  void syncObject(rcl::exp& expr, bool readp)

#define RCL_SYNC_FIELD_FULL(RCLFIELD, VARNAME) \
  rcl::syncField(RCLFIELD, VARNAME, expr, readp)

#define RCL_SYNC_FIELD(VARNAME) \
  RCL_SYNC_FIELD_FULL(#VARNAME, VARNAME)

#define RCL_SYNC_FIELD_IF_DEFINED_FULL(RCLFIELD, VARNAME, VARDEFINED) \
  rcl::syncFieldIfDefined(RCLFIELD, VARNAME, VARDEFINED, expr, readp)

#define RCL_SYNC_FIELD_IF_DEFINED(VARNAME, VARDEFINED) \
  RCL_SYNC_FIELD_IF_DEFINED_FULL(#VARNAME, VARNAME, VARDEFINED)

#define RCL_SYNC_FIELD_DEFAULT(VARNAME, DEFAULTVALUE) \
  rcl::syncFieldDefault(#VARNAME, VARNAME, DEFAULTVALUE, expr, readp)

#define RCL_SYNC_FIELD_NEW(VARNAME) \
  rcl::syncFieldNew(#VARNAME, VARNAME, expr, readp)

#define RCL_SYNC_VECTOR(VARNAME) \
  rcl::syncVector(VARNAME, expr, readp)

#define RCL_SYNC_ENUM_VALUE(ENUM_STRING, ENUM_INT) \
  if (readp) {                                     \
    if (expr.getString() == ENUM_STRING) {         \
      value = ENUM_INT;                            \
      return;                                      \
    }                                              \
  } else {                                         \
    if (ENUM_INT == value) {                       \
      expr = ENUM_STRING;                          \
      return;                                      \
    }                                              \
  }

#define RCL_SYNC_ENUM_ERR_CHECK(CLASSNAME)               \
    if (readp) {                                         \
      rcl::callErrorHandler                              \
	(std::string("reading enumerated type ")         \
	 + #CLASSNAME + ": unexpected string value \""   \
	 + expr.getString() + "\"");                     \
    } else {                                             \
      rcl::callErrorHandler                              \
	(std::string("writing enumerated type ")         \
	 + #CLASSNAME + ": unexpected enum value "       \
	 + rcl::toString(value));                        \
    }

#define RCL_DEFINE_OPERATORS_INTERNAL(CLASS_NAME) \
  CLASS_NAME& operator=(const rcl::exp& expr) { \
    readFrom(expr); \
    return (*this); \
  } \
  operator rcl::exp() const { \
    rcl::exp expr; \
    writeTo(expr); \
    return expr; \
  } \
  rcl::exp toExp() const { return (rcl::exp) (*this); } \
  std::string toStringConfig(void) const { return rcl::toStringConfig(toExp()); } \
  std::string toStringCompact(void) const { return rcl::toStringCompact(toExp()); } \
  std::string toString(void) const { return rcl::toString(toExp()); }
};

// normally use this version
#define RCL_DEFINE_OPERATORS(CLASS_NAME) \
  CLASS_NAME(const rcl::exp& expr) { \
    readFrom(expr); \
  } \
  RCL_DEFINE_OPERATORS_INTERNAL(CLASS_NAME)

// ... but if you need to do something before the readFrom() call in
// the rcl::exp constructor, define the preReadFromInRCLExpConstructor()
// function and use this version (yes, this is a kludge, but necessary
// for STRING_TYPE and RCL_STRING_TYPE in LITA's commonTypes.h)
#define RCL_DEFINE_OPERATORS_USE_PRE_READ_FROM(CLASS_NAME) \
  CLASS_NAME(const rcl::exp& expr) { \
    preReadFromInRCLExpConstructor(); \
    readFrom(expr); \
  } \
  RCL_DEFINE_OPERATORS_INTERNAL(CLASS_NAME)
  

#endif // INCRCLObject_h

/***************************************************************************
 * REVISION HISTORY:
 * $Log: RCLObject.h,v $
 * Revision 1.13  2005/12/07 18:15:34  trey
 * committing dom's changes for gcc 3.4.0
 *
 * Revision 1.12  2004/12/01 18:38:10  trey
 * changed some string references to std::string
 *
 * Revision 1.11  2004/09/06 16:37:28  trey
 * added RCL_SYNC_ENUM functions
 *
 * Revision 1.10  2004/09/01 20:39:08  trey
 * changed behavior of RCL_SYNC_FIELD_DEFAULT to be less error-prone
 *
 * Revision 1.9  2004/08/09 02:25:48  trey
 * added RCL_SYNC_FIELD_NEW
 *
 * Revision 1.8  2004/08/06 21:56:03  trey
 * added RCL_SYNC_FIELD_DEFAULT()
 *
 * Revision 1.7  2004/07/26 00:13:55  trey
 * added RCL_SYNC_VECTOR
 *
 * Revision 1.6  2004/07/06 15:35:12  trey
 * added optional memory tracking and plugged some memory leaks
 *
 * Revision 1.5  2004/06/18 19:23:04  trey
 * changed to properly handle getValue<T>() when T is a class with RCLObject operators defined
 *
 * Revision 1.4  2004/06/04 13:24:33  trey
 * modified to catch possible future problem with rcl::exp constructor
 *
 * Revision 1.3  2004/05/24 18:29:03  trey
 * minor fixes and additions to API
 *
 * Revision 1.2  2004/05/12 19:26:07  trey
 * added RCLObject.h to installed headers
 *
 * Revision 1.1  2004/05/04 17:45:15  trey
 * initial check-in
 *
 *
 ***************************************************************************/
