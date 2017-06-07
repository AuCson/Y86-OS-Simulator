#ifndef CPU_H
#define CPU_H
#include "core.h"
#include "bit.h"
#include "mem.h"
#include "cache.h"
#include "vm.h"
#include <map>
#include <vector>
#include <cstdio>
#define MAX_CORE 4

class CPU{
public:
    static const int L1CACHE = 0;
    static const int MEM = 1;
    Core *core[MAX_CORE];
    int cpu_id = 0;
    CPU(std::vector<CPU*>* cpu_set,std::vector<L1Cache*>* all_l1_cache,Mem* _mem)
    {
        cpu_id = cpu_set->size();
        core[0] = new Core(this);
        corenum = 1;
        l1cache = new L1Cache(all_l1_cache,_mem);
        cpu_set->push_back(this);
        mem = _mem;
    }

    void add_core(){
        if(corenum >= 4)
            return;
        core[corenum++] = new Core(this);
    }

    int corenum = 1;
    L1Cache *l1cache;
    Mem* mem;

    WORD read(WORD physical_addr,int &error,int &src)
    {
        if(error)
            return 0;
        WORD value = mem->read(physical_addr,error);
        src = MEM;
        return value;
        /*
        int terror = 0;
        WORD value = l1cache->read(physical_addr,terror);
        src = L1CACHE;
        if(terror == 1){ //no hit
            l1cache->burst_load(physical_addr,terror);
            //printf("1");
            value = mem->read(physical_addr,terror);
            src = MEM;
        }
        return value;
        */

    }

    void write(WORD physical_addr,int &error,BYTE byte){
        //l1cache->lazy_write(physical_addr,byte,error);
        mem->write(physical_addr,byte,error);
    }


};

#endif // CPU_H
