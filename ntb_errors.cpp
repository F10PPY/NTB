#include "ntb_errors.h"

namespace ntb
{
static int nerr=135;
static CharrP  *errlist;
static CharrP unknown_errno=CharrP("Unknown errno");


char* errString(const int err)//, u_char *errstr, size_t size)
{
	//ntb_string_str  *msg;
	//msg = ((uint32_t) err < sys_nerr) ? &ntb_sys_errlist[err]: &ntb_unknown_errno;
	//size = std::min(size, msg->len);
	//return( u_char*) msg->data;// memcpy(errstr, msg->data, size);
	//return (err < sys_nerr) ? &ntb_sys_errlist[err] : ntb_unknown_errno;
   // return (err<sys_nerr-1)? sys_errlist[err]:unknown_errno;
     return (err<nerr-1)? errlist[err].data:unknown_errno.data;

}

bool errStringInit()
{
	char       *msg;
    char     *p;
	size_t      len;
	try
	{
        len =nerr * sizeof(CharrP);

        errlist = new CharrP[len];

        for (int err = 0; err < nerr; err++)
		{
			msg = strerror(err);
			len = strlen(msg);
            p = new char[len];

			memcpy(p, msg, len);
            errlist[err].len = len;
            errlist[err].data = p;
		}
        return true;
	}
	catch(...)
	{
        printf("eeStringInit error: %s/n",strerror(errno));
        return false;
	}
}
const char * Exception::what () const noexcept
{
	return message_.c_str();
}

}//namespace end
