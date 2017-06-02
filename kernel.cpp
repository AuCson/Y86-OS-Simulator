#include "kernel.h"
#include "stdio.h"
#include <algorithm>

void Kernel::init(){
    mem = new Mem();
    CPU* cpu0 = new CPU(&cpu_set,&all_l1_cache,mem);
    VM* vm = new VM(cpu0,1,mem);
    pid_set[1] = vm;
    clk_cycle = 0;
    int error;

    //create file stdin, stdout
    Vnode* ystdin = new Vnode();
    ystdin->filename = "stdin";
    Vnode* ystdout = new Vnode();
    ystdout->filename = "stdout";
    vnode_list.push_back(ystdin);
    vnode_list.push_back(ystdout);

    yopen(error,"stdin",vm);
    yopen(error,"stdout",vm);

    ELF* startup_script = new ELF();
    //startup_script->load_from_strings("70050000007000000000","FFFFFFFF","FFFFFFFF");
    //startup_script->load_from_strings("30F430000000802000000030F20A000000000000000000000000000000000000E0020000002023","FFFFFFFF","FFFFFFFF");
    //startup_script->load_from_strings("308004000000308301000000308105000000308260000000E000000000308001000000E0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000","68656C6C6F","68656C6C6F");
    startup_script->load_from_strings("000000003080030000003083000000003081010000003084000000402042a038e00000000050640400000030877b00000061767404000000308002000000e000000000620074040000003082fcff0040308003000000e000000000507200000000308604000000616230867d000000617674500000003081000000004012040000003083fcff00404032000000004012fcffffff308604000000202330800b000000e000000000101010101010","abcdabcd","abcdabcd");
    yexecve(error,startup_script,vm);
}

void Kernel::context_switch(int &error,VM* old_vm,VM* new_vm,CPU* cpu,int running_core){
    Core* core = cpu->core[running_core];
    if(old_vm!=NULL && !old_vm->saved){
        old_vm->saved_pc = core->saved_pc;
        for(int i = 0;i<8;++i)
            old_vm->saved_reg[i] = core->REG[i];
        old_vm->saved = 1;
        old_vm->status = VM::S;
        old_vm->cpu = NULL;
        pid_run_time[old_vm->pid] = pid_stop_time[old_vm->pid] = 0;
    }
    if(new_vm->saved){
        core->init();
        core->f_predPC = new_vm->saved_pc;
        for(int i =0;i<8;++i)
            core->REG[i] = new_vm->saved_reg[i];
        new_vm->saved = 0;
    }
    else{
        core->init();
        core->REG[4]=cpu->core[running_core]->REG[5] = new_vm->stack_high;
    }


    core->vm = new_vm;
    new_vm->cpu = cpu;
    new_vm->corenum = running_core;
    new_vm->status = VM::R;
    pid_run_time[new_vm->pid] = pid_stop_time[new_vm->pid] = 0;
    add_log(QString("")+"Process "+QString::number(new_vm->pid)+" started on core "
            +QString::number(new_vm->corenum),Style::MINOR);
}

void Kernel::cycle(){
    // run every core
    char buf[1000];
    for(size_t i = 0;i<cpu_set.size();++i){
        CPU* cpu = cpu_set[i];
        for(int j = 0;j<cpu->corenum;++j){
            Core* core = cpu->core[j];
            if(core->vm!=NULL){
                core->inslen = core->vm->elf_text.size();
                core->single_run();
                //check syscall or cpu error
                if(core->stat == core->SSYS){
                    ysyscall_handler(core);
                    suspend(core->vm);
                }
                else if(core->stat == core->SSPD){
                    suspend(core->vm);
                }
                else if(core->stat == core->SADR || core->stat == core->SHLT || core->stat == core->SINS){
                    terminate(core->vm);
                    add_log("Core "+QString::number(j)+" at CPU "+QString::number(i)+" Fault, Process terminated:"+QString::number(core->stat),Style::CRITICAL);
                }
            }
        }
    }

    // run time calc
    for(auto i = pid_set.begin();i!=pid_set.end();++i){
        VM *vm = i->second;
        if(vm->status == VM::R){
            if(!pid_run_time.count(vm->pid))
                pid_run_time[vm->pid] = 1;
            else
                pid_run_time[vm->pid]++;
        }
        else if(vm->status == VM::S){
            if(!pid_stop_time.count(vm->pid))
                pid_stop_time[vm->pid] = 1;
            else
                pid_stop_time[vm->pid]++;
        }
    }

    arrange();
}

