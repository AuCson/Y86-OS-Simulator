#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <cstring>
#include <cstdio>
void MainWindow::hardware_tree_viewer(){
    ui->hardwares->clear();
    for(int i =0;i<kernel->cpu_set.size();++i){
        QTreeWidgetItem* item = new QTreeWidgetItem(ui->hardwares,QStringList(QString("CPU ")+QString::number(i)));
        for(int j = 0;j<kernel->cpu_set[i]->corenum;++j){
            QTreeWidgetItem* child_item = new QTreeWidgetItem(item,QStringList(QString("Core ")+QString::number(j)));
            connect(ui->hardwares,SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),this,SLOT(alter_cpu_core_view(QTreeWidgetItem*,int)));
        }
    }
}


void MainWindow::alter_cpu_core_view(QTreeWidgetItem* item,int r){
    QString text = item->text(0);
    char buf[100];
    strcpy(buf,text.toStdString().c_str());
    char type[10];
    int corenum;
    sscanf(buf,"%s %d",type,&corenum);
    if(!strcmp("Core",type)){
        //debug_msg(item->text(0));
        current_core_num = corenum;
        QTreeWidgetItem* parent = item->parent();
        text = parent->text(0);
        strcpy(buf,text.toStdString().c_str());
        int cpu_id;
        sscanf(buf,"%s %d",type,&cpu_id);
        current_cpu_id = cpu_id;
        cpu_viewer(current_cpu_id,current_core_num);

        current_vm = kernel->cpu_set[current_cpu_id]->core[current_core_num]->vm;

        ui->stacked->setCurrentIndex(1);

    }
    if(!strcmp("DRAM",type)){
        //debug_msg(item->text(0));
        ui->stacked->setCurrentIndex(0);
    }
}

void MainWindow::show_addr_slot(int row,int col){
    QTableWidgetItem * item = ui->t_addr->item(row,0);
    if(!item || item->text() == ""){
        return;
    }
    QString vm_addr = item->text();
    std::string vm_addr_std = vm_addr.toStdString();
    WORD addr;
    sscanf(vm_addr_std.c_str(),"%x",&addr);

    VM *vm = current_vm;

    int error = 0;
    int src = 0;
    WORD phy_addr = 0;
    BYTE value = vm->read(addr,error,src,phy_addr);

    if(!error){
        char buf[100];
        sprintf(buf,"0x%x",phy_addr);
        ui->t_addr->setItem(row,1,new QTableWidgetItem(QString(buf)));
        sprintf(buf,"0x%x",value);
        ui->t_addr->setItem(row,2,new QTableWidgetItem(QString(buf)));
        ui->t_addr->setItem(row,3,new QTableWidgetItem(QString::number(src)));
    }
}



