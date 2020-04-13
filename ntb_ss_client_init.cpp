#include "ntb_ss_client.h"
#include <semaphore.h>
namespace ntb
{
uint64_t SSClient::service_id;
__syscall_slong_t SSClient::msg_timeout;
uint64_t SSClient::nodeInit()
{
    sem_t *sem;
	service_id = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
    while(nullptr==(sem=sem_open(std::to_string(service_id).substr(4).c_str(),O_CREAT|O_EXCL,S_IRWXU,1)))
    {
        std::cout<<std::to_string(service_id).substr(4)<<" sem_open err: "<<strerror(errno)<<std::endl;
		usleep(1000);
		service_id =static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
    }
    usleep(1000);//need for new time
    sem_close(sem);
    if(-1==sem_unlink(std::to_string(service_id).substr(4).c_str()))
    {
        puts(strerror(errno));
    }
	return service_id;
}
void SSClient::init()
{
	events=new epoll_event[static_cast<unsigned long>(max_events)];
	in.recv.buff=new char[SERVER_MSG_BUFF_SIZE+1];//+1 for \0 terminate
	if(-1==(out.connect_fd=timerfd_create(CLOCK_MONOTONIC,TFD_NONBLOCK)))
	{
        throw(ntbException(std::string ("timerfd_create error:").append(errString(errno))));
	}
	if(-1==(epollMod(ss_epoll,EPOLLIN|EPOLLET,out.connect_fd,EPOLL_CTL_ADD)))
	{
        throw(ntbException(std::string ("epollMod error:").append(errString(errno))));
	}
	if(-1==timerSet(out.connect_fd))
	{
        throw(ntbException(std::string ("timerSet error:").append(errString(errno))));
	}

	if(-1==(in.connect_fd=timerfd_create(CLOCK_MONOTONIC,TFD_NONBLOCK)))
	{
        throw(ntbException(std::string ("timerfd_create error:").append(errString(errno))));
	}
	if(-1==(epollMod(ss_epoll,EPOLLIN|EPOLLET,in.connect_fd,EPOLL_CTL_ADD)))
	{
        throw(ntbException(std::string ("epollMod error:").append(errString(errno))));
	}
	if(-1==timerSet(in.connect_fd))
	{
        throw(ntbException(std::string ("timerSet error:").append(errString(errno))));
	}

}


}//namespace end
