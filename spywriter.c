#include "spywriter.h"
#include <fcntl.h>

extern size_t filesize;
extern unsigned char *metafile_content;
extern size_t cursor;
extern BitStream *bitstream;

size_t spy_cursor;
enum SPYMODE spymode = SPY_STILL;

void bit_ctrl_0(uint8_t* pflag, int bit)
{
    *pflag &= ~(1 << bit);
}
void bit_ctrl_1(uint8_t* pflag, int bit)
{
    *pflag |= (1 << bit);
}

//这里是从高位到低位写入字符.
void write_bit(){
    if((bitstream->spychars[bitstream->current_char_index] & (1 << (7 - bitstream->char_count--))) > 0)
    {
        bit_ctrl_1(metafile_content + spy_cursor, bitstream->bit_count);
    }else{
        bit_ctrl_0(metafile_content + spy_cursor, bitstream->bit_count);
    }
    
    if(bitstream->char_count == 0){
        bitstream->current_char_index = bitstream->current_char_index + 1;
        bitstream->char_count = 7;
    }
}
//TODO: 读取的时候添加一些事情:
//      1.读dc的时候就正常读.
//      2.读取ac的时候,一边读如果读到的是00, f0就直接什么都不做
//      有spy_current和cursor,还有read出来的value 所以如果有code_len以及cursor是可以做到修改任意位的.
//      具体算法就是for read_nu8() in (code_len % 8) 算出spy_cursor
//      然后根据spy_cursor write_value(spied_value)就ok了

// 倒退1bit 直接根据bitstream中的count来计算该写入那个字符的那个位
void retreat_write_value(uint8_t code_len)
{
    if(bitstream->current_char_index > bitstream->strlen){
        return;
    }
    bitstream->bit_count = bitstream->count == 0 ? 7 : bitstream->count - 1;
    spy_cursor = bitstream->count == 0 ? cursor - 1 : cursor;
    write_bit();
}

void encrypt(char *encryptStr, char* filename)
{
    spymode = SPY_ENCODE; 
    int len = sizeof(encryptStr);

    if(len > pow(2, 64)){
        jpgexit(STR_OUT_OF_LENGTH, __FILE__, __LINE__);
    }
    init_Bitstream();
    bitstream->strlen = len;
    bitstream->spychars = encryptStr;
    bitstream->current_char_index = 0;
    bitstream->char_count = 7;
    data_reader();

    int fd = open(filename, O_RDWR|O_CREAT|O_TRUNC, 06666);
    write(fd, metafile_content, filesize);
    printf("done!");
}