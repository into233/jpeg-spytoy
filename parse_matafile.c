#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
// #include "parse_matafile.h"


long filesize;
unsigned char *metafile_content = NULL;
long cursor = 0;

char* marker_info(unsigned char marker){
    char* info;
    switch(marker){
        case 0xE0:
            info = "APP0, JFIF 的额外信息";
            break;
        case 0xDB:
            info = "DQT, define quantization table, 量化表";
            break;
        case 0xC4:
            info = "DHT, define huffman table, 哈弗曼表";
            break;
        case 0xC0:
            info = "SOF0, start of frame(baseline)";
            break;
        case 0xDa:
            info = "SOS, start of scan, 压缩的图像数据";
            break;
        default:
            info = "Unknown marker";
            break;
    }
    return info;
}

unsigned long get_length_from_u16(long i){
    return (unsigned long)(metafile_content[i + 2] * 256) + (unsigned long)metafile_content[i + 3];
}

void marker_detector(){
    long length = 0;
    long t_cursor = cursor;

    for(cursor = 0;cursor < filesize - 1;cursor+=1){
        if(metafile_content[cursor] != 0xFF){
            // i+=1;
            continue;
        }

        unsigned char marker = metafile_content[cursor+1];
        length = get_length_from_u16(cursor);
        switch (marker)
        {
        case 0x00:
            continue;
            break;
        case 0x01://TEM can ignore
            continue;
            break;
        case 0xD8:
            printf("%08x SOI, start of image, 图像起始,长度为：%04x\n", cursor, length);
            length = 0;
            break;
        case 0xD9:
            printf("%08x EOI, end of image, 图片末尾\n", cursor);
            break;
        case 0xFF:
            break;
        default:
            printf("%08x marker:%2x, recognized as %s, 长度为：%04x\n", cursor, marker, marker_info(marker), length);
            break;
        }
    }
    cursor = t_cursor;
}

int read_matafile(char *metafile_name)
{
    long i;

    FILE *fp = fopen(metafile_name, "rb");
    if(fp == NULL){
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    filesize = ftell(fp);

    if(filesize == -1){
        printf("%s:%d fseek failed\n", __FILE__, __LINE__);
        return -1;
    }
    metafile_content = (unsigned char*)malloc(filesize+1);

    if(metafile_content == NULL){
        printf("%s:%d malloc failed\n", __FILE__, __LINE__);
        return -1;
    }

    fseek(fp, 0, SEEK_SET);
    for(i = 0;i < filesize;++i){
        metafile_content[i] = fgetc(fp);
    }
    metafile_content[i] = '\0';
    fclose(fp);
    printf("jpeg file's size is ：%ld\n", filesize);

    return 0;
}


 int find_keyword(char *keyword,long *position)
 {
	 long i;
	 *position=-1;
	 if(keyword==NULL)
		 return 0;
	 for(i=0;i<filesize-strlen(keyword);i++)
	 {
		 if(memcmp(&metafile_content[i],keyword,strlen(keyword))==0)
		 {
			 *position=i;
			 return 1;
		 }
	 }
	 return 0;
 }
 
 

int main(){
    read_matafile("./flower.jpg");
    marker_detector();

}