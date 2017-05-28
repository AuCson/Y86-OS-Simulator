#ifndef MEM_H
#define MEM_H
#include "bit.h"
#include <map>
#include <set>
class Mem{
public:
    static const int penalty = 3;
    std::map<WORD,BYTE> mem;
    std::set<WORD> allocated_page;

    BYTE read(WORD physical_addr,int &error)
    {
        if(!mem.count(physical_addr)){
            mem[physical_addr] = 0;
        }
        return mem[physical_addr];
    }
    void write(WORD physical_addr,BYTE value,int &error)
    {
        if(int(physical_addr) < 0)
        {
            error = 1;
        }
        mem[physical_addr] = value;
    }
    WORD allocate_page(int &error){
        for(int i =0;i<(1<<30)-1;i+=4096){
            if(allocated_page.count(i) == 0){
                allocated_page.insert(i);
                error = 0;
                return i;
            }
        }
        error = 1;
        return 0;
    }
    void free_page(int &error,int physical_addr){
        if(physical_addr % 4096){
            error = 1;
            return;
        }
        auto itr = allocated_page.find(physical_addr);
        if(itr!=allocated_page.end()){
            allocated_page.erase(itr);
        }
        error =0;
    }
};

#endif // MEM_H
