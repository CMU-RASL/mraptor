/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.6 $  $Author: dom $  $Date: 2004/04/28 17:15:21 $
 *  
 * PROJECT: FIRE Architecture Project
 *
 * @file    FSignal.h
 * @brief   No brief
 ***************************************************************************/

#ifndef INCFSignal_h
#define INCFSignal_h

/***************************************************************************
 * INCLUDES
 ***************************************************************************/

#include <list>
#include "mrFSlot.h"

/***************************************************************************
 * CLASSES AND TYPEDEFS
 ***************************************************************************/

/**********************************************************************
Here is some documentation about the memory model and usage for
FConnection objects:

- you can disconnect and reconnect a connection using the
  FConnection disconnect() and connect() functions

- you should (hopefully) not need to create pointers to connections with
  'new'.  instead, just use assignment, like this:

  FConnection foo = mySignal.connect(mySlot);
  FConnection goo = foo;

  calling disconnect() on foo or on goo is equivalent.  in either case,
  it disconnects the original connection.  this works because the
  FConnection object is actually a wrapper around an internal SmartRef.

- when an FConnection object goes out of scope (or an FConnection * is
  deleted), nothing happens to the connection.  you just lose the
  ability to disconnect it later.  you don't need to call release() or
  anything like that.
**********************************************************************/

namespace microraptor {

struct FConnection {
  struct Impl {
    virtual ~Impl(void) {}
    virtual void disconnect(void) = 0;
  };
  SmartRef<Impl> impl;
  FConnection(Impl* _impl) : impl(_impl) {}
  void disconnect(void) {
    impl->disconnect();
  }
};

struct FSignalInterface : public FSlotObject {
  virtual ~FSignalInterface(void) {}
  virtual int getNumConnections(void) = 0;
  virtual void numConnectionsChangeHandler(int oldNum, int newNum) {}
};

struct FSignal0Interface : public FSignalInterface {
  virtual void emit(void) = 0;
  virtual FConnection connect(FSlot0 slot) = 0;
};

struct FSignal0 : public FSignal0Interface {
  typedef std::list< FSlot0 > SlotList;
  typedef SlotList::iterator SlotIterator;

  struct Impl : public FSignal0Interface {
    SlotList slotList;
    bool released;

    Impl(void) : released(false) {}
    ~Impl(void) {
#if 0
      if (!released && getNumConnections() > 0) {
	cerr << 
	  "ERROR: attempting to delete FSignal object that still\n"
	  "   has slot connections without calling release(): aborting\n";
	abort();
      }
#endif
    }

    void release(void) { released = true; }
    int getNumConnections(void) { return slotList.size(); }
    void emit(void) {
      for (SlotIterator i=slotList.begin(); i != slotList.end(); i++) {
	i->callHandler();
      }
    }
    void disconnect(SlotIterator elt) {
      SlotIterator eltPre = elt;
      SlotIterator eltPost = ++elt;
      slotList.erase(eltPre,eltPost);
      numConnectionsChangeHandler(getNumConnections()+1, getNumConnections());
    }
    FConnection connect(FSlot0 slot);
  };

  SmartRef<Impl> impl;
  FSignal0(void) : impl(new Impl) {}
  FSignal0(Impl* _impl) : impl(_impl) {}
  
  int getNumConnections(void) {
    return impl->getNumConnections();
  }
  void emit(void) {
    impl->emit();
  }
  void disconnect(SlotIterator elt) {
    impl->disconnect(elt);
  }
  void release(void) { impl->release(); }
  FConnection connect(FSlot0 slot) {
    return impl->connect(slot);
  }
};

template <class Arg0>
struct FSignal1Interface : public FSignalInterface {
  FConnection connect(FSlot1<Arg0&> slot) {
    return xconnect(slot);
  }
  FConnection connect(FSlot1<Arg0> slot) {
    return xconnect(refArg(slot));
  }
  FConnection connect(FSlot1<const Arg0&> slot) {
    return xconnect(unConstArg(slot));
  }

  // not passing by reference, so you can say "emit(3)" instead of
  // "int n=3; emit(n)"
  void emit(Arg0 arg0) { emitByRef(arg0); }
  // or explicitly pass by reference for better efficiency on large data types
  virtual void emitByRef(Arg0& arg0) = 0;
  virtual FConnection xconnect(FSlot1<Arg0&> slot) = 0;
};

template <class Arg0>
struct FSignal1 : public FSignal1Interface<Arg0> {
  typedef std::list< FSlot1<Arg0&> > SlotList;
  typedef typename SlotList::iterator SlotIterator;

  struct Impl : public FSignal1Interface<Arg0> {
    SlotList slotList;
    bool released;
    
