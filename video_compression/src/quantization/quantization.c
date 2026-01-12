#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<stdint.h>
#include"quantization/quantization.h"
#include"quantization/jpeg/quant_jpeg.h"
#include"yuv.h"
#include"block.h"


/*  function: quantize_frame()
    Params:
        YUVFrame* frame      : yuv raw data frame
        QuantType quant_type : 使用量化的方式

    Return:
        對padded data做DCT forward結果

    Result:
        1. 對frame的padded buffer做128-shift (不是對raw data的buffer做shift)
        2. 對shift結果做DCT
 */
void quantize_frame(YUVFrame* frame, QuantType quant_type)
{
    if (quant_type == JPEG_QUANT_STANDARD) {
        jpeg_standard_quant(frame);
    }
}

void dequantize_frame(YUVFrame* frame, QuantType quant_type)
{
    if (quant_type == JPEG_QUANT_STANDARD) {
        jpeg_standard_dequant(frame);
    }
}