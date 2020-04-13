#ifndef NTB_MEMPOOL_H
#define NTB_MEMPOOL_H
#include <sys/types.h>
#include <array>
#include <stdexcept>
#include <iostream>
#include "ska_hash/bytell_hash_map.hpp"
#include "ntb_spinlock.h"

namespace ntb
{

enum SubBlkSize{sub512,sub1024,sub2048,sub4096};
class MempoolFixedPO2
{
    int8_t block_size_shift;
    int32_t free_block;
    int32_t *free_blocks=nullptr;
public:
    unsigned char *pool_begin=nullptr;
    MempoolFixedPO2()
    {}
    void resize(int32_t max_blocks,int32_t block_size)
    {
        if(max_blocks<=0)throw std::invalid_argument("max_blocks <=0");
        if(block_size<=0)throw std::invalid_argument("block_size <=0");
        if(1!=__builtin_popcountl(static_cast<u_long>(block_size)))throw std::invalid_argument("block_size is not power of 2");
        cleanup();
        block_size_shift=static_cast<int8_t>(log2(block_size));
        pool_begin=new unsigned char[static_cast<uint64_t>(max_blocks*block_size)];
        free_blocks=new int32_t[static_cast<uint64_t>(max_blocks)];
        free_block=max_blocks;
        for(int32_t i=0;i<max_blocks;i++)
        {
            free_blocks[i]=i;
        }
    }
    unsigned char* alloc()noexcept
    {
        if(free_block!=0)
        {
            return pool_begin+(free_blocks[--free_block]<<block_size_shift);
        }
        else return nullptr;
    }
    void dealloc(unsigned char *p)noexcept
    {
        free_blocks[free_block++]=static_cast<int32_t>((p-pool_begin)>>block_size_shift);
    }
    void cleanup()noexcept
    {
        if (nullptr!=pool_begin)delete[]pool_begin;
        pool_begin=nullptr;
        if (nullptr!=free_blocks)delete[]free_blocks;
        free_blocks=nullptr;
    }
    ~MempoolFixedPO2()noexcept
    {
        cleanup();
    }
};
class MempoolTSFixed//thread safe
{
    Spinlock sl;
    int32_t free_block=0;
    char  **free_blocks=nullptr;
    void cleanup()noexcept
    {
        if (nullptr!=free_blocks)delete[]free_blocks;
        free_blocks=nullptr;
    }
public:
    MempoolTSFixed()
    {}
    void resize(int32_t max_blocks,int32_t block_size)
    {
        sl.lock();
        if(max_blocks<=0)throw std::invalid_argument("max_blocks <=0");
        if(block_size<=0)throw std::invalid_argument("block_size <=0");
        cleanup();

        char * pool_begin=new  char[static_cast<uint64_t>(max_blocks*block_size)];
        free_blocks=new  char *[static_cast<uint64_t>(max_blocks)];
        free_block=max_blocks;
        for(int32_t i=0;i<max_blocks;i++)
        {
            free_blocks[i]=pool_begin+block_size*i;
        }
        sl.unlock();
    }
     char* alloc()noexcept
    {
        sl.lock();
        if(free_block!=0)
        {
            int32_t temp_pos=--free_block;
            sl.unlock();
            return free_blocks[temp_pos];
        }
        else
        {
            sl.unlock();
            return nullptr;
        }
    }
    void dealloc( char *p)noexcept
    {
        sl.lock();
        free_blocks[free_block++]=p;
        sl.unlock();
    }