    Impl(void) : released(false) {}
    ~Impl(void) {
#if 0
      if (!released && getNumConnections() > 0) {
	cerr << 
	  "ERROR: attempting to delete FSignal object that still\n"
	  "   has slot connections without calling release(): aborting\n";
	abort();
      }
#endif
    }

    void release(void) { released = true; }
    void emitByRef(Arg0& arg0) {
      for (SlotIterator i=slotList.begin(); i != slotList.end(); i++) {
	i->callHandler(arg0);
      }
    }
    void disconnect(SlotIterator elt) {
      SlotIterator eltPre = elt;
      SlotIterator eltPost = ++elt;
      slotList.erase(eltPre,eltPost);
      this->numConnectionsChangeHandler(getNumConnections()+1, getNumConnections());
    }
    int getNumConnections(void) { return slotList.size(); }
    FConnection xconnect(FSlot1<Arg0&> slot);
  };

  SmartRef<Impl> impl;
  FSignal1(void) : impl(new Impl) {}
  FSignal1(Impl* _impl) : impl(_impl) {}

  void emitByRef(Arg0& arg0) {
    impl->emitByRef(arg0);
  }
  void disconnect(SlotIterator elt) {
    impl->disconnect(elt);
  }
  int getNumConnections(void) {
    return impl->getNumConnections();
  }
  void release(void) { impl->release(); }
  FConnection xconnect(FSlot1<Arg0&> slot) {
    return impl->xconnect(slot);
  }
};

template <class SigType>
struct FConnectionList : public FConnection::Impl {
  typedef typename SigType::SlotList SlotList;
  typename SigType::Impl* sig;
  typename SlotList::iterator elt;
  bool connected;
  FConnectionList(typename SigType::Impl* _sig,
		  typename SlotList::iterator _elt)
    : sig(_sig), elt(_elt), connected(true) {}
  void disconnect(void) {
    assert(connected);
    sig->disconnect(elt);
    connected = false;
  }
};

template <class Arg0>
FConnection
FSignal1<Arg0>::Impl::xconnect(FSlot1<Arg0&> slot) {
  slotList.push_back(slot);
  this->numConnectionsChangeHandler(getNumConnections()-1, getNumConnections());
  return FConnection(new FConnectionList< FSignal1<Arg0> >
		     (this, --slotList.end()));
}

} // namespace microraptor

/***************************************************************************
 * GLOBALS AND FUNCTION PROTOTYPES
 ***************************************************************************/

#endif // INCFSignal_h

/***************************************************************************
 * REVISION HISTORY:
 * $Log: mrFSignal.h,v $
 * Revision 1.6  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.5  2003/10/05 02:54:29  trey
 * re-fixed some gcc 3.2 issues
 *
 * Revision 1.4  2003/10/04 21:56:48  trey
 * tweaked things to eliminate the need for the common/overlap directory and libmrcommonoverlap.a
 *
 * Revision 1.3  2003/08/08 19:19:58  trey
 * made changes to enable compilation under g++ 3.2
 *
 * Revision 1.2  2003/02/19 16:26:31  trey
 * got rid of this confusing release() business, caveat scriptor
 *
 * Revision 1.1  2003/02/14 05:58:05  trey
 * initial atacama check-in
 *
 * Revision 1.10  2002/05/14 00:31:59  trey
 * added a bit of FConnection documentation
 *
 * Revision 1.9  2002/04/25 02:55:17  trey
 * implemented a release() function for FSignal0 and FSignal1<T> datatypes
 *
 * Revision 1.8  2001/11/08 16:25:59  trey
 * made FSignalInterface inherit from FSlotObject
 *
 * Revision 1.7  2001/11/06 17:51:37  trey
 * added FSlotObject reference counting, avoiding some errors
 *
 * Revision 1.6  2001/11/02 21:06:07  trey
 * fixed problem with returning FSignal datatype from a function by adding a wrapper layer around everything
 *
 * Revision 1.5  2001/10/31 21:43:01  trey
 * made FSignal::emit() not take a ref argument, based on hersh's observation that it's often a pain
 *
 * Revision 1.4  2001/10/31 02:36:02  trey
 * did a little code cleanup
 *
 * Revision 1.3  2001/10/31 02:33:52  trey
 * overhaul of signals and connections
 *
 * Revision 1.2  2001/10/30 05:36:01  trey
 * complete overhaul, better syntax
 *
 * Revision 1.1  2001/10/25 21:07:17  trey
 * initial check-in
 *
 * Revision 1.1  2001/08/27 17:49:16  trey
 * initial check-in
 *
 *
 ***************************************************************************/
