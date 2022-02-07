/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.6 $  $Author: trey $  $Date: 2004/08/02 02:56:24 $
 *  
 * PROJECT: Life in the Atacama
 *
 * @file    RCLXML.h
 * @brief   No brief
 ***************************************************************************/

#ifndef INCRCLXML_h
#define INCRCLXML_h

#include "RCL.h"

namespace rcl {

  namespace xml {

    /********** parsing functions */

    exp readFromFD(const string& fileName, int fd);
    exp readFromFile(const string& configFile);
    exp readFromFile(const std::string& filename, FILE* configFile);
    exp readFromString(const string& s);
    
    /********** output functions */

    void writeXML(std::ostream& out, const exp& expr);
    std::string toString(const exp& expr);

    /********** internalfunctions */

    void writeXMLRecurse(std::ostream& out, const exp& expr,
			 int indentChars);

  }; // namespace xml

}; // namespace rcl

#endif // INCRCLXML_h

/***************************************************************************
 * REVISION HISTORY:
 * $Log: RCLXML.h,v $
 * Revision 1.6  2004/08/02 02:56:24  trey
 * strings are now quoted more often
 *
 * Revision 1.5  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.4  2004/04/13 21:12:38  trey
 * did some stress testing, fixed several obscure bugs
 *
 * Revision 1.3  2004/04/13 03:21:11  trey
 * xml front end now in relatively stable form
 *
 * Revision 1.2  2004/04/09 20:31:33  trey
 * major enhancement to rcl interface
 *
 * Revision 1.1  2004/04/09 00:48:12  trey
 * initial check-in
 *
 *
 ***************************************************************************/
