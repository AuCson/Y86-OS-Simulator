#ifndef VM_H
#define VM_H
#include "mem.h"
#include "bit.h"
#include <map>
#include <vector>
typedef VMADDR WORD;
class VM{
    struct VM_AREA_STRUCT{
        VMADDR start;
        VMADDR end;
        int prot; //privilege
        int flags; //public or private
        int type;
    };
    std::map<WORD,WORD> pgd;
    std::vector<VM_AREA_STRUCT> vm_area;


};

#endif // VM_H
