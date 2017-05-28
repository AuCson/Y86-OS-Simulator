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
class VM{
public:
    VM(CPU* _cpu,int _pid,Mem *_mem){
        cpu = _cpu;
        pid = _pid;
        mem = _mem;
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
    Mem* mem;
    //heap,stack,args,envp
    WORD heap_low,heap_high,mmap_low,mmap_high,stack_low,stack_high;
    WORD argv_low,argv_high,envp_low,envp_high;
    //kernal stack
    int pid;
    int status;
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

    void init();

    void allocate_page_pte(WORD vm_addr,int &error);

    void free_page_pte(WORD vm_addr,int &error);

    inline WORD phy_addr(WORD vm_addr);

    WORD allocate_section(std::string section_name,WORD start_vm_addr,BYTE* data,int data_byte,int prot,int flag,int &error);

    WORD read(WORD vm_addr,int &error,int &src);
    void write(WORD vm_addr,int &error,int &src,BYTE byte);

};

#endif // VM_H
