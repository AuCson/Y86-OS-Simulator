#ifndef KERNEL_H
#define KERNEL_H
#include "cpu.h"
#include "elf.h"
#include "ysyscall.h"
#include <vector>
#include <list>
#include <map>
#include <set>

class Kernel{
public:
    Kernel(){
        init();
    }

    std::vector<CPU*> cpu_set;
    std::vector<L1Cache*> all_l1_cache;
    std::map<int,VM*> pid_set;
    int clk_cycle;

    void init();
    void test();
};

#endif // KERNEL_H
