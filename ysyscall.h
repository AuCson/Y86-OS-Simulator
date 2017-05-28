#ifndef YSYSCALL_H
#define YSYSCALL_H
#include <set>
#include <map>
class CPU;
class ELF;
class Mem;
class VM;

int ystartup();
int yfork(int &error,int parent_pid,std::map<int,VM*>* pid_map,VM *old_vm);
int yexecve(int &error,ELF* elf,VM* vm);

int context_switch(int &error,VM* old_vm,VM* new_vm,CPU* cpu,int running_core);

#endif // YSYSCALL_H
