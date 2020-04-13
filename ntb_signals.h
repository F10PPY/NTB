#ifndef NTB_SIGNALS_H
#define NTB_SIGNALS_H
#include <sys/signalfd.h>
#include <signal.h>
#include "common.h"
#include "ntb_errors.h"

namespace ntb
{
bool initSigAltStack()noexcept;
void sigEventsCycle();
bool setSigHandlers();
}//namespace end
#endif // NTB_SIGNALS_H
