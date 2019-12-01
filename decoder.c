#include "decoder.h"
#include <math.h>
#include "parse_metafile.h"
#include "reader.h"
#include "spywriter.h"
#include "spyreader.h"


float cc(uint8_t i, uint8_t j)
{
    if(i == 0 && j == 0){
        return 1.0 / 2.0;
    }else if (i == 0 || j == 0){
        return 1.0 / sqrt(2.0);
    } else {
        return 1.0;
    }
}

uint8_t chomp(float x)
{
    if(x >= 255.0){
        return 255;
    }else if(x <= 0.0)
    {
        return 0;
    }else {
        return (uint8_t)round(x);
    }
}

const int ZZ[8][8] = {
    [0] =  {0,  1,  5,  6, 14, 15, 27, 28},
    [1] =  {2,  4,  7, 13, 16, 26, 29, 42},
    [2] =  {3,  8, 12, 17, 25, 30, 41, 43},
    [3] =  {9, 11, 18, 24, 31, 40, 44, 53},
    [4] =  {10, 19, 23, 32, 39, 45, 52, 54},
    [5] =  {20, 22, 33, 38, 46, 51, 55, 60},
    [6] =  {21, 34, 37, 47, 50, 56, 59, 61},
    [7] =  {35, 36, 48, 49, 57, 58, 62, 63}
};

int main()
{
    // read_jpgfile("./kaola.jpg");
    read_jpgfile("./kaola_out.jpg");
    // marker_detector();
    // JpegMetaData *jpegmetadata = data_reader();
    // printf("please enter the code that you want to encrypt")
    // encrypt("abcdef", "kaola_out.jpg");
    decrypt();
    
    return 0;
}