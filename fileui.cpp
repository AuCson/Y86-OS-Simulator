#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <string>
#include <cstdio>

void MainWindow::load_binary_file(){
    QString filename = QFileDialog::getOpenFileName(this);
    char buf[1000];
    strcpy(buf,filename.toStdString().c_str());
    FILE* fp = fopen(buf,"rb");
    if(!fp)
        return;
    std::string long_filename = filename.toStdString();
    std::string short_filename;
    int pos = 0;
    for(int i = long_filename.size()-1;i>=0;--i){
        if(long_filename[i] == '/'){
            pos = i;
            break;
        }
    }
    for(int i = pos+1;i<long_filename.size();++i){
        short_filename.push_back(long_filename[i]);
    }
    //add file to vnode_list
    Vnode* vnode = new Vnode();
    strcpy(vnode->filename,short_filename.c_str());

    while(fread(&vnode->file_content[vnode->filesize],sizeof(unsigned char),1,fp)){
        vnode->filesize++;
        if(vnode->filesize == 10000)
            break;
    }
    vnode->filetype = Vnode::BIN;
    kernel->vnode_list.push_back(vnode);
}

void MainWindow::load_ascii_file(){
    QString filename = QFileDialog::getOpenFileName(this);
    char buf[1000];
    strcpy(buf,filename.toStdString().c_str());
    FILE* fp = fopen(buf,"rb");
    if(!fp)
        return;
    std::string long_filename = filename.toStdString();
    std::string short_filename;
    int pos = 0;
    for(int i = long_filename.size()-1;i>=0;--i){
        if(long_filename[i] == '/'){
            pos = i;
            break;
        }
    }
    for(int i = pos+1;i<long_filename.size();++i){
        short_filename.push_back(long_filename[i]);
    }
    //add file to vnode_list
    Vnode* vnode = new Vnode();
    strcpy(vnode->filename,short_filename.c_str());

    while(fread(&vnode->file_content[vnode->filesize],sizeof(unsigned char),1,fp)){
        vnode->filesize++;
        if(vnode->filesize == 10000)
            break;
    }
    vnode->filetype = Vnode::ASCII;
    kernel->vnode_list.push_back(vnode);
}

void MainWindow::refresh_file_list(){
    ui->list_file->clear();
    for(auto i = kernel->vnode_list.begin();i!=kernel->vnode_list.end();++i){
        ui->list_file->addItem(QString((*i)->filename));
    }
    connect(ui->list_file,SIGNAL(itemClicked(QListWidgetItem*)),this,SLOT(load_file_info(QListWidgetItem*)));
}

void MainWindow::stdin_readline(QString cmd){
    Vnode* node = kernel->vnode_list[0];
    std::string cmd_std = cmd.toStdString();
    for(int i =0;i<cmd_std.size();++i){
        if(node->filesize >= 10000)
            break;
        node->file_content[node->filesize++] = cmd_std[i];
    }
    return;
}

QString MainWindow::bin_to_ascii(char *bin,int len){
    QString s;
    for(int j=0;j<len;++j){
        char xbuf[3];
        sprintf(xbuf,"%x",(unsigned char)bin[j]);
        if(strlen(xbuf)==1){
            xbuf[2] = xbuf[1];
            xbuf[1] = xbuf[0];
            xbuf[0] = '0';
        }
        s.append(xbuf);
    }
    return s;
}

char* MainWindow::ascii_to_bin(QString ascii_qstr){
    std::string ascii = ascii_qstr.toStdString();
    int len = ascii.size() / 2;
    char* res = new char[len+1];
    for(int i =0;i<2*len;i+=2){
        char xbuf[3];
        xbuf[0] = ascii[i];
        xbuf[1] = ascii[i+1];
        xbuf[2] = 0;
        int t;
        sscanf(xbuf,"%x",&t);
        res[i/2] = (char)t;
    }
    res[len] = 0;
    return res;
}

void MainWindow::load_file_info(QListWidgetItem *item){
    std::string filename = item->text().toStdString();
    ui->stacked->setCurrentIndex(2);
    for(auto &i:kernel->vnode_list){
        if(!strcmp(filename.c_str(),i->filename)){
            Vnode* vnode = i;
            ui->l_filename->setText(vnode->filename);
            ui->l_file_size->setText(QString::number(vnode->filesize));
            char *file_type;
            if(vnode->filetype == Vnode::ASCII)
                file_type = "ASCII";
            else
                file_type = "BINARY";
            ui->l_file_type->setText(file_type);
            //fix \0 if possible
            if(vnode->filetype == Vnode::ASCII){
                vnode->file_content[vnode->filesize] = 0;
                ui->file_content->setText((char*)(vnode->file_content));
            }
            else if(vnode->filetype == Vnode::BIN){
                ui->file_content->clear();
                QString s;
                s = bin_to_ascii((char*)(vnode->file_content),vnode->filesize);
                ui->file_content->setText(s);
            }
        }
    }
}

void MainWindow::parse_elf_ui(){
    QString text = ui->file_content->toPlainText();
    BYTE* buf = (BYTE*)ascii_to_bin(text);

    ELF *elf = new ELF();
    //try{
        elf->parse_from_binary(buf,10010);
    //}
    //catch(...){
    //    debug_msg("ELF invalid");
     //   return;
    //}
    char xbuf[3];
    QString res;
    res += "<b>.text\n</b>";
    res += bin_to_ascii(elf->text,elf->text_len);
    res += "<b>\n.data\n</b>";
    res += bin_to_ascii(elf->data,elf->data_len);
    res += "<b>\n.rodata\n</b>";
    res += bin_to_ascii(elf->rodata,elf->rodata_len);
    res += "<b>\n.bss\n</b>";
    ui->elf_parsed->setText(res);
}

