#ifndef READER_H
#include <stdint.h>
#include <ctype.h>
#include <float.h>
#include <stdlib.h>
#include <stdbool.h>

const uint8_t MARKER_PREFIX = 0xFF;
const uint8_t SOI_MARKER = 0xD8;
const uint8_t EOI_MARKER = 0xD9;

const uint8_t APP0_MARKER = 0xE0;

const uint8_t DQT_MARKER = 0xDB;
const uint8_t DHT_MARKER = 0xC4;


const uint8_t SOF0_MARKER = 0xC0;
const uint8_t SOS_MARKER = 0xDA; 

const uint8_t DC = 0x00;
const uint8_t AC = 0x01;

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
    uint8_t code;
    
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
    float **tables[64];
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


#endif