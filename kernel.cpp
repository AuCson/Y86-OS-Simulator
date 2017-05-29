#include "kernel.h"
#include "stdio.h"
#include <algorithm>

void Kernel::init(){
    Mem* mem = new Mem();
    CPU* cpu0 = new CPU(&cpu_set,&all_l1_cache,mem);
    VM* vm = new VM(cpu0,1,mem);
    pid_set[1] = vm;
    clk_cycle = 0;
    int error;
    ELF* startup_script = new ELF();
    //startup_script->load_from_strings("70050000007000000000","FFFFFFFF","FFFFFFFF");
    startup_script->load_from_strings("30F430000000802000000030F20A000000000000000000000000000000000000E0010000002023","FFFFFFFF","FFFFFFFF");
    yexecve(error,startup_script,vm);
}

void Kernel::context_switch(int &error,VM* old_vm,VM* new_vm,CPU* cpu,int running_core){
    Core* core = cpu->core[running_core];
    if(old_vm!=NULL){
        old_vm->saved_core = core;
        old_vm->status = VM::S;
        old_vm->cpu = NULL;
        pid_run_time[old_vm->pid] = pid_stop_time[old_vm->pid] = 0;
    }
    if(new_vm->saved_core!=NULL){
        cpu->core[running_core] = new_vm->saved_core;
    }
    else{
        cpu->core[running_core] = new Core(cpu);
        cpu->core[running_core]->REG[4]=cpu->core[running_core]->REG[5] = new_vm->stack_high;
    }
    cpu->core[running_core]->vm = new_vm;
    new_vm->cpu = cpu;
    new_vm->corenum = running_core;
    new_vm->status = VM::R;
    pid_run_time[new_vm->pid] = pid_stop_time[new_vm->pid] = 0;
}

void Kernel::cycle(){
    // run every core
    for(size_t i = 0;i<cpu_set.size();++i){
        CPU* cpu = cpu_set[i];
        for(int j = 0;j<cpu->corenum;++j){
            Core* core = cpu->core[j];
            if(core->vm!=NULL){
                core->inslen = core->vm->elf_text.size();
                core->single_run();
                //check syscall or cpu error
                if(core->stat == core->SSYS){
                    WORD sys_num = core->sys_call_num;
                    WORD sys_arg = core->REG[core->REAX];
                    if(sys_num == Syscall::EXIT){
                        int pid = core->vm->pid;
                        terminate(core->vm);
                        char buf[1000];
                        sprintf(buf,"Process %d exited with status %d",pid,sys_arg);
                        add_log(buf,Style::NORMAL);
                    }
                    else if(sys_num == Syscall::FORK){
                        int error;
                        yfork(error,core->vm->pid,&pid_set,core->vm);
                    }
                    else if(sys_num == Syscall::EXECVE){

                    }
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
    for(auto i = pid_set.begin();i!=pid_set.end();++i){
        VM* vm = i->second;
        if(vm->status == VM::R && vm->stall_clk-clk_cycle > 5){
            context_switch(error,vm,waiting[next_idx++],vm->cpu,vm->corenum);
            if(next_idx == waiting.size())
                break;
        }
    }

    for(auto i = cpu_set.begin();i!=cpu_set.end();++i){
        CPU* cpu = *i;
        for(int j = 0;j<cpu->corenum;++j){
            if(cpu->core[j]->vm == NULL){
                context_switch(error,NULL,waiting[next_idx++],cpu,j);
                if(next_idx == waiting.size())
                    break;
            }
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

void Kernel::add_log(QString s,int type){
    log.append(">>>");
    if(type == Style::CRITICAL){
        log.append("<font color = '#FF0000'>"+s+"</font><br/>");
    }
    else if(type == Style::IMPORTANT){
        log.append("<b>"+s+"</b><br/>");
    }
    else{
        log.append(s+"<br/>");
    }
}

