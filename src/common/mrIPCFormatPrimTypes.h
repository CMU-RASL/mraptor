/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.3 $  $Author: dom $  $Date: 2005/03/28 16:49:16 $
 *
 * PROJECT:      Distributed Robotic Agents
 * DESCRIPTION:  defines the IPC format corresponding to all the primitive
 *               types, using a call to getIPCFormatT<type>().  if there
 *               is no explicit specialization of getIPCFormatT() for a given
 *               type T, T::getIPCFormatT() will be called.
 ***************************************************************************/

#ifndef INCIPCFormatPrimTypes_h
#define INCIPCFormatPrimTypes_h

#include <stdio.h>
#include <string.h>

template <class T>
struct IPCFormatStruct {
  static const char* get(void) { return T::getIPCFormat(); }
};

template <class T>
const char*
getIPCFormatT(void) {
  return IPCFormatStruct<T>::get();
}

// specialization for pointer types
template <class T>
struct IPCFormatStruct<T*> {
  static const char* get(void) {
    static char* staticBuf = 0;
    if (0 == staticBuf) {
      const char* fmtT = getIPCFormatT<T>();
      staticBuf = new char[strlen(fmtT)+2];
      snprintf(staticBuf,strlen(fmtT)+2,"*%s",fmtT);
    }
    return staticBuf;
  }
};

#define IPC_FMT_TEMPLATE_TYPE_HEADER(T,format) \
template <> \
struct IPCFormatStruct<T> { \
  static const char* get(void); \
};

#define IPC_FMT_TEMPLATE_TYPE(T,format) \
const char* \
IPCFormatStruct<T>::get(void)  { return format; }

IPC_FMT_TEMPLATE_TYPE_HEADER(void,            "{}"    );
IPC_FMT_TEMPLATE_TYPE_HEADER(char *,          "string");
IPC_FMT_TEMPLATE_TYPE_HEADER(unsigned char,   "uchar" );
IPC_FMT_TEMPLATE_TYPE_HEADER(char,            "char"  );
IPC_FMT_TEMPLATE_TYPE_HEADER(unsigned int,    "uint"  );
IPC_FMT_TEMPLATE_TYPE_HEADER(int,             "int"   );
IPC_FMT_TEMPLATE_TYPE_HEADER(bool,            "char"  );
IPC_FMT_TEMPLATE_TYPE_HEADER(float,           "float" );
IPC_FMT_TEMPLATE_TYPE_HEADER(double,          "double");

#endif // INCIPCFormatPrimTypes_h

/***************************************************************************
 * REVISION HISTORY:
 * $Log: mrIPCFormatPrimTypes.h,v $
 * Revision 1.3  2005/03/28 16:49:16  dom
 * Added include for strlen
 *
 * Revision 1.2  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.1  2003/02/14 05:58:05  trey
 * initial atacama check-in
 *
 * Revision 1.5  2002/08/15 21:12:19  trey
 * minor mods
 *
 * Revision 1.4  2001/11/06 22:00:56  trey
 * corrected IPC format for bools
 *
 * Revision 1.3  2001/11/01 16:41:17  trey
 * made getIPCFormatT<T> work for arbitrary pointer types
 *
 * Revision 1.2  2001/10/31 23:40:26  trey
 * changed name of getIPCFormat<T>() to getIPCFormatT<T>(), avoiding name conflict with T::getIPCFormat(), as defined by xdrgen
 *
 * Revision 1.1  2001/10/25 21:07:18  trey
 * initial check-in
 *
 *
 ***************************************************************************/
