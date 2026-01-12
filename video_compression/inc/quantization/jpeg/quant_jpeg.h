#ifndef QUANT_JPEG_H
#define QUANT_JPEG_H

#include<stdint.h>
#include"yuv.h"
 
extern const uint8_t jpeg_luminance_quant_table[64];;
extern const uint8_t jpeg_chrominance_quant_table[64];

void jpeg_standard_quant(YUVFrame* frame);
void jpeg_standard_dequant(YUVFrame* frame);

#endif // QUANT_JPEG_H