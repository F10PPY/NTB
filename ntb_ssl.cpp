#include "ntb_ssl.h"
namespace ntb
{
static char ssl_err_buff[80];
static int ret;
int sendall_ssl(WOLFSSL* ssl, const char* buf,const int len)
{
	int err=0;
	int total=0;
	while(total<len)
	{
		ret=wolfSSL_send(ssl,buf+total,len-total,MSG_NOSIGNAL);
		if(ret>0)
		{
			total+=ret;
			continue;
		}
		err=wolfSSL_get_error(ssl, ret);

		if(errno==EAGAIN||errno==EWOULDBLOCK)
		{
			return total;
		}
		if((err!= SSL_ERROR_WANT_WRITE)&&(err!=SSL_ERROR_WANT_READ))
		{
#if defined NTB_DEBUG_OUTPUT
#if defined BAICAL_DEBUG
				pTrace->P7_ERROR(nullptr,TM("wolf_send err = %i, %s\n"),err,wolfSSL_ERR_error_string(err, ssl_err_buff));
#endif
#endif
            return ntb::NTB_SSL_SEND_ERR;
		}
	}
	return total;
}

bool ssl_init(WOLFSSL_CTX **ctx)
{
#define DEBUG_WOLFSSL
	wolfSSL_Debugging_ON();
	WOLFSSL_METHOD* method;

	wolfSSL_Init();
	method = wolfSSLv23_server_method();

	if ( (*ctx = wolfSSL_CTX_new(method)) == nullptr)
	{
		printf("wolfSSL_CTX_new error\n");
		return false;
	}
//	char buff[8000];
//	wolfSSL_get_ciphers(buff,8000);
//	printf("%s\n",buff);

    if(wolfSSL_CTX_SetTmpDH_file(*ctx,"../shared/certs/dh2048.der",SSL_FILETYPE_DEFAULT)!= SSL_SUCCESS)
	{
		printf("Error loading dh params\n");
		return false;
	}
	if(SSL_SUCCESS!=wolfSSL_CTX_set_cipher_list(*ctx,"ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:"
												"ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES128-SHA256:"
												"ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA:"
												"ECDHE-RSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-RSA-AES256-SHA256:DHE-RSA-AES256-SHA:ECDHE-ECDSA-DES-CBC3-SHA:ECDHE-RSA-DES-CBC3-SHA:"
												"EDH-RSA-DES-CBC3-SHA:AES128-GCM-SHA256:AES256-GCM-SHA384:AES128-SHA256:AES256-SHA256:AES128-SHA:AES256-SHA:DES-CBC3-SHA:!DSS"))
	{
		printf("Error setting cipher list\n");
		return false;
	}
    if (wolfSSL_CTX_use_certificate_file(*ctx, "../shared/certs/server-cert.pem",
										 SSL_FILETYPE_PEM) != SSL_SUCCESS)
	{
		printf("Error loading certs/server-cert.pem\n");
		return false;
	}
    if (wolfSSL_CTX_use_PrivateKey_file(*ctx, "../shared/certs/server-key.pem",
										SSL_FILETYPE_PEM) != SSL_SUCCESS)
	{
		printf("Error loading certs/server-key.pem\n");
		return false;
	}

	//wolfSSL_CTX_set_verify(*ctx, SSL_VERIFY_PEER, nullptr);
    if (wolfSSL_CTX_load_verify_locations(*ctx,"../shared/certs/client-cert.pem",nullptr) != SSL_SUCCESS)
	{
		printf("Error loading verify\n");
		return false;
	}


	return true;
}
}//namespace end
