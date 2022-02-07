/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.13 $  $Author: trey $  $Date: 2004/12/14 23:56:38 $
 *  
 * PROJECT: FIRE Architecture Project
 *
 * @file    RCLVector.h
 * @brief   No brief
 ***************************************************************************/

#ifndef INCRCLVector_h
#define INCRCLVector_h

#include <vector>

#include "RCLTypes.h"
#include "RCLDebugInfo.h"

namespace rcl {

  struct vector {
    typedef std::vector< exp > RCLVectorVector;
    
    typedef RCLVectorVector::value_type value_type;
    typedef RCLVectorVector::pointer pointer;
    typedef RCLVectorVector::const_pointer const_pointer;
    typedef RCLVectorVector::iterator iterator;
    typedef RCLVectorVector::const_iterator const_iterator;
    typedef RCLVectorVector::reference reference;
    typedef RCLVectorVector::const_reference const_reference;
    typedef RCLVectorVector::size_type size_type;

    /********** accessor functions */

    exp& operator[](int index);
    const exp& operator[](int index) const;

    /********** information functions */

    size_t size(void) const;
    bool empty(void) const;
    string getDebugTrace(void) const;

    /********** iteration functions */

    iterator begin(void);
    iterator end(void);

    const_iterator begin(void) const;
    const_iterator end(void) const;

    /********** misc functions */

    void clear(void);
    void deleteElement(unsigned index);
    void addFrom(const vector& rhs);
    void push_back(const exp& expr);  
    bool isNumeric(void) const;

    /********** functions to aid debugging (mostly used by the parser) */

    void setFileLineNumber(const string& _inputFile,
			   int _lineNumber);
    vector(const string& _inputFile, int _lineNumber);

    /********** internal -- you shouldn't need these */

    DebugInfo debugInfo;
    RCLVectorVector body;
    
    vector(void) {}
    void add(const exp& expr);  
    exp& getExpressionRef(unsigned index);  
    const exp& getExpression(unsigned index) const;
    void setExpression(unsigned index, const exp& expr);
    void writeCompact(std::ostream& out) const;
    void writeCommand(std::ostream& out) const;
    void writeConfig(std::ostream& out, int indent) const;
    
  protected:
    size_t getDefinedSize(void) const;
  };

}; // namespace rcl

std::ostream& operator<<(std::ostream& out, const rcl::vector& m);

#endif // INCRCLVector_h

/***************************************************************************
 * REVISION HISTORY:
 * $Log: RCLVector.h,v $
 * Revision 1.13  2004/12/14 23:56:38  trey
 * improved printing of vectors that are entirely numeric
 *
 * Revision 1.12  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.11  2004/04/13 21:12:38  trey
 * did some stress testing, fixed several obscure bugs
 *
 * Revision 1.10  2004/04/12 19:53:42  trey
 * fixed some problems with push_back()
 *
 * Revision 1.9  2004/04/12 19:24:50  trey
 * more api cleanup
 *
 * Revision 1.8  2004/04/09 20:31:33  trey
 * major enhancement to rcl interface
 *
 * Revision 1.7  2004/04/08 16:42:56  trey
 * overhauled exp
 *
 * Revision 1.6  2003/06/06 14:38:19  trey
 * changed addFromHash to addFrom, made it also accept a vector
 *
 * Revision 1.5  2003/03/14 02:24:12  trey
 * kludge fixed problem that checking whether an element of a hash or vector is defined() creates a new entry for it
 *
 * Revision 1.4  2003/02/14 00:13:11  trey
 * added better support for checking the type of numeric constants
 *
 * Revision 1.3  2003/02/05 22:50:51  trey
 * mostly ready for mraptor now
 *
 * Revision 1.2  2003/02/05 20:23:20  trey
 * implemented command syntax and switched error behavior to throw exceptions
 *
 * Revision 1.1  2003/02/05 03:57:54  trey
 * initial atacama check-in
 *
 * Revision 1.1  2002/09/24 21:50:25  trey
 * rebuilt RCL from scratch to support nested vectors and hashes
 *
 *
 ***************************************************************************/
