#include "kernel.h"
#include "stdio.h"
void Kernel::init(){
    Mem* mem = new Mem();
    CPU* cpu0 = new CPU(&cpu_set,&all_l1_cache,mem);
    VM* vm = new VM(cpu0,1,mem);
    pid_set[1] = vm;
    clk_cycle = 0;
    int error;
    ELF* startup_script = new ELF();
    startup_script->load_from_strings("7000000000","FFFFFFFF","FFFFFFFF");
    yexecve(error,startup_script,vm);
}
