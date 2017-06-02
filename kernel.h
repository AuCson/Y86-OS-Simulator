#ifndef KERNEL_H
#define KERNEL_H
#include "cpu.h"
#include "elf.h"
#include "ysyscall.h"
#include "csapp.h"
#include <vector>
#include <list>
#include <map>
#include <set>
#include <QString>
struct Vnode{
    Vnode(){
        file_max_len = 10000;
        filesize = 0;
        filename = new char[10000];
        filetype = 0;
        init_content();
    }
    void init_content(){
        file_content = new BYTE[10000];
        if(file_content == NULL){
            file_content = NULL;
        }
    }

    static const int ASCII = 0;
    static const int BIN = 2;
    int filesize;
    int filetype;
    int file_max_len;
    char* filename;
    BYTE* file_content;
};

struct OpenFileTable{
    OpenFileTable(){
        pos = 0;
        refcnt = 0;
    }

    int pos;
    int refcnt;
    Vnode* vnode;
};

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
    Mem* mem;

    QString log;

    int clk_cycle;
    std::vector<Vnode*> vnode_list;
    std::vector<OpenFileTable*> open_file_table;

    void cycle();
    void context_switch(int &error,VM* old_vm,VM* new_vm,CPU* cpu,int running_core);
    void init();
    void arrange();
    void terminate(VM *vm);
    void suspend(VM *vm);
    int ystartup();

    int ysyscall_handler(Core* core);
    int yfork(int &error, int parent_pid, std::map<int,VM*>* pid_map, VM *old_vm, VM*& new_vm);
    void yexecve(int &error,ELF* elf,VM* vm);
    int yopen(int &error,char* filename, VM* vm);
    int yread(int &error,int fd,int charnum,WORD store_vm_addr,VM* vm);
    int ywrite(int &error,int fd,int charnum,WORD store_vm_addr,VM* vm);
    int yexecve_wrapper(BYTE **argv, int argc, VM* vm, int &error);

    void add_cpu();
    void add_log(QString text,int type);

    void test();

    struct Style{
        static const int NORMAL = 0;
        static const int CRITICAL = 1;
        static const int IMPORTANT = 2;
        static const int MINOR = 3;
        static const int FILEOUT = 4;
    };
    struct Syscall{
        static const int EXIT = 1;
        static const int FORK = 2;
        static const int READ = 3;
        static const int WRITE = 4;
        static const int OPEN = 5;
        static const int EXECVE = 11;
    };
};



#endif // KERNEL_H
