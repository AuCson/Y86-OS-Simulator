#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    startup();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::startup(){
    kernel = new Kernel();
    context_switch(error,NULL,kernel->pid_set[1],kernel->cpu_set[0],0);
}

void MainWindow::clk(){
    for(size_t i = 0;i<kernel->cpu_set.size();++i){
        CPU* cpu = kernel->cpu_set[i];
        for(int j = 0;j<cpu->corenum;++j){
            Core* core = cpu->core[j];
            core->single_run();
            cpu_viewer();

        }
    }
}



void MainWindow::on_step_clicked()
{
    clk();
}
