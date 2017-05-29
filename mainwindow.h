#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "kernel.h"
#include <QMessageBox>

class QTreeWidgetItem;
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

    //CPU viewer
    int current_cpu_id = -1;
    int current_core_num = 0;
    void cpu_viewer(int cpu_id, int corenum);
    void hardware_tree_viewer();

    void debug_msg(QString s){
        QMessageBox box(QMessageBox::Warning,"Debug",s);
        box.exec();
    }

    QString log;
    void show_log();

private slots:
    void on_step_clicked();
    void alter_cpu_core_view(QTreeWidgetItem *item, int r=0);
private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
