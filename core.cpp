#include "core.h"
#include <assert.h>
#include <cstring>
#include <iostream>
#include "csapp.h"
#include "cpu.h"
#include "vm.h"
void Core::init()
{
    icode = ifun = 0;
    imem_error = 0;

    //Fetch
    f_predPC = F_predPC = f_pc = f_icode = f_ifun =f_valC = f_valP = 0;
    f_predPC = 4;
    f_stat = SAOK;

    //Decode
    D_icode = D_ifun  = D_valC = D_valP = 0;
    d_icode = d_ifun = d_rvalA = d_rvalB = d_valA = d_valB = D_rA = D_rB = 0;
    D_stat= d_stat = SAOK;

    //Execute
    E_icode = E_ifun = E_valC = E_valB = E_valA = E_dstE = E_dstM = 0;
    e_valE = e_cnd = e_setcc = 0;
    E_stat = SAOK;

    //Memory
    M_icode = M_ifun = M_valE = M_valA =  M_Cnd = 0;
    dmem_error = m_valM = m_stat = m_read = 0;
    m_stat = M_stat = SAOK;

    //Write
    W_icode = W_valE = W_valM = 0;
    d_srcA = d_srcB = d_dstM = d_dstE = E_dstE = E_dstM =
           M_dstE = M_dstM = D_srcA = D_srcB = E_srcA = E_srcB = W_dstE = W_dstM = e_dstE = D_rA = D_rB = RNONE;
    W_stat = SAOK;

    funcid = 0;

    signal_suspend = 0;

    stat = SAOK;
    mem_write = false;
    mem_read = false;

    //Registers
    for(int i=0;i<8;++i)
        REG[i]=0;
    REG[RESP] = 0x1000;

    for(int i =0;i<6;++i)
        hist_stat[i] = -1;
    //Flags
    OF = SF = 0;
    ZF = 1;

    C_i = C_b = 0;
    CLK = 0;
    mem.clear();
    memset(instr,0,sizeof(instr));
    stat = SAOK;

    logs.clear();

    for(int i=0;i<5;++i)
        inspos[i]=0;
}

bool Core::isIn(int s,int a[],int len)
{
    for(int i=0;i<len;++i)
        if(s==a[i])
            return true;
    return false;
}

int Core::getcnd(int m_ifun)
{
    switch(m_ifun)
    {
    case 0:return 1;//jmp
    case 1:return SF^OF | ZF;//jle
    case 2:return SF^OF;//jl
    case 3:return ZF;//je
    case 4:return !ZF;//jne
    case 5:return !(SF ^ ZF);//jge
    case 6:return !(SF ^ OF | ZF);//jg
    default:return 0;
    }
}

int Core::alucalc(int aluA,int aluB,int alufun,bool setcc)
{
    int res;
    switch (alufun)
    {
    case 0:res = aluB + aluA;break;
    case 1:res = aluB - aluA;break;
    case 2:res = aluB & aluA;break;
    case 3:res = aluB ^ aluA;break;
    default:return 0;
    }
    if(setcc)
    {
        if(res == 0)
        {
            ZF = 1;
            sprintf(buf,"E:Set ZF to 1");
            logs.push_front((std::string)buf);
        }
        else ZF = 0;
        if(res < 0)
        {
            SF = 1;
            sprintf(buf,"E:Set SF to 1");
            logs.push_front((std::string)buf);
        }
        else SF = 0;
        if((alufun == ALUADD && (aluA<0) == (aluB<0) && (aluA < 0)!= (res < 0)
                )||( alufun == ALUSUB && (aluB < 0) == (aluA > 0) && (aluB<0) != (res < 0))){
            OF = 1;
            sprintf(buf,"E:Set OF to 1");
            logs.push_front((std::string)buf);
        }
        else OF=0;
    }
    return res;
}

void Core::writemem(int addr,int val,int& error) //WORD write
{
    sprintf(buf,"M:Write Memory at %x to %x",addr,val);
    error = 0;
    logs.push_front((std::string)buf);
    for(int i=0;i<4;++i)
    {
        if(1073741820==addr)
            int a = 1;
        vm->write(addr,error,mem_src,val & 0xFF);
        if(error)
            return;
        //mem[addr] = val & 0xFF;
        val >>= 8;
        ++addr;
    }

    return;
}

