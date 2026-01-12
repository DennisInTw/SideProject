#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include"file_io.h"


/*  function: create_bit_writer()
    Params:
        BitWriter* bit_writer : 紀錄bistream寫入的資訊
        FILE* fp              : 檔案位置

    Return:
        得到 bit writer，用來記錄整張frame的bitstream寫檔情況

    Result:
 */
void create_bit_writer(BitWriter* bit_writer, FILE* fp)
{
    bit_writer->fp = fp;
    bit_writer->buffer = 0;
    bit_writer->bit_count = 0;
}

void create_bit_reader(BitReader* bit_reader, FILE* fp)
{
    bit_reader->fp = fp;
    bit_reader->buffer = 0;
    bit_reader->bit_left = 0;
}
