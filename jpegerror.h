#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>

#define FILE_FD_ERR     				-1		//无效的文件描述符
#define FILE_READ_ERR   				-2		//读文件失败
#define FILE_WRITE_ERR  				-3		//写文件失败
#define INVALID_METAFILE_ERR			-4		//无效的文件
#define INVALID_HUFFMAN_CODE_ERROR      -5
#define INVALID_SWITCH_ERROR            -6
#define UNKNOWN_ERROR                   -7
#define DQT_PRECISION_ERROR             -8
#define ASSERT_ERROR                    -9
#define INVALID_PARAMETER_ERR			-10		//无效的函数参数
#define INVALID_ALLOCATE_MEM_ERR		-11		//申请动态内存失败
#define NO_BUFFER_ERR					-12		//没有足够的缓冲区
#define RECEIVE_EXIT_SIGNAL_ERR			-15		//接收到退出程序的信号


void jpgexit(int errno, char *file, int line);

void jpeg_assert(bool b, char* str);