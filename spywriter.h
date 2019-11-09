#include "reader.h"
#include "jpegerror.h"
#include <math.h>

enum SPYMODE{
    SPY_ENCODE,
    SPY_DECODE,
    SPY_STILL  //does nothing
};

void encrypt(char *encryptStr, char* filename);

void retreat_write_value(uint8_t code_len);