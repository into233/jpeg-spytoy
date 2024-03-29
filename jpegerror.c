#include "jpegerror.h"

void jpgexit(int errno, char *file, int line)
{
    printf("exit at %s:%d with error number:%d\n", file, line, errno);
    exit(errno);
}

void jpeg_assert(bool b, char *str)
{
    if (!b)
    {
        printf("assert error:%s\n", str);
        exit(ASSERT_ERROR);
    }
}