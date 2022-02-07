
#include "mrException.h"
#include "mrQuickInterface.h"

int main(int argc, char** argv) {
  try {
    microraptor::restart_process("zoeController");
  } catch (MRException err) {
    /* If we reach this point, an error message was already printed out.
       Without try/catch, the exception would cause a core dump.  (Note:
       dumping core may be want you want for debugging.) */
    exit(1);
  }
  return 0;
}
