/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.8 $  $Author: trey $  $Date: 2005/06/16 00:25:24 $
 *  
 * PROJECT: FIRE Architecture Project
 *
 * @file    RCLMap.h
 * @brief   No brief
 ***************************************************************************/

#ifndef INCRCLMap_h
#define INCRCLMap_h

#include <vector>

#include "RCLTypes.h"
#include "RCLDebugInfo.h"

namespace rcl {

  struct DebugInfo;

  struct map {
    typedef std::pair< string, exp > RCLMapPair;
    typedef std::vector< RCLMapPair > RCLMapVector;
  
    typedef RCLMapVector::value_type value_type;
    typedef RCLMapVector::pointer pointer;
    typedef RCLMapVector::const_pointer const_pointer;
    typedef RCLMapVector::iterator iterator;
    typedef RCLMapVector::const_iterator const_iterator;
    typedef RCLMapVector::reference reference;
    typedef RCLMapVector::const_reference const_reference;
    typedef RCLMapVector::size_type size_type;

    /********** accessor functions */

    exp& operator()(const string& name);
    const exp& operator()(const string& name) const;

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
    void push_back(const string& name, const exp& expr);
    void addFrom(const map& rhs);
    void erase(const string& key);

    /********** functions to aid debugging (mostly used by the parser) */

    void setFileLineNumber(const string& _inputFile,
			   int _lineNumber);
    map(const string& _inputFile, int _lineNumber);

    /********** internal -- you shouldn't need these */

    DebugInfo debugInfo;
    RCLMapVector body;

    map(void) {}

    int find(const string& name) const;
    void setExpression(const string& name, const exp& expr);
    void add(const string& name, const exp& expr);

    exp& getExpressionRef(const string& name);  
    const exp& getExpression(const string& name) const;
    
    void writeCompact(std::ostream& out) const;
    void writeConfig(std::ostream& out, int indent) const;
    
  protected:
    int getLastDefined(void) const;
  };

}; // namespace rcl

std::ostream& operator<<(std::ostream& out, const rcl::map& m);

#endif // INCRCLMap_h

/***************************************************************************
 * REVISION HISTORY:
 * $Log: RCLMap.h,v $
 * Revision 1.8  2005/06/16 00:25:24  trey
 * removed declaration of ifdefd out function canonicalize
 *
 * Revision 1.7  2005/06/14 15:50:03  trey
 * removed use of std::map from rcl::map, now simpler but less efficient for really big maps
 *
 * Revision 1.6  2004/10/08 02:24:33  trey
 * added erase() function
 *
 * Revision 1.5  2004/06/08 14:53:39  dom
 * Removed redundant map:: qualifier on map::genLookup declaration to avoid compiler warnings.
 *
 * Revision 1.4  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.3  2004/04/13 21:12:38  trey
 * did some stress testing, fixed several obscure bugs
 *
 * Revision 1.2  2004/04/12 19:24:50  trey
 * more api cleanup
 *
 * Revision 1.1  2004/04/09 20:31:33  trey
 * major enhancement to rcl interface
 *
 * Revision 1.7  2004/04/09 00:48:34  trey
 * made changes to support XML front end
 *
 * Revision 1.6  2004/04/08 16:42:56  trey
 * overhauled exp
 *
 * Revision 1.5  2003/06/06 16:11:40  trey
 * fixed addFrom() to overwrite as advertised
 *
 * Revision 1.4  2003/03/14 02:24:12  trey
 * kludge fixed problem that checking whether an element of a hash or vector is defined() creates a new entry for it
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
 * Revision 1.1  2002/09/24 21:50:24  trey
 * rebuilt RCL from scratch to support nested vectors and hashes
 *
 *
 ***************************************************************************/
