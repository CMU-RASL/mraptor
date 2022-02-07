/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.5 $  $Author: trey $  $Date: 2004/06/28 15:46:18 $
 *  
 * PROJECT: FIRE Architecture Project
 *
 * @file    IPCMessage.h
 * @brief   No brief
 ***************************************************************************/

#ifndef INCIPCMessage_h
#define INCIPCMessage_h

/***************************************************************************
 * INCLUDES
 ***************************************************************************/

#include "ipc.h"

#include "mrIPCHelper.h"
#include "mrIPCFormatPrimTypes.h"
#include "mrFSignal.h"

/***************************************************************************
 * MACROS
 ***************************************************************************/

/***************************************************************************
 * CLASSES AND TYPEDEFS
 ***************************************************************************/

namespace microraptor {

// this class factors out common elements from IPCMessageInternal<T> that
// do not need to be duplicated for every type T.  it will never be
// instantiated.
class IPCMessageInternalGeneric {
public:
  std::string messageName, format;
  int dataSize;

  IPCMessageInternalGeneric(const std::string& _messageName, 
			    const std::string& userSpecifiedFormat,
			    const std::string& defaultFormat);
  virtual ~IPCMessageInternalGeneric(void);
  void connHandlerInternal(int oldNum, int newNum);  
  void publishOnlyWhenSubscribedTo(void);
  void publishAlways(void);

protected:
  int numSubscribers;

  virtual void doEmit(void *info) = 0;
  void subscribe(void);
  void unsubscribe(void);
  void publishInternal(const void *info, bool verifySubs);
  static void ipcHandler(MSG_INSTANCE msgRef,
			 BYTE_ARRAY callData,
			 void *clientData);
  static void handlerChangeHandler(const char *_msgName,
				   int numHandlers,
				   void *clientData);
};

template <class Arg0>
class IPCMessageInternal :
  public IPCMessageInternalGeneric,
  public FSignal1<Arg0>::Impl
{
public:
  IPCMessageInternal(const std::string& _messageName, 
		     const std::string& _format = "") :
    IPCMessageInternalGeneric(_messageName,
			      _format,
			      getIPCFormatT<Arg0>())
  { }
  
  void numConnectionsChangeHandler(int oldNum, int newNum) {
    connHandlerInternal(oldNum, newNum);
  }
  void publish(const Arg0& info) {
    publishInternal(&info, false);
  }
  void publishVerifySubs(const Arg0& info) {
    publishInternal(&info, true);
  }
protected:
  void doEmit(void *infoVoid) {
    Arg0 *info = (Arg0 *) infoVoid;
    this->emitByRef(*info);
    delete info;
  }
};

class IPCMessage0 : public FSignal0Interface {
protected:
  SmartRef< IPCMessageInternal<Unit> > internal;
public:
  IPCMessage0(const std::string& _messageName)
    : internal(new IPCMessageInternal<Unit>(_messageName)) {}
  void publish(void) {
    internal->publish(NIL);
  }
  void publishVerifySubs(void) {
    internal->publishVerifySubs(NIL);
  }
  FSlot0 getPublishSlot(void) {
    return fslot(internal.pointer(), &IPCMessageInternal<Unit>::publish)
      .bindArg(NIL);
  }

  /* implement FSignalInterface abstract functions */
  FConnection connect(FSlot0 slot) {
    return internal->connect(unitArg(slot));
  }
  void emit(void) {
    internal->emitByRef(NIL);
  }
  int getNumConnections(void) {
    return internal->getNumConnections();
  }
  std::string getMessageName(void) {
    return internal->messageName;
  }
  void publishOnlyWhenSubscribedTo(void) {
    internal->publishOnlyWhenSubscribedTo();
  }
  void release(void) {
    internal->release();
  }
};

template <class Arg0>
class IPCMessage1 : public FSignal1Interface<Arg0> {
protected:
  SmartRef< IPCMessageInternal<Arg0> > internal;
  bool released;
public:
  IPCMessage1(const std::string& _messageName, 
	      const std::string& _format = "")
    : internal(new IPCMessageInternal<Arg0>(_messageName,_format)) {}

