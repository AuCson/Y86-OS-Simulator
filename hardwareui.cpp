#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <cstring>
#include <cstdio>
void MainWindow::hardware_tree_viewer(){
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

        ui->stacked->setCurrentIndex(1);

    }
    if(!strcmp("DRAM",type)){
        //debug_msg(item->text(0));
        ui->stacked->setCurrentIndex(0);
    }
}

void MainWindow::show_log(){
    ui->console->setText(kernel->log);
}


