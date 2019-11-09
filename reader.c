#include "reader.h"
#include <stdio.h>
#include "jpegerror.h"
#include "spywriter.h"

extern long filesize;
extern unsigned char *metafile_content;
extern long cursor;
extern enum SPYMODE spymode; 

AppInfo *appinfo = NULL; 
BitStream *bitstream = NULL;
DHTInfo *dhtinfo = NULL;
SofInfo *sofinfo = NULL;
TableMapping *tablemapping = NULL;

uint8_t read_u8(){
    uint8_t t_char = '\0';
    t_char = metafile_content[cursor++];

    return t_char;
}

uint16_t read_u16(){
    uint8_t t_char[2] = {0};
    t_char[0] = metafile_content[cursor++];
    t_char[1] = metafile_content[cursor++];
    return (size_t)t_char[0] * 256 + t_char[1];
}

AppInfo* read_app0()
{
    size_t len = read_u16();
    printf("区段长度为：%04zu\n", len);

    AppInfo *appinfo;
    appinfo = (AppInfo*)malloc(sizeof(AppInfo));
    if(appinfo == NULL){
        return NULL;
    }
    //read identifier
    read_u8();
    read_u16();
    read_u16();

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

int add_dht_node(DHTRoot dhtroot, uint16_t code_word, int height, uint8_t source_symbol, int code){
    DHTNode *p = dhtroot;
    DHTNode *q = NULL;
    for(int i = 0;i < height;++i){
        if(q == NULL){
            q = (DHTNode*)malloc(sizeof(DHTNode));
            q->is_leaf = false;
            q->source_symbol = 0;
            q->leftNode = NULL;
            q->rightNode = NULL;
            q->h = 0;
            q->code = 0;
        }

        switch ((code_word >> (height - i - 1)) & 0x01)
        {
        case 0:
            if(p->leftNode == NULL){
                p->leftNode = q;
                q = NULL;
            }
            p = p->leftNode;
            break;
        case 1:
            if(p->rightNode == NULL){
                p->rightNode = q;
                q = NULL;
            }
            p = p->rightNode;
            break;
        default:
            jpgexit(UNKNOWN_ERROR, __FILE__, __LINE__);
            return -1;
        }
    }
    p->is_leaf = true;
    p->source_symbol = source_symbol;
    p->code = code;
    p->h = height;
    return 0;
}
//codeword
//这里的len实际的长度为len+1
uint16_t get_huffman_codeword(int len, int i, uint8_t height_info[])
{
    uint16_t code_word = 0;
    for(int mi = 0;mi<=len;++mi)
    {
        for(int mj = 0;mj < height_info[mi];++mj)
        {
            if(mi == len && i == mj){
                return code_word;
            }
            code_word += 1;
            if(mi == len && i == mj + 1){
                return code_word;
            }
        }
        
        code_word <<= 1;
    }
    return code_word;
}


DHTInfo* read_dht(){
    if(dhtinfo == NULL){
        dhtinfo = (DHTInfo*)malloc(sizeof(DHTInfo));
        dhtinfo->length = 0;
        for(int i = 0;i < 4;++i){
            dhtinfo->DhtTable[i] = NULL;
        }
    }

    uint16_t len = read_u16();
    printf("read_dht block length %hu bytes\n", len);
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
        dhtroot->leftNode = NULL;
        dhtroot->rightNode = NULL;
        dhtroot->is_leaf = false;
        dhtroot->h = 0;
        dhtroot->source_symbol = 0;
        dhtroot->code = 0;

        int code = 0;
        for(int i = 0;i < 16;++i){
            for(int j = 0;j < height_info[i];++j){
                uint8_t source_symbol = read_u8();
                add_dht_node(dhtroot, get_huffman_codeword(i, j, height_info), i + 1, source_symbol, code);
                len -= 1;
                code += 1;
            }
            code <<= 1;
        }
        m_dhttable->dhtroot = dhtroot;
        dhtinfo->DhtTable[dhtinfo->length++] = m_dhttable;
    }
    return dhtinfo;
}

