#ifndef READER_H
#define READER_H
#include <stdint.h>
#include <ctype.h>
#include <float.h>
#include <stdlib.h>
#include <stdbool.h>

#define MARKER_PREFIX 0xFF
#define SOI_MARKER 0xD8
#define EOI_MARKER 0xD9

#define APP0_MARKER 0xE0

#define DQT_MARKER 0xDB
#define DHT_MARKER 0xC4


#define SOF0_MARKER 0xC0
#define SOS_MARKER 0xDA 

#define DC 0x00
#define AC 0x01

enum AcValue{
    SixteenZeros,
    AllZeros,
    Normal
};

typedef struct _App_Info
{
    uint8_t version_major_id;
    uint8_t version_minor_id;

    uint8_t units;

    uint16_t x_density;
    uint16_t y_density;

    uint8_t x_thumbnail;
    uint8_t y_thumbnail;
}AppInfo;


typedef struct _DHT_Node
{
    bool is_leaf;
    uint8_t source_symbol;
    int code;
    int h;

    
    struct _DHT_Node *leftNode;
    struct _DHT_Node *rightNode;
}DHTNode, *DHTRoot;

typedef struct _DHT_Table
{
    uint8_t id;
    uint8_t ac_dc;

    DHTRoot dhtroot;
}DHTTable;

// nomally it has four tables
// [0] = lightA
typedef struct _DHTInfo
{
    DHTTable *DhtTable[4];
    int length;
}DHTInfo;

typedef struct _DQT_table
{
    float *tables[4];
    int *ids;
    size_t table_length;
}DQTTable;

typedef struct _Component_Info
{
    uint8_t horizontal_sampling;
    uint8_t vertical_sampling;
    uint8_t quant_table_id;

}ComponentInfo;

typedef struct _Sof_Info
{
    uint8_t precision;
    size_t height;
    size_t width;
    ComponentInfo *componentInfos[3];
    uint8_t max_vertical_sampling;
    uint8_t max_horizontal_sampling;
}SofInfo;

enum ACORDC{
    AOD_Y,
    AOD_Cb,
    AOD_Cr
};
//Y, Cr, Cb颜色分量分别使用哪个id的表 默认为Y, Cb, Cr
typedef struct
{
    uint8_t ac_ids[3];
    uint8_t dc_ids[3];
}TableMapping;


typedef struct _Bit_Stream{
    uint8_t buf;
    uint8_t count;
    double last_dc[3];

    //AcValue
    size_t zeros;
    double value;
}BitStream;


typedef float Block;


typedef struct _MCU
{
    Block **blocks;

}MCU;

typedef struct
{
    MCU **mcu;
    int w;
    int h;
}MCUS;

typedef struct
{
    AppInfo* app_info;
    SofInfo* sof_info;
    DQTTable* dqt_table;
    TableMapping* table_mapping;
    DHTInfo* dht_info;
    MCUS* mcus;
}JpegMetaData;


JpegMetaData* data_reader();
TableMapping* read_sos();
char* component_name(uint8_t id);
MCUS* read_mcus(JpegMetaData *jpegdata);
MCU* read_mcu(BitStream *bits, JpegMetaData *jpeg_meta_data);
void setBlocks(Block* blocks, int width, int height, int w, int h, int count, Block value);
float readBlocks(Block* blocks, int width, int height, int w, int h, int count);
int read_ac(DHTTable *dhttable);
double read_dc(DHTTable *dhttable, int id);
double read_value(uint8_t code_len);
uint8_t matchHuffman(DHTTable *dhttable);
uint8_t HuffmanGetLength(DHTTable *dthtable, uint8_t huffman_len, uint16_t huffman_code);
uint8_t get_a_bit();
void init_Bitstream();
SofInfo* read_sof0();
ComponentInfo* read_sof0_component();
DQTTable* read_dqt();
DHTInfo* read_dht();
uint16_t get_huffman_codeword(int len, int i, uint8_t height_info[]);
int add_dht_node(DHTRoot dhtroot, uint16_t code_word, int height, uint8_t source_symbol, int code);
AppInfo* read_app0();
uint16_t read_u16();
uint8_t read_u8();


#endif