    ~MempoolTSFixed()noexcept
    {
        cleanup();
        sl.unlock();//in case of exception in resize() //may be change to nothrow?
    }
};
class MempoolFixed
{
public:
    unsigned char  **free_blocks=nullptr;
    int32_t free_block;
    MempoolFixed()
    {}
    void resize(int32_t max_blocks,int32_t block_size)
    {
        if(max_blocks<=0)throw std::invalid_argument("max_blocks <=0");
        if(block_size<=0)throw std::invalid_argument("block_size <=0");
        cleanup();

        unsigned char * pool_begin=new unsigned char[static_cast<uint64_t>(max_blocks*block_size)];
        free_blocks=new unsigned char *[static_cast<uint64_t>(max_blocks)];
        free_block=max_blocks;
        for(int32_t i=0;i<max_blocks;i++)
        {
            free_blocks[i]=pool_begin+block_size*i;
        }
    }
    unsigned char* alloc()noexcept
    {
        if(free_block!=0)
        {
            return free_blocks[--free_block];
        }
        else return nullptr;
    }
    void dealloc(unsigned char *p)noexcept
    {
        free_blocks[free_block++]=p;
    }
    void cleanup()noexcept
    {
        if (nullptr!=free_blocks)delete[]free_blocks;
        free_blocks=nullptr;
    }
    ~MempoolFixed()noexcept
    {
        cleanup();
    }
};
class MempoolFixedPosArr
{
    int32_t block_size;
    int32_t free_block;
    int32_t *free_blocks=nullptr;

public:
    unsigned char *pool_begin=nullptr;
    MempoolFixedPosArr()
    {}
    void resize(int32_t max_blocks,int32_t _block_size)
    {
        if(max_blocks<=0)throw std::invalid_argument("max_blocks <=0");
        if(_block_size<=0)throw std::invalid_argument("block_size <=0");
        cleanup();
        block_size=_block_size;
        pool_begin=new unsigned char[static_cast<uint64_t>(max_blocks*_block_size)];
        free_blocks=new int32_t[static_cast<uint64_t>(max_blocks)];
        free_block=max_blocks;
        for(int32_t i=0;i<max_blocks;i++)
        {
            free_blocks[i]=i;
        }
    }
    unsigned char* alloc()noexcept
    {
        if(free_block!=0)
        {
            return pool_begin+(free_blocks[--free_block]*block_size);
        }
        else return nullptr;
    }
    void dealloc(unsigned char *p)noexcept
    {
        free_blocks[free_block++]=static_cast<int32_t>((p-pool_begin)/block_size);
    }
    void cleanup()noexcept
    {
        if (nullptr!=pool_begin)delete[]pool_begin;
        pool_begin=nullptr;
        if (nullptr!=free_blocks)delete[]free_blocks;
        free_blocks=nullptr;
    }
    ~MempoolFixedPosArr()noexcept
    {
        cleanup();
    }
};

template <typename MaxSubBlocks>
class Mempool
{
    int8_t max_block_shift;
    int8_t sub_blocks_num_minus1;
    MaxSubBlocks max_val;
    MaxSubBlocks max_val_minus1;
    int32_t max_blocks;
    int32_t max_block_size;
    int32_t max_block_size_minus1;
    int32_t free_block;
    int32_t *free_blocks=nullptr;

