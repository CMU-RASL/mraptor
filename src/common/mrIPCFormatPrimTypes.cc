/****************************************************************************
 * $Revision: 1.1 $  $Author: trey $  $Date: 2003/02/14 05:58:05 $
 *
 * PROJECT:      Distributed Robotic Agents
 * DESCRIPTION:  
 *
 * (c) Copyright 2001 CMU. All rights reserved.
 ***************************************************************************/

/***************************************************************************
 * INCLUDES
 ***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "mrIPCFormatPrimTypes.h"

/***************************************************************************
 * FUNCTIONS
 ***************************************************************************/

IPC_FMT_TEMPLATE_TYPE(void,            "{}"    );
IPC_FMT_TEMPLATE_TYPE(char *,          "string");
IPC_FMT_TEMPLATE_TYPE(unsigned char,   "uchar" );
IPC_FMT_TEMPLATE_TYPE(char,            "char"  );
IPC_FMT_TEMPLATE_TYPE(unsigned int,    "uint"  );
IPC_FMT_TEMPLATE_TYPE(int,             "int"   );
IPC_FMT_TEMPLATE_TYPE(bool,            "char"   );
IPC_FMT_TEMPLATE_TYPE(float,           "float" );
IPC_FMT_TEMPLATE_TYPE(double,          "double");

/***************************************************************************
 * REVISION HISTORY:
 * $Log: mrIPCFormatPrimTypes.cc,v $
 * Revision 1.1  2003/02/14 05:58:05  trey
 * initial atacama check-in
 *
 * Revision 1.5  2002/08/15 21:12:19  trey
 * minor mods
 *
 * Revision 1.4  2001/11/06 22:00:57  trey
 * corrected IPC format for bools
 *
 * Revision 1.3  2001/11/01 16:41:16  trey
 * made getIPCFormatT<T> work for arbitrary pointer types
 *
 * Revision 1.2  2001/10/31 23:40:26  trey
 * changed name of getIPCFormat<T>() to getIPCFormatT<T>(), avoiding name conflict with T::getIPCFormat(), as defined by xdrgen
 *
 * Revision 1.1  2001/10/25 21:07:18  trey
 * initial check-in
 *
 * Revision 1.2  2001/03/05 23:52:04  trey
 * fixed format bug
 *
 * Revision 1.1  2001/03/04 23:06:48  trey
 * added ipcFormatPrimTypes
 *
 *
 ***************************************************************************/