void Kernel::arrange(){
    int error;
    //naive simulation:
    std::vector<VM*> waiting;
    for(auto i = pid_set.begin();i!=pid_set.end();++i){
        VM* vm = i->second;
        if(vm->status == VM::S)
            waiting.push_back(vm);
    }

    if(waiting.size()==0)
        return;
    for(int j =0;j<waiting.size();++j){
        int idx = -1;
        int max_waiting = -1;
        for(int i =j;i<waiting.size();++i){
            if(pid_stop_time[waiting[i]->pid]>max_waiting){
                max_waiting = pid_stop_time[waiting[i]->pid];
                idx = i;
            }
        }
        VM* t = waiting[j];
        waiting[j] = waiting[idx];
        waiting[idx] = t;
    }

    int next_idx = 0;

    for(auto i = cpu_set.begin();i!=cpu_set.end();++i){
        CPU* cpu = *i;
        for(int j = 0;j<cpu->corenum;++j){
            if(cpu->core[j]->vm == NULL){
                context_switch(error,NULL,waiting[next_idx++],cpu,j);
                if(next_idx == waiting.size())
                    return;
            }
        }
    }

    for(auto i = pid_set.begin();i!=pid_set.end();++i){
        VM* vm = i->second;
        if((vm->status == VM::R && pid_run_time[vm->pid] > 10)){
            vm->cpu->core[vm->corenum]->signal_suspend = 1;
            if(next_idx == waiting.size())
                return;
        }
    }

}

void Kernel::terminate(VM* vm){
    if(vm->status == VM::R){
        Core *core = vm->cpu->core[vm->corenum];
        core->vm = NULL;
        vm->cpu = NULL;
    }
    vm->status = VM::T;

    for(auto i =pid_stop_time.begin();i!=pid_stop_time.end();++i){
        if(i->first == vm->pid){
            pid_stop_time.erase(i);
        }
    }
    for(auto i = pid_run_time.begin();i!=pid_run_time.end();++i){
        if(i->first == vm->pid){
            pid_run_time.erase(i);
        }
    }
}

void Kernel::suspend(VM* vm){
    if(vm->status == VM::T){
        return;
    }
    vm->status = VM::S;
    Core *core = vm->cpu->core[vm->corenum];
    core->vm = NULL;
    vm->cpu = NULL;
    vm->saved_pc = core->saved_pc;
    for(int i = 0;i<8;++i){
        vm->saved_reg[i] = core->REG[i];
    }
    vm->saved = 1;
    for(auto i = pid_run_time.begin();i!=pid_run_time.end();++i){
        if(i->first == vm->pid){
            pid_run_time.erase(i);
        }
    }
    add_log(QString("")+"Process "+QString::number(vm->pid)+" was suspended",Style::MINOR);
}

void Kernel::add_cpu(){
    CPU* cpu = new CPU(&cpu_set,&all_l1_cache,mem);

}

void Kernel::add_log(QString s,int type){
    if(type!=Style::FILEOUT)
        log.append(">>>");
    if(type == Style::CRITICAL){
        log.append("<font color = '#FF0000'>"+s+"</font><br/>");
    }
    else if(type == Style::MINOR){
        log.append("<font color = '#A9A9A9'>"+s+"</font><br/>");
    }
    else if(type == Style::IMPORTANT){
        log.append("<b>"+s+"</b><br/>");
    }
    else{
        log.append(s+"<br/>");
    }
}

void ELF::parse_from_binary(BYTE *_buf, int buf_len){
    unsigned char* buf = (unsigned char*)_buf;
    int i = 0;
    int section_header_begin = 0;
    int text_begin = 0;
    int data_begin = 0;
    int rodata_begin = 0;
    int bss_begin = 0;
    for(;i<4;++i){
        section_header_begin += buf[i]<<(8*i);
    }
    i = section_header_begin;
    for(;i<section_header_begin+4;++i){
        text_begin += buf[i] << (8*(i-section_header_begin));
    }
    for(;i<section_header_begin+8;++i){
        data_begin += buf[i] << (8*(i-section_header_begin-4));
    }
    for(;i<section_header_begin+12;++i){
        rodata_begin += buf[i] << (8*(i-section_header_begin-8));
    }
    for(;i<section_header_begin+16;++i){
        bss_begin += buf[i] << (8*(i-section_header_begin-12));
    }
    text_len = 0;
    text = new char[section_header_begin+4];
    data = new char[section_header_begin];
    rodata = new char[section_header_begin];
    for(i = text_begin;i<data_begin;++i){
        text[i-text_begin] = buf[i];
        text_len ++;
    }
    //fix nop
    for(int i = section_header_begin + 3;i>=4;--i){
        text[i] = text[i-4];
    }
    text[0] = text[1] = text[2] = text[3] = 0x10; //nop
    data_len = 0;
    for(i = data_begin;i<rodata_begin;++i){
        data[i-data_begin] = buf[i];
        data_len++;
    }
    rodata_len = 0;
    for(i = rodata_begin;i<bss_begin;++i){
        rodata[i-rodata_begin] = 0;
        rodata_len ++;
    }
    bss_len = section_header_begin - i;
}

