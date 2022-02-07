/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.1 $  $Author: trey $  $Date: 2005/06/24 17:57:54 $
 *  
 * PROJECT: Life in the Atacama
 *
 * @file    mrSets.h
 * @brief   No brief
 ***************************************************************************/

#ifndef INCmrSets_h
#define INCmrSets_h

#include <set>

#if 0
// return true if A is a subset of B
template <class T, class U>
bool subset(const T& A, const U& B) {
  FOR_EACH(eltp, A) {
    if ( B.find(*eltp) == B.end() ) return false;
  }
  return true;
}
#endif

// returns true if x is an element of A
template <class T>
bool set_contains(const std::list<T>& A,
		  const T& x)
			 
{
  FOR_EACH ( eltp, A ) {
    if ( *eltp == x ) return true;
  }
  return false;
}

// return true if A is a subset of B
template <class T>
bool subset(const std::list<T>& A,
	    const std::list<T>& B)
{
  FOR_EACH ( eltp, A ) {
    if ( !set_contains( B, *eltp )) return false;
  }
  return true;
}

template <class T>
void set_erase(std::list<T>& A,
	       const T& x)
{
  typename std::list<T>::iterator i, j;
  for (i = A.begin(); i != A.end(); i++) {
    if (x == *i) goto erase_i;
  }
  return;
 erase_i:
  j = i;
  j++;
  A.erase( i, j );
}

template <class T>
void set_insert(std::list<T>& A,
		const T& x)
{
  if (!set_contains(A,x)) A.push_back( x );
}

// A = A union B
template <class T>
void set_add(std::list<T>& A,
	     const std::list<T>& B)
{
  FOR_EACH (eltp, B) {
    set_insert( A, *eltp );
  }
}

// result = A intersect B
template <class T>
void set_intersection(std::list<T>& result,
		      const std::list<T>& A,
		      const std::list<T>& B)
{
  result.clear();
  FOR_EACH (eltp, A) {
    if (set_contains( B, *eltp )) {
      set_insert( result, *eltp );
    }
  }
}

// result = A setminus B
template <class T>
void set_diff(std::list<T>& result,
	      const std::list<T>& A,
	      const std::list<T>& B)
{
  result.clear();
  FOR_EACH (eltp, A) {
    if (!set_contains( B, *eltp )) {
      set_insert( result, *eltp );
    }
  }
}

#endif // INCmrSets_h

/***************************************************************************
 * REVISION HISTORY:
 * $Log: mrSets.h,v $
 * Revision 1.1  2005/06/24 17:57:54  trey
 * initial check-in
 *
 *
 ***************************************************************************/
