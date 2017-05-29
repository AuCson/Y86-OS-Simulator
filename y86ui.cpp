#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <string>
#include <cstdio>

#define histstat Logic->hist_stat

int gethex(const std::string &s,int i,int n)
{
    int ans = 0,t;
    for(int j=0;j<n;j+=2)
    {
        char bufh[3];
        bufh[0]=s[i+j];
        bufh[1] = s[i+j+1];
        bufh[2]='\0';
        sscanf(bufh,"%x",&t);
        ans += t << (4*j);
    }
    return ans;
}

inline QString ascii(int ch)
{
    char temp[10];
    sprintf(temp,"%x",ch);
    return QString(temp);
}
inline QString asciistat(char c)
{
    switch(c)
    {
    case 0:return QString("BUB");
    case 1:return QString("AOK");
    case 2:return QString("HLT");
    case 3:return QString("ADR");
    case 4:return QString("INS");
    default:return QString("???");
    }
}
inline QString asciireg(char c)
{
    switch(c)
    {
    case 0:return QString("0/%eax");
    case 1:return QString("1/%ecx");
    case 2:return QString("2/%edx");
    case 3:return QString("3/%ebx");
    case 4:return QString("4/%esp");
    case 5:return QString("5/%ebp");
    case 6:return QString("6/%edi");
    case 7:return QString("7/%esi");
    case 0xF:return QString("F/NONE");
    default:return QString("?/?");
    }
}

std::string regname(int i,int &error)
{
    std::string name[9] = {"%eax","%ecx","%edx","%ecx","%esp","%ebp","%esi","%edi","???"};
    if(i<8 && i>=0)
        return name[i];
    else
        return name[8];
}

void MainWindow::disass(std::string s)
{
    if(s.size()==0)
        return;
    ui->tdisas->clear();
    int i=0,r=0;
    int segerr = 0;
    char buf[1000];
    char a[4][5]={"addl","subl","andl","xorl"};
    char b[7][5]={"jmp","jle","jl","je","jne","jge","jg"};
    ui->tdisas->setColumnCount(3);
    ui->tdisas->setRowCount(s.size()/2);
    ui->tdisas->setColumnWidth(0,50);
    ui->tdisas->setColumnWidth(1,180);
    ui->tdisas->setColumnWidth(2,150);
    QStringList header;
    header<<"Addr"<<"Instr"<<"Status";
    ui->tdisas->setHorizontalHeaderLabels(header);
    while(i<s.size())
    {
        int err = 0;
        if(isspace(s[i]))
        {
            i++;
            continue;
        }
        sprintf(buf,"%x",i/2);
        ui->tdisas->setItem(r,0,new QTableWidgetItem(buf));
        char t[12];
        t[0]=s[i];
        t[1]=0;
        switch (s[i]) {
        case '0':
            ui->tdisas->setItem(r,1,new QTableWidgetItem("halt"));
            i+=2;
            break;
        case '1':
            ui->tdisas->setItem(r,1,new QTableWidgetItem("nop"));
            i+=2;
            break;
        case '2':
            if(i+3>=s.size()){i+=4;segerr = 1;break;}
            sprintf(buf,"rrmovl %s,%s",regname(s[i+2]-'0',err).c_str(),regname(s[i+3]-'0',err).c_str());
            ui->tdisas->setItem(r,1,new QTableWidgetItem(buf));
            if(err){i+=2;break;}
            i+=4;
            break;
        case '3':
            if(i+11>=s.size()){i+=12;segerr = 1;break;}
            sprintf(buf,"irmovl $0x%x,%s",gethex(s,i+4,8),regname(s[i+3]-'0',err).c_str());
            ui->tdisas->setItem(r,1,new QTableWidgetItem(buf));
            ;
            if(err){i+=2;break;}
            i+=12;
            break;
        case '4':
            if(i+11>=s.size()){i+=12;segerr = 1;break;}
            sprintf(buf,"rmmovl %s,0x%x(%s)",regname(s[i+2]-'0',err).c_str(),gethex(s,i+4,8),regname(s[i+3]-'0',err).c_str());
            ui->tdisas->setItem(r,1,new QTableWidgetItem(buf));
            if(err){i+=2;break;}
            i+=12;
            break;
        case '5':
            if(i+11>=s.size()){i+=12;segerr = 1;break;}
            sprintf(buf,"mrmovl 0x%x(%s),%s",gethex(s,i+4,8),regname(s[i+3]-'0',err).c_str(),regname(s[i+2]-'0',err).c_str());
            ui->tdisas->setItem(r,1,new QTableWidgetItem(buf));
            if(err){i+=2;break;}
            i+=12;
            break;
        case '6':
            if(i+3>=s.size()){i+=4;segerr = 1;break;}
            sprintf(buf,"%s %s,%s",a[s[i+1]-'0'],regname(s[i+2]-'0',err).c_str(),regname(s[i+3]-'0',err).c_str());
            ui->tdisas->setItem(r,1,new QTableWidgetItem(buf));
            if(err){i+=2;break;}
            i+=4;
            break;
        case '7':
            if(i+9>=s.size()){i+=10;segerr = 1;break;}
            sprintf(buf,"%s 0x%x",b[s[i+1]-'0'],gethex(s,i+2,8));
            ui->tdisas->setItem(r,1,new QTableWidgetItem(buf));
            i+=10;
            break;
        case '8':
            if(i+9>=s.size()){i+=10;segerr = 1;break;}
            sprintf(buf,"call 0x%x",gethex(s,i+2,8));
            ui->tdisas->setItem(r,1,new QTableWidgetItem(buf));
            i+=10;
            break;
        case '9':
            ui->tdisas->setItem(r,1,new QTableWidgetItem("ret"));
            i+=2;
            break;
        case 'A':case 'a':
            if(i+3>=s.size()){i+=4;segerr = 1;break;}
            sprintf(buf,"pushl %s",regname(s[i+2]-'0',err).c_str());
            ui->tdisas->setItem(r,1,new QTableWidgetItem(buf));
            if(err){i+=2;break;}
            i+=4;
            break;
        case 'B':case'b':
            if(i+3>=s.size()){i+=4;segerr = 1;break;}
            sprintf(buf,"popl %s",regname(s[i+2]-'0',err).c_str());
            ui->tdisas->setItem(r,1,new QTableWidgetItem(buf));
            if(err){i+=2;break;}
            i+=4;
            break;
        case 'C':case'c':
            if(i+3>=s.size()){i+=4;segerr = 1;break;}
            sprintf(buf,"cmpl %s,%s",regname(s[i+2]-'0',err).c_str(),regname(s[i+3]-'0',err).c_str());
            ui->tdisas->setItem(r,1,new QTableWidgetItem(buf));
            if(err){i+=2;break;}
            i+=4;
            break;
        case 'D':case 'd':
            ui->tdisas->setItem(r,1,new QTableWidgetItem("leave"));
            i+=2;
            break;
        case 'E':
            if(i+9>=s.size()){i+=10;segerr = 1;break;}
            sprintf(buf,"syscall 0x%x",gethex(s,i+2,8));
            ui->tdisas->setItem(r,1,new QTableWidgetItem(buf));
            i+=10;
            break;
        default:
            ui->tdisas->setItem(r,1,new QTableWidgetItem("???"));
            i+=2;
            break;
        }

        r++;

    }
}

