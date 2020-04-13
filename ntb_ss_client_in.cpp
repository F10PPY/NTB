#include "ntb_ss_client.h"
namespace ntb
{
static thread_local int res;
void SSClient::inEpollin()
{
	while((recv_result=recv(in.sock,
							&in.recv.buff[in.recv.len],
							SERVER_MSG_BUFF_SIZE-in.recv.len,
							MSG_NOSIGNAL))>0)
	{
		in.recv.len+=static_cast<size_t>(recv_result);
		if((in.recv.len>MSG_END_LEN)&&(strncmp(&in.recv.buff[in.recv.len]-MSG_END_LEN,MSG_END,MSG_END_LEN)))
		{
			in.recv.buff[in.recv.len]='\0';
			{
				printf("in.recv msg: ");
				printAll(in.recv.buff,in.recv.len);
				if (!jdom.ParseInsitu(in.recv.buff).HasParseError())
				{
					jit = jdom.FindMember("method");
					if(jit != jdom.MemberEnd())
					{
						if(jit->value.IsInt())
						{
							method=jit->value.GetInt();
							jit = jdom.FindMember("res");
							if(jit != jdom.MemberEnd()&&jit->value.IsInt())
							{
								res=jit->value.GetInt();

								switch (method)
								{
								case server_chan::chan_reg:
									if(ResultCode::res_ok==res)
									{
										in.chan_ready=true;
										if(!msg_queue.isEmpty())
										{
											if(out.chan_ready&&out.msg_status==out.ready)
											{
												eventfd_write(req_msg_evfd,0);
											}
										}
									}
									else inReconnect();
									break;
								default: //case server_chan::res:
									send_result=ssSendAll(in.sock,resp_ok.data(),resp_ok.length());
									if(SEND_OK!=send_result)
									{
										inReconnect();
									}
									//serverMsgHandler_(res,method,jdom,this);
									jit = jdom.FindMember("msg_id");
									if((jit != jdom.MemberEnd())&&(jit->value.IsUint64()))
									{
										resMsgAdd(jit->value.GetUint64(),res);
									}

								}
								return;
							}
						}
					}
				}
				goto in_400;
			}
		}
	}
	if(-1==recv_result)
	{
		if((errno==EAGAIN)||(errno==EWOULDBLOCK))
		{
			return;
		}
	}
	//recv==0(conn closed or buffer is full due too big msg) or rest of -1
in_400:
	send_result=ssSendAll(in.sock,resp_bad_request.data(),resp_bad_request.length());
	if(SEND_OK!=send_result)
	{
		inReconnect();
	}
}

}//namespace end
