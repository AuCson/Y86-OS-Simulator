#ifndef KERNEL_H
#define KERNEL_H
#include "cpu.h"
#include "elf.h"
#include "ysyscall.h"
#include <vector>
#include <list>
#include <map>
#include <set>
#include <QString>

class Kernel{
public:
    Kernel(){
        init();
    }

    std::vector<CPU*> cpu_set;
    std::vector<L1Cache*> all_l1_cache;
    std::map<int,VM*> pid_set;
    std::map<int,int> pid_run_time;
    std::map<int,int> pid_stop_time;

    QString log;

    int clk_cycle;

    void cycle();
    void context_switch(int &error,VM* old_vm,VM* new_vm,CPU* cpu,int running_core);
    void init();
    void arrange();
    void terminate(VM *vm);
    int ystartup();
    int yfork(int &error,int parent_pid,std::map<int,VM*>* pid_map,VM *old_vm);
    void yexecve(int &error,ELF* elf,VM* vm);

    void add_log(QString text,int type);

    void test();

    struct Style{
        static const int NORMAL = 0;
        static const int CRITICAL = 1;
        static const int IMPORTANT = 2;
        static const int MINOR = 3;
    };
    struct Syscall{
        static const int EXIT = 1;
        static const int FORK = 2;
        static const int EXECVE = 11;
    };
};

#endif // KERNEL_H
