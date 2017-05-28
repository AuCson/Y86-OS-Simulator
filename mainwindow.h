#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "kernel.h"

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
    CPU *current_cpu = NULL;
    int current_core_num = 0;
    void cpu_viewer();

private slots:
    void on_step_clicked();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
