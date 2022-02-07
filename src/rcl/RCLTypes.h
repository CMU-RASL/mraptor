/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.17 $  $Author: trey $  $Date: 2005/12/07 18:15:34 $
 *  
 * PROJECT: FIRE Architecture Project
 *
 * @file    RCLTypes.h
 * @brief   No brief
 ***************************************************************************/

#ifndef INCRCLTypes_h
#define INCRCLTypes_h

#include <string>
// Made change to compile under gcc 3.2 in Cygwin; should work elsewhere
#include <sstream>

/**********************************************************************
 * Meta-macros
 **********************************************************************/

// repeat macro for the primitive types that are directly stored
#define RCL_REPEAT_MACRO_DIRECT(macro) \
  macro(string); \
  macro(double); \
  macro(bool);

// repeat macro for the numeric types that are cast to double for storage
#define RCL_REPEAT_MACRO_NUMERIC(macro) \
  macro(float); \
  macro(int); \
  macro(long);

// repeat macro for all primitive types (directly stored and cast)
#define RCL_REPEAT_MACRO_ALL(macro) \
  RCL_REPEAT_MACRO_DIRECT(macro); \
  RCL_REPEAT_MACRO_NUMERIC(macro);

namespace rcl {

  /**********************************************************************
   * Forward declarations
   **********************************************************************/

  struct exp;
  struct map;
  struct vector;
  typedef std::string string;
  struct undefinedExp {};

  /**********************************************************************
   * Printing primitive data types
   **********************************************************************/
  
  template <class T>
  void writeCompact(std::ostream& out, const T& data) {
    data.writeCompact(out);
  }
  
  template <class T>
  void writeConfig(std::ostream& out, const T& data, int indent) {
    data.writeConfig(out, indent);
  }

#define RCL_WRITE_HEADER(type) \
  void writeCompact(std::ostream& out, const type& data); \
  void writeConfig(std::ostream& out, const type& data, int indent);

  RCL_WRITE_HEADER(undefinedExp);
  RCL_REPEAT_MACRO_ALL(RCL_WRITE_HEADER);

  void writeStringOptQuotes(std::ostream& out, const string& data);

  const char *indent(int indent);

  /**********************************************************************
   * Type names
   **********************************************************************/

  template <class T>
  struct TypeStruct {
    static string getType(void) {
      return "externally_defined_type";
    }
  };
  
#define RCL_TYPE_STRUCT_NAMED(ctype,rcltypename) \
  template <> struct TypeStruct< ctype > { \
    static string getType(void) { \
      return rcltypename; \
    } \
  };
#define RCL_TYPE_STRUCT(type) RCL_TYPE_STRUCT_NAMED(type,#type)

  RCL_REPEAT_MACRO_ALL(RCL_TYPE_STRUCT);

  RCL_TYPE_STRUCT_NAMED(map,"map");
  RCL_TYPE_STRUCT_NAMED(vector,"vector");
  RCL_TYPE_STRUCT_NAMED(undefinedExp,"undefined");

#define RCL_MAP_TYPE (rcl::typeName<rcl::map>())
#define RCL_VECTOR_TYPE (rcl::typeName<rcl::vector>())

  template <class T>
  string typeName(void) {
    return TypeStruct<T>::getType();
  }

  template <class T>
  string toString(const T& data) {
    // Made change to compile under gcc 3.2 in Cygwin; should work elsewhere
    std::ostringstream os;
    writeCompact(os, data);
    return os.str();
  }
  
  string toString(const exp& data);
  string toString(const vector& data);
  
  // same as rclToString, but makes a vector "[a, b, c]" instead of "a b c"
  string toStringCompact(const exp& data);
  
  // same as rclToString, but stringifies to the multi-line pretty-printed
  // form of the expression.
  string toStringConfig(const exp& data);

}; // namespace rcl


#endif // INCRCLTypes_h

/***************************************************************************
 * REVISION HISTORY:
 * $Log: RCLTypes.h,v $
 * Revision 1.17  2005/12/07 18:15:34  trey
 * committing dom's changes for gcc 3.4.0
 *
 * Revision 1.16  2004/08/02 02:55:59  trey
 * strings are now quoted more often
 *
 * Revision 1.15  2004/06/18 19:22:12  trey
 * changed to explicitly distinguish between an undefined expression and an externally defined type (i.e., a type that is not among the small number that rcl normally works with)
 *
 * Revision 1.14  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.13  2004/04/12 20:23:38  trey
 * fixed toString(vector) to act like toString(exp)
 *
 * Revision 1.12  2004/04/09 20:31:33  trey
 * major enhancement to rcl interface
 *
 * Revision 1.11  2004/04/08 16:42:56  trey
 * overhauled exp
 *
 * Revision 1.10  2003/08/19 16:03:15  brennan
 * Conflict resolution.
 *
 * Revision 1.9  2003/08/08 19:19:58  trey
 * made changes to enable compilation under g++ 3.2
 *
 * Revision 1.8  2003/08/08 15:29:27  brennan
 * Attempted to make microraptor compile:
 *   (1) on Cygwin and
 *   (2) under gcc 3.2
 * This attempt has not yet succeeded
 *
 * Revision 1.7  2003/03/10 22:55:22  trey
 * added dependency checking
 *
 * Revision 1.6  2003/03/06 23:51:37  trey
 * added rclToStringConfig() function
 *
 * Revision 1.5  2003/03/06 15:03:17  trey
 * made minor modifications so things compile under insure
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
 * Revision 1.5  2002/09/24 21:50:25  trey
 * rebuilt RCL from scratch to support nested vectors and hashes
 *
 *
 ***************************************************************************/
