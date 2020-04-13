#ifndef NTB_TIMERS_H
#define NTB_TIMERS_H
#include <sys/timerfd.h>
namespace ntb
{
void timersInit();
int timerSetInt(const int fd, const __syscall_slong_t int_ms, const int flags=0);
int timerSet(const int fd, const __syscall_slong_t val_ms, const int flags=0);
int timerSet(const int fd);
//int timerSetns(const int fd, const __syscall_slong_t val_ms);
}//namespace end
#endif // NTB_TIMERS_H
