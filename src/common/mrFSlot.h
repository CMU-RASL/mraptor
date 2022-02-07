/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.6 $  $Author: dom $  $Date: 2004/04/28 17:15:21 $
 *  
 * PROJECT: FIRE Architecture Project
 *
 * @file    FSlot.h
 * @brief   No brief
 ***************************************************************************/

#ifndef INCFSlot_h
#define INCFSlot_h

/***************************************************************************
 * INCLUDES
 ***************************************************************************/

#include <memory>
#include <iostream>
#include "mrSmartRef.h"

namespace microraptor {

struct Unit {
#define Unit_IPC_FORMAT "{}"
  static const char *getIPCFormat(void) {
    return Unit_IPC_FORMAT;
  }
};
extern Unit NIL;

/***************************************************************************
 * REFERENCE COUNTING FOR INSTANCE OBJECTS
 ***************************************************************************/

// all objects whose member functions are used to generate FSlots must
//   inherit from FSlotObject (or FSlotObjectNoTrack, see below).
//   deleting an FSlotObject that is still pointed to by an FSlot generates
//   an error.
struct FSlotObject {
protected:
  int refCount;
public:
  FSlotObject(void) : refCount(0) {}
  ~FSlotObject(void) {
#if 0
    std::cout << "~FSlotObject: refCount=" << refCount << endl;
#endif
    if (0 != refCount) {
      std::cerr <<
	"ERROR: trying to delete an object which is pointed to by an FSlot\n"
	"   (this could cause bizarre errors if the FSlot is activated):\n"
	"   aborting\n";
      abort();
    }
  }
  int getRefCount(void) { return refCount; }
  void addRefCount(int change) {
#if 0
    std::cout << "FSlotObject::addRefCount refCount=" << refCount
	 << ", change=" << change << endl;
#endif
    refCount += change;
  }
};

// only use this one if you absolutely must.  it has the advantage
//   that objects which inherit from it can still be transmitted via IPC.
struct FSlotObjectNoTrack {
  void addRefCount(void) {}
};

/***************************************************************************
 * BASIC SLOT TYPES
 ***************************************************************************/

struct FSlot0 {
  struct Impl {
    virtual ~Impl(void) {}
    virtual void callHandler(void) = 0;
  };
  SmartRef<Impl> impl;
  FSlot0(Impl* _impl) : impl(_impl) {}
  void callHandler(void) {
    impl->callHandler();
  }
};

template <class Arg0>
struct FSlot1Base {
  struct Impl {
    virtual ~Impl(void) {}
    virtual void callHandler(Arg0 arg0) = 0;
  };
  SmartRef<Impl> impl;
  FSlot1Base(void) : impl(0) {}
  FSlot1Base(Impl* _impl) : impl(_impl) {}
  void callHandler(Arg0 arg0) {
    impl->callHandler(arg0);
  }
};

template <class Arg0>
struct FSlot1 : public FSlot1Base<Arg0> {
  // avoid implicit typename warning in 3.2
  typedef typename FSlot1Base<Arg0>::Impl Impl;

  FSlot1(void) : FSlot1Base<Arg0>() {}
  FSlot1(Impl* impl) : FSlot1Base<Arg0>(impl) {}
  FSlot0 bindArg(Arg0 arg0);
};

// need to explicitly specialize when dealing with FSlot<T&> as opposed
// to FSlot<T> so that bindArg keeps a copy of the data in the bound
// argument, not a ref to it that can go away.
template <class Arg0>
struct FSlot1<Arg0&> : public FSlot1Base<Arg0&> {
  // avoid implicit typename warning in 3.2
  typedef typename FSlot1Base<Arg0&>::Impl Impl;

