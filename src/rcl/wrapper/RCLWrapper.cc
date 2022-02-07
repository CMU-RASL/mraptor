/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.2 $  $Author: trey $  $Date: 2005/07/18 14:56:01 $
 *  
 * PROJECT: Life in the Atacama
 *
 * @file    RCLWrapper.cc
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

#include "RCLWrapper.h"

using namespace std;

/***************************************************************************
 * INITIALIZATION
 ***************************************************************************/

struct RCLInitializer {
  RCLInitializer(void) {
    rcl::setErrorHandler(rcl::onErrorThrowException);
  }
};
RCLInitializer dummyToForceInitialization;

/***************************************************************************
 * EXP FUNCTIONS
 ***************************************************************************/

void Exp::writeToFile(const char* fileName) {
  data.writeToFile(fileName);
}

/***************************************************************************
 * READ_CONTEXT FUNCTIONS
 ***************************************************************************/

ReadContext::ReadContext(const Exp* expr) {
  contextStack.push_back(ContextEntry(expr->data));
}

Exp* ReadContext::getExp(void) {
  return new Exp(contextStack.back().expr);
}

const char* ReadContext::getType(void) {
  return contextStack.back().expr.getType().c_str();
}

double ReadContext::getDouble(void) {
  return contextStack.back().expr.getDouble();
}

long ReadContext::getLong(void) {
  return contextStack.back().expr.getLong();
}

bool ReadContext::getBool(void) {
  return contextStack.back().expr.getBool();
}

const char* ReadContext::getString(void) {
  return contextStack.back().expr.getString().c_str();
}

void ReadContext::vectorEnter(void) {
  ContextEntry& context = contextStack.back();

  context.currentEntry = -1;
  contextStack.push_back(ContextEntry(rcl::exp()));

#if 0
  cout << "entering vector: " << rcl::toStringConfig(context.expr) << endl;
  cout << "context is now: "
       <<  rcl::toStringConfig(contextStack.back().expr) << endl;
#endif
}

void ReadContext::vectorNextEntry(void) {
  contextStack.pop_back();
  ContextEntry& context = contextStack.back();
  contextStack.push_back
    (ContextEntry(context.expr[++context.currentEntry]));
}

bool ReadContext::vectorIsAtEnd(void) {
  // set vectorContext to be the item just before the back() of the stack
  typeof(contextStack.end()) iter = contextStack.end();
  iter--; iter--;
  ContextEntry& vectorContext = *iter;
  
  return (vectorContext.currentEntry+1
	  >= ((int) vectorContext.expr.getVector().size()));
}

void ReadContext::vectorExit(void) {
  contextStack.pop_back();
  contextStack.back().currentEntry = -1;
}

void ReadContext::mapEnter(void) {
  ContextEntry& context = contextStack.back();

  context.currentEntry = -1;
  contextStack.push_back(ContextEntry(rcl::exp()));

#if 0
  cout << "entering map: " << rcl::toStringConfig(context.expr) << endl;
  cout << "context is now: "
       <<  rcl::toStringConfig(contextStack.back().expr) << endl;
#endif
}

void ReadContext::mapNextEntry(void) {
  contextStack.pop_back();
  ContextEntry& context = contextStack.back();
  rcl::map& m = context.expr.getMap();
  
  if (context.currentEntry >= ((int) m.size())) {
    rcl::callErrorHandler("overran end of map in ReadContext::mapNextEntry()\n");
  } else {
    contextStack.push_back
      (ContextEntry(m.body[++context.currentEntry].second));
  }
}

const char* ReadContext::mapGetFieldName(void) {
  // set mapContext to be the item just before the back() of the stack
  typeof(contextStack.end()) iter = contextStack.end();
  iter--; iter--;
  ContextEntry& mapContext = *iter;
  
  return mapContext.expr.getMap().body[mapContext.currentEntry].first.c_str();
}

