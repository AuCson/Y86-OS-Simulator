#include "ysyscall.h"
#include "vm.h"
#include "cpu.h"
#include "elf.h"
#include "kernel.h"
#include <set>
#include <map>
#include <string>
#include <cstring>

int Kernel::ysyscall_handler(Core* core){
    char buf[1000];
    int error = 0;
    WORD sys_num = core->REG[core->REAX];
    sprintf(buf,"Process %d called system call %d",core->vm->pid,sys_num);
    add_log(buf,Style::NORMAL);
    if(sys_num == Syscall::EXIT){
        int pid = core->vm->pid;
        terminate(core->vm);
        WORD sys_arg = core->REG[core->REBX];
        sprintf(buf,"Process %d exited with status %d",pid,sys_arg);
        add_log(buf,Style::NORMAL);
    }
    else if(sys_num == Syscall::FORK){
        int error;
        VM* new_vm = NULL;
        int new_pid = yfork(error,core->vm->pid,&pid_set,core->vm,new_vm);
        new_vm->saved_reg[core->REAX] = 0;
        new_vm->saved_pc = core->saved_pc;
        new_vm->saved = 1;
        core->REG[core->REAX] = new_pid;
    }
    else if(sys_num == Syscall::EXECVE){
        int error = 0;
        VM* vm = core->vm;
        WORD args_start_addr = core->REG[core->REBX];
        //BYTE arg_buf[10][100];
        BYTE **arg_buf = new BYTE*[10];
        for(int i =0;i<10;++i){
            arg_buf[i] = new BYTE[100];
        }

        int arg_cnt = 0;
        for(int i =0;i<4;i+=4){
            int src;
            WORD arg_vm_addr = 0;
            for(int k = 0;k<4;++k){
                BYTE byte = vm->read(args_start_addr-i+k,error,src);
                arg_vm_addr += byte << (8*k);
                //++i;
            }
            arg_cnt = 0;
            if(arg_vm_addr == 0)
                break;
            else
                arg_cnt ++;
            for(int j = 0;j<100-1;++j){
                int src;
                BYTE byte = vm->read(arg_vm_addr-j*4,error,src);
                arg_buf[i/4][j] = byte;
                if(byte == 0)
                    break;
            }
        }
        //now filename is stored arg_buf[1][0]
        int res = yexecve_wrapper(arg_buf,1,vm,error);
        if(!res){
            error = 1;
        }
    }
    else if(sys_num == Syscall::OPEN){
        int error;
        WORD string_arg_addr = core->REG[core->REBX];
        char filename[110];
        int i;
        for(i = 0;i<100;++i){
            int src;
            BYTE byte = core->vm->read(string_arg_addr + i,error,src);
            filename[i] = byte;
            if(byte == 0)
                break;
            if(error)
                return error;
        }
        int fd = yopen(error,filename,core->vm);
        core->REG[core->REAX] = fd;

    }
    else if(sys_num == Syscall::READ){
        int error;
        int fd = core->REG[core->REBX];
        int charnum = core->REG[core->RECX];
        int store_addr = core->REG[core->REDX];
        int ret = yread(error,fd,charnum,store_addr,core->vm);
        core->REG[core->REAX] = ret;
    }
    else if(sys_num == Syscall::WRITE){
        int error;
        int fd = core->REG[core->REBX];
        int charnum = core->REG[core->RECX];
        int store_addr = core->REG[core->REDX];
        int ret = ywrite(error,fd,charnum,store_addr,core->vm);
        core->REG[core->REAX] = ret;
    }
    return error;
}

int Kernel::yfork(int &error,int parent_pid,std::map<int,VM*>* pid_map,VM *old_vm, VM*& new_vm){
    int new_pid;
    for(int i=1;;++i){
        if(pid_map->count(i)==0){
            new_pid = i;
            break;
        }
    }
    new_vm = new VM(*old_vm);
    new_vm->pid = new_pid;
    new_vm->status = VM::S;
    (*pid_map)[new_pid] = new_vm;
    //set all the PRIVATE pages to RDONLY(copy-on-write)
    for(auto i = old_vm->vm_area.begin();i!=old_vm->vm_area.end();++i){
        if(i->flags == VM::VM_AREA_STRUCT::PRIVATE && i->prot == VM::VM_AREA_STRUCT::RW){
            i->prot = VM::VM_AREA_STRUCT::COPY_ON_WRITE;
            WORD addr= i->start & 0xFFFFF000;
            WORD end_addr = i->end & 0xFFFFF000;
            for(;addr<=end_addr;addr+=4096){
                old_vm->pgd_valid[addr] = VM::RONLY;
            }
        }
    }
    for(auto i = new_vm->vm_area.begin();i!=new_vm->vm_area.end();++i){
        if(i->flags == VM::VM_AREA_STRUCT::PRIVATE && i->prot == VM::VM_AREA_STRUCT::RW){
            i->prot = VM::VM_AREA_STRUCT::COPY_ON_WRITE;
            WORD addr= i->start & 0xFFFFF000;
            WORD end_addr = i->end & 0xFFFFF000;
            for(;addr<=end_addr;addr+=4096){
                old_vm->pgd_valid[addr] = VM::RONLY;
            }
        }
    }
    error =0 ;
    return new_pid;
}