  FSlot1(void) : FSlot1Base<Arg0&>() {}
  FSlot1(Impl* impl) : FSlot1Base<Arg0&>(impl) {}
  FSlot0 bindArg(Arg0 arg0);
};

template <class Arg0, class Arg1>
struct FSlot2Base {
  struct Impl {
    virtual ~Impl(void) {}
    virtual void callHandler(Arg0 arg0, Arg1 arg1) = 0;
  };
  SmartRef<Impl> impl;
  FSlot2Base(Impl* _impl) : impl(_impl) {}
  void callHandler(Arg0 arg0, Arg1 arg1) {
    impl->callHandler(arg0, arg1);
  }
};

template <class Arg0, class Arg1>
struct FSlot2 : public FSlot2Base<Arg0,Arg1> {
  // avoid implicit typename warning in 3.2
  typedef typename FSlot2Base<Arg0,Arg1>::Impl Impl;

  FSlot2(Impl* impl) : FSlot2Base<Arg0,Arg1>(impl) {}
  FSlot1<Arg1> bindFirstArg(Arg0 arg0);
};

// need to explicitly specialize when dealing with FSlot<T&> as opposed
// to FSlot<T> so that bindArg keeps a copy of the data in the bound
// argument, not a ref to it that can go away.
template <class Arg0, class Arg1>
struct FSlot2<Arg0&,Arg1> : public FSlot2Base<Arg0&,Arg1> {
  // avoid implicit typename warning in 3.2
  typedef typename FSlot2Base<Arg0&,Arg1>::Impl Impl;

  FSlot2(Impl* impl) : FSlot2Base<Arg0&,Arg1>(impl) {}
  FSlot1<Arg1> bindFirstArg(Arg0 arg0);
};

/***************************************************************************
 * SLOT IMPLEMENTATIONS FOR NON-MEMBER OR STATIC MEMBER FUNCTIONS
 ***************************************************************************/

struct FSlot0_Hnd : public FSlot0::Impl {
  typedef void (*HandlerFunc)(void);
  HandlerFunc h;
  FSlot0_Hnd(HandlerFunc _h) : h(_h) {}
  void callHandler(void) {
    (*h)();
  }
};

template <class Arg0>
struct FSlot1_Hnd : public FSlot1<Arg0>::Impl {
  typedef void (*HandlerFunc)(Arg0);
  HandlerFunc h;
  FSlot1_Hnd(HandlerFunc _h) : h(_h) {}
  void callHandler(Arg0 arg0) {
    (*h)(arg0);
  }
};

template <class Arg0, class Arg1>
struct FSlot2_Hnd : public FSlot2<Arg0,Arg1>::Impl {
  typedef void (*HandlerFunc)(Arg0,Arg1);
  HandlerFunc h;
  FSlot2_Hnd(HandlerFunc _h) : h(_h) {}
  void callHandler(Arg0 arg0, Arg1 arg1) {
    (*h)(arg0,arg1);
  }
};

/***************************************************************************
 * SLOT IMPLEMENTATIONS FOR NON-STATIC MEMBER FUNCTIONS
 ***************************************************************************/

template <class HandlerOwner>
struct FSlot0_I : public FSlot0::Impl {
  typedef void (HandlerOwner::*HandlerFunc)(void);
  HandlerOwner* inst;
  HandlerFunc h;
  FSlot0_I(HandlerOwner* _inst, HandlerFunc _h)
    : inst(_inst), h(_h) {
    inst->addRefCount(1);
  }
  ~FSlot0_I(void) {
    inst->addRefCount(-1);
  }
  void callHandler(void) {
    (inst->*h)();
  }
};

template <class HandlerOwner, class Arg0>
struct FSlot1_I : public FSlot1<Arg0>::Impl {
  typedef void (HandlerOwner::*HandlerFunc)(Arg0);
  HandlerOwner* inst;
  HandlerFunc h;
  FSlot1_I(HandlerOwner* _inst, HandlerFunc _h)
    : inst(_inst), h(_h) {
    inst->addRefCount(1);
  }
  ~FSlot1_I(void) {
    inst->addRefCount(-1);
  }
  void callHandler(Arg0 arg0) {
    (inst->*h)(arg0);
  }
};

template <class HandlerOwner, class Arg0, class Arg1>
struct FSlot2_I : public FSlot2<Arg0,Arg1>::Impl {
  typedef void (HandlerOwner::*HandlerFunc)(Arg0,Arg1);
  HandlerOwner* inst;
  HandlerFunc h;
  FSlot2_I(HandlerOwner* _inst, HandlerFunc _h)
    : inst(_inst), h(_h) {
    inst->addRefCount(1);
  }
  ~FSlot2_I(void) {
    inst->addRefCount(-1);
  }
  
