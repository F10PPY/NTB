#ifndef NTB_UTIL_H
#define NTB_UTIL_H
#include <string_view>
#include <chrono>
#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <utility>
#include "ntb_colors.h"
using namespace std::chrono;
#ifdef __GNUC__
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#else
#define likely(x)       (x)
#define unlikely(x)     (x)
#endif
namespace ntb
{
// CircularCounter version is 40% faster (not including updateStats()), additional performance cost of updateStats() depends on the size of the collection
// CircularCounter consumes 8 bytes per stored sample(int64_t)
struct Stats
{
    intmax_t average=0,samples_count=0;
    intmax_t min=INTMAX_MAX,max=0;
    std::string name;
    Stats(std::string_view _name)noexcept
    {
        name=_name;
    }
    void reset()noexcept
    {
        average=0;samples_count=0;
        min=INTMAX_MAX;max=0;
    }
    //        Stats& operator+=(const Stats& rhs)noexcept
    //        {
    //            min+= rhs.min;
    //            max+= rhs.max;
    //            samples_count+= rhs.samples_count;
    //            average+= rhs.average;
    //            return *this;
    //        }
};

struct StatsCircular:Stats
{
    bool full_flag=false;
    size_t max_samples=0;
    size_t current_pos=0;
    StatsCircular(size_t _max_samples,std::string_view _name)noexcept:Stats(_name)
    {
       max_samples=_max_samples;
    }
};

class CounterBase
{
protected:
    using float_seconds=std::chrono::duration<float>;
    size_t total_collections=0;
    std::chrono::high_resolution_clock::time_point stop;
    std::chrono::high_resolution_clock::time_point start;
public:
    CounterBase()noexcept
    {
        start=high_resolution_clock::now();
    }
    inline void reset()noexcept
    {
        start=high_resolution_clock::now();
    }
    inline std::string elapsedString(std::string_view str="")
    {
        stop=high_resolution_clock::now();
        std::string result("Timestamp \"");
        return result+str.data()+"\": "+(std::to_string(duration_cast<float_seconds> (stop-start).count()))
                .append(" seconds  = " )
                .append(std::to_string(duration_cast<nanoseconds> (stop-start).count())).append(" nanos");
    }
    inline void elapsedPrint(std::string_view str="")
    {
        std::cout<<elapsedString(str)<<std::endl;
    }

};
template<typename Duration>
class Counter: public  CounterBase
{
    std::vector<Stats>collections_vec;
public:
    inline void resetStats(const size_t n)noexcept
    {
        if(n<=total_collections)
        {
            collections_vec[n].reset();
        }
    }
    inline void resetAllStats()noexcept
    {
        for(auto &&it:collections_vec)
        {
            it.reset();
        }
    }
    inline void printStats(const size_t n)
    {
        if(n<=total_collections)
        {
            std::cout<<collections_vec[n].name<<"->min:"<<COLGREEN<<std::to_string(collections_vec[n].min)<<COLRESET
                    <<", max:"<<COLRED<<std::to_string(collections_vec[n].max)<<COLRESET
                   <<", ave:"<<COLYELLOW<<std::to_string(collections_vec[n].average)<<COLRESET
                  <<", samples:"<<COLCYAN<<std::to_string(collections_vec[n].samples_count)<<COLRESET<<std::endl;
        }
    }
    inline size_t addCollection(std::string_view name="")
    {
        collections_vec.push_back(Stats(name));
        return total_collections++;
    }
    inline void addSample(const size_t n)
    {
        if(n<=total_collections)
        {
            int64_t t=duration_cast<Duration> (high_resolution_clock::now()-start).count();
            if(unlikely(collections_vec[n].min>t))collections_vec[n].min=t;
            if(unlikely(collections_vec[n].max<t))collections_vec[n].max=t;
            collections_vec[n].average+=(t-collections_vec[n].average)/++collections_vec[n].samples_count;
        }
    }
};

template<typename Duration>
class CounterCircular: public  CounterBase
{
    std::vector<std::pair<std::vector<int64_t>,StatsCircular>>collections_vec;
public:
    inline size_t addCollection(size_t max_samples,std::string_view name="")
    {
        collections_vec.push_back({std::vector<int64_t>(),StatsCircular(max_samples,name)});
        collections_vec.back().first.resize(max_samples);
        return total_collections++;
    }
    inline void addSample(const size_t n)
    {
        if(n<=total_collections)
        {
            stop=high_resolution_clock::now();
            if(unlikely(collections_vec[n].second.current_pos==collections_vec[n].second.max_samples))
            {
                collections_vec[n].second.current_pos=0;
                collections_vec[n].second.full_flag=true;
            }
            collections_vec[n].first[collections_vec[n].second.current_pos++]=duration_cast<Duration> (stop-start).count();
        }
    }
    inline void resetStats(const size_t n)noexcept
    {
        if(n<=total_collections)
        {
            collections_vec[n].second.reset();
        }
    }
    inline void resetAllStats()noexcept
    {
        for(auto &&it:collections_vec)
        {
            it.second.reset();
        }
    }
    inline void printStats(const size_t n)
    {
        if(n<=total_collections)
        {
            std::cout<<collections_vec[n].second.name<<"->min:"<<COLGREEN<<std::to_string(collections_vec[n].second.min)<<COLRESET
                    <<", max:"<<COLRED<<std::to_string(collections_vec[n].second.max)<<COLRESET
                   <<", ave:"<<COLYELLOW<<std::to_string(collections_vec[n].second.average)<<COLRESET
                  <<", samples:"<<COLCYAN<<std::to_string(collections_vec[n].second.samples_count)<<COLRESET<<std::endl;
        }
    }
    inline void updateStats(const size_t n)noexcept
    {
        if(n<=total_collections)
        {
            if(collections_vec[n].second.full_flag==true)
            {
                for(auto it=collections_vec[n].first.begin();it!=collections_vec[n].first.end();it++)
                {
                    {
                        if(unlikely(collections_vec[n].second.min>*it))collections_vec[n].second.min=*it;
                        if(unlikely(collections_vec[n].second.max<*it))collections_vec[n].second.max=*it;
                        collections_vec[n].second.average+=(*it-collections_vec[n].second.average)/++collections_vec[n].second.samples_count;

                    }
                }
                collections_vec[n].second.full_flag=false;
            }
            else
            {
                for(size_t i=0;i<collections_vec[n].second.current_pos;i++)
                {
                    {
                        if(unlikely(collections_vec[n].second.min>collections_vec[n].first[i]))collections_vec[n].second.min=collections_vec[n].first[i];
                        if(unlikely(collections_vec[n].second.max<collections_vec[n].first[i]))collections_vec[n].second.max=collections_vec[n].first[i];
                        collections_vec[n].second.average+=(collections_vec[n].first[i]-collections_vec[n].second.average)/++collections_vec[n].second.samples_count;

                    }
                }
            }
            collections_vec[n].second.current_pos=0;
        }
    }
};

inline void printAll(const char * buff,const size_t bytes) noexcept
{
    for(unsigned int i=0;i<bytes;++i)
    {
        switch (buff[i])
        {
        case '\n':
            puts("\\n");
            break;
        case '\r':
            puts("\\r");
            break;
        case '\t':
            puts("\\t");
            break;
        default:
            //if ((buff[i] < 0x20) || (buff[i] > 0x7f)) {
            //	printf("\\%03o", (unsigned char)buff[i]);
            //	} else {
            //		printf("%c", buff[i]);
            //	}
            printf("%c",buff[i]);
            break;
        }
    }
    fflush(stdout);
}
}//namespace end

#endif // NTB_UTIL_H
