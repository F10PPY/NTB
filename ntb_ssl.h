#ifndef NTB_SSL_H
#define NTB_SSL_H
#include "wolfssl/options.h"
#include "wolfssl/ssl.h"

#ifndef SSL_MODE_ENABLE_PARTIAL_WRITE
   #define SSL_MODE_ENABLE_PARTIAL_WRITE 2
#endif
namespace ntb
{
enum SendAllSSL{NTB_SSL_SEND_ERR=-1};
int sendall_ssl(WOLFSSL* ssl, const char* buf,const int len);
bool ssl_init(WOLFSSL_CTX **ctx);
}//namespace end
#endif // NTB_SSL_H