DQTTable* read_dqt()
{
    size_t len = read_u16();

    printf("区块长度为%ld bytes\n", len);
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
            jpgexit(DQT_PRECISION_ERROR, __FILE__, __LINE__);
        }
        for(int i = 0;i < 8;++i){
            for(int j = 0;j < 8;++j){
                printf("%f ", table[i * 8 + j]);
            }
            printf("\n");
        }

        // *dqttable->tables[dqttable->table_length] = (float*)malloc(sizeof(float));
        (dqttable->tables[dqttable->table_length++]) = table;
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
    printf("区块长度为%ld bytes\n", len);

    SofInfo* sof_info = NULL;
    sof_info = (SofInfo*)malloc(sizeof(SofInfo));

    if(sof_info == NULL){
        printf("sof_info malloc error\n");
        return NULL;
    }
    sof_info->precision = read_u8();
    sof_info->height = read_u16();
    sof_info->width = read_u16();

    uint8_t number_of_component = read_u8();

    for(int i = 0;i < number_of_component;++i){
        //id 1 for Y, 2 for Cb, 3 for Cr
        uint8_t component_id = read_u8();
        sof_info->componentInfos[component_id - 1] = read_sof0_component();
    }
    uint8_t max_h_s=0, max_v_s=0;
    //颜色分量固定为3
    if(number_of_component != 3){
        jpgexit(INVALID_PARAMETER_ERR, __FILE__, __LINE__);
    }
    for(int i = 0;i < number_of_component - 1;++i){
        if(sof_info->componentInfos[i]->vertical_sampling>max_v_s){
            max_v_s = sof_info->componentInfos[i]->vertical_sampling;
        }
        if(sof_info->componentInfos[i]->horizontal_sampling>max_h_s){
            max_h_s = sof_info->componentInfos[i]->horizontal_sampling;
        }
    }
    sof_info->max_horizontal_sampling = max_h_s;
    sof_info->max_vertical_sampling = max_v_s;

    return sof_info;
}
// BitStream
void init_Bitstream()
{
    bitstream = (BitStream*)malloc(sizeof(BitStream));
    bitstream->buf = 0;
    bitstream->count = 0;
    // bitstream->last_dc;
}

uint8_t get_a_bit()
{
    if(bitstream->count == 0)
    {
        bitstream->buf = read_u8();
        if(bitstream->buf==0XFF){
            uint8_t check = read_u8();
            if(check != 0x00){
                printf("0x00 not in compose imagedata\n");
            }
        }
    }
    uint8_t ret = (bitstream->buf & (1 << (7 - bitstream->count))) > 0 ? 1 : 0;
    bitstream->count = bitstream->count == 7 ? 0 : bitstream->count + 1;
    return ret;
}
uint8_t HuffmanGetLength(DHTTable *dthtable, uint8_t huffman_len, uint16_t huffman_code){
    DHTRoot p = dthtable->dhtroot;

    for(size_t i = 1;i <= huffman_len;++i){
        switch ((huffman_code & (uint16_t)(1 << (huffman_len - i))) > 0 ? 1 : 0)
        {
        case 0:
            p = p->leftNode;
            break;
        case 1:
            p = p->rightNode;
            break;
        default:
            jpgexit(UNKNOWN_ERROR, __FILE__, __LINE__);
            break;
        }
    }
    if(p->is_leaf == 1){
        return p->source_symbol;
    }
    return 0xff;
}

/**
 * return the length encoded by the huffmantable.
 * (h + 1), code, symbol code
*/
uint8_t matchHuffman(DHTTable *dhttable) {
    uint16_t code = 0;
    uint8_t len = 1;

    uint8_t ret;
    while(true){
        code = code << 1;
        code += (uint16_t)get_a_bit();
        ret = HuffmanGetLength(dhttable, len, code);
        if(ret != 0xff){
            return (uint8_t)ret;
        }
        len += 1;
        if(len > 16){
            jpgexit(INVALID_HUFFMAN_CODE_ERROR, __FILE__, __LINE__);
        }
    }
}
double read_value(uint8_t code_len)
{
    double ret = 1;
    uint8_t first = get_a_bit();

    for(size_t i = 1;i < code_len;++i){
        uint8_t b = get_a_bit();
        ret = ret * 2;
        //2019年11月06日01:14:54 TODO!!!
        ret += ((first == b) ? 1 : 0);
    }
    ret = first == 1 ? ret : -ret;
    return (double)ret;
}
double read_dc(DHTTable *dhttable, int id){
    uint8_t code_len = matchHuffman(dhttable);
    if(code_len==0){
        return bitstream->last_dc[id];
    }
    bitstream->last_dc[id] += read_value(code_len);
    return bitstream->last_dc[id];
}
int read_ac(DHTTable *dhttable){
    uint8_t code_len = matchHuffman(dhttable);
    switch(code_len){
        case 0x00:
            return AllZeros;
        case 0xF0:
            return SixteenZeros;
        default:{
            bitstream->zeros = (size_t)(code_len>>4);
            bitstream->value = read_value(code_len & 0x0F);
            //TODO!!!HERE retreat_write_value(code_len & 0x0F)
            switch (spymode)
            {
            case SPY_STILL:
                break;
            case SPY_ENCODE:
                retreat_write_value(code_len);
                break;
            default:
                break;
            }
            return Normal;
        }
    }
}


