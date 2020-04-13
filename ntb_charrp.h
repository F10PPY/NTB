#ifndef NTB_STRING_H
#define NTB_STRING_H
#include <cstring>
#include <algorithm>
namespace ntb
{
struct  CharrP
{
    size_t      len;
    char     *data=nullptr;
    CharrP()
    {}
    CharrP(const char* src)
    {
        len=strlen(src);
        data=new char(len);
        std::copy(src,src+len,data);
    }
    ~CharrP()
    {
        if(data)delete data;
    }
};
}//namespace end
#endif // NTB_STRING_H
