#ifndef CPU_H
#define CPU_H
#include "core.h"
#include "bit.h"
#include "mem.h"
#include "cache.h"
#include <map>
#include <vector>
#define MAX_CORE 4

class CPU{
public:
    CPU(std::vector<CPU*>* cpu_set,std::vector<L1Cache*>* all_l1_cache,Mem* mem)
    {
        core[0] = new Core(this);
        corenum = 1;
        cache = new L1Cache(all_l1_cache,mem);
        cpu_set->push_back(this);
    }

    Core *core[MAX_CORE];
    int corenum;
    L1Cache *cache;


};

#endif // CPU_H