  void callHandler(Arg0 arg0, Arg1 arg1) {
    (inst->*h)(arg0, arg1);
  }
};

/***************************************************************************
 * SLOT IMPLEMENTATIONS FOR BOUND ARGUMENTS
 ***************************************************************************/

template <class Arg0>
struct FSlot0_Arg : public FSlot0::Impl {
  FSlot1<Arg0> h;
  Arg0 arg0;
  FSlot0_Arg(FSlot1<Arg0> _h, Arg0 _arg0) : h(_h), arg0(_arg0) {}
  void callHandler(void) {
    h.callHandler(arg0);
  }
};

template <class Arg0>
struct FSlot0_RefArg : public FSlot0::Impl {
  FSlot1<Arg0&> h;
  Arg0 arg0;
  FSlot0_RefArg(FSlot1<Arg0&> _h, Arg0 _arg0) : h(_h), arg0(_arg0) {}
  void callHandler(void) {
    h.callHandler(arg0);
  }
};

template <class Arg0, class Arg1>
struct FSlot1_Arg : public FSlot1<Arg1>::Impl {
  FSlot2<Arg0,Arg1> h;
  Arg0 arg0;
  FSlot1_Arg(FSlot2<Arg0,Arg1> _h, Arg0 _arg0) : h(_h), arg0(_arg0) {}
  void callHandler(Arg1 arg1) {
    h.callHandler(arg0,arg1);
  }
};

template <class Arg0, class Arg1>
struct FSlot1_RefArg : public FSlot1<Arg1>::Impl {
  FSlot2<Arg0&,Arg1> h;
  Arg0 arg0;
  FSlot1_RefArg(FSlot2<Arg0&,Arg1> _h, Arg0 _arg0) : h(_h), arg0(_arg0) {}
  void callHandler(Arg1 arg1) {
    h.callHandler(arg0,arg1);
  }
};

/***************************************************************************
 * SLOT IMPLEMENTATIONS FOR TYPE MANIPULATION
 ***************************************************************************/

template <class Arg0>
struct FSlot1_Ref : public FSlot1<Arg0&>::Impl {
  FSlot1<Arg0> h;
  FSlot1_Ref(FSlot1<Arg0>& _h) : h(_h) {}
  void callHandler(Arg0& arg0) {
    h.callHandler(arg0);
  }
};

template <class Arg0>
struct FSlot1_Const : public FSlot1<Arg0&>::Impl {
  FSlot1<const Arg0&> h;
  FSlot1_Const(FSlot1<const Arg0&>& _h) : h(_h) {}
  void callHandler(Arg0& arg0) {
    h.callHandler(arg0);
  }
};

struct FSlot1_Unit : public FSlot1<Unit&>::Impl {
  FSlot0 h;
  FSlot1_Unit(FSlot0& _h) : h(_h) {}
  void callHandler(Unit& arg0) {
    h.callHandler();
  }
};

/***************************************************************************
 * ARGUMENT BINDING FUNCTIONS
 ***************************************************************************/

template <class Arg0>
FSlot0
FSlot1<Arg0>::bindArg(Arg0 arg0) {
  return FSlot0(new FSlot0_Arg<Arg0>(*this, arg0));
}

template <class Arg0>
FSlot0
FSlot1<Arg0&>::bindArg(Arg0 arg0) {
  return FSlot0(new FSlot0_RefArg<Arg0>(*this, arg0));
}

template <class Arg0, class Arg1>
FSlot1<Arg1>
FSlot2<Arg0,Arg1>::bindFirstArg(Arg0 arg0) {
  return FSlot1<Arg1>(new FSlot1_Arg<Arg0,Arg1>(*this, arg0));
}

template <class Arg0, class Arg1>
FSlot1<Arg1>
FSlot2<Arg0&,Arg1>::bindFirstArg(Arg0 arg0) {
  return FSlot1<Arg1>(new FSlot1_RefArg<Arg0,Arg1>(*this, arg0));
}

/***************************************************************************
 * TYPE MANIPULATION FUNCTIONS
 ***************************************************************************/

template <class Arg0>
FSlot1<Arg0&>
refArg(FSlot1<Arg0>& sl) {
  return FSlot1<Arg0&>(new FSlot1_Ref<Arg0>(sl));
}

template <class Arg0>
FSlot1<Arg0&>
unConstArg(FSlot1<const Arg0&>& sl) {
  return FSlot1<Arg0&>(new FSlot1_Const<Arg0>(sl));
}

// implemented in FSlot.cc because it's not a template
FSlot1<Unit&> unitArg(FSlot0& sl);

/***************************************************************************
 * PUBLIC CONSTRUCTORS
 ***************************************************************************/

// implemented in FSlot.cc because it's not a template
FSlot0 fslot(void (*h)(void));

template <class Arg0>
FSlot1<Arg0>
fslot(void (*h)(Arg0)) {
  return FSlot1<Arg0>(new FSlot1_Hnd<Arg0>(h));
}

template <class Arg0, class Arg1>
FSlot2<Arg0,Arg1>
fslot(void (*h)(Arg0, Arg1)) {
  return FSlot2<Arg0,Arg1>(new FSlot2_Hnd<Arg0,Arg1>(h));
}

template <class HandlerOwner>
FSlot0
fslot(HandlerOwner* inst, void (HandlerOwner::*h)(void)) {
  return FSlot0(new FSlot0_I<HandlerOwner>(inst,h));
}
  
template <class HandlerOwner, class Arg0>
FSlot1<Arg0>
fslot(HandlerOwner* inst, void (HandlerOwner::*h)(Arg0)) {
  return FSlot1<Arg0>(new FSlot1_I<HandlerOwner,Arg0>(inst,h));
}
  
template <class HandlerOwner, class Arg0, class Arg1>
FSlot2<Arg0,Arg1>
fslot(HandlerOwner* inst, void (HandlerOwner::*h)(Arg0, Arg1)) {
  return FSlot2<Arg0,Arg1>(new FSlot2_I<HandlerOwner,Arg0,Arg1>(inst,h));
}

} // namespace microraptor
  
#endif // INCFSlot_h

/***************************************************************************
 * REVISION HISTORY:
 * $Log: mrFSlot.h,v $
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
 * Revision 1.2  2003/06/06 01:45:37  trey
 * added void constructor because not having it was a pain
 *
 * Revision 1.1  2003/02/14 05:58:05  trey
 * initial atacama check-in
 *
 * Revision 1.9  2002/07/08 17:31:48  soa
 * Added namespace directives and includes for gcc 3.0.1 compatibility
 *
 * Revision 1.8  2002/05/18 03:46:36  trey
 * added ability to query ref count; handy for debugging
 *
 * Revision 1.7  2001/11/17 02:56:38  trey
 * compensate for commenting of "extern Unit NIL;" in commonTypes.xdr
 *
 * Revision 1.6  2001/11/08 23:09:48  trey
 * ifdefd out debugging statements
 *
 * Revision 1.5  2001/11/06 17:51:37  trey
 * added FSlotObject reference counting, avoiding some errors
 *
 * Revision 1.4  2001/10/31 02:33:52  trey
 * overhaul of signals and connections
 *
 * Revision 1.3  2001/10/30 05:36:01  trey
 * complete overhaul, better syntax
 *
 *
 ***************************************************************************/
