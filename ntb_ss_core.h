#ifndef NTB_SS_CORE_H
#define NTB_SS_CORE_H
#include "ntb_common.h"

#define MSG_END "\r\r"
constexpr const size_t MSG_END_LEN= std::char_traits<char>::length(MSG_END);
#define CLIENT_MSG_BUFF_SIZE 512
#define SERVER_MSG_BUFF_SIZE 2048
#define RES_BUFF_SIZE 512
//Json Keys
//#define JK_SVC_TYPE "svc_type"
//#define JK_CHANNEL "channel_type"
//
namespace ntb
{

//user defined
using ClientFD=int;
struct Test
{
	int i;
};


using result_handler_variants=std::variant<ClientFD,Test>;
//enum ResVars{COMMON,TEST};


namespace server_chan
{
//enum UserMethod{resp_authorize,resp_blabla};
enum UtilMethod{chan_reg=10000,common};
}
namespace client_chan
{
struct SSAuthMethod
{
	enum{authorize=0,new_user};
};
enum UtilMethod{chan_reg=10000,common};
}

enum SSType{SS_GTW,SS_AUTH,SS_CORE};
enum MsgChannel{client_channel,server_channel};
enum ResultCode{res_none=0,
				res_not_ready=100,
				res_ok=200,res_ok_body=201,
				res_bad_request=400,res_expired=401,
				res_busy=500,res_too_many_attempts=501,res_err=502,res_unavailable=503};
constexpr std::string_view resp_ok="{\"res\":200}" MSG_END;
constexpr std::string_view resp_bad_request="{\"res\":400}" MSG_END;
}//namespace end



#endif  // NTB_SS_CORE_H
