#include "ntb_ss_client.h"

namespace ntb
{
bool SSClient::start()
{
    if(-1==(ss_epoll=epoll_create1(0)))
    {
        return false;
    }
    if(-1==(req_msg_evfd=eventfd(1,O_NONBLOCK)))
    {
        return false;
    }
    if(-1==(epollMod(ss_epoll,EPOLLIN|EPOLLET,req_msg_evfd,EPOLL_CTL_ADD)))
    {
        return false;
    }

    if(-1==(exp_check_tmfd=timerfd_create(CLOCK_REALTIME,TFD_NONBLOCK)))
    {
        return false;
    }
    if(-1==(epollMod(ss_epoll,EPOLLIN|EPOLLET,exp_check_tmfd,EPOLL_CTL_ADD)))
    {
        return false;
    }
    if(-1==timerSet(exp_check_tmfd))
    {
        return false;
    }
    int n;
    int64_t time_now;
restart:
    try
    {
        init();
        while(true)
        {

            n=epoll_wait(ss_epoll,events,max_events,-1);
            if (-1==n)
            {
                ntbLog(ntb::SINK_COMMON,NTB_LOG_ERR,"epoll_wait error:%s",errString(errno));
                usleep(100);
                continue;
            }
            for(i=0;i<n;++i)
            {
                if ((events[i].events & EPOLLHUP)||(events[i].events & EPOLLRDHUP))
                {
                    if(events[i].data.fd==in.sock)
                    {
                        ntbLog(ntb::SINK_COMMON,NTB_LOG_CRIT,"in.sock EPOLL(RD)HUP. Reconnecting to &s",remote_path.data());
                        inReconnect();
                    }
                    else if(events[i].data.fd==out.sock)
                    {
                        ntbLog(ntb::SINK_COMMON,NTB_LOG_CRIT,"out.sock EPOLL(RD)HUP. Reconnecting to &s",remote_path.data());
                        outReconnect();
                    }
                }
                else if ((events[i].events & EPOLLERR))
                {

                    if(events[i].data.fd==in.sock)
                    {
                        ntbLog(ntb::SINK_COMMON,NTB_LOG_CRIT,"in.sock EPOLLERR:%s. Reconnecting to %s",errString(errno),remote_path.data());
                        inReconnect();
                    }
                    else if(events[i].data.fd==out.sock)
                    {
                        ntbLog(ntb::SINK_COMMON,NTB_LOG_CRIT,"out.sock EPOLLERR:%s. Reconnecting to %s",errString(errno),remote_path.data());
                        outReconnect();
                    }
                }
                else if(events[i].data.fd==out.connect_fd)
                {
                    if(ssConnect(out.sock))
                    {
                        epollMod(ss_epoll,EPOLLIN|EPOLLRDHUP,out.sock,EPOLL_CTL_ADD);
                    }
                    else
                    {
                        timerSet(out.connect_fd,out.reconnect_timer);
                    }
                }
                else if(events[i].data.fd==in.connect_fd)
                {
                    if(ssConnect(in.sock))
                    {
                        epollMod(ss_epoll,EPOLLIN|EPOLLRDHUP,in.sock,EPOLL_CTL_ADD);
                    }
                    else
                    {
                        timerSet(in.connect_fd,in.reconnect_timer);
                    }
                }
                else if((events[i].data.fd==in.sock)&&(events[i].events & EPOLLIN))
                {
                    inEpollin();
                }
                //			else if((events[i].data.fd==in.sock)&&(events[i].events & EPOLLOUT))
                //			{

                //			}
                else if((events[i].data.fd==out.sock)&&(events[i].events & EPOLLIN))
                {
                    outEpollin();
                }
                else if((events[i].data.fd==out.sock)&&(events[i].events & EPOLLOUT))
                {

                }
                else if(events[i].data.fd==req_msg_evfd)
                {
                    if(msg_queue.isEmpty())continue;
                    if((out.ready==out.msg_status)&&out.chan_ready&&in.chan_ready)
                    {
                        if(out.send.attempts>=out.send.max_attempts)
                        {
                            resMsgAdd(msg_queue.beg->data.msg_id,res_too_many_attempts);
                            reqMsgDel(msg_queue.beg);
                            continue;
                        }

                        //out.send.buff=msg_queue.beg->data.payload;//+msg_queue.beg->data.msg_id
                        out.send.pos=msg_queue.beg->data.msg.length();
                        send_result=ssSendAll(out.sock,msg_queue.beg->data.msg.data(),out.send.pos);
                        switch (send_result)
                        {
                        case SEND_OK:
                            out.msg_status=out.busy;
                            break;
                        case SEND_ERR:
                            resMsgAdd(msg_queue.beg->data.msg_id,res_err);
                            reqMsgDel(msg_queue.beg);
                            outReconnect();
                            break;
                        default:
                            if(-1!=epollMod(ss_epoll,EPOLLOUT|EPOLLRDHUP,out.sock,EPOLL_CTL_MOD))
                            {
                                out.send.attempts++;
                                break;
                            }
                            resMsgAdd(msg_queue.beg->data.msg_id,res_err);
                            reqMsgDel(msg_queue.beg);
                            outReconnect();
                        }
                    }
                    else
                    {
                        if(false==resMsgAdd(msg_queue.beg->data.msg_id,res_unavailable,server_chan::common,"svc unavailable"))
                            cout<<"resMsgAdd err"<<endl;
                        reqMsgDel(msg_queue.beg);
                        eventfd_write(req_msg_evfd,0);//TESTING
                    }
                }

                else if(events[i].data.fd==exp_check_tmfd)
                {continue;
                    time_now=duration_cast< milliseconds >(system_clock::now().time_since_epoch()).count();
                    exp_mmap_lock.lock();
                    for(auto it=exp_stamps.begin();it!=exp_stamps.end();)
                    {
                        if(time_now>=it->first)
                        {
                            resMsgAdd(it->second->data.msg_id,res_expired,server_chan::common,"expired msg_id: "+std::to_string(it->second->data.msg_id));
                            msg_queue.nodeUnlink(it->second);
                            it=exp_stamps.erase(it);
                        }
                        else
                        {
                            timerSet(exp_check_tmfd,it->first,TFD_TIMER_ABSTIME);
                            break;
                        }
                    }
                    exp_mmap_lock.unlock();
                }
            }
        }
    }

    catch(const ntb::ntbException& e)
    {
        //	printf("%s\n",e.what());
        ntbLog(ntb::SINK_COMMON,NTB_LOG_CRIT,"SS exception: %s, RESTARTING",e.what());
    }
    catch(const std::exception& e)
    {
         ntbLog(ntb::SINK_COMMON,NTB_LOG_CRIT,"%s caught, RESTARTING",e.what());
    }
    catch(...)
    {
        ntbLog(ntb::SINK_COMMON,NTB_LOG_CRIT,"Unknown exception caught, RESTARTING");
    }
    cleanAll();
    usleep(1000);
    goto restart;
}
static MsgResult *msg_p;
bool SSClient::resMsgAdd(const uint64_t msg_id,const int res_code,int method, const std::string str) noexcept
{
    res_umap_lock.lock();
    try
    {
        msg_p=&res_umap.at(msg_id);
    }
    catch (...)
    {
        res_umap_lock.unlock();
        return false;
    }
    res_umap_lock.unlock();

    msg_p->rescode=res_code;
    msg_p->payload=std::move(str);
    msg_p->method=method;

    res_set_lock.lock();
    //auto ret=
    res_set.emplace(msg_id);
    res_set_lock.unlock();

    //if(ret.second==false)
    //return false;
    eventfd_write(res_msg_evfd,0);
    return true;
}
void SSClient::cleanAll()
{
    in.reset();
    if(-1==close(in.connect_fd))
        epollMod(ss_epoll,0,in.connect_fd,EPOLL_CTL_DEL);
    if(-1==close(in.sock))
        epollMod(ss_epoll,0,in.sock,EPOLL_CTL_DEL);

    out.reset();
    if(-1==close(out.connect_fd))
        epollMod(ss_epoll,0,out.connect_fd,EPOLL_CTL_DEL);
    if(-1==close(out.sock))
        epollMod(ss_epoll,0,out.sock,EPOLL_CTL_DEL);
}


void SSClient::inReconnect()
{
    in.chan_ready=false;
    if(-1==close(in.sock))
    {
        if(-1==epollMod(ss_epoll,0,in.sock,EPOLL_CTL_DEL))
        {
            throw(ntbException(std::string ("epollMod error:").append(errString(errno))));
        }
    }
    in.reset();
    if(!ssConnect(in.sock))
    {
        if(-1==timerSet(in.connect_fd,in.reconnect_timer))
        {
            throw(ntbException(std::string ("timerSet error:").append(errString(errno))));
        }
    }
}
void SSClient::outReconnect()
{
    out.chan_ready=false;
    if(-1==close(out.sock))
    {
        if(-1==epollMod(ss_epoll,0,out.sock,EPOLL_CTL_DEL))
        {
            throw(ntbException(std::string ("epollMod error:").append(errString(errno))));
        }
    }
    out.reset();
    if(!ssConnect(out.sock))
    {
        if(-1==timerSet(out.connect_fd,out.reconnect_timer))
        {
            throw(ntbException(std::string ("timerSet error:").append(errString(errno))));
        }
    }
}
bool SSClient::ssConnect(int &sock)
{
    if(!ssTcpConnect(sock))
    {
        ntbLog(ntb::SINK_COMMON,NTB_LOG_ERR,"Couldn't connect to %s",remote_path.data());
        close(sock);
        sock=-1;
        return false;
    }
    ntbLog(ntb::SINK_COMMON,NTB_LOG_INFO,"Connected to %s",remote_path.data());

    if(!ssReg(sock))
    {
        ntbLog(ntb::SINK_COMMON,NTB_LOG_ERR,"Channel registration request failed.");
        close(sock);
        sock=-1;
        return false;
    }
    ntbLog(ntb::SINK_COMMON,NTB_LOG_INFO,"Channel registration request sent.");
    return true;
}
bool SSClient::ssTcpConnect(int &sock)
{
    sockaddr_un remote;
    sock = socket(AF_UNIX, SOCK_STREAM|SOCK_NONBLOCK, 0);
    remote.sun_family=AF_UNIX;
    strcpy(remote.sun_path,remote_path.data());

    if (-1==connect(sock, reinterpret_cast<sockaddr*>(&remote), sizeof(remote)))
    {
        return false;
    }
    return true;
}
bool SSClient::ssReg(const int sock)
{
    std::string reg="{\"svc_id\":"+std::to_string(service_id)+
            ",\"msg_type\":"+((sock==out.sock)? std::to_string(client_chan::chan_reg) : std::to_string(server_chan::chan_reg))+"}" MSG_END;
    if(SEND_OK==ssSendAll(sock,reg.c_str(),reg.length()))return true;
    return false;
}
ssize_t SSClient::ssSendAll(const int fd, const char* buf, const size_t len) noexcept
{
    size_t total=0;
    ssize_t ret;
    while(total<len)
    {
        ret=sendto(fd,buf+total,len-total,MSG_NOSIGNAL,nullptr,0);
        if(-1==ret)
        {
            if((errno==EAGAIN)||(errno==EWOULDBLOCK))
            {
                return static_cast <ssize_t>(total);
            }
            return SEND_ERR;
        }
        total+=static_cast <size_t>(ret);
    }
    return SEND_OK;
}
//int SSClient::ssRecvAll()noexcept
//{
//	ssize_t recv_result;

//	while((recv_result=recv(in.sock,
//							&in.recv.buff[in.recv.len],
//							SERVER_MSG_BUFF_SIZE-in.recv.len,
//							MSG_NOSIGNAL))>0)
//	{
//		in.recv.len+=static_cast<size_t>(recv_result);
//		if((in.recv.len>MSG_END_LEN)&&(strncmp(&in.recv.buff[in.recv.len],MSG_END,MSG_END_LEN)))
//		{
//			in.recv.buff[in.recv.len]='\0';
//			return RECV_OK;
//		}
//		if(in.recv.len==SERVER_MSG_BUFF_SIZE)//msg is too big
//			return RECV_ERR;
//	}
//	if(-1==recv_result)
//	{
//		if((errno==EAGAIN)||(errno==EWOULDBLOCK))
//		{
//			return RECV_INCOMPLETE;
//		}
//	}
//	return RECV_ERR;//recv==0
//}


}//end of namespace


