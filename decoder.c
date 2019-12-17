#include "decoder.h"
#include <math.h>
#include "parse_metafile.h"
#include "reader.h"
#include "spywriter.h"
#include "spyreader.h"

JpegMetaData *jpeg_meta_data = NULL;

float cc(uint8_t i, uint8_t j)
{
    if (i == 0 && j == 0)
    {
        return 1.0 / 2.0;
    }
    else if (i == 0 || j == 0)
    {
        return 1.0 / sqrt((float)2.0);
    }
    else
    {
        return 1.0;
    }
}

uint8_t chomp(float x)
{
    if (x >= 255.0)
    {
        return 255;
    }
    else if (x <= 0.0)
    {
        return 0;
    }
    else
    {
        return (uint8_t)round(x);
    }
}

const int ZZ[8][8] = {
    [0] = {0, 1, 5, 6, 14, 15, 27, 28},
    [1] = {2, 4, 7, 13, 16, 26, 29, 42},
    [2] = {3, 8, 12, 17, 25, 30, 41, 43},
    [3] = {9, 11, 18, 24, 31, 40, 44, 53},
    [4] = {10, 19, 23, 32, 39, 45, 52, 54},
    [5] = {20, 22, 33, 38, 46, 51, 55, 60},
    [6] = {21, 34, 37, 47, 50, 56, 59, 61},
    [7] = {35, 36, 48, 49, 57, 58, 62, 63}};
void mulBlocks(Block *blocks, int w, int h, int count, Block value)
{
    int wh = h * 2 + w;

    *(blocks + (wh)*8 * 8 + count) *= value;
    // printf("%.1f ", *(blocks + (h) * 8 * 8 + (count / 8) * 8 + (count % 8)));
    // if(count % 8 == 8){
    //     printf("\n");
    // }
}
void plusBlocks(Block *blocks, int w, int h, int count, Block value)
{
    int wh = h * 2 + w;

    *(blocks + (wh)*8 * 8 + count) += value;
    // printf("%.1f ", *(blocks + (h) * 8 * 8 + (count / 8) * 8 + (count % 8)));
    // if(count % 8 == 8){
    //     printf("\n");
    // }
}
void dequantize(MCU *mcu)
{
    SofInfo *sof_info = jpeg_meta_data->sof_info;
    ComponentInfo **component_info = sof_info->componentInfos;
    DQTTable *dqt_tables = jpeg_meta_data->dqt_table;

    for (int id = 0; id < 3; ++id)
    {
        ComponentInfo *c_info = component_info[id];
        uint8_t height = jpeg_meta_data->sof_info->componentInfos[id]->vertical_sampling;
        uint8_t width = jpeg_meta_data->sof_info->componentInfos[id]->horizontal_sampling;
        for (int h = 0; h < height; ++h)
        {
            for (int w = 0; w < width; ++w)
            {
                for (int i = 0; i < 8; ++i)
                {
                    for (int j = 0; j < 8; ++j)
                    {
                        mulBlocks(mcu->blocks[id],
                                  w, h,
                                  (i * 8) + j,
                                  dqt_tables->tables[c_info->quant_table_id][i * 8 + j]);
                    }
                }
            }
        }
    }
    print_test(mcu, "after dequantize");

}

