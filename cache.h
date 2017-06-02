#ifndef CACHE_H
#define CACHE_H
#include "core.h"
#include "bit.h"
#include "mem.h"
#include <map>
#include <vector>
#define MAX_CORE 4

class L1Cache{
public:
    L1Cache(std::vector<L1Cache*>* _all_l1_cache,Mem *_mem){
        all_l1_cache = _all_l1_cache;
        all_l1_cache->push_back(this);
        mem = _mem;
    }

    std::vector<L1Cache*>* all_l1_cache;
    Mem *mem;
    int error = 0;

    static const int s_value = 8; //256 set - cache with 1 line (2 int)
    static const int b_value = 6; //64 byte per block
    static const int I = 0,M = 1,E = 2,S = 3;
    int valid[1<<s_value]={0};
    BYTE block[1<<s_value][1<<b_value];
    WORD tag[1<<s_value];

    BYTE read(WORD physical_addr,int &error){
        WORD idx = get_bit(physical_addr,b_value,s_value+b_value-1);
        if(valid[idx]==I)
        {
            error = 1;
            return 0;
        }
        WORD tag_t = get_bit(physical_addr,s_value+b_value,WORD_SIZE-1);
        if(tag_t!=tag[idx]){
            error = 1;
            return 0;
        }
        WORD offset = get_bit(physical_addr,0,b_value-1);
        error = 0;
        return block[idx][offset];
    }
    void immediate_write(WORD physical_addr,int &error)
    {
        WORD idx = get_bit(physical_addr,b_value,s_value+b_value-1);
        WORD tag_t = get_bit(physical_addr,s_value+b_value,WORD_SIZE-1);
        WORD low = physical_addr >> b_value << b_value;
        WORD high = low + (1<<b_value);
        for(WORD i = low;i<high;++i)
        {
            WORD offset = get_bit(i,0,b_value-1);
            mem->write(i,block[idx][offset],error);
        }
        valid[idx] = S;
    }

    void burst_load(WORD physical_addr,int &error)
    {
        WORD idx = get_bit(physical_addr,b_value,s_value+b_value-1);
        WORD tag_t = get_bit(physical_addr,s_value+b_value,WORD_SIZE-1);
        WORD low = physical_addr >> b_value << b_value;
        WORD high = low + (1<<b_value);
        //if in some caches the status is M, obligate to write back to memory
        for(auto i = all_l1_cache->begin();i!=all_l1_cache->end();++i)
        {
            if(*i!=this){
                WORD tag2 = (*i)->tag[idx];
                if(tag2==tag_t){
                    if((*i)->valid[idx] == M)
                        (*i)->immediate_write(physical_addr,error);
                }
            }
        }
        for(int i = low;i<high;++i)//i: physical addr
        {
            WORD offset = get_bit(i,0,b_value-1);
            block[idx][offset] = mem->read(i,error);
        }
        valid[idx] = S;
    }

    void lazy_write(WORD physical_addr,BYTE byte,int &error)
    {
        WORD idx = get_bit(physical_addr,b_value,s_value+b_value-1);
        WORD offset = get_bit(physical_addr,0,b_value-1);
        WORD tag_t = get_bit(physical_addr,s_value+b_value,WORD_SIZE-1);
        if(idx == 0 && offset == 0)
            int a = 2;
        if(valid[idx]!=E)
            valid[idx] = E;
        for(auto i = all_l1_cache->begin();i!=all_l1_cache->end();++i)
        {
            if(*i!=this)
            {
                WORD tag2 = (*i)->tag[idx];
                if(tag2 == tag_t){
                    (*i)->valid[idx] = I;
                }
            }
        }
        if(tag[idx]!=tag_t){
            immediate_write((tag[idx]<<s_value+b_value) | (idx<<b_value) | offset,error);
        }
        block[idx][offset] = byte;
        tag[idx] = tag_t;
        valid[idx] = M;
    }
};

#endif // CACHE_H