int Core::readmem(int addr,int &error)
{
    error =0;
    int res = 0;
    int src;
    for(int i=0;i<4;++i)
    {
        if(error)
            return 0;
        if(addr ==1073807356 )
            int a=  1;
        int t_res = vm->read(addr,error,src);
        if(error)
            return 0;
        mem_src = src;
        res = res + (t_res<<(8*i));
        ++addr;
    }
    sprintf(buf,"M:Read Memory at %x : %x",addr-4,res);
    logs.push_front((std::string)buf);
    return res;
}

unsigned char Core::readmem_byte(int addr,int &error){
    error = 0;
    int res = 0;
    int src;
    res = vm->read(addr,error,src);
    if(error)
        return 0;
    mem_src = src;
    return res;
}

void Core::PipeLogic()
{

    int a[] = { IMRMOVL, IPOPL, ILEAVE };
    int b[] = { d_srcA, d_srcB };
    int c[] = { D_icode, E_icode, M_icode };
    F_stall = (isIn(E_icode, a,3)  && isIn(E_dstM, b,2))  || isIn(IRET, c,3);
    if(signal_suspend){
        //kernel ask the process to suspend
        signal_suspend = 0;
        F_stall = true;
        f_stat = SSPD;
    }
    if(F_stall)
    {
        sprintf(buf,"PipeLogic:F_stall");
        logs.push_front((std::string)buf);
    }

    int d[] = { IMRMOVL, IPOPL, ILEAVE };
    int e[] = { d_srcA, d_srcB };
    D_stall = isIn(E_icode, d,3) && isIn(E_dstM,e,2);
    if(D_stall)
    {
        sprintf(buf,"PipeLogic:D_stall");
        logs.push_front((std::string)buf);
    }

    int f[] = { IMRMOVL, IPOPL, ILEAVE };
    int g[] = { d_srcA, d_srcB };
    int h[] = { D_icode, E_icode, M_icode };
    D_bubble = (E_icode == IJXX && !e_cnd) ||
            !( isIn(E_icode,f,3) && isIn(E_dstM,g,2)) && (isIn(IRET,h,3));
    if(D_bubble)
    {
        sprintf(buf,"PipeLogic:D_bubble");
        logs.push_front((std::string)buf);
    }

    int i[] = {IMRMOVL,IPOPL};
    int j[] = { d_srcA, d_srcB};

    E_bubble = (E_icode == IJXX && !e_cnd) || isIn(E_icode,i,2) && isIn(E_dstM,j,2);
    if(E_bubble)
    {
        sprintf(buf,"PipeLogic:E_bubble");
        logs.push_front((std::string)buf);
    }

    int k[] = { SADR, SINS, SHLT, SSYS, SSPD };
    int l[] = { SADR, SINS, SHLT, SSYS, SSPD };
    M_bubble = isIn(m_stat, k,5) || isIn(W_stat, l,5);
    if(M_bubble)
    {
        sprintf(buf,"PipeLogic:M_bubble");
        logs.push_front((std::string)buf);
    }

    int m[] = { SADR, SINS, SHLT, SSYS,SSPD };
    W_stall = isIn(W_stat,m,5);
    if(W_stall)
    {
        sprintf(buf,"PipeLogic:W_stall");
        logs.push_front((std::string)buf);
    }

}

void Core::stageW()
{
    int a[] = {SADR,SINS,SHLT};

    if(isIn(W_stat,a,3))
        return;
    W_stat = m_stat;
    W_icode = M_icode;
    W_valE = M_valE;
    W_valM = m_valM;
    W_dstE = M_dstE;
    W_dstM = M_dstM;

    if(W_dstE!=RNONE)
    {

        REG[W_dstE] = W_valE;
        sprintf(buf,"W:Write Reg %s <= %x", regname[W_dstE].c_str(),W_valE);
        logs.push_front((std::string)buf);
    }
    if(W_dstM != RNONE)
    {
        REG[W_dstM] = W_valM;
        sprintf(buf,"W:Write Reg %s <= %x", regname[W_dstM].c_str(),W_valM);
    }

    stat = W_stat == SBUB ? SAOK : W_stat;

}

