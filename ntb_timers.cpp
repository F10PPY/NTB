#include "ntb_timers.h"

namespace ntb
{
static itimerspec instant;
void timersInit()
{
	instant.it_value.tv_sec =0;
	instant.it_value.tv_nsec =1;
	instant.it_interval.tv_sec =0;
	instant.it_interval.tv_nsec =0;
}
int timerSetInt(const int fd, const __syscall_slong_t int_ms, const int flags)
{
	itimerspec new_value;
	new_value.it_value.tv_sec =0;
	new_value.it_value.tv_nsec =1;
	new_value.it_interval.tv_sec = int_ms/1000;;
	new_value.it_interval.tv_nsec =(int_ms % 1000)*1'000'000;
	return timerfd_settime(fd,flags,&new_value,nullptr);
}
int timerSet(const int fd,const __syscall_slong_t val_ms, const int flags)
{
	itimerspec new_value;
	new_value.it_value.tv_sec = val_ms/1000;
	new_value.it_value.tv_nsec = (val_ms % 1000)*1'000'000;
	new_value.it_interval.tv_sec = 0;
	new_value.it_interval.tv_nsec =0;
	return timerfd_settime(fd,flags,&new_value,nullptr);
}
int timerSet(const int fd)
{
	return timerfd_settime(fd,TFD_TIMER_ABSTIME,&instant,nullptr);
}
//int timerSetns(const int fd, const __syscall_slong_t val_ms)
//{
//	itimerspec new_value;
//	new_value.it_value.tv_sec =0;
//	new_value.it_value.tv_nsec =10000000000000000;
//	new_value.it_interval.tv_sec =0;
//	new_value.it_interval.tv_nsec =0;
//	return timerfd_settime(fd,0,&new_value,nullptr);
//}
}
