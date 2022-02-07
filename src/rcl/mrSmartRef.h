/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.5 $  $Author: trey $  $Date: 2004/08/09 02:24:14 $
 *  
 * PROJECT: FIRE Architecture Project
 *
 * @file    SmartRef.h
 * @brief   from http://www.geocities.com/botstein/refcnt.html
 *          Author: Boris Botstein, botstein@yahoo.com
 ***************************************************************************/

#ifndef INCmrSmartRef_h
#define INCmrSmartRef_h

/***************************************************************************
 * INCLUDES
 ***************************************************************************/

#include <assert.h>
#if USE_MEM_TRACKER
#  include "memTracker.h"
#endif

/***************************************************************************
 * MACROS
 ***************************************************************************/

/***************************************************************************
 * CLASSES AND TYPEDEFS
 ***************************************************************************/

namespace microraptor {

struct GenericSmartRef
{
  virtual ~GenericSmartRef(void) {}
};

template < class X >
class SmartRef : public GenericSmartRef
{
 protected:
  class holder {
  public:
    X* rep;
    unsigned count, accessed;
    
#if USE_MEM_TRACKER
    holder(void) : rep(NULL), count(1), accessed(0) {}
#endif
    holder(X* ptr) : rep(ptr), count(1), accessed(0) {}
    ~holder() {
#if USE_MEM_TRACKER
      MTR_DELETE(rep);
#else
      delete rep;
#endif
    }
  };
  holder* value;
  
  void unbind() {
    if(--value->count == 0) {
#if USE_MEM_TRACKER
      MTR_DELETE(value);
#else
      delete value;
#endif
    }
  }
  
  X* get(void) const {
    assert(value->rep);
    value->accessed++;
    return value->rep;
  }
  
 public:
  SmartRef(void)
#if !USE_MEM_TRACKER
    : value(new holder(0))
#endif
  {
#if USE_MEM_TRACKER
    MTR_NEW(value, holder(0));
#endif
  }
  SmartRef(X* ptr)
#if !USE_MEM_TRACKER
    : value(new holder(ptr))
#endif
  {
#if USE_MEM_TRACKER
    MTR_NEW(value, holder(ptr));
#endif
  }
  SmartRef(const SmartRef< X >& rhs) : value(rhs.value) {
    value->count++;
  }
  ~SmartRef() { unbind(); }
  
  void bind(const SmartRef< X >& rhs) {
    if(rhs.value != value) {
      unbind();
      value = rhs.value;
      value->count++;
    }
  }
  void bind(X* ptr) {
    if(value->rep != ptr) {
      unbind();
#if USE_MEM_TRACKER
      MTR_NEW(value, holder(ptr));
#else
      value = new holder(ptr);
#endif
    }
  }
  
  SmartRef< X >& operator=(const SmartRef< X >& rhs) {
    bind(rhs);
    return *this;
  }
  SmartRef< X >& operator=(X* ptr) {
    bind(ptr);
    return *this;
  }
  
  X& reference() const { return *get(); }
  X* pointer() const { return get(); }
  
  X* operator->() { return get(); }
  X& operator*() { return *get(); }
  
  const X* operator->() const { return get(); }
  const X& operator*() const { return *get(); }

  bool operator!() const { return 0 == value->rep; }
  bool null() const { return 0 == value->rep; }
  
  unsigned counter() const { return value->count; }
  unsigned accessed() const { return value->accessed; }
};

}; // namespace microraptor

#endif /* INCmrSmartRef_h */

/***************************************************************************
 * REVISION HISTORY:
 * $Log: mrSmartRef.h,v $
 * Revision 1.5  2004/08/09 02:24:14  trey
 * placed SmartRef in microraptor namespace to head off confusion between microraptor/common/mrSmartRef.h and common/SmartRef.h
 *
 * Revision 1.4  2004/07/06 15:35:30  trey
 * added optional memory tracking and plugged some memory leaks
 *
 * Revision 1.3  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.2  2004/04/13 19:30:53  trey
 * added null() function
 *
 * Revision 1.1  2003/02/05 22:54:45  trey
 * made independent copy and renamed to avoid collisions
 *
 * Revision 1.1  2003/02/05 03:57:54  trey
 * initial atacama check-in
 *
 * Revision 1.5  2002/10/09 00:19:17  trey
 * added ! operator as syntactic sugar
 *
 * Revision 1.4  2001/11/08 23:11:34  trey
 * added include of assert.h in case including .cc file lacks it
 *
 * Revision 1.3  2001/11/06 22:01:18  trey
 * made SmartRef initialization less picky
 *
 * Revision 1.2  2001/10/25 22:52:17  trey
 * fixed missing final #endif
 *
 * Revision 1.1  2001/10/25 21:23:03  trey
 * initial check-in
 *
 *
 ***************************************************************************/