void Core::stageM()
{
    if (M_bubble)
    {
        M_stat = SBUB;
        M_icode = 1;
        M_ifun = 0;
        m_valM = 0;
        M_dstE = M_dstM = RNONE;
        return;
    }

    M_stat = E_stat;
    M_icode = E_icode;
    M_Cnd = e_cnd;
    M_valE = e_valE;
    M_valA = E_valA;
    M_dstE = e_dstE;
    M_dstM = E_dstM;
    m_stat = M_stat;
    int a[4] = { IRMMOVL, IPUSHL, ICALL, IMRMOVL };
    int b[3] = { IPOPL, IRET ,ILEAVE};
    // Select memory address
    if (isIn(M_icode,a,4))
        mem_addr = M_valE;
    else if (isIn(M_icode, b,3))
        mem_addr = M_valA;

    // Should read
    int c[4] = { IMRMOVL, IPOPL, IRET, ILEAVE };
    mem_read = isIn(M_icode,c,4);
    if(mem_addr == 0x4000FFF4)
        int aa = 0;
    // Should write
    int d[3] = { IRMMOVL, IPUSHL, ICALL };
    mem_write = isIn(M_icode, d,3);
    if (mem_write) {
        writemem(mem_addr, M_valA,dmem_error);

    }

    // Update the status
    if (dmem_error)
        m_stat = SADR;

    if(mem_read)
    {
        m_valM = readmem(mem_addr,dmem_error);
    }

    if (dmem_error)
        m_stat = SADR;

    if(M_icode == ICALL)
    {
        stkaddr.push_back(mem_addr);
        stkfunc.push_back(funcid);
        stkclk.push_back(CLK);
    }
    if(M_icode == IRET)
    {
        if(!stkaddr.empty()){
            stkaddr.pop_back();
            stkfunc.pop_back();
            stkclk.pop_back();
        }
    }

}

void Core::stageE()
{
    if (E_bubble)
    {
        E_stat = SBUB;
        E_icode = 1;
        E_ifun = e_valE =  E_valA = 0;
        e_dstE = E_dstE = E_dstM = RNONE;
        C_b++;
        return;
    }

    C_i ++;
    E_stat = D_stat;
    E_icode = D_icode;
    E_ifun = D_ifun;
    E_valC = D_valC;
    E_dstE = d_dstE;
    E_dstM = d_dstM;
    E_srcA = d_srcA;
    E_srcB = d_srcB;
    E_valA = d_valA;
    E_valB = d_valB;

    int a[3] = { IRRMOVL, IOPL, ICMPL };
    int b[3] = { IIRMOVL, IRMMOVL, IMRMOVL };
    int c[2] = { ICALL, IPUSHL };
    int d[3] = { IRET, IPOPL, ILEAVE };

    int aluA = 0;
    if (isIn(E_icode,a,3))
        aluA = E_valA;
    else if (isIn(E_icode,b,3))
        aluA = E_valC;
    else if (isIn(E_icode,c,2))
        aluA =-4;
    else if (isIn(E_icode, d,3))
        aluA = 4;

    int e[9] = { IRMMOVL, IMRMOVL, IOPL, ICALL, IPUSHL, IRET, IPOPL, ICMPL, ILEAVE};
    int f[2] = { IRRMOVL, IIRMOVL };
    int aluB = 0;
    if (isIn(E_icode, e,9))
        aluB = E_valB;
    else if (isIn(E_icode, f,2))
        aluB = 0;

    int  alufun;
    if (E_icode == IOPL)
        alufun = E_ifun;
    else if(E_icode == ICMPL)
        alufun = ALUSUB;
    else
        alufun = ALUADD;

    int g[] = { SADR, SINS, SHLT, SSYS,SSPD };
    int h[] =  { SADR, SINS, SHLT, SSYS,SSPD };
    bool set_cc = (E_icode == IOPL || E_icode == ICMPL) && (!isIn(m_stat, g,5)) && (!isIn(W_stat, h,5));

    e_valE = alucalc(aluA, aluB, alufun, set_cc);
    e_cnd = getcnd(E_ifun);

    //cmovxx
    if (E_icode == IRRMOVL && !e_cnd)
        e_dstE = RNONE;
    else
        e_dstE = E_dstE;
}


