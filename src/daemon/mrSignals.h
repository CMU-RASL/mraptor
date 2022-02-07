/********** tell emacs we use -*- c++ -*- style comments *******************
 * $Revision: 1.1 $  $Author: trey $  $Date: 2006/06/23 16:39:53 $
 *  
 * @file    mrSignals.h
 * @brief   No brief
 ***************************************************************************/

#ifndef INCmrSignals_h
#define INCmrSignals_h

namespace microraptor {

  void make_signal_table(void);
  int get_sig_num_for_name(const char* name);
  const char* get_name_for_sig_num(int i);
  const char* get_description_for_sig_num(int i);

}; // namespace microraptor

#endif // INCmrSignals_h

/***************************************************************************
 * REVISION HISTORY:
 * $Log: mrSignals.h,v $
 * Revision 1.1  2006/06/23 16:39:53  trey
 * initial check-in
 *
 *
 ***************************************************************************/
