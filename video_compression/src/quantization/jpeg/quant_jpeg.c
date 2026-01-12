#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<stdint.h>
#include"quantization/jpeg/quant_jpeg.h"
#include"yuv.h"


/*  function: jpeg_standard_block_luminance_quant()
    Params:
        int16_t* block   : frame在DCT後的padded data裡的一塊block
        int b_height     : block height
        int b_width      : block width
        int padded_width : padded data的width

    Return:
        對Y padded data做jpeg standard quantization的結果

    Result:
        對frame的Y padded buffer的blocks各自做jpeg standard quantization的結果
 */
void jpeg_standard_block_luminance_quant(int16_t* block, int b_height, int b_width, int padded_width)
{
    /* 使用Y的quantization table對block的係數做量化 */
    for (int row = 0; row < b_height; row++) {
        for (int col = 0; col < b_width; col++) {
            /* 標準JPEG量化方式: DCT_coef./step_size */
            block[row * padded_width + col] = (int16_t)round(block[row * padded_width + col] / jpeg_luminance_quant_table[row * b_width + col]);
        }
    }
}

void jpeg_standard_block_chrominance_quant(int16_t* block, int b_height, int b_width, int padded_width)
{
    /* 使用UV的quantization table對block的係數做量化 */
    for (int row = 0; row < b_height; row++) {
        for (int col = 0; col < b_width; col++) {
            /* 標準JPEG量化方式: DCT_coef./step_size */
            block[row * padded_width + col] = (int16_t)round(block[row * padded_width + col] / jpeg_chrominance_quant_table[row * b_width + col]);
        }
    }
}

/*  function: jpeg_standard_quant()
    Params:
        YUVFrame* frame : yuv raw data frame

    Return:
        對padded data做jpeg standard quantization的結果

    Result:
        對frame的padded buffer的blocks各自做jpeg standard quantization的結果
 */
void jpeg_standard_quant(YUVFrame* frame)
{
    // Y component
    for (int row = 0; row < frame->y.padded_height; row += frame->y.block_info.height) {
        for (int col = 0; col < frame->y.padded_width; col += frame->y.block_info.width) {
            jpeg_standard_block_luminance_quant(frame->y.padded_data + (row * frame->y.padded_width + col), frame->y.block_info.height, frame->y.block_info.width, frame->y.padded_width);
        }
    }
    // U component
    for (int row = 0; row < frame->u.padded_height; row += frame->u.block_info.height) {
        for (int col = 0; col < frame->u.padded_width; col += frame->u.block_info.width) {
            jpeg_standard_block_chrominance_quant(frame->u.padded_data + (row * frame->u.padded_width + col), frame->u.block_info.height, frame->u.block_info.width, frame->u.padded_width);
        }
    }
    // V component
    for (int row = 0; row < frame->v.padded_height; row += frame->v.block_info.height) {
        for (int col = 0; col < frame->v.padded_width; col += frame->v.block_info.width) {
            jpeg_standard_block_chrominance_quant(frame->v.padded_data + (row * frame->v.padded_width + col), frame->v.block_info.height, frame->v.block_info.width, frame->v.padded_width);
        }
    }
}

void jpeg_standard_block_luminance_dequant(int16_t* block, int b_height, int b_width, int padded_width)
{
    /* 標準JPEG反量化方式: DCT_coef. * step_size */
    for (int row = 0; row < b_height; row++) {
        for (int col = 0; col < b_width; col++) {
            block[row * padded_width + col] *= jpeg_luminance_quant_table[row * b_width + col];
        }
    }
}

void jpeg_standard_block_chrominance_dequant(int16_t* block, int b_height, int b_width, int padded_width)
{
    /* 標準JPEG反量化方式: DCT_coef. * step_size */
    for (int row = 0; row < b_height; row++) {
        for (int col = 0; col < b_width; col++) {
            block[row * padded_width + col] *= jpeg_chrominance_quant_table[row * b_width + col];
        }
    }
}

void jpeg_standard_dequant(YUVFrame* frame)
{
    // Y component
    for (int row = 0; row < frame->y.padded_height; row += frame->y.block_info.height) {
        for (int col = 0; col < frame->y.padded_width; col += frame->y.block_info.width) {
            jpeg_standard_block_luminance_dequant(frame->y.padded_data + (row * frame->y.padded_width + col), frame->y.block_info.height, frame->y.block_info.width, frame->y.padded_width);
        }
    }
    // U component
    for (int row = 0; row < frame->u.padded_height; row += frame->u.block_info.height) {
        for (int col = 0; col < frame->u.padded_width; col += frame->u.block_info.width) {
            jpeg_standard_block_chrominance_dequant(frame->u.padded_data + (row * frame->u.padded_width + col), frame->u.block_info.height, frame->u.block_info.width, frame->u.padded_width);
        }
    }
    // V component
    for (int row = 0; row < frame->v.padded_height; row += frame->v.block_info.height) {
        for (int col = 0; col < frame->v.padded_width; col += frame->v.block_info.width) {
            jpeg_standard_block_chrominance_dequant(frame->v.padded_data + (row * frame->v.padded_width + col), frame->v.block_info.height, frame->v.block_info.width, frame->v.padded_width);
        }
    }
}