void Core::stageD()
{
    if (D_stall){
        goto updateD;
        }
    D_stat = f_stat;
    D_ifun = f_ifun;
    D_icode = f_icode;
    D_valP = f_valP;
    D_valC = f_valC;
    D_rA = f_rA;
    D_rB = f_rB;


    if (D_bubble) {
        D_stat = SBUB;
        D_icode = 1;
        D_ifun = 0;
        D_rA = 0;
        D_rB= 0;
        D_valC = 0;
        D_valP = 0;
        d_srcA = RNONE;
        d_srcB = RNONE;
        d_dstE = RNONE;
        d_dstM = RNONE;
        return;
    }
updateD:
    int a[5] = { IRRMOVL, IRMMOVL, IOPL, IPUSHL, ICMPL };
    int b[2] = { IPOPL, IRET };
    int b2[1] = {ILEAVE};

    if (isIn(D_icode, a,5))
        d_srcA = D_rA;
    else if (isIn(D_icode, b,2))
        d_srcA = RESP;
    else if(isIn(D_icode,b2,1))
        d_srcA = REBP;
    else
        d_srcA = RNONE;

    if(d_srcA != RNONE){
        d_rvalA = REG[d_srcA];
    }
    if(d_srcA==12){
        int a = 1;
    }

    int c[4] = { IOPL, IRMMOVL, IMRMOVL,ICMPL };
    int d[4] = { IPUSHL, IPOPL, ICALL, IRET };
    int d2[] = { ILEAVE };
    if (isIn(D_icode, c,4))
        d_srcB = D_rB;
    else if (isIn(D_icode, d,4))
        d_srcB = RESP;
    else if(isIn(D_icode,d2,1))
        d_srcB = REBP;
    else
        d_srcB = RNONE;
    if(d_srcB!=RNONE)
        d_rvalB = REG[d_srcB];

    int e[3] = { IRRMOVL, IIRMOVL, IOPL};
    int f[5] = { IPUSHL, IPOPL, ICALL, IRET, ILEAVE};
    if (isIn(D_icode,e,3))
        d_dstE = D_rB;
    else if (isIn(D_icode,f,5))
        d_dstE = RESP;
    else
        d_dstE = RNONE;

    int g[2] = { IMRMOVL, IPOPL };
    int g2[1] = {ILEAVE};
    if (isIn(D_icode, g,2))
        d_dstM = D_rA;
    else if(isIn(D_icode,g2,1))
        d_dstM = REBP;
    else
        d_dstM = RNONE;

    int h[2] = { ICALL, IJXX };
    if (isIn(D_icode, h,2))
    {
        d_valA = D_valP;
        sprintf(buf,"D:d_valA from D_valP : %x",d_valA);
        logs.push_front(buf);
    }
    else if (d_srcA == e_dstE)
    {
        d_valA = e_valE;
        sprintf(buf,"D:Forward d_valA from e_valE : %x",d_valA);
        logs.push_front(buf);
    }
    else if(d_srcA == M_dstM)
    {
        d_valA = m_valM;
        sprintf(buf,"D:Forward d_valA from m_valM : %x",d_valA);
        logs.push_front(buf);
    }
    else if (d_srcA == M_dstE){
        d_valA = M_valE;
        sprintf(buf,"D:Forward d_valA from M_valE : %x",d_valA);
        logs.push_front(buf);
    }
    else if (d_srcA == W_dstM)
    {
        d_valA = W_valM;
        sprintf(buf,"D:Forward d_valA from W_valM : %x",d_valA);
        logs.push_front(buf);
    }
    else if (d_srcA == W_dstE)
    {
        d_valA = W_valE;
        sprintf(buf,"D:Forward d_valA from W_valE : %x",d_valA);
        logs.push_front(buf);
    }
    else
    {
        d_valA = d_rvalA;
        sprintf(buf,"D:d_valA from d_srcA %s : %x",regname[d_srcA].c_str(),d_valA);
        logs.push_front(buf);
    }

    if (d_srcB == e_dstE)
    {
        d_valB = e_valE;
        sprintf(buf,"D:d_valB from D_valP : %x",d_valB);
        logs.push_front(buf);
    }
    else if (d_srcB == M_dstM)
    {
        d_valB = m_valM;
        sprintf(buf,"D:Forward d_valB from m_valM : %x",d_valB);
        logs.push_front(buf);
    }
    else if (d_srcB == M_dstE)
    {
        d_valB = M_valE;
        sprintf(buf,"D:Forward d_valB from M_valE : %x",d_valB);
        logs.push_front(buf);
    }
    else if (d_srcB == W_dstM)
    {
        d_valB = W_valM;
        sprintf(buf,"D:Forward d_valB from W_valM : %x",d_valB);
        logs.push_front(buf);
    }
    else if (d_srcB == W_dstE)
    {
        d_valB = W_valE;
        sprintf(buf,"D:Forward d_valB from W_valE : %x",d_valB);
        logs.push_front(buf);
    }
    else
    {
        d_valB = d_rvalB;
        sprintf(buf,"D:d_valB from %s : %x",regname[d_srcB].c_str(),d_valB);
        logs.push_front(buf);
    }

}

