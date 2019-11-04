#include "reader.h"
#include <stdio.h>
#include <stdlib.h>
#include "jpegerror.h"
extern long filesize;
extern char *metafile_content;
extern long cursor;
AppInfo *appinfo = NULL; 
BitStream *bitstream = NULL;
DHTInfo *dhtinfo = NULL;

uint8_t read_u8(){
    uint8_t t_char = '\0';
    t_char = metafile_content[cursor++];

    return t_char;
}

size_t read_u16(){
    uint8_t t_char[2] = {0};
    t_char[0] = metafile_content[cursor++];
    t_char[1] = metafile_content[cursor++];
    return (size_t)t_char[0] * 256 + c[1];
}

AppInfo* read_app0()
{
    size_t len = read_u16();
    printf("区段长度为：%04x", len);

    AppInfo *appinfo;
    appinfo = (AppInfo*)malloc(sizeof(AppInfo));
    if(appinfo == NULL){
        return NULL;
    }
    appinfo->version_major_id = read_u8();
    appinfo->version_minor_id = read_u8();

    appinfo->units = read_u8();

    appinfo->x_density = read_u16();
    appinfo->y_density = read_u16();

    appinfo->x_thumbnail = read_u8();
    appinfo->y_thumbnail = read_u8();

    size_t thumbail_length = 3 * (size_t)(appinfo->x_thumbnail) * (appinfo->y_thumbnail);
    cursor += thumbail_length;

    return appinfo;
}

int add_dht_node(DHTRoot dhtroot, uint16_t code_word, int height, uint8_t code){
    DHTNode *p = dhtroot;
    DHTNode *q;
    for(int i = 0;i < height;++i){
        q = (DHTNode*)malloc(sizeof(DHTNode));
        q->is_leaf = false;

        switch ((m_code << (height - i - 1)) & 0x0001)
        {
        case 0:
            if(p->leftNode == NULL){
                p->leftNode = q;
            }else free(q);
            p = p->leftNode;
            break;
        case 1:
            if(p->rightNode == NULL){
                p->rightNode = q;
            }else free(q);
            p = p->rightNode;
            break;
        default:
            return -1;
        }
    }
    p->is_leaf = true;
    p->code = code;
    return 0;
}
//codeword

uint16_t get_huffman_codeword(int len, int i, uint8_t height_info[])
{
    uint16_t code_word = 0;
    for(int mi = 0;mi<=len;++mi)
    {
        for(int mj = 0;mj < height_info[mi];++mj)
        {
            code_word += 1;
        }
        code_word += 1;
        code_word <<= 1;
    }
    return code_word;
}


DHTInfo* read_dht(){
    dhtinfo = (DHTINFO*)malloc(sizeof(DHTINFO));
    dhtinfo->length = 0;

    uint16_t len = read_u16();
    printf("read_dht block length %zu bytes", len);
    len -=  2;
    while(len > 0){
        DHTTable *m_dhttable = NULL;
        m_dhttable = (DHTTable*)malloc(sizeof(DHTTable));

        uint8_t c = read_u8();
        m_dhttable->ac_dc = c >> 4;
        m_dhttable->id = c & 0x0F;
        DHTRoot dhtroot;
        uint8_t height_info[16];
        len -= 17;

        for(int i = 0;i < 16;++i){
            height_info[i] = read_u8();
        }

        dhtroot = (DHTNode*)malloc(sizeof(DHTNode));
        for(int i = 0;i < 16;++i){
            for(int j = 0;j < height_info[i];++j){
                uint8_t source_symbol = read_u8();
                add_dht_node(dhtroot, get_huffman_codeword(i, j, height_info), height_info[i])
                len -= 1;
            }
        }
        m_dhttable->dhtroot = dhtroot;
        dhtinfo->DhtTable[(dhtinfo->length)] = m_dhttable;
    }
}

