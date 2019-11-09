#include "decoder.h"
#include <math.h>
#include "parse_metafile.h"
#include "reader.h"
#include "spywriter.h"

int main()
{
    read_jpgfile("./kaola.jpg");
    // marker_detector();
    // JpegMetaData *jpegmetadata = data_reader();
    // printf("please enter the code that you want to encrypt")
    encrypt("123456", "out.jpg");
    
    return 0;
}