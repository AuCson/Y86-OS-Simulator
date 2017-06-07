#ifndef VM_H
#define VM_H
#include "mem.h"
#include "bit.h"
#include "elf.h"

#include <map>
#include <list>
#include <string>
typedef WORD VMADDR;
class CPU;
class Core;
class Kernel;
class OpenFileTable;
class VM{
public:
    VM(CPU* _cpu,int _pid,Mem *_mem){
        cpu = _cpu;
        pid = _pid;
        mem = _mem;
        status = R;
    }

    VM(Mem *_mem){
        mem = _mem;
        status = S;
    }


    static const int PAGE_SIZE = 4096;
    static const int INVALID = 0;
    static const int RW = 1;
    static const int RONLY = 2;
    static const int R = 0;
    static const int S = 1;
    static const int T = 2;
    int error = 0;
    //associated cpu
    CPU* cpu;
    int corenum;
    Mem* mem;
    //heap,stack,args,envp
    WORD heap_low,heap_high,mmap_low,mmap_high,stack_low,stack_high;
    WORD argv_low,argv_high,envp_low,envp_high;
    //kernal stack
    int pid;
    int status;
    //slow syscall waiting
    int stall_clk = 0;
    //fd ref
    OpenFileTable* fd_set[512] = {0};
    //waitpid / pause...
    int waiting_pid = -1;

    int saved = 0;
    int saved_reg[8];
    int saved_pc;
    int syscall_return;
    Core* saved_core = NULL;

    //for display
    std::string elf_text;

    struct VM_AREA_STRUCT{
        static const int SHARED = 0;
        static const int PRIVATE = 1;
        static const int RONLY = 1;
        static const int WONLY = 2;
        static const int RW = 3;
        static const int COPY_ON_WRITE = 4;
        VMADDR start;
        VMADDR end;
        int prot; //privilege(R,W,RW)
        int flags; //public or private(S,P)
        std::string section_name;
    };
    std::map<WORD,WORD> pgd;
    std::map<WORD,int> pgd_valid;
    std::list<VM_AREA_STRUCT> vm_area;

    int activate(CPU* _cpu,int _corenum){
        if(status != T){
            cpu = _cpu;
            corenum = _corenum;
            status = R;
            return 1;
        }
        else
            return 0;
    }
    void stop(){
        cpu = NULL;
        corenum = -1;
        status = S;
    }
    void terminate(){
        cpu = NULL;
        corenum = -1;
        status = T;
    }


    void allocate_page_pte(WORD vm_addr,int &error);

    void free_page_pte(WORD vm_addr,int &error);

    inline WORD phy_addr(WORD vm_addr);
    WORD read(WORD vm_addr, int &error, int &src, WORD& physical_addr);
    WORD allocate_section(std::string section_name,WORD start_vm_addr,BYTE* data,int data_byte,int prot,int flag,int &error,int init=true);

    WORD read(WORD vm_addr,int &error,int &src);
    void write(WORD vm_addr,int &error,int &src,BYTE byte);

};

#endif // VM_H
