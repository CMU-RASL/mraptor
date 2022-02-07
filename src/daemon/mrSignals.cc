/********** tell emacs we use -*- c++ -*- style comments *******************
 * $Revision: 1.3 $  $Author: brennan $  $Date: 2006/12/20 19:06:54 $
 *  
 * @file    mrSignals.cc
 * @brief   No brief
 ***************************************************************************/

/***************************************************************************
 * INCLUDES
 ***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>

#include <iostream>

#include <string.h>

#include "mrSignals.h"
#include "mrCommonDefs.h"

using namespace std;

namespace microraptor {

struct SigEntry {
  const char* name;
  const char* description;
};

static SigEntry* signal_table_g = NULL;

void make_signal_table(void)
{
  signal_table_g = new SigEntry[SIGRTMAX];
  for (int i=0; i < SIGRTMAX; i++) {
    signal_table_g[i].name = NULL;
    signal_table_g[i].description = NULL;
  }
  
#define ADD_ENTRY(NAME,DESC) \
  signal_table_g[SIG##NAME].name = #NAME; \
  signal_table_g[SIG##NAME].description = DESC;

  ADD_ENTRY(HUP,  "hangup; not often seen running under mraptord");
  ADD_ENTRY(INT,  "interrupt; e.g. a Ctrl+C");
  ADD_ENTRY(ILL,  "illegal instruction; e.g. corrupted binary");
  ADD_ENTRY(ABRT, "abort; e.g. failed assertion or uncaught exception");
  ADD_ENTRY(BUS,  "bus error; e.g. array out of bounds");
  ADD_ENTRY(KILL, "e.g. explicit kill through mraptord and process did not die on first attempt");
  ADD_ENTRY(SEGV, "segmentation fault; e.g. dereferenced null pointer");
  ADD_ENTRY(PIPE, "broken pipe");
  ADD_ENTRY(TERM, "e.g. explicit kill through mraptord");
}

int get_sig_num_for_name(const char* name)
{
  for (int i=0; i < SIGRTMAX; i++) {
    const char* ni = signal_table_g[i].name;
    if (NULL != ni && 0 == strcmp(name, ni)) {
      return i;
    }
  }
  return -1;
}

const char* get_name_for_sig_num(int i)
{
  return signal_table_g[i].name;
}

const char* get_description_for_sig_num(int i)
{
  return signal_table_g[i].description;
}

}; // namespace microraptor

/***************************************************************************
 * REVISION HISTORY:
 * $Log: mrSignals.cc,v $
 * Revision 1.3  2006/12/20 19:06:54  brennan
 * 'MRAPTORD:' outputs are now pushed into the relevant line buffers,
 * so that they're returned when a playback is requested.
 *
 * Revision 1.2  2006/11/16 18:55:43  brennan
 * Made rate limits (the period, limit, and limit-after-stdin) command
 * line arguments for mraptord, and added options to mraptord.conf.
 *
 * Split up rate limit warning into a two messages (a continued line and
 * its terminator), since it was getting truncated by mraptord's
 * 80-character line limit.
 *
 * Revision 1.1  2006/06/23 16:39:53  trey
 * initial check-in
 *
 *
 ***************************************************************************/
