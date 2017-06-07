#ifndef ELF_H
#define ELF_H
#include "bit.h"
#include "stdio.h"
#include <string>
class ELF{
public:
    char *text = NULL;
    char *data = NULL;
    char *rodata = NULL;
    int text_len,data_len,rodata_len,bss_len;

    std::string string_text;

    void parse_from_binary(BYTE* buf,int buf_len);
    char* ascii_to_bin(std::string ascii);
    std::string bin_to_ascii(char *bin,int len);
    void load_from_strings(std::string _text,std::string _data,std::string _rodata){
        text = new char[_text.size()];
        data = new char[_data.size()];
        rodata = new char[_rodata.size()];
        string_text = _text;
        text_len = _text.size()/2;
        data_len = _data.size()/2;
        rodata_len = _rodata.size()/2;
        for(size_t i=0;i<_text.size();i+=2){
            char buf[3];
            buf[0] = _text[i];
            buf[1] = _text[i+1];
            buf[2] = 0;
            int t;
            sscanf(buf,"%x",&t);
            text[i/2] = (char)t;
        }
        for(size_t i=0;i<_data.size();i+=2){
            char buf[3];
            buf[0] = _data[i];
            buf[1] = _data[i+1];
            buf[2] = 0;
            int t;
            sscanf(buf,"%x",&t);
            data[i/2] = (char)t;
        }
        for(size_t i=0;i<_rodata.size();i+=2){
            char buf[3];
            buf[0] = _rodata[i];
            buf[1] = _rodata[i+1];
            buf[2] = 0;
            int t;
            sscanf(buf,"%x",&t);
            rodata[i/2] = (char)t;
        }
    }
};

#endif // ELF_H
