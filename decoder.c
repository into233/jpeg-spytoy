#include "decoder.h"
#include <math.h>
#include "parse_metafile.h"
#include "reader.h"
#include "spywriter.h"
#include "spyreader.h"

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