/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.6 $  $Author: dom $  $Date: 2004/04/28 17:15:21 $
 *  
 * PROJECT: FIRE Architecture Project
 *
 * @file    processStatus.cc
 * @brief   No brief
 ***************************************************************************/

/***************************************************************************
 * INCLUDES
 ***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <iostream>

#include "mrHashMap.h"
#include "mrCommonDefs.h"
#include "processStatus.h"
#include "RCL.h"

using namespace std;

/**********************************************************************
 * FORWARD DECLARATIONS
 **********************************************************************/

static string downcase(const string& s);

/**********************************************************************
 * GLOBALS
 **********************************************************************/

#define STATUS_ENTRY(x) pair<int,string>(MR_##x,downcase(#x))
static pair<int,string> status_table_temp_g[] = {
  STATUS_ENTRY(NOT_STARTED),
  STATUS_ENTRY(PENDING),
  STATUS_ENTRY(STARTING),
  STATUS_ENTRY(RUNNING),
  STATUS_ENTRY(CLEAN_EXIT),
  STATUS_ENTRY(ERROR_EXIT),
  STATUS_ENTRY(SIGNAL_EXIT)
};
static hash_map<int,string> status_table_g
  (status_table_temp_g, &status_table_temp_g[MR_MAX_STATUS]);

static string downcase(const string& s) {
  string s2 = s;
  FOR (i, (int)s2.size()) {
    if (isupper(s2[i])) s2[i] += ('a' - 'A');
  }
  return s2;
}

/**********************************************************************
 * NON-MEMBER FUNCTIONS
 **********************************************************************/

template <class MapType>
typename MapType::key_type
safe_map_rget(MapType& m, const typename MapType::data_type& val,
	      const string& action_text)
{
  FOR_EACH(pr, m) {
    if (pr->second == val) {
      return pr->first;
    }
  }
  throw MRException(action_text + ": couldn't find `"
		    + rcl::toString(val) + "' in lookup table");
}

/**********************************************************************
 * STATUS_INTERFACE FUNCTIONS
 **********************************************************************/

string Process_Status::to_string(int status) {
  if (status_table_g.find(status) == status_table_g.end()) {
    throw MRException(string("invalid status value ")
		      + rcl::toString(status));
  }
  return status_table_g[status];
}

int Process_Status::from_string(const string& s) {
  return safe_map_rget(status_table_g, s,
		       "while parsing status value");
}

bool Process_Status::is_runnable(int status) {
  return (status != MR_PENDING) && (status != MR_STARTING)
    && (status != MR_RUNNING);
}

bool Process_Status::has_console(int status) {
  return (status == MR_STARTING) || (status == MR_RUNNING);
}

/***************************************************************************
 * REVISION HISTORY:
 * $Log: processStatus.cc,v $
 * Revision 1.6  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.5  2004/04/09 20:31:33  trey
 * major enhancement to rcl interface
 *
 * Revision 1.4  2004/02/10 17:39:44  brennan
 * Made things happy with gcc 3.0.3.
 *
 * Revision 1.3  2003/10/05 02:56:28  trey
 * re-fixed some gcc 3.2 issues
 *
 * Revision 1.2  2003/08/08 19:19:58  trey
 * made changes to enable compilation under g++ 3.2
 *
 * Revision 1.1  2003/03/10 16:02:40  trey
 * added new process status values: pending and starting
 *
 *
 ***************************************************************************/
