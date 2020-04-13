#ifndef NTB_ERRORS_H
#define NTB_ERRORS_H
#include <errno.h>
#include <stdio.h>
#include <array>
#include <string.h>
#include "ntb_charrp.h"
namespace ntb
{
char *errString(const int err);
bool errStringInit();
class Exception : public std::exception
{
	std::string message_;
public:
	Exception(std::string_view message):message_(message){}
	virtual const char * what () const noexcept override;

};
class ntbException:public Exception
{
public:
	ntbException(std::string_view message):Exception(message){}
};
}//namespace end
#endif // NTB_ERRORS_H