void MainWindow::f_pc_show(int noupdate,Core *Logic)
{
    for(int i=0;i<ui->tdisas->rowCount();++i)
    {
        if(ui->tdisas->item(i,1)!=NULL && ui->tdisas->item(i,0)!=NULL && ui->tdisas->item(i,2)!=NULL)
        {
            ui->tdisas->item(i,2)->setBackgroundColor(QColor::fromRgb(255,255,255));
            ui->tdisas->item(i,1)->setBackgroundColor(QColor::fromRgb(255,255,255));
            ui->tdisas->item(i,0)->setBackgroundColor(QColor::fromRgb(255,255,255));
            ui->tdisas->item(i,2)->setFont(QFont("ubuntu",9,QFont::Normal));
            ui->tdisas->item(i,1)->setFont(QFont("ubuntu",9,QFont::Normal));
            ui->tdisas->item(i,0)->setFont(QFont("ubuntu",9,QFont::Normal));
            ui->tdisas->item(i,2)->setText("");
        }
        else{
            for(int j =0;j<3;++j){
                if(ui->tdisas->item(i,j)==NULL)
                    ui->tdisas->setItem(i,j,new QTableWidgetItem());

            }
        }
    }
    int x;
    int r = 0;

    if(!noupdate)
    {
        for(int i =0;i<100;++i)
        {
            int cnt = ui->tdisas->rowCount();
            QTableWidgetItem* item = ui->tdisas->item(i,0);
            QString t = item->text();

            if(t.isEmpty())
                break;
            sscanf(t.toStdString().c_str(),"%x",&x);

            if(Logic->f_pc >= Logic->inslen)
            {
                r = -1;
                break;
            }

            if(x == Logic->f_pc)
            {
                r = i;
                break;
            }
        }
        for(int i=4;i>=0;--i)
        {
            if(histstat[i]>=0){
                if(ui->tdisas->item(histstat[i],2)==NULL)
                    ui->tdisas->setItem(histstat[i],2,new QTableWidgetItem());
                ui->tdisas->item(histstat[i],2)->setText("");
            }
            if(!(i==0 && Logic->D_stall))
                histstat[i+1]=histstat[i];

        }

        if(Logic->D_bubble)
            histstat[1]=-1;
        if(Logic->E_bubble)
            histstat[2]=-1;
        if(Logic->M_bubble)
            histstat[3]=-1;
        histstat[0] = r;
    }
    char name[5][3] = {"F","D","E","M","W"};
    for(int i=0;i<5;++i)
    {
        if(histstat[i]!=-1 && ui->tdisas->item(histstat[i],1)!=NULL)
        {
            ui->tdisas->setItem(histstat[i],2,new QTableWidgetItem(name[i]));
            ui->tdisas->item(histstat[i],2)->setBackgroundColor(QColor::fromRgb(249,236,168));
            ui->tdisas->item(histstat[i],1)->setBackgroundColor(QColor::fromRgb(249,236,168));
            ui->tdisas->item(histstat[i],0)->setBackgroundColor(QColor::fromRgb(249,236,168));
            ui->tdisas->item(histstat[i],2)->setFont(QFont("ubuntu",9,QFont::Bold));
            ui->tdisas->item(histstat[i],1)->setFont(QFont("ubuntu",9,QFont::Bold));
            ui->tdisas->item(histstat[i],0)->setFont(QFont("ubuntu",9,QFont::Bold));
        }
    }

}

void MainWindow::cpu_viewer(int cpu_id,int corenum){
    //debug
    if(cpu_id == -1 || cpu_id >= kernel->cpu_set.size()){
        ui->tdisas->clear();
        return;
    }

    CPU* current_cpu = kernel->cpu_set[cpu_id];
    if(corenum >= current_cpu->corenum){
        ui->tdisas->clear();
        return;
    }
    Core* core = current_cpu->core[corenum];
    if(core->vm == NULL){
        ui->tdisas->clear();
        return;
    }
    std::string text = core->vm->elf_text;

    disass(text);
    f_pc_show(0,core);
}