    std::pair<MaxSubBlocks,uint8_t> *sub_blocks=nullptr;// pair<free sub blocks are marked as set bit, SubBlkSize>
    std::array<int32_t,4> blk_sizes{512,1024,2048,4096};
    std::array<int8_t,4>max_sub_blocks;
    std::array<int8_t,5>blk_sz_shift{9,10,11,12,13};//powers of two for shift, 9 is 512
    std::array<ska::flat_hash_set<int32_t>,4> free_sub_arr;
public:
    unsigned char *pool_begin=nullptr;
    unsigned char* alloc(int8_t req_blk_size)noexcept
    {
        int32_t blk_n;
        if(free_sub_arr[req_blk_size].size()!=0)
        {//always not full and not empty
            blk_n=*free_sub_arr[req_blk_size].begin();
            int8_t sub= __builtin_ffsl (sub_blocks[blk_n].first)-1;
            sub_blocks[blk_n].first ^= (1<<sub);//set to busy(toggle from 1 to 0)
            if(max_sub_blocks[req_blk_size]==__builtin_popcountl(sub_blocks[blk_n].first))//if full   //if  last sub block left
            {
                free_sub_arr[req_blk_size].erase(blk_n);
            }
            return pool_begin+(blk_n<<max_block_shift)+(sub<<blk_sz_shift[req_blk_size]);
        }
        else if(free_block!=0)
        {
            blk_n=free_blocks[--free_block];
            sub_blocks[blk_n].first=max_val_minus1;
            sub_blocks[blk_n].second=req_blk_size;
            if(blk_sizes[req_blk_size]!=max_block_size)
                free_sub_arr[req_blk_size].emplace(blk_n);
            return pool_begin+(blk_n<<max_block_shift);
        }
        return nullptr;
    }
    void dealloc(const unsigned char *p)noexcept
    {
        ptrdiff_t diff=p-pool_begin;
        int32_t blk_n=(diff)>>max_block_shift;
        auto &sub=sub_blocks[blk_n];
        //        sub.first |= (1<<((diff&max_block_size_minus1)>>blk_sz_shift[sub.second]));//set bit to 1(means empty)
        //         if(max_val==sub.first)//empty
        //         {
        //             free_blocks[free_block++]=blk_n;
        //             free_sub_arr[sub.second].erase(blk_n);
        //         }
        //         else
        //         {
        //             if(max_sub_blocks[sub.second]==__builtin_popcountl(sub.first))
        //             {
        //                 free_sub_arr[sub.second].emplace(blk_n);//bcoz it wasn't there as was full
        //             }
        //         }
        if(max_sub_blocks[sub.second]!=__builtin_popcountl(sub.first))//if not full
        {
            sub.first |= (1<<((diff&max_block_size_minus1)>>blk_sz_shift[sub.second]));//set bit to 1(means empty)
            if(max_val==sub.first)//empty
            {
                free_blocks[free_block++]=blk_n;
                free_sub_arr[sub.second].erase(blk_n);
            }
        }
        else
        {
            sub.first |= (1<<((diff&max_block_size_minus1)>>blk_sz_shift[sub.second]));//set bit to 1(means empty)
            free_sub_arr[sub.second].emplace(blk_n);//bcoz it wasn't there as was full
        }
    }
    Mempool()
    {}
    void resize(int32_t _max_blocks,int32_t _max_block_size)
    {
        if(1!=__builtin_popcountl(static_cast<u_long>(_max_block_size)))throw std::invalid_argument("max_block_size is not power of 2");
        if(_max_block_size<=0)throw std::invalid_argument("max_block_size <=0");
        if(_max_blocks<=0)throw std::invalid_argument("max_blocks <=0");
        cleanup();
        max_blocks=_max_blocks;
        free_block=max_blocks;
        max_block_size=_max_block_size;
        max_block_size_minus1=max_block_size-1;
        {
            max_val= ~(max_val & 0);//set all bits to 1(means empty)
            max_val_minus1=max_val-1;
        }
        max_block_shift=log2(max_block_size);

        sub_blocks=new std::pair<MaxSubBlocks,uint8_t>[max_blocks];
        pool_begin= new unsigned char [static_cast<uint64_t>(max_blocks*max_block_size)];
        free_blocks=new int32_t[max_blocks];
        for(int32_t i=0;i<max_blocks;i++)
        {
            free_blocks[i]=i;
        }

        sub_blocks_num_minus1=(sizeof(MaxSubBlocks)*8);
        for (int32_t i=0;i<max_sub_blocks.size();i++)
        {
            max_sub_blocks[i]=sub_blocks_num_minus1-(max_block_size/blk_sizes[i]);
        }
        sub_blocks_num_minus1--;
    }
    void cleanup()
    {
        if (nullptr!=pool_begin)delete[]pool_begin;
        pool_begin=nullptr;
        if (nullptr!=sub_blocks)delete[]sub_blocks;
        sub_blocks=nullptr;
        if (nullptr!=free_blocks)delete[]free_blocks;
        free_blocks=nullptr;
    }
    ~Mempool()
    {
        cleanup();
    }
};
}
#endif // NTB_MEMPOOL_H
