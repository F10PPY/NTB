#include "ntb_signals.h"
#include "ntb_log.h"
namespace ntb
{
void sigEventsCycle()
{
    sigset_t mask;
    if(sigfillset(&mask)<0)printf("Sigfillset error: %s",ntb::errString(errno));
    if(-1==sigdelset(&mask,SIGSEGV)){	puts("sigdelset err");	return;	}
    if(-1==sigdelset(&mask,SIGBUS)){	puts("sigdelset err");	return;	}
    if(-1==sigdelset(&mask,SIGFPE)){	puts("sigdelset err");	return;	}
    if(-1==sigdelset(&mask,SIGILL)){	puts("sigdelset err");	return;	}
    int sfd = signalfd(-1, &mask,0);
    if (sfd < 0)
    {
        ntbLog(ntb::LogSink::SINK_COMMON,NTB_LOG_CRIT,"Signalfd error:%s",ntb::errString(errno))
        return;
    }

    int sig_epoll=epoll_create1(0);
    if(-1==sig_epoll)
    {
        ntbLog(ntb::LogSink::SINK_COMMON,NTB_LOG_CRIT,"Epoll_create error:%s",ntb::errString(errno))
        return;
    }
    if(-1==epollMod(sig_epoll,EPOLLIN,sfd,EPOLL_CTL_ADD))
    {
        ntbLog(ntb::LogSink::SINK_COMMON,NTB_LOG_CRIT,"Epoll_ctl error:%s",ntb::errString(errno))
        return;
    }
    epoll_event events[4];
    struct signalfd_siginfo si;
    int n,i;

    while(true)
    {
        n=epoll_wait(sig_epoll,events,4,-1);
        if (-1==n)
        {
            ntbLog(ntb::LogSink::SINK_COMMON,NTB_LOG_ERR,"Epoll_wait error:%s",ntb::errString(errno))
            usleep(50000);
            continue;
        }
        for (i=0;i<n;++i)
        {
            if(-1==read(sfd, &si, sizeof(si)))
            {
                ntbLog(ntb::LogSink::SINK_COMMON,NTB_LOG_ERR,"Read error:%s",ntb::errString(errno))
                continue;
            }
            if (si.ssi_signo == SIGINT)
            {
                ntbLog(ntb::LogSink::SINK_COMMON,NTB_LOG_WARN,"SIGINT received, terminating...")
                write(STDERR_FILENO, "SIGINT\n",7);
                P7_Exceptional_Flush();
                usleep(50000);
                _exit(EXIT_FAILURE);
            }
            else
            {
                ntbLog(ntb::LogSink::SINK_COMMON,NTB_LOG_WARN,"%s received",strsignal(static_cast<int>(si.ssi_signo)))
                printf("received signal: %s",strsignal(static_cast<int>(si.ssi_signo)));
            }
        }
    }
}
const char *signalString(const int signum)
{
    switch (signum)
    {
    case SIGSEGV:   return "SIGSEGV";
    case SIGBUS:   return "SIGBUS";
    case SIGFPE:   return "SIGFPE";
    case SIGILL:   return "SIGILL";
    default:        return "UNCAUGHT SIG";
    }
}
[[noreturn]]  void sigHandler(const int signum)noexcept
{
    ntbLog(ntb::LogSink::SINK_COMMON,NTB_LOG_CRIT,"Critical signal received:%s",signalString(signum))
    write(STDERR_FILENO, "critical signal received\n",26);
    P7_Exceptional_Flush();
    usleep(50000);
    _exit(EXIT_FAILURE);
}
bool initSigAltStack()noexcept
{
    stack_t ss;
    try
    {
        ss.ss_sp = new void*[SIGSTKSZ];
        ss.ss_size = SIGSTKSZ ;
        ss.ss_flags = 0;
        if(-1==sigaltstack(&ss, nullptr))return false;
        return true;
    }
    catch (...)
    {
        return false;
    }
}
bool setSigHandlers()
{
    sigset_t mask;
    if(sigfillset(&mask)<0)return false;
    if(-1==sigdelset(&mask,SIGSEGV))return false;
    if(-1==sigdelset(&mask,SIGBUS))return false;
    if(-1==sigdelset(&mask,SIGFPE))return false;
    if(-1==sigdelset(&mask,SIGILL))return false;
    if (pthread_sigmask(SIG_SETMASK, &mask,nullptr) < 0)return false;

    initSigAltStack();
    struct sigaction sa = {};
    sa.sa_handler = sigHandler;
    sa.sa_flags = SA_ONSTACK;

    if(-1==sigaction(SIGSEGV, &sa, nullptr))return false;
    if(-1==sigaction(SIGBUS, &sa, nullptr))return false;
    if(-1==sigaction(SIGFPE, &sa, nullptr))return false;
    if(-1==sigaction(SIGILL, &sa, nullptr))return false;

    return true;
}
}//namespace end
