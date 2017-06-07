#ifndef CORE_H
#define CORE_H
#include <map>
#include <string>
#include <list>
#include <stack>
#include <queue>
#include <vector>
#include "bit.h"
class CPU;
class VM;
class Core
{
public:
    Core(CPU* _cpu)
    {
        cpu = _cpu;
        init();
    }

    void set_vm(VM* _vm){
        vm = _vm;
    }

    CPU *cpu;
    VM* vm = NULL;
    int mem_src;
    int context_switching = 0;
    int core_error;
    int sys_call_num = -1;
    std::queue<int> saved_pc;

    //Stat
    const int IHALT = 0,INOP = 1,IRRMOVL = 2, IIRMOVL = 3,IRMMOVL = 4,IMRMOVL = 5,IOPL = 6,
            IJXX = 7,ICALL = 8,IRET = 9,IPUSHL = 0xA, IPOPL = 0xB, ICMPL = 0xC, ILEAVE = 0xD,ISYSCALL = 0xE;
    const int REAX = 0,RECX = 1,REDX = 2,REBX = 3,RESP = 4,REBP = 5,RESI = 6,REDI = 7,RNONE = 0xF;
    const int SBUB = 0,SAOK = 1,SHLT = 2,SADR = 3,SINS = 4,SSYS=5,SSPD = 6;
    const int ALUADD = 0,ALUSUB = 1,ALUAND = 2,ALUXOR =3;
    std::string regname[9];

    //pipeline Stats
    int icode,ifun;
    int imem_error,instr_valid;
    //Fetch
    int F_predPC;
    int f_predPC,f_pc,f_icode,f_ifun,f_valC,f_valP,f_stat,f_rA,f_rB;

    //Decode
    int D_rA,D_rB,D_stat,D_icode,D_ifun,D_srcA,D_srcB,D_valC,D_valP;
    int d_stat,d_icode,d_ifun,d_rvalA,d_rvalB,d_srcA,d_srcB,d_dstM,d_dstE,d_valA,d_valB;

    //Execute
    int E_stat,E_icode,E_ifun,E_valC,E_valB,E_valA,E_dstE,E_dstM,E_srcA,E_srcB;
    int e_valE,e_dstE,e_cnd,e_setcc;

    //Memory
    int M_stat,M_icode,M_ifun,M_valE,M_valA,M_dstE,M_dstM,M_Cnd;
    int dmem_error,m_valM,m_stat,m_read;
    bool mem_write,mem_read;
    int mem_addr;

    //Write
    int W_stat,W_icode,W_valE,W_valM,W_dstE,W_dstM;

    //Bubble and Stall
    bool F_stall,F_bubble,D_stall,D_bubble,E_stall,E_bubble,M_stall,M_bubble,W_stall,W_bubble;
    int signal_suspend;

    //Registers
    int REG[8];

    //Flags
    int ZF,OF,SF;

    int stat;

    int inspos[5];
    int alucalc(int aluA, int aluB, int aluFUN, bool setcc);

    void init();
    void stageF();
    void stageD();
    void stageE();
    void stageM();
    void stageW();

    void PipeLogic();

    bool isIn(int s, int a[], int len);
    int getcnd(int m_func);
    void writemem(int addr, int val, int &error);
    int readmem(int addr, int &error);
    unsigned char readmem_byte(int addr,int &error);
    void statlog();
    double cpi(){return (double)(C_i + C_b)/C_i;}

    void single_run();


    unsigned char instr[1000];
    int inslen;
    std::map<int,int> mem;
    std::list<std::string> logs;
    char buf[1000];
    int hist_stat[6];

    int C_i;
    int C_b;
    int CLK;
    int funcid;

    std::vector<int> stkfunc,stkaddr,stkclk;

};

#endif // CORE_H
