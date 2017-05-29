#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    startup();

    hardware_tree_viewer();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::startup(){
    kernel = new Kernel();
    kernel->context_switch(error,NULL,kernel->pid_set[1],kernel->cpu_set[0],0);
    kernel->add_log("Starting Y86-OS simulator",Kernel::Style::IMPORTANT);
}

void MainWindow::clk(){
    kernel->cycle();
    cpu_viewer(current_cpu_id,current_core_num);
    show_log();
}



void MainWindow::on_step_clicked()
{
    clk();
}