bool ReadContext::mapIsAtEnd(void) {
  // set mapContext to be the item just before the back() of the stack
  typeof(contextStack.end()) iter = contextStack.end();
  iter--; iter--;
  ContextEntry& mapContext = *iter;
  
  return (mapContext.currentEntry+1
	  >= ((int) mapContext.expr.getMap().size()));
}

void ReadContext::mapExit(void) {
  contextStack.pop_back();
  contextStack.back().currentEntry = -1;
}

/***************************************************************************
 * WRITE_CONTEXT FUNCTIONS
 ***************************************************************************/

WriteContext::WriteContext(void) {
  contextStack.push_back(ContextEntry(rcl::exp()));
}

void WriteContext::setDouble(double x) {
  contextStack.back().expr = x;
}

void WriteContext::setLong(long x) {
  contextStack.back().expr = x;
}

void WriteContext::setBool(bool x) {
  contextStack.back().expr = x;
}

void WriteContext::setString(const char* x) {
  contextStack.back().expr = x;
}

void WriteContext::setVector(void) {
  contextStack.back().expr = rcl::vector();
}

void WriteContext::setMap(void) {
  contextStack.back().expr = rcl::map();
}

void WriteContext::vectorEnter(void) {
  ContextEntry& context = contextStack.back();

  context.currentEntry = 0;
  contextStack.push_back(ContextEntry(rcl::exp()));

#if 0
  cout << "entering vector: " << rcl::toStringConfig(context.expr) << endl;
#endif
}

void WriteContext::vectorPushBackEntry(void) {
  // set vectorContext to be the item just before the back() of the stack
  typeof(contextStack.end()) iter = contextStack.end();
  iter--; iter--;
  ContextEntry& vectorContext = *iter;
  
  vectorContext.expr.push_back(contextStack.back().expr);

  contextStack.pop_back();
  contextStack.push_back(ContextEntry(rcl::exp()));
}

void WriteContext::vectorExit(void) {
  contextStack.pop_back();
  contextStack.back().currentEntry = -1;
}

void WriteContext::mapEnter(void) {
  ContextEntry& context = contextStack.back();

  context.currentEntry = 0;
  contextStack.push_back(ContextEntry(rcl::exp()));

#if 0
  cout << "entering map: " << rcl::toStringConfig(context.expr) << endl;
#endif
}

void WriteContext::mapPushBackEntry(const char* fieldName) {
  // set mapContext to be the item just before the back() of the stack
  typeof(contextStack.end()) iter = contextStack.end();
  iter--; iter--;
  ContextEntry& mapContext = *iter;
  
  mapContext.expr.getMap().push_back(fieldName, contextStack.back().expr);
}

void WriteContext::mapExit(void) {
  contextStack.pop_back();
  contextStack.back().currentEntry = -1;
}

Exp* WriteContext::getExp(void) {
  return new Exp(contextStack.back().expr);
}

/***************************************************************************
 * NON-CLASS FUNCTIONS
 ***************************************************************************/

const char* toString(Exp* expr) {
  return rcl::toString(expr->data).c_str();
}
const char* toStringConfig(Exp* expr) {
  return rcl::toStringConfig(expr->data).c_str();
}
const char* toStringCompact(Exp* expr) {
  return rcl::toStringCompact(expr->data).c_str();
}

Exp* readFromString(const char* s) {
  return new Exp(rcl::readFromString(s));
}

Exp* readFromFile(const char* fileName) {
  return new Exp(rcl::readFromFile(fileName));
}

Exp* loadConfigFile(const char* fileName) {
  return new Exp(rcl::loadConfigFile(fileName));
}

/***************************************************************************
 * REVISION HISTORY:
 * $Log: RCLWrapper.cc,v $
 * Revision 1.2  2005/07/18 14:56:01  trey
 * fixed a serious problem in the ReadContext implementation, causing the rcl python library to core dump when passed an empty map or vector
 *
 * Revision 1.1  2004/07/01 00:26:00  trey
 * initial check-in
 *
 *
 ***************************************************************************/
