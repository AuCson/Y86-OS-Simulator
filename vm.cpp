#include "vm.h"
#include "cpu.h"
#include "elf.h"
#include "ysyscall.h"
extern int yexecve(int &error, VM *vm, ELF *elf);
void VM::allocate_page_pte(WORD vm_addr,int &error){
    if(vm_addr % 4096){
        error = 1;
        return;
    }
    if(pgd_valid.count(vm_addr) && pgd_valid[vm_addr]){
        error = 2;
        return;
    }
    pgd[vm_addr] = mem->allocate_page(error);
    pgd_valid[vm_addr] = 1;
    error = 0;
}

void VM::free_page_pte(WORD vm_addr,int &error){
    if(!(vm_addr)%4096){
        error = 1;
        return;
    }
    mem->free_page(error,pgd[vm_addr]);
    pgd_valid[vm_addr] = 0;
}

inline WORD VM::phy_addr(WORD vm_addr){
     WORD vm_pid = vm_addr & 0xFFFFF000;
     WORD physical_addr= pgd[vm_pid] | (vm_addr & 0xFFF);
     return physical_addr;
}

WORD VM::allocate_section(std::string section_name,WORD start_vm_addr,BYTE* data,int data_byte,int prot,int flag,int &error){
    if(start_vm_addr % 4096 != 0){
        error = 1;
        return 0;
    }
    VM_AREA_STRUCT* area = new VM_AREA_STRUCT();
    area->start = start_vm_addr;
    WORD addr;
    for(addr = start_vm_addr;addr<start_vm_addr+data_byte;addr+=4096){
        allocate_page_pte(addr,error);
    }
    area->end = addr;
    area->section_name = section_name;
    area->prot = VM::VM_AREA_STRUCT::RW;
    area->flags = flag;
    vm_area.push_back(*area); //vm_area.prot
    for(int i =0;i<data_byte;++i){
        int src;
        write(start_vm_addr+i,error,src,data[i]);
    }
    vm_area.pop_back();
    area->prot = prot;
    vm_area.push_back(*area);
    return addr;//next available page
}

WORD VM::read(WORD vm_addr,int &error,int &src){
    WORD vm_pid = vm_addr & 0xFFFFF000;
    if(!pgd_valid.count(vm_pid) || pgd_valid[vm_pid]==INVALID){
        //TODO: page fault handler, check the vm_area_struct
        error = 1;
        return 0;
    }
    //find the vm_area_struct
    for(auto i=vm_area.begin();i!=vm_area.end();++i){
        if(vm_addr>=i->start && vm_addr<=i->end){
            if(i->prot == VM_AREA_STRUCT::RONLY || i->prot == VM_AREA_STRUCT::RW){
                WORD physical_addr= pgd[vm_pid] | (vm_addr & 0xFFF);
                return cpu->read(physical_addr,error,src);
            }
            else{
                error = 3;
                return 0;
            }
        }
    }
    error = 2;
    return 0;

}

void VM::write(WORD vm_addr,int &error,int &src,BYTE byte){
    WORD vm_pid = vm_addr & 0xFFFFF000;
    //check page table and page table prots;
    if(!pgd_valid.count(vm_pid) || pgd_valid[vm_pid]==INVALID){
        //TODO: page fault handler, check the vm_area_struct
        error = 1;
    }
    for(auto i=vm_area.begin();i!=vm_area.end();++i){
        if(vm_addr>=i->start && vm_addr<=i->end){
            if(i->prot == VM_AREA_STRUCT::RONLY){
                error = 2;
            }
            if(i->prot == VM_AREA_STRUCT::COPY_ON_WRITE){
                //copy-on-write, if pte is tagged as 'RONLY', allocate a new page and copy;
                if(pgd_valid[vm_pid]==RONLY){
                    BYTE buf[4096];
                    for(WORD t_addr = vm_pid;t_addr<vm_pid+4096;++t_addr){
                        buf[t_addr-vm_pid] = cpu->read(phy_addr(t_addr),error,src);
                    }
                    WORD new_physcial_addr = mem->allocate_page(error);
                    pgd[vm_pid] = new_physcial_addr;
                    pgd_valid[vm_pid] = RW;
                    for(WORD t_addr = vm_pid;t_addr<vm_pid+4096;++t_addr){
                        cpu->write(phy_addr(t_addr),error,buf[t_addr-vm_pid]);
                    }
                }
            }
            //write the byte
            cpu->write(phy_addr(vm_addr),error,byte);
            error =0;
            return;
        }
    }
    error = 1;
    return;
}

void VM::init(){
    ELF* start_script = new ELF();
    start_script->load_from_strings(
                std::string("70000000"),//jmp 0x0
                std::string("80808080"),
                std::string("FFFFFFFF")
                );
    int error = 0;
    yexecve(error,start_script,this);
}
