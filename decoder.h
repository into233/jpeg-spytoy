#include "reader.h"
#define PI 3.1415926

typedef struct Pixel
{
    uint8_t R;
    uint8_t G;
    uint8_t B;
} Pixel;

typedef struct Image
{
    int height;
    int width;
    struct Pixel *pixels;
} Image;

void print_test(MCU *mcu, char* msg);
