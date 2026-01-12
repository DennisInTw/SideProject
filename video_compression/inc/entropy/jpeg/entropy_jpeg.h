#ifndef ENTROPY_JPEG_H
#define ENTROPY_JPEG_H

#include<stdint.h>
#include"yuv.h"
#include"entropy/entropy.h"


/* 儲存zigzag scan後的係數 */
typedef struct {
    int16_t dc;
    int16_t ac[63];
}JpegBlockCoeffs;

/* 儲存DPCM後的DC係數*/
typedef struct {
    uint8_t size;       // 儲存dc差值需要的bit個數 (也就是儲存該係數是哪一個category，不同category需要的bits也不同)
    uint16_t amplitude; // 儲存dc差值的1's complement表示法
}JpegDcEncoded;

/* 儲存RLE後的symbol資訊 */
typedef struct {
    uint8_t run_length; // 非0係數前面的0的個數 (只會用到4 bits: 範圍[0,15])
    uint8_t size;       // 非0係數需要的bit個數 (只會用到4 bits)
    uint16_t amplitude; // 非0係數的1's complement表示法
}JpegAcSymbol;

/* 每個block的AC編碼結果 */
typedef struct {
    JpegAcSymbol symbols[64];  // 最多63個AC係數 + 1個EOB符號
    uint8_t num_symbols;       // 實際符號數量（不包含EOB）
}JpegAcEncoded;

void entropy_decode_jpeg(YUVFrame* frame, QuantType quant_type, CompressionType compression_type, EntropyType entropy_type, const char* out_bitstream_path);
void entropy_encode_jpeg(YUVFrame* frame, QuantType quant_type, CompressionType compression_type, EntropyType entropy_type, const char* out_bitstream_path);

#endif /* ENTROPY_JPEG_H */
