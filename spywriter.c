#include "spywriter.h"

extern size_t filesize;
extern unsigned char *metafile_content;
extern size_t cursor;

size_t spy_cursor;

//TODO:就是读取的时候添加一些事情:
//      1.读dc的时候就正常读.
//      2.读取ac的时候,一边读如果读到的是00, f0就直接什么都不做
//      我有spy_current和cursor,还有read出来的value 所以如果有code_len以及cursor应该是可以做到修改任意位的.
//      具体算法就是for read_nu8() in (code_len % 8) 算出spy_cursor
//      然后根据spy_cursor write_value(spied_value)就ok了

// 以bit为单位倒退当前bitstream code_len bit
void retreat_write_value(uint8_t code_len, uint8_t bit_1o0)
{

}


void encrypt(char *encryptStr)
{
    int len = sizeof(encryptStr);

    if(len > pow(2, 64)){
        jpgexit(STR_OUT_OF_LENGTH, __FILE__, __LINE__);
    }



}