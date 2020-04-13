#include "ntb_ss_client.h"
namespace ntb
{
static thread_local int res;
void SSClient::outEpollin()
{
	while((recv_result=recv(out.sock,
							&out.recv.buff[out.recv.len],
							SERVER_MSG_BUFF_SIZE-out.recv.len,
							MSG_NOSIGNAL))>0)
	{
		out.recv.len+=static_cast<size_t>(recv_result);
		if((out.recv.len>MSG_END_LEN)&&(strncmp(&out.recv.buff[out.recv.len]-MSG_END_LEN,MSG_END,MSG_END_LEN)))
		{
			out.recv.buff[out.recv.len]='\0';
			{
				printf("out.recv msg: ");
				printAll(out.recv.buff,out.recv.len);
				if (!jdom.ParseInsitu(out.recv.buff).HasParseError())
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
								case client_chan::chan_reg:
									if(ResultCode::res_ok==res)
									{
										out.msg_status=out.ready;
										out.chan_ready=true;
									}
									else if(ResultCode::res_not_ready==res)
									{
										out.chan_ready=true;
									}
									else outReconnect();
									break;
								default:
									out.msg_status=out.ready;
									switch(res)
									{
									case ResultCode::res_err:
										resMsgAdd(msg_queue.beg->data.msg_id,res);
										[[fallthrough]]; case ResultCode::res_ok:
										reqMsgDel(msg_queue.beg);
										break;
									case ResultCode::res_busy:
										out.send.attempts++;
										break;

									}
								}
								if(!msg_queue.isEmpty())
								{
									if(in.chan_ready&&out.msg_status==out.ready)
									{
										eventfd_write(req_msg_evfd,0);
									}
								}
								return;
							}
						}
					}
				}
				outReconnect();
			}
		}
	}
}
}//namespace end
