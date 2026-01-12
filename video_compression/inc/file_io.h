#ifndef FILE_IO_H
#define FILE_IO_H

#include<stdio.h>
#include<stdint.h>

typedef struct {
    FILE* fp;
    uint8_t buffer;  // 儲存讀取bits的buffer，等滿8個bits再寫到檔案 (bit packing方式)
    int bit_count;   // 計算目前buffer儲放多少bits
}BitWriter;

typedef struct {
    FILE* fp;
    uint8_t buffer;  // 從檔案讀取byte by byte，再對buffer做bit unpacking
    int bit_left;    // 計算buffer還有多少bits未decode
}BitReader;

void create_bit_writer(BitWriter* bit_writer, FILE* fp);
void create_bit_reader(BitReader* bit_reader, FILE* fp);

#endif // FILE_IO_H