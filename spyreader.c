#include "spyreader.h"
#include "spywriter.h"

extern long filesize;
extern uint8_t *metafile_content;
extern long cursor;
extern BitStream *bitstream;

extern long spy_cursor;
extern enum SPYMODE spymode;
int reader_length_index = 15;

void rbit_ctrl_0(uint16_t *pflag, int bit)
{
    *pflag &= ~(1 << bit);
}
void rbit_ctrl_1(uint16_t *pflag, int bit)
{
    *pflag |= (1 << bit);
}

void cbit_ctrl_0(uint8_t *pflag, int bit)
{
    *pflag &= ~(1 << bit);
}
void cbit_ctrl_1(uint8_t *pflag, int bit)
{
    *pflag |= (1 << bit);
}

//这里是从高位到低位读入字符.
void read_bit()
{
    uint8_t value = *(metafile_content + spy_cursor);
    if (reader_length_index >= 0)
    {
        if ((value & (1 << (7 - bitstream->bit_count))) > 0)
        {
            rbit_ctrl_1(&(bitstream->strlen), reader_length_index--);
        }
        else
        {
            rbit_ctrl_0(&(bitstream->strlen), reader_length_index--);
        }
        if (reader_length_index < 0)
        {
            jpeg_assert(bitstream->strlen > 0, "解密的字符必须大于零");
            bitstream->spychars = (uint8_t *)malloc(sizeof(uint8_t) * bitstream->strlen + 1);
            bitstream->spychars[bitstream->strlen] = '\0';
        }
        return;
    }

    if ((value & (1 << (7 - bitstream->bit_count))) > 0)
    {
        cbit_ctrl_1(bitstream->spychars + bitstream->current_char_index, bitstream->char_count--);
    }
    else
    {
        cbit_ctrl_0(bitstream->spychars + bitstream->current_char_index, bitstream->char_count--);
    }

    if (bitstream->char_count < 0)
    {
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
void retreat_read_value(uint8_t code_len)
{
    if (bitstream->current_char_index >= bitstream->strlen && reader_length_index < 0)
    {
        return;
    }
    bitstream->bit_count = bitstream->count == 0 ? 7 : bitstream->count - 1;
    spy_cursor = cursor - 1;
    read_bit();
}

void decrypt()
{
    spymode = SPY_DECODE;

    init_Bitstream();
    bitstream->current_char_index = 0;
    bitstream->char_count = 7;
    bitstream->strlen = 0;

    data_reader();
    printf("done!\n");
    printf("the encrypted code is:%s", bitstream->spychars);
}