float readBlocks(Block* blocks, int width, int height, int w, int h, int count)
{
    return *(blocks + h * width * 8 * 8 + w * 8 * 8 + (count / 8) * 8 + (count % 8));
}
void setBlocks(Block* blocks, int width, int height, int w, int h, int count, Block value)
{
    *(blocks + h * width * 8 * 8 + w * 8 * 8 + (count / 8) * 8 + (count % 8)) = value;
    printf("%d ", (int)value);
    if(count % 8 == 7){
        printf("\n");
    }
}

DHTTable* get_dhttable(DHTInfo* dhtinfo, uint8_t AC_DC,int id){
    for(int i = 0;i < 4;++i){
        if(dhtinfo->DhtTable[i]->ac_dc == AC_DC && dhtinfo->DhtTable[i]->id == id){
            return dhtinfo->DhtTable[i];
        }
    }
    return NULL;
}


MCU* read_mcu(BitStream *bits, JpegMetaData *jpeg_meta_data)
{
    // ComponentInfo *component_info = jpeg_meta_data->sof_info->componentInfos[0];
    TableMapping *table_mapping = jpeg_meta_data->table_mapping;

    //MCU
    MCU *mcu = (MCU*)malloc(sizeof(MCU));
    mcu->blocks = (Block **)malloc(sizeof(Block**) * 3);

    for(int id = 0;id < 3;++id)
    {
        uint8_t height = jpeg_meta_data->sof_info->componentInfos[id]->vertical_sampling;
        uint8_t width = jpeg_meta_data->sof_info->componentInfos[id]->horizontal_sampling;
        Block *blocks = (Block*)malloc(sizeof(Block) * height * width * 8 * 8);
        DHTTable *dc_table = get_dhttable(jpeg_meta_data->dht_info, DC, table_mapping->dc_ids[id]);
        // DHTTable *ac_table = jpeg_meta_data->dht_info->DhtTable[table_mapping->ac_ids[id]];
        DHTTable *ac_table = get_dhttable(jpeg_meta_data->dht_info, AC, table_mapping->ac_ids[id]);


        for(int h = 0;h < height;++h)
        {
            for(int w = 0;w < width;++w)
            {
                double dc_value = read_dc(dc_table, id);
                *(blocks + (h * width * 8 * 8)) = dc_value;
                printf("\n%d ", (int)dc_value);
                int count = 1;
                while(count < 64)
                {
                    int ac_value = read_ac(ac_table);
                    switch (ac_value)
                    {
                    case SixteenZeros:
                        for(int slash = 0;slash < 16;++slash){
                            // *(blocks + h * width * 8 * 8 + w * 8 * 8 + (count / 8) * 8 + (count % 8)) = 0.0;
                            setBlocks(blocks, width, height, w, h, count, 0.0);
                            count +=1;
                        }
                        break;
                    case AllZeros:
                        while(count < 64){
                            setBlocks(blocks, width, height, w, h, count, 0.0);
                            // *(blocks + h * width * 8 * 8 + w * 8 * 8 + (count / 8) * 8 + (count % 8)) = 0.0;
                            count += 1;
                        }
                        break;
                    case Normal:
                        for(int zeros = 0;zeros < bits->zeros;++zeros)
                        {
                            setBlocks(blocks, width, height, w, h, count, 0.0);
                            // *(blocks + h * width * 8 * 8 + w * 8 * 8 + (count / 8) * 8 + (count % 8)) = 0.0;
                            count++;                            
                        }
                            setBlocks(blocks, width, height, w, h, count, bits->value);
                        // *(blocks + h * width * 8 * 8 + w * 8 * 8 + (count / 8) * 8 + (count % 8)) = bits->value;
                        count += 1;
                        break;
                    default:
                        jpgexit(INVALID_SWITCH_ERROR, __FILE__, __LINE__);
                        break;
                    }
                }
            }

        }
        mcu->blocks[id] = blocks;
    }
    return mcu;
}

