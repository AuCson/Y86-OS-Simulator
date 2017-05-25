#ifndef MEM_H
#define MEM_H
#include "bit.h"
#include <map>
class Mem{
public:
    const int penalty = 3;
    std::map<WORD,WORD> mem;
    WORD read(WORD physical_addr,int &error)
    {
        if(!mem.count(physical_addr)){
            mem[physical_addr] = 0;
        }
        return mem[physical_addr];
    }
    void write(WORD physical_addr,WORD value,int &error)
    {
        if(int(physical_addr) < 0 || physical_addr % 4 != 0)
        {
            error = 1;
            return 0;
        }
        mem[physical_addr] = value;
    }
};

#endif // MEM_H
