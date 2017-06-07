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
    connect(ui->t_addr,SIGNAL(cellClicked(int,int)),this,SLOT(show_addr_slot(int,int)));
    kernel->context_switch(error,NULL,kernel->pid_set[1],kernel->cpu_set[0],0);
    kernel->add_log("Starting Y86-OS simulator",Kernel::Style::IMPORTANT);
    refresh_file_list();
}

void MainWindow::clk(){

    kernel->cycle();
    cycle_ui();
    show_log();
}



void MainWindow::on_step_clicked()
{
    clk();
}

void MainWindow::on_add_cpu_clicked()
{
    kernel->add_cpu();
    hardware_tree_viewer();
}

void MainWindow::on_cmd_btn_clicked()
{
    QString text = ui->cmd_input->text();
    if(text.size()!=0){
        stdin_readline(text);
    }
}

void MainWindow::on_load_ascii_clicked()
{
    load_ascii_file();
    refresh_file_list();
}

void MainWindow::on_load_bin_clicked()
{
    load_binary_file();
    refresh_file_list();
}

void MainWindow::on_b_parse_elf_clicked()
{
    parse_elf_ui();
}

void MainWindow::on_b_run_clicked()
{
    speed = ui->s_speed->text().toInt();
    running = !running;
    autorun_handler();
}

void MainWindow::autorun_handler()
{

    while(running)
    {
        QElapsedTimer t;
        t.start();
        while(t.elapsed() * speed<10000)
            QApplication::processEvents();
        clk();
        if(!running)
        {
            running =0;
            break;
        }
    }

}

void MainWindow::on_b_add_core_clicked()
{
    current_vm->cpu->add_core();
    hardware_tree_viewer();
}

void MainWindow::on_checkBox_clicked()
{
    kernel->block_minor =  (ui->checkBox->checkState() == Qt::Checked);
}

void MainWindow::on_checkBox_2_clicked()
{
    kernel->block_normal = (ui->checkBox->checkState() == Qt::Checked);
}