MCUS* read_mcus(JpegMetaData *jpegdata){
    if(bitstream == NULL){
        init_Bitstream();
        printf("init_Bitstream()\n");
    }
    SofInfo *sof_info = jpegdata->sof_info;
    int image_width = sof_info->width;
    int image_height = sof_info->height;

    int w = (image_width - 1) / (8 * sof_info->max_horizontal_sampling) + 1;
    int h = (image_height - 1) / (8 * sof_info->max_vertical_sampling) + 1;
    printf("宽度上有%d个MCU, 高度上有%d个MCU\n", w, h);

    MCUS *mcus = (MCUS*)malloc(sizeof(MCUS*));
    mcus->w = w;
    mcus->h = h;
    mcus->mcu = (MCU**)malloc(sizeof(MCU) * w * h);

    for(int i = 0;i < h;++i){
        for(int j = 0;j < w;++j){
            *(mcus->mcu + i * w + j) = read_mcu(bitstream, jpegdata);
        }
    }
    return mcus;
}
char* component_name(uint8_t id)
{
    switch(id)
    {
        case (1):
            return "Y";
        case (2):
            return "Cb";
        case (3):
            return "Cr";
        default:
            printf("Unknown color id:%hhu", id);
            jpgexit(INVALID_SWITCH_ERROR, __FILE__, __LINE__);
    }
    jpgexit(INVALID_SWITCH_ERROR, __FILE__, __LINE__);
    return "";
}

TableMapping* read_sos()
{
    uint16_t len = read_u16();
    printf("区块长度为%hu\n", len);
    
    TableMapping *tablemapping = (TableMapping*)malloc(sizeof(TableMapping));

    uint8_t component_number = read_u8();
    if(component_number != 3)
        jpgexit(INVALID_PARAMETER_ERR, __FILE__, __LINE__);

    for(int i = 0;i < 3;++i){
        //1表示Y, 2表示Cb, 3表示Cr
        uint8_t component = read_u8();
        // id高四位代表直流(DC)哈弗曼表, 第四位代表交流(AC)哈弗曼表
        uint8_t id = read_u8();
        uint8_t dc_id = id >> 4;
        uint8_t ac_id = id & 0x0F;
        printf("%s颜色分量, 直流哈弗曼表id=%hhu, 交流哈弗曼表id=%hhu\n", component_name(component), dc_id, ac_id);
        tablemapping->ac_ids[component-1] = ac_id;
        tablemapping->dc_ids[component-1] = dc_id;
    }
    uint8_t c = read_u8();
    jpeg_assert(c == 0x00, "read_sos c");
    c = read_u8();
    jpeg_assert(c == 0x3F, "read_sos c");
    c = read_u8();
    jpeg_assert(c == 0x00, "read_sos c");

    return tablemapping;
}
JpegMetaData* data_reader()
{
    JpegMetaData* jpeg_meta_data = (JpegMetaData*)malloc(sizeof(JpegMetaData));
    MCUS *mcus = NULL;

    while(1){
        if(cursor > filesize){
            break;
        }
        uint8_t c = read_u8();
        if(c != MARKER_PREFIX)
        {
            continue;
        }
        
        c = read_u8();
        switch(c){
            case SOI_MARKER:
                printf("---------扫描SOI, 图片起始----------\n");
                break;
            case EOI_MARKER:
                printf("---------扫描SOI, 图片结束----------\n");
                if(c < filesize - 10)break;//TODO:处理有缩略图的情况, 忽略的话要吧下面的几个函数free一下
                return jpeg_meta_data;
            case SOF0_MARKER:
                //TODO: free 缩略图
                sofinfo = read_sof0();
                jpeg_meta_data->sof_info = sofinfo;
                printf("--------------扫描SOF--------------\n");
                break;
            case SOS_MARKER:
                printf("--------------扫描SOS--------------\n");
                //TODO: free 缩略图
                tablemapping = read_sos();
                jpeg_meta_data->table_mapping = tablemapping;
                jpeg_meta_data->MCUS_CURSOR = cursor;
                mcus = read_mcus(jpeg_meta_data);
                printf("--------------扫描SOS结束 %08x--------------\n", cursor);
                break;
            case APP0_MARKER:
                printf("--------------扫描 APP0------------\n");
                jpeg_meta_data->app_info = read_app0();
                break;
            case DHT_MARKER:
                printf("--------------扫描 DHT --------------\n");
                jpeg_meta_data->dht_info = read_dht();
                break;
            case DQT_MARKER:
                printf("--------------扫描 DQT --------------\n");
                jpeg_meta_data->dqt_table = read_dqt();
                break;
            default:
                printf("other marker:%hhu\n", c);
                break;
        }
    }
    return jpeg_meta_data;
}