DQTTable* read_dqt()
{
    size_t len = read_u16();
    int dqt_index = 0;

    printf("区块长度为%ld bytes", len);
    len -= 2;

    DQTTable *dqttable;
    dqttable = (DQTTable*)malloc(sizeof(DQTTable));
    dqttable->table_length = 0;
    while(len > 0){
        uint8_t c = read_u8();
        uint8_t id = c & 0x0F;
        uint8_t precision = c >> 4;
        printf("量化表 %c, 精度为 %c\n", id, precision);

        float *table = (float*)malloc(sizeof(float) * 64);
        if(precision == 0){
            for(int i = 0;i <= 64;++i){
                table[i] = (float)read_u8();
            }
            len -= 65;
        } else if(precision == 1){
            for(int i = 0;i <= 65;++i){
                table[i] = (float)read_u16();
            }
            len -= 129;
        }else{
            printf("量化表%c的精度为%c，不符合规范\n", id, precision);
        }
        for(int i = 0;i <= 8;++i){
            for(int j = 0;j < 8;++j){
                printf("%f ", table[i * 8 + j])
            }
            printf("\n");
        }

        dqttable->table_length++;
        if(dqttable->table_length == 1)
            dqttable->tables = (float*)malloc(sizeof(float));
        else
            dqttable->tables = (float*)realloc(sizeof(float) * dqttable->table_length);
        dqttable[dqttable->table_length-1] = table;
    }
    return dqttable;
}

ComponentInfo* read_sof0_component()
{
    ComponentInfo* componentinfo = NULL;
    componentinfo = (ComponentInfo*)malloc(sizeof(ComponentInfo));
    
    uint8_t c = read_u8();
    componentinfo->horizontal_sampling = c>>4;
    componentinfo->vertical_sampling = c&0x0F;
    componentinfo->quant_table_id = read_u8();

    return componentinfo;
}

SofInfo* read_sof0(){
    size_t len = read_u16();
    printf("区块长度为%ld bytes", len);

    SofInfo* sof_info = NULL;
    sof_info = (SofInfo*)malloc(sizeof(SofInfo));

    if(sof_info == NULL){
        printf("sof_info malloc error");
        return NULL;
    }
    sof_info->precision = read_u8();
    sof_info->height = read_u16();
    sof_info->width = read_u16();

    uint8_t number_of_component = read_u8();

    for(int i = 0;i <= number_of_component;++i){
        uint8_t component_id = read_u8();
        sof_info->componentInfos[component_id - 1] = read_sof0_component();
    }
    uint8_t max_h_s=0, max_v_s=0;

    for(int i = 0;i < 3;++i){
        if(sof_info->componentInfos[i]->vertical_sampling>max_v_s){
            max_v_s = sof_info->componentInfos[i]->vertical_sampling;
        }
        if(sof_info->componentInfos[i]->horizontal_sampling>max_h_s){
            max_h_s = sof_info->componentInfos[i]->horizontal_sampling;
        }
    }
}
// BitStream
void init_Bitstream()
{
    bitstream = (BitStream*)malloc(sizeof(BitStream));
    bitstream->buf = 0;
    bitstream->count = 0;
    bitstream->last_dc = {0};
}

uint8_t get_a_bit()
{
    if(bitstream == NULL){
        init_Bitstream();
    }
    if(bitstream->count == 0)
    {
        bitstream->count = read_u8();
        if(bitstream->buf==0XFF){
            uint8_t check = read_u8();
            if(check == 0x00){
                printf("0x00 not in compose imagedata");
            }
        }
    }
    uint8_t ret = bitstream->buf & (1 << (7 - bitstream->count)) > 0 ? 1 : 0;
    bitstream->count = bitstream->count == 7 ? 0 : bitstream->count + 1;
    return ret;
}

uint8_t matchHuffman() {
    uint16_t code = 0;
    uint8_t len = 1;

    uint8_t ret;
    while(true){
        code = code << 1;
        code += (uint16_t)get_a_bit();
        //TODO:HuffmanGet
        ret = HuffmanGet(len, code);
        if(ret != -1){
            return ret;
        }
        len += 1;
        if(len > 16){
            jpgexit(INVALID_HUFFMAN_CODE_ERROR, __FILE__, __LINE__);
        }
    }
}
double read_value(uint8_t code_len)
{
    int16_t ret = 1;
    uint8_t first = get_a_bit();

    for(size_t i = 1;i < code_len;++i){
        uint8_t b = get_a_bit();
        ret = ret<<1;
        ret += first == b ? 1 : 0;
    }
    ret = first == 1 ? ret : -ret;
    return (double)ret;
}
double read_dc(size_t id){
    uint8_t code_len = matchHuffman();
    if(code_len==0){
        return bitstream->last_dc[id];
    }
    bitstream->last_dc[id] += read_value(code_len);
    return bitstream->last_dc[id];
}
int read_ac(){
    uint8_t code_len = matchHuffman();
    switch(code_len){
        case 0x00:
            return AllZeros;
        case 0xF0:
            return SixteenZeros;
        default:{
            bitstream->zeros = (size_t)(x>>4);
            bitstream->value = read_value(x & 0x0F);
            return Normal;
        }
    }
}
