#ifndef QUANTIZATION_H
#define QUANTIZATION_H

#include<stdint.h>
#include"yuv.h"
#include"block.h"

typedef enum {
    JPEG_QUANT_STANDARD=0,
    H264_QUANT
}QuantType;


void quantize_frame(YUVFrame* frame, QuantType quant_type);
void dequantize_frame(YUVFrame* frame, QuantType quant_type);

#endif /* QUANTIZATION_H */