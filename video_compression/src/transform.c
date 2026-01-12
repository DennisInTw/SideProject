#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<stdint.h>
#include "yuv.h"
#include"block.h"


/*  function: shift_128()
    Params:
        Component* component : frame裡的y/u/v其中一個component

    Return:
        對component做128-shift的結果

    Result:
        對frame的y/u/v其中一個component的padded buffer做128-shift
 */
void shift_128(Component* component)
{
    /* 將raw data複製到padded data */
    for (int row = 0; row < component->height; row++) {
        for (int col = 0; col < component->width; col++) {
            component->padded_data[row * component->padded_width + col] = component->raw_data[row * component->width + col];
        }
    }

    /* 對padded data做-128位移 */
    size_t com_size = component->padded_width * component->padded_height;
    for (size_t i = 0; i < com_size; i++) {
        component->padded_data[i] = (int16_t)(component->padded_data[i] - 128);
    }
}

void unshift_128(Component* component)
{
    for (int row = 0; row < component->padded_height; row++) {
        for (int col = 0; col < component->padded_width; col++) {
            int16_t value = component->padded_data[row * component->padded_width + col] + 128;

            // 限制shift回來的值必須在[0,255]
            if (value < 0) value = 0;
            else if (value > 255) value = 255;

            component->padded_data[row * component->padded_width + col] = value;
        }
    }
}

/*  function: dct_block_8x8()
    Params:
        int16_t* block   : yuv padded data的一個block資料
        int padded_width : padded data的width

    Return:
        對block data做DCT (type-III)

    Result:
        得到block data的DCT (type-III)結果
 */
void dct_block_8x8(int16_t* block, int padded_width)
{
    const double PI = 3.14159265358979323846;
    const int N = 8;
    int16_t temp_block[64] = {0};

    static double cosine_table[8][8];
    static int initialized = 0;

    /* 建立好cosine table，減少運算，只需在第一次進入function時建立 */
    if (!initialized) {
        for (int u = 0; u < N; u++) {
            for (int x = 0; x < N; x++) {
                cosine_table[u][x] = cos(u*PI*(2*x+1)/(2*N));
            }
        }
        initialized = 1;
    }

    // Type-III DCT
    for (int u = 0; u < N; u++) {  // u: 垂直方向 , v: 水平方向
        for (int v = 0; v < N; v++) {
            double sum = 0.0;
            double cu = (u == 0) ? (1.0/sqrt(2.0)) : 1.0;
            double cv = (v == 0) ? (1.0/sqrt(2.0)) : 1.0;

            for (int j = 0; j < N; j++) {  // j: 垂直方向 , i: 水平方向
                for (int i = 0; i < N; i++) {
                    sum += block[j*padded_width + i] * cosine_table[v][i] * cosine_table[u][j];
                }
            }

            /* 存放每個頻率的DCT計算結果 */
            temp_block[u*N + v] = (int16_t)round(0.25 * cu * cv * sum);
        }
    }

    /* 將DCT結果寫回原本的block */
    for (int j = 0; j < N; j++) {
        for (int i = 0; i < N; i++) {
            block[j*padded_width + i] = temp_block[j*N + i];
        }
    }
}

void idct_block_8x8(int16_t* block, int padded_width)
{
    const double PI = 3.14159265358979323846;
    const int N = 8;
    int16_t temp_block[64] = {0};

    static double cosine_table[8][8];
    static int initialized = 0;

    // 建立好cosine table,減少運算
    if (!initialized) {
        for (int u = 0; u < N; u++) {
            for (int x = 0; x < N; x++) {
                cosine_table[u][x] = cos(u*PI*(2*x+1)/(2*N));
            }
        }
        initialized = 1;
    }

    // Inverse Type-III DCT
    for (int j = 0; j < N; j++) {  // j: 垂直方向 , i: 水平方向
        for (int i = 0; i < N; i++) {
            double sum = 0.0;

            for (int u = 0; u < N; u++) {
                for (int v = 0; v < N; v++) {
                    double cu = (u == 0) ? (1.0/sqrt(2.0)) : 1.0;
                    double cv = (v == 0) ? (1.0/sqrt(2.0)) : 1.0;
                    sum += cu * cv * block[u*padded_width + v] * cosine_table[v][i] * cosine_table[u][j];
                }
            }
            temp_block[j*N + i] = (int16_t)round(0.25 * sum);
        }
    }

    // 將IDCT結果寫回padded data的block
    for (int j = 0; j < N; j++) {
        for (int i = 0; i < N; i++) {
            block[j*padded_width + i] = temp_block[j*N + i];
        }
    }

}

