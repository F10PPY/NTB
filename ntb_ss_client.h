#ifndef NTB_SS_CLIENT_H
#define NTB_SS_CLIENT_H

#include <semaphore.h>
#include <map>
#include "ntb_log.h"
#include "ntb_ss_core.h"
#include "ntb_errors.h"
#include "ntb_timers.h"
#include "rapidjson/document.h"
#include "ntb_pdllist.h"
using namespace rapidjson;
namespace ntb
{
struct MsgResult
{
	std::string payload;
	int method;
	int rescode;
	result_handler_variants hdl_vars;
	MsgResult(int fd):hdl_vars(fd){}
	MsgResult()
	{}

};
struct MsgResultFD :MsgResult
{

};


enum SendAllResult{SEND_ERR=-1,SEND_OK=0};
//enum RecvAllResult{RECV_OK=0,RECV_INCOMPLETE,RECV_ERR};

struct recv_struct
{
	char * buff=nullptr;
	uint len=0;// len==position
	uint attempts=0;
	uint max_attempts=5;
	void reset()
	{
		len=0;
		//attempts=0;
	}
	~recv_struct()noexcept
	{
		delete [] buff;
	}
};
struct send_struct
{
	std::string buff;
	uint attempts=0;
	uint max_attempts=5;
	size_t pos=0;
	void reset()
	{
		pos=0;
		//	attempts=0;
	}
};
struct in_struct
{
	bool chan_ready=false;
	unsigned int reconnect_timer=5000;//ms
	int sock=-1;
	int connect_fd=-1;
	recv_struct recv;
	send_struct send;
	void reset()noexcept
	{
		sock=-1;
		send.reset();
		recv.reset();
	}
	~in_struct()noexcept
	{
		close(connect_fd);
		close(sock);
	}
};
struct out_struct
{
	bool chan_ready=false;
	enum MsgStatus{busy,ready};
	int msg_status=ready;
	unsigned int reconnect_timer=5000;//ms
	int sock=-1;
	int connect_fd=-1;
	send_struct send;
	recv_struct recv;
	void reset()noexcept
	{
		recv.reset();
		send.reset();
		sock=-1;
	}
	~out_struct()noexcept
	{
		close(connect_fd);
		close(sock);
	}
};
struct MessageStruct
{
	uint64_t msg_id;
	std::multimap <__syscall_slong_t,List<MessageStruct>::node*>::iterator exp_stamp_it;
	std::string msg;
	MessageStruct()
	{
		msg.reserve(512);// doesn't make sense bcoz of move
	}
	MessageStruct& operator=(MessageStruct&& other)
	{
		if (this != &other)
		{
			msg_id=other.msg_id;
			exp_stamp_it=other.exp_stamp_it;
			msg=std::move(other.msg);
		}
		return *this;
	}
};
class SSClient
{
	static uint64_t service_id;
	const SSType service_type;
	const std::string_view remote_path;
	const int max_events;
	epoll_event *events=nullptr;
	in_struct in;
	out_struct out;
	Document jdom;
	Value::MemberIterator jit;
	int method=-1;
	uint64_t msg_id=0; //change to atomic if calling msgAdd from multiple threads
	static __syscall_slong_t msg_timeout;//ms
	int req_msg_evfd;
	int ss_epoll;
	int i;
	ssize_t recv_result;
	ssize_t send_result;
	ssize_t snd_bytes=0;

	void init();
	void cleanAll();
	ssize_t ssSendAll(const int fd, const char* buf, const size_t len) noexcept;
	int ssRecvAll()noexcept;
	void inEpollin();
	void outEpollin();
	void inReconnect();
	void outReconnect();
	bool ssConnect(int &sock);
	bool ssTcpConnect(int &sock);
	bool ssReg(const int sock);
	bool resMsgAdd(const uint64_t msg_id, const int res_code,const int method=server_chan::common, const std::string str="") noexcept;

public:
	Spinlock res_umap_lock;
	Spinlock res_set_lock;
	Spinlock exp_mmap_lock;
	std::unordered_map <uint64_t,MsgResult> res_umap;
	std::set <uint64_t>res_set;
	int res_msg_evfd;
	int exp_check_tmfd;
	List <MessageStruct> msg_queue;
	std::multimap <__syscall_slong_t,List<MessageStruct>::node*> exp_stamps;
	bool reqMsgAdd(List<MessageStruct>::node* node,int client_fd,__syscall_slong_t timeout=msg_timeout)
	{
		node->data.msg_id=++msg_id;
		node->data.msg.append(",\"msg_id\":").append(std::to_string(msg_id)).append("}").append(MSG_END);

		res_umap_lock.lock();
		std::pair <std::unordered_map <uint64_t,MsgResult>::iterator,bool> pr;
		pr=res_umap.emplace(std::piecewise_construct,
							std::forward_as_tuple(msg_id),
							std::forward_as_tuple(client_fd));
		res_umap_lock.unlock();
		if(pr.second==true)
		{
			exp_mmap_lock.lock();
			node->data.exp_stamp_it=exp_stamps.emplace(std::piecewise_construct,
													   std::forward_as_tuple(duration_cast< milliseconds >(system_clock::now().time_since_epoch()).count()+timeout),
													   std::forward_as_tuple(node));
			if(node->data.exp_stamp_it==exp_stamps.begin())// set exp timer if timeouts set is empty
			{
				ntb::timerSet(exp_check_tmfd);
			}
			exp_mmap_lock.unlock();

			msg_queue.nodeLinkBack(node);
			eventfd_write(req_msg_evfd,0);
			return true;
		}

		msg_queue.nodeUnlink(node);
		return false;
	}
	void reqMsgDel(List<MessageStruct>::node* node)
	{
		exp_mmap_lock.lock();
		if(node->data.exp_stamp_it==exp_stamps.begin())
		{
			if(exp_stamps.size()>1)
			{
				timerSet(exp_check_tmfd,std::next(exp_stamps.begin())->first);
			}
		}
		exp_stamps.erase(node->data.exp_stamp_it);
		exp_mmap_lock.unlock();
		msg_queue.nodeUnlink(node);
	}

	static uint64_t nodeInit();
	bool start();

	SSClient(const SSType service_type,const std::string_view target_path,const int max_events,const uint max_queue):
		service_type(service_type),remote_path(target_path), max_events(max_events),msg_queue(max_queue)
	{
		in.send.max_attempts=5;
		out.send.max_attempts=5;
		in.recv.max_attempts=5;
		out.recv.max_attempts=5;
		msg_timeout=300;
		res_umap.reserve(max_queue);
	}
	~SSClient()
	{
		close(ss_epoll);
		close(req_msg_evfd);
		delete []events;
	}
};
}//end of namespace



#endif // NTB_SS_CLIENT_H
