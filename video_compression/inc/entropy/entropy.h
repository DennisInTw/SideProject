#ifndef ENTROPY_H
#define ENTROPY_H

#include"yuv.h"
#include"quantization/quantization.h"

typedef enum {
    JPEG_SEQUENTIAL = 0 // Jpeg Sequential baseline
}CompressionType;

typedef enum {
    HUFFMAN = 0
}EntropyType;

void entropy_initialization(EntropyType entropy_type);
void entropy_destropy(EntropyType entropy_type);
void entropy_encode(YUVFrame* frame, QuantType quant_type, CompressionType compression_type, EntropyType entropy_type, const char* out_bitstream_path);
void entropy_decode(YUVFrame* frame, QuantType quant_type, CompressionType compression_type, EntropyType entropy_type, const char* out_bitstream_path);

#endif /* ENTROPY_H */