/*  function: dct_2d()
    Params:
        YUVFrame* frame : yuv frame

    Return:
        根據YUV format，對yuv的padded data的每個block各自做DCT

    Result:
        得到yuv的padded data的DCT結果
 */
void dct_2d(YUVFrame* frame)
{
    // Y component
    if (frame->y.block_info.b_size == BLOCK_8x8) {
        for (int row = 0; row < frame->y.padded_height; row += 8) {
            for (int col = 0; col < frame->y.padded_width; col += 8) {
                dct_block_8x8(frame->y.padded_data + (row * frame->y.padded_width + col), frame->y.padded_width);
            }
        }
    } else {

    }

    // U component
    if (frame->u.block_info.b_size == BLOCK_8x8) {
        for (int row = 0; row < frame->u.padded_height; row += 8) {
            for (int col = 0; col < frame->u.padded_width; col += 8) {
                dct_block_8x8(frame->u.padded_data + (row * frame->u.padded_width + col), frame->u.padded_width);
            }
        }
    } else {

    }

    // V component
    if (frame->v.block_info.b_size == BLOCK_8x8) {
        for (int row = 0; row < frame->v.padded_height; row += 8) {
            for (int col = 0; col < frame->v.padded_width; col += 8) {
                dct_block_8x8(frame->v.padded_data + (row * frame->v.padded_width + col), frame->v.padded_width);
            }
        }
    } else {

    }
}

void idct_2d(YUVFrame* frame)
{
    // Y component
    if (frame->y.block_info.b_size == BLOCK_8x8) {
        for (int row = 0; row < frame->y.padded_height; row += 8) {
            for (int col = 0; col < frame->y.padded_width; col += 8) {
                idct_block_8x8(frame->y.padded_data + (row * frame->y.padded_width + col), frame->y.padded_width);
            }
        }
    } else {

    }

    // U component
    if (frame->u.block_info.b_size == BLOCK_8x8) {
        for (int row = 0; row < frame->u.padded_height; row += 8) {
            for (int col = 0; col < frame->u.padded_width; col += 8) {
                idct_block_8x8(frame->u.padded_data + (row * frame->u.padded_width + col), frame->u.padded_width);
            }
        }
    } else {

    }

    // V component
    if (frame->v.block_info.b_size == BLOCK_8x8) {
        for (int row = 0; row < frame->v.padded_height; row += 8) {
            for (int col = 0; col < frame->v.padded_width; col += 8) {
                idct_block_8x8(frame->v.padded_data + (row * frame->v.padded_width + col), frame->v.padded_width);
            }
        }
    } else {

    }
}

/*  function: transform_frame()
    Params:
        YUVFrame* frame : yuv raw data frame

    Return:
        對padded data做DCT forward結果

    Result:
        1. 對frame的padded buffer做128-shift (不是對raw data的buffer做shift)
        2. 對shift結果做DCT
 */
void transform_frame(YUVFrame* frame)
{
    /* 對y/u/v的padded buffer做128-shift */
    shift_128(&frame->y);
    shift_128(&frame->u);
    shift_128(&frame->v);

    /* 對shifted data做DCT forward */
    dct_2d(frame);
}


void reverse_transform_frame(YUVFrame* frame)
{
    idct_2d(frame);

    unshift_128(&frame->y);
    unshift_128(&frame->u);
    unshift_128(&frame->v);
}
