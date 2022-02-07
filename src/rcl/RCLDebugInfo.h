/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.5 $  $Author: trey $  $Date: 2005/05/16 21:10:42 $
 *  
 * @file    RCLDebugInfo.h
 * @brief   No brief
 ***************************************************************************/

#ifndef INCRCLDebugInfo_h
#define INCRCLDebugInfo_h

#include <string>

namespace rcl {

  struct DebugInfo {
    std::string inputFile;
    int lineNumber;
    std::string objectName;
    std::string undefCreationTrace;
    
    void setFileLineNumber(std::string _inputFile, int _lineNumber);
    std::string parseObjectName(void) const;
    std::string getUndefDebugTrace(void) const;
    std::string getDebugTrace(void) const;
  protected:
    std::string getDebugTraceInternal(bool undefFormat) const;
  };

}; // namespace rcl

#endif // INCRCLDebugInfo_h

/***************************************************************************
 * REVISION HISTORY:
 * $Log: RCLDebugInfo.h,v $
 * Revision 1.5  2005/05/16 21:10:42  trey
 * improved debug output
 *
 * Revision 1.4  2005/04/21 18:58:23  trey
 * made getDebugTrace() output more readable
 *
 * Revision 1.3  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.2  2004/04/09 20:31:33  trey
 * major enhancement to rcl interface
 *
 * Revision 1.1  2004/04/08 16:42:56  trey
 * overhauled exp
 *
 *
 ***************************************************************************/