int Kernel::yexecve_wrapper(BYTE **argv,int argc,VM* vm,int &error){
    //file-name is stored at argv[0]
    for(auto vnode:vnode_list){
        if(!strcmp(vnode->filename,(char*)argv[0])){
            ELF* elf = new ELF;
            elf->parse_from_binary(vnode->file_content,vnode->filesize);
            yexecve(error,elf,vm);
            //sysarg stack is not smashed!
            return 0;
        }
    }
    return -1;
}

void Kernel::yexecve(int &error,ELF* elf,VM* vm){
    //destroy the user-space of the VM
    VM::VM_AREA_STRUCT sys_arg_area;
    for(auto i =vm->vm_area.begin();i!=vm->vm_area.end();++i){
        if(i->section_name == "cmd_args"){
            sys_arg_area = *i;
            break;
        }
    }
    vm->vm_area.clear();
    for(auto i = vm->pgd.begin();i!=vm->pgd.end();++i){
        WORD vm_addr = i->first;
        if(vm->pgd_valid[vm_addr]){
            if(vm_addr & 0xFFFFF000< sys_arg_area.start & 0xFFFFF000 || vm_addr & 0xFFFFF000 > sys_arg_area.end & 0xFFFFF000)
                vm->free_page_pte(vm_addr,error);
        }
    }
    //build vm space
    WORD next_addr = vm->allocate_section("text",0,(BYTE*)(elf->text),elf->text_len,
                                          VM::VM_AREA_STRUCT::RONLY,VM::VM_AREA_STRUCT::PRIVATE,error);
    next_addr = vm->allocate_section("data",next_addr,(BYTE*)(elf->data),elf->data_len,
                                     VM::VM_AREA_STRUCT::RW,VM::VM_AREA_STRUCT::PRIVATE,error);
    BYTE *bss = new BYTE[elf->bss_len];
    for(int i =0;i<elf->bss_len;++i){
        bss[i] = 0;
    }
    next_addr = vm->allocate_section("bss",next_addr,bss,elf->bss_len,
                                     VM::VM_AREA_STRUCT::RW,VM::VM_AREA_STRUCT::PRIVATE,error);
    next_addr = vm->allocate_section("stack",0x3FFF0000,NULL,0x10000,VM::VM_AREA_STRUCT::RW,VM::VM_AREA_STRUCT::PRIVATE,error,false);
    next_addr = vm->allocate_section("cmd_args",0x40000000,NULL,0x10000,VM::VM_AREA_STRUCT::RW,VM::VM_AREA_STRUCT::PRIVATE,error,false);

    vm->heap_low = vm->heap_high = next_addr;
    vm->stack_low = vm->stack_high = 0x40000000;
    vm->elf_text = elf->string_text;
}

int Kernel::yopen(int &error,char* filename,VM* vm){
    error = 0;
    for(auto i = vnode_list.begin();i!=vnode_list.end();++i){
        if(!strcmp((*i)->filename,filename)){
            OpenFileTable* item = new OpenFileTable();
            item->pos = 0;
            item->refcnt =1;
            item->vnode=*i;
            open_file_table.push_back(item);
            int fd;
            for(int j =0;j<512;++j){
                if(vm->fd_set[j] == 0){
                    fd = j;
                    break;
                }
            }
            if(fd == 512){
                error = 2;
                return -1; //fd table full
            }
            vm->fd_set[fd] = item;
            return fd;
        }
    }
    error = 1;//file not found
    return -1;
}

int Kernel::yread(int &error,int fd,int charnum,WORD store_vm_addr,VM* vm){
    error = 0;
    OpenFileTable* opened_file = vm->fd_set[fd];
    if(!opened_file){
        error = 1; // file not open
        return -1;
    }
    BYTE* file_content = opened_file->vnode->file_content;
    int file_pos = opened_file->pos;
    int file_len = opened_file->vnode->filesize;
    int read_cnt = 0;
    for(int i = file_pos; i<file_len && i<file_pos+charnum; ++i){
        int src;
        vm->write(store_vm_addr+read_cnt,error,src,file_content[i]);
        if(error){
            return -1; // write mem error
        }
        read_cnt++;
    }
    opened_file->pos += read_cnt;
    return charnum - read_cnt;
}

int Kernel::ywrite(int &error,int fd,int charnum,WORD store_vm_addr,VM* vm){
    error = 0;
    OpenFileTable* opened_file = vm->fd_set[fd];
    if(!opened_file){
        error = 1; // file not open
        return -1;
    }
    BYTE* file_content = opened_file->vnode->file_content;
    int file_pos = opened_file->pos;
    int file_len = opened_file->vnode->filesize;
    int read_cnt = 0;
    if(file_pos + charnum >file_len)
        file_len = file_pos + charnum;
    if(file_len >= 10000)
        return -1;
    opened_file->vnode->filesize = file_len;
    for(int i = file_pos;i<file_pos+charnum; ++i){
        int src;
        BYTE byte = vm->read(store_vm_addr+read_cnt,error,src);
        if(error)
            return -2;
        file_content[i] = byte;
        if(error){
            return -1;
        }
        read_cnt++;
    }
    opened_file->pos += read_cnt;
    return charnum - read_cnt;
}
