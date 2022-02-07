/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.1 $  $Author: trey $  $Date: 2004/07/01 00:26:00 $
 *  
 * PROJECT: Life in the Atacama
 *
 * @file    RCLWrapper.h
 * @brief   No brief
 ***************************************************************************/

#ifndef INCRCLWrapper_h
#define INCRCLWrapper_h

#include "RCL.h"

struct ReadContext;

struct Exp {
  rcl::exp data;

  Exp(const rcl::exp& _data) :
    data(_data)
  {}
  void writeToFile(const char* fileName);
};

#ifndef SWIG
struct ContextEntry {
  rcl::exp expr;
  int currentEntry;

  ContextEntry(const rcl::exp& _expr,
	       int _currentEntry = -1) :
    expr(_expr),
    currentEntry(_currentEntry)
  {}
};
#endif

struct ReadContext {
#ifndef SWIG
  std::vector<ContextEntry> contextStack;
#endif

  ReadContext(const Exp* expr);
  Exp* getExp();

  const char* getType();
  
  double getDouble();
  long getLong();
  bool getBool();
  const char* getString();
  
  void vectorEnter();
  void vectorNextEntry();
  bool vectorIsAtEnd();
  void vectorExit();

  void mapEnter();
  void mapNextEntry();
  const char* mapGetFieldName();
  bool mapIsAtEnd();
  void mapExit();
};

struct WriteContext {
#ifndef SWIG
  std::vector<ContextEntry> contextStack;
#endif
  
  WriteContext();
  Exp* getExp();

  void setDouble(double x);
  void setLong(long x);
  void setBool(bool x);
  void setString(const char* x);
  void setVector();
  void setMap();

  void vectorEnter();
  void vectorPushBackEntry();
  void vectorExit();

  void mapEnter();
  void mapPushBackEntry(const char* fieldName);
  void mapExit();
};

const char* toString(Exp* expr);
const char* toStringConfig(Exp* expr);
const char* toStringCompact(Exp* expr);

Exp* readFromString(const char* s);
Exp* readFromFile(const char* fileName);
Exp* loadConfigFile(const char* fileName);

#endif // INCRCLWrapper_h

/***************************************************************************
 * REVISION HISTORY:
 * $Log: RCLWrapper.h,v $
 * Revision 1.1  2004/07/01 00:26:00  trey
 * initial check-in
 *
 *
 ***************************************************************************/