void Core::stageF()
{
    if(F_stall)
        goto updateF;
    F_predPC = f_predPC;

updateF:
    if (M_icode == IJXX && !M_Cnd)
    {
        f_pc = M_valA;
        sprintf(buf,"F:Mispredicted branch: set PC to %x",M_valA);
        logs.push_front(buf);
    }
    else if (W_icode == IRET)
    {
        f_pc = W_valM;
        sprintf(buf,"F:Processing RET: set PC to %x",W_valM);
        logs.push_front(buf);
    }
    else
    {
        f_pc = F_predPC;
        sprintf(buf,"F:Set PC to predicted PC: %x",f_pc);
        logs.push_front(buf);
    }

    if(f_pc<0 || f_pc >= (1<<16)-6)
        imem_error = 1;

    f_icode  = readmem_byte(f_pc,core_error);
    f_ifun = f_icode & 0xF;
    f_icode >>= 4;
    f_icode &= 0xF;

    f_valP = f_pc + 1;

    if (imem_error)
        f_icode = INOP;
    else
        f_icode = f_icode;

    if (imem_error)
        f_ifun = 0;
    else
        f_ifun = f_ifun;

    int a[] = { INOP, IHALT, IRRMOVL, IIRMOVL, IRMMOVL, IMRMOVL,IOPL,ICMPL, IJXX, ICALL, IRET, IPUSHL, IPOPL,ILEAVE,ISYSCALL };
    instr_valid = isIn(f_icode,a,15);


    if (imem_error)
        f_stat = SADR;
    else if (!instr_valid)
        f_stat = SINS;
    else if (f_icode == IHALT)
        f_stat = SHLT;
    //update syscall
    else if (f_icode == ISYSCALL)
        f_stat = SSYS;
    else if(f_stat != SSPD)
        f_stat = SAOK;


    int b[] = { IRRMOVL, IOPL, ICMPL,IPUSHL, IPOPL, IIRMOVL, IRMMOVL, IMRMOVL };
    bool need_regids = isIn(f_icode,b,8);
    if (need_regids)
    {
        f_rA = readmem_byte(f_valP,core_error);
        f_rB = f_rA & 0xf;
        f_rA >>= 4;
        f_valP ++;
    }

    int c[] = { IIRMOVL, IRMMOVL, IMRMOVL, IJXX, ICALL, ISYSCALL };
    bool need_valC = isIn(f_icode, c,6);
    if (need_valC) {
        f_valC = 0;
        for(int i=0;i<4;++i)
        {
           f_valC += readmem_byte(f_valP++,core_error)<<(i*8);
        }
    }

    int d[] = { IJXX, ICALL };
    if (isIn(f_icode, d,2))
        f_predPC = f_valC;
    else
        f_predPC = f_valP;

    if(f_icode == ICALL)
        funcid = f_predPC;

    if(f_stat == SSYS || f_stat == SSPD){
        this->saved_pc = f_predPC;
    }
}

void Core::single_run(){
    logs.clear();
    PipeLogic();
    stageW();
    stageM();
    stageE();
    stageD();
    stageF();
}

