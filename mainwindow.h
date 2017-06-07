#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "kernel.h"
#include <QMessageBox>
#include <QElapsedTimer>

class QTreeWidgetItem;
class QListWidgetItem;
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:


    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    int error;
    Kernel *kernel;
    void init();
    void startup();
    void clk();

    void disass(std::string s);
    void f_pc_show(int noupdate, Core *Logic);
    void reg_log(Core* core);

    //CPU viewer
    int current_cpu_id = -1;
    int current_core_num = 0;
    VM* current_vm;
    int running = 0;
    int speed = 200;


    void cycle_ui();
    void cpu_viewer(int cpu_id, int corenum);
    void hardware_tree_viewer();
    void print_stdout();
    void vm_area_log(VM* vm);
    void stdin_readline(QString s);
    void status_log(VM* vm);

    void debug_msg(QString s){
        QMessageBox box(QMessageBox::Warning,"Debug",s);
        box.exec();
    }

    QString log;
    void show_log();
    void load_binary_file();
    void load_ascii_file();
    void refresh_file_list();
    QString bin_to_ascii(char *bin,int len);
    char* ascii_to_bin(QString ascii_qstr);
    void parse_elf_ui();
    void autorun_handler();
private slots:
    void on_step_clicked();
    void alter_cpu_core_view(QTreeWidgetItem *item, int r=0);
    void on_add_cpu_clicked();
    void show_addr_slot(int row,int col);

    void on_cmd_btn_clicked();
    void load_file_info(QListWidgetItem* item);

    void on_load_ascii_clicked();

    void on_load_bin_clicked();

    void on_b_parse_elf_clicked();

    void on_b_run_clicked();

    void on_b_add_core_clicked();

    void on_checkBox_clicked();

    void on_checkBox_2_clicked();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
