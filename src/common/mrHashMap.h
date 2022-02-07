/***** tell emacs we use -*- c++ -*- style comments *****
 * $Revision: 1.5 $ $Author: dom $ $Date: 2004/04/28 18:58:53 $
 *
 * COPYRIGHT 2004, Carnegie Mellon University 
 *
 * PROJECT: Exploration Robotics (Life in the Atacama)
 *
 * MODULE:
 *
 * FILE: microraptor/common/mrHashMap.h
 *
 * DESCRIPTION:
 *
 ********************************************************/

#include <string>

// Relaxing this; it looks like 3.0.3 puts its headers in the same place
// Hopefuly nothing between 3.0.3 and 3.2 do it differently...
#if (__GNUC__ == 3 && __GNUC_MINOR__ >= 0) || __GNUC__ >= 4
#define _CPP_BACKWARD_BACKWARD_WARNING_H
#define _BACKWARD_BACKWARD_WARNING_H
#include <ext/hash_map>
#else
#include <hash_map>
#define __gnu_cxx std
#endif

#ifndef __HAVE_HASH_STRING
#define __HAVE_HASH_STRING

// in 3.2, headers in include/ext use the __gnu_cxx namespace
#if (__GNUC__ == 3 && __GNUC_MINOR__ >= 2) || __GNUC__ >= 4
using __gnu_cxx::hash;
using __gnu_cxx::hash_map;

// this is needed so that we can use hash_map<string, x, hash<string> > later.
namespace __gnu_cxx {
  template <>
  struct hash<std::string> {
    size_t operator()(const std::string&str) const {
      hash<char const *> h;
      return (h(str.c_str()));
    }
  };
}
#else
using namespace std;
namespace std {
  template <>
  struct hash<std::string> {
    size_t operator()(const std::string&str) const {
      hash<char const *> h;
      return (h(str.c_str()));
    }
  };
}
#endif
#endif
/***************************************************************
 * Revision History:
 * $Log: mrHashMap.h,v $
 * Revision 1.5  2004/04/28 18:58:53  dom
 * Appended log directive
 * 
 ***************************************************************/