void dezigzag(MCU *mcu)
{
    SofInfo *sof_info = jpeg_meta_data->sof_info;
    ComponentInfo **component_info = sof_info->componentInfos;

    for (int id = 0; id < 3; ++id)
    {
        uint8_t height = jpeg_meta_data->sof_info->componentInfos[id]->vertical_sampling;
        uint8_t width = jpeg_meta_data->sof_info->componentInfos[id]->horizontal_sampling;
        Block *blocks = (Block *)malloc(sizeof(Block) * height * width * 8 * 8);
        memcpy(blocks, mcu->blocks[id], sizeof(Block) * height * width * 8 * 8);

        ComponentInfo *c_info = component_info[id];
        for (int h = 0; h < c_info->vertical_sampling; ++h)
        {
            for (int w = 0; w < c_info->horizontal_sampling; ++w)
            {
                for (int i = 0; i < 8; ++i)
                {
                    for (int j = 0; j < 8; ++j)
                    {
                        setBlocks(mcu->blocks[id], width,
                                  w, h,
                                  (i * 8) + j,
                                  readBlocks(blocks, width, w, h, ZZ[i][j] / 8, ZZ[i][j] % 8));
                    }
                }
            }
        }
    }
    print_test(mcu, "after dsigzag");

}
void idct(MCU *mcu)
{
    SofInfo *sof_info = jpeg_meta_data->sof_info;
    ComponentInfo **component_info = sof_info->componentInfos;
    float i_cos;
    float j_cos;
    float aft;
    float tmp = 0;

    for (int id = 0; id < 3; ++id)
    {
        uint8_t height = jpeg_meta_data->sof_info->componentInfos[id]->vertical_sampling;
        uint8_t width = jpeg_meta_data->sof_info->componentInfos[id]->horizontal_sampling;
        Block *blocks = (Block *)malloc(sizeof(Block) * height * width * 8 * 8);
        memset(blocks, 0, sizeof(Block) * height * width * 8 * 8);

        ComponentInfo *c_info = component_info[id];
        for (int h = 0; h < c_info->vertical_sampling; ++h)
        {
            for (int w = 0; w < c_info->horizontal_sampling; ++w)
            {
                for (int i = 0; i < 8; ++i)
                {
                    for (int j = 0; j < 8; ++j)
                    {
                        tmp = 0;
                        for (int x = 0; x < 8; ++x)
                        {
                            for (int y = 0; y < 8; ++y)
                            {
                                i_cos = cos((float)(2 * i + 1.0) * PI / 16.0 * x);
                                j_cos = cos((float)(2 * j + 1.0) * PI / 16.0 * y);
                                aft = cc(x, y) * readBlocks(mcu->blocks[id], width, w, h, x, y) * i_cos * j_cos;
                                
                                tmp += aft;
                            }
                        }
                        plusBlocks(blocks,
                                           w, h,
                                           (i * 8) + j,
                                           tmp);
                        setBlocks(blocks, width,
                                  w, h,
                                  (i * 8) + j,
                                  readBlocks(blocks, width, w, h, i, j) / 4.0);
                        // printf("tmp = %.5f, act = %.5f\n", tmp / 4, readBlocks(blocks, width, w, h, i, j));
                    }
                }
            }
        }
        memcpy(mcu->blocks[id], blocks, sizeof(Block) * height * width * 8 * 8);
        free(blocks);
    }
    print_test(mcu, "after idct");
}