  void publish(const Arg0& info) {
    internal->publish(info);
  }
  void publishVerifySubs(const Arg0& info) {
    internal->publishVerifySubs(info);
  }
  FSlot1<const Arg0&> getPublishSlot(void) {
    return fslot(internal.pointer(), &IPCMessageInternal<Arg0>::publish);
  }

  /* implement FSignalInterface abstract functions */
  FConnection xconnect(FSlot1<Arg0&> sl) {
    return internal->xconnect(sl);
  }
  void emitByRef(Arg0& output) {
    internal->emitByRef(output);
  }
  int getNumConnections(void) {
    return internal->getNumConnections();
  }
  std::string getMessageName(void) {
    return internal->messageName;
  }
  void publishOnlyWhenSubscribedTo(void) {
    internal->publishOnlyWhenSubscribedTo();
  }
  void release(void) {
    internal->release();
  }
};

} // namespace microraptor

/***************************************************************************
 * GLOBAL VARIABLES AND STATIC CLASS MEMBERS
 ***************************************************************************/

/***************************************************************************
 * FUNCTIONS
 ***************************************************************************/

#endif /* INCIPCMessage_h */

/***************************************************************************
 * REVISION HISTORY:
 * $Log: mrIPCMessage.h,v $
 * Revision 1.5  2004/06/28 15:46:18  trey
 * fixed to properly find ipc.h and libipc.a in new atacama external area
 *
 * Revision 1.4  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.3  2003/10/05 02:54:29  trey
 * re-fixed some gcc 3.2 issues
 *
 * Revision 1.2  2003/10/04 21:56:48  trey
 * tweaked things to eliminate the need for the common/overlap directory and libmrcommonoverlap.a
 *
 * Revision 1.1  2003/02/14 05:58:05  trey
 * initial atacama check-in
 *
 * Revision 1.15  2002/05/19 21:13:31  trey
 * factored out duplicate code in templates
 *
 * Revision 1.14  2001/11/08 23:11:06  trey
 * ifdefd out debugging statements
 *
 * Revision 1.13  2001/11/08 16:26:10  trey
 * made FSignalInterface inherit from FSlotObject
 *
 * Revision 1.12  2001/11/06 17:51:37  trey
 * added FSlotObject reference counting, avoiding some errors
 *
 * Revision 1.11  2001/11/06 01:40:46  trey
 * added publishVerifySubs() function
 *
 * Revision 1.10  2001/11/06 01:22:28  trey
 * added publishVerifySubs() function
 *
 * Revision 1.9  2001/11/05 19:08:56  trey
 * added include guard (how did it ever get left out???)
 *
 * Revision 1.8  2001/11/02 21:06:07  trey
 * fixed problem with returning FSignal datatype from a function by adding a wrapper layer around everything
 *
 * Revision 1.7  2001/10/31 23:40:26  trey
 * changed name of getIPCFormat<T>() to getIPCFormatT<T>(), avoiding name conflict with T::getIPCFormat(), as defined by xdrgen
 *
 * Revision 1.6  2001/10/31 22:48:50  trey
 * fixed problem of dangling pointers in IPC after IPCMessage<T>::destroy() is called
 *
 * Revision 1.5  2001/10/31 21:43:02  trey
 * made FSignal::emit() not take a ref argument, based on hersh's observation that it's often a pain
 *
 * Revision 1.4  2001/10/31 02:33:52  trey
 * overhaul of signals and connections
 *
 * Revision 1.3  2001/10/30 05:36:01  trey
 * complete overhaul, better syntax
 *
 * Revision 1.2  2001/10/25 22:51:44  trey
 * changed char* arguments to strings for convenience
 *
 * Revision 1.1  2001/10/25 21:07:18  trey
 * initial check-in
 *
 *
 ***************************************************************************/