void print_test(MCU *mcu, char* msg)
{
    return;
    printf("%s\n", msg);
    printf("h = %d, w = %d\n", mcu->h, mcu->w);
    SofInfo *sof_info = jpeg_meta_data->sof_info;
    ComponentInfo **component_info = sof_info->componentInfos;
    printf("after idct:\n");
    for (int id = 0; id < 3; ++id)
    {
        ComponentInfo *c_info = component_info[id];
        uint8_t height = jpeg_meta_data->sof_info->componentInfos[id]->vertical_sampling;
        uint8_t width = jpeg_meta_data->sof_info->componentInfos[id]->horizontal_sampling;
        for (int h = 0; h < height; ++h)
        {
            for (int w = 0; w < width; ++w)
            {
                printf("%s 颜色分量, %d, %d\n", component_name(id + 1), h, w);
                for (int i = 0; i < 8; ++i)
                {
                    for (int j = 0; j < 8; ++j)
                    {
                        // if (id == 0)
                            printf("%.6f ", readBlocks(mcu->blocks[id], width, w, h, i, j));
                    }
                    // if (id == 0)
                        printf("\n");
                }
                // if (id == 0)
                    printf("\n");
            }
        }
    }
}
Image *initImage(int width, int height)
{
    Image *image = (Image *)malloc(sizeof(Image));
    image->pixels = (Pixel *)malloc(sizeof(Pixel) * width * height);
    image->width = width;
    image->height = height;
    return image;
}
size_t pixel_sum = 0;
void mcuToRgb(Image *image, size_t w, size_t h, MCU *mcu)
{
    SofInfo *sof_info = jpeg_meta_data->sof_info;
    ComponentInfo **component_info = sof_info->componentInfos;
    uint8_t max_vertical_sampling = sof_info->max_vertical_sampling;
    uint8_t max_horizontal_sampling = sof_info->max_horizontal_sampling;
    size_t mcu_height = 8 * max_vertical_sampling;
    size_t mcu_width = 8 * max_horizontal_sampling;
    size_t image_width = sof_info->width;
    size_t image_height = sof_info->height;

    for (int i = 0; i < mcu_height; ++i)
    {
        for (int j = 0; j < mcu_width; ++j)
        {
            float YCbCr[3] = {0.0, 0.0, 0.0};
            for (int id = 0; id < 3; ++id)
            {
                uint8_t width = jpeg_meta_data->sof_info->componentInfos[id]->horizontal_sampling;
                uint8_t height = jpeg_meta_data->sof_info->componentInfos[id]->vertical_sampling;

                int vh = (i * component_info[id]->vertical_sampling / max_vertical_sampling);
                int vw = (j * component_info[id]->horizontal_sampling / max_horizontal_sampling);

                YCbCr[id] = readBlocks(mcu->blocks[id], width, vw / 8, vh / 8, i % 8, j % 8);
                // if(id == 1)
                // printf("id = %d, at %d, %d, mcu_i=%d, mcu_j=%d, res=%f, w=%zu, h=%zu\n", id, vw / 8, vh / 8, i, j, YCbCr[id], w, h);
            }
            float Y = YCbCr[0];
            float Cb = YCbCr[1];
            float Cr = YCbCr[2];

            uint8_t R = chomp(Y + 1.402 * Cr + 128.0);
            uint8_t G = chomp(Y - 0.34414 * Cb - 0.71414 * Cr + 128.0);
            uint8_t B = chomp(Y + 1.772 * Cb + 128.0);

            if (mcu_height * h + i <= image_height && mcu_width * w + j <= image_width)
            {
                pixel_sum ++;
                // printf("(y, x): (%zu, %zu), R:%u, G:%u, B:%u\n", (h * mcu_height + i), w * mcu_width + j, R, G, B);
                (image->pixels + image_width * (h * mcu_width + i) + w * mcu_width + j)->R = R;
                (image->pixels + image_width * (h * mcu_width + i) + w * mcu_width + j)->G = G;
                (image->pixels + image_width * (h * mcu_width + i) + w * mcu_width + j)->B = B;
            }
        }
    }
}

Image *decoder()
{
    SofInfo *sof_info = jpeg_meta_data->sof_info;
    ComponentInfo **component_info = sof_info->componentInfos;

    MCUS *mcus = jpeg_meta_data->mcus;
    int image_width = sof_info->width;
    int image_height = sof_info->height;

    Image *image = initImage(image_width, image_height);

    int w = (image_width - 1) / (8 * sof_info->max_horizontal_sampling) + 1;
    int h = (image_height - 1) / (8 * sof_info->max_vertical_sampling) + 1;

    for (int i = 0; i < h; ++i)
    {
        for (int j = 0; j < w; ++j)
        {
            dequantize(*(mcus->mcu + i * w + j));
            dezigzag(*(mcus->mcu + i * w + j));
            idct(*(mcus->mcu + i * w + j));
            mcuToRgb(image, j, i, *(mcus->mcu + i * w + j));
        }
    }
    printf("edit %zu pixels", pixel_sum);
    return image;
}

void to_ppm(Image *image, char *filename)
{
    int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 06666);
    char str[100];
    sprintf(str, "P6\n%d %d\n255\n", image->width, image->height);

    write(fd, str, strlen(str));
    write(fd, image->pixels, sizeof(Pixel) * image->width * image->height);
    // for(int row = 0;row < image->height;++row)
    // {
    //     for(int col = 0;col < image->width;++col)
    //     {
    //         write(fd, (image->pixels + row * image->width + col)->R, 1);
    //         write(fd, (image->pixels + row * image->width + col)->G, 1);
    //         write(fd, (image->pixels + row * image->width + col)->B, 1);
    //     }
    // }

    printf("done!\n");
}

int main()
{
    // read_jpgfile("./kaola.jpg");
    read_jpgfile("./rainbow.jpg");
    // marker_detector();
    jpeg_meta_data = data_reader();
    // printf("please enter the code that you want to encrypt")
    // encrypt("abcdef", "kaola_out.jpg");
    to_ppm(decoder(), "rainbow_ppm.ppm");

    // decrypt();

    return 0;
}