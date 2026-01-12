#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include"yuv.h"
#include"entropy/jpeg/entropy_jpeg.h"
#include"entropy/entropy.h"
#include"entropy/algorithms/huffman.h"
#include"file_io.h"

// 8x8 Zig-zag掃描順序: 對應到block的位置
const uint8_t zigzag_8x8[64] = {
     0,  1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};


/*  function: zigzag_scan()
    Params:
        int16_t* block              : frame裡的component其中一塊padded data block
        int b_height                : block height
        int b_width                 : block width
        int padded_width            : padded data的width
        JpegBlockCoeffs* jpeg_block : 儲存scan後的結果

    Return:
        將scan順序的資料擺放在DC/AC裡

    Result:
        得到scan後的資料順序
 */
void zigzag_scan(int16_t* block, int b_height, int b_width, int padded_width, JpegBlockCoeffs* jpeg_block)
{
    // DC係數
    jpeg_block->dc = block[0];

    // AC係數
    for (int i = 1; i < 64; i++) {
        int index = zigzag_8x8[i];

        /* 轉換座標 */
        int row = index / 8;
        int col = index % 8;
        int offset = row * padded_width + col;
        jpeg_block->ac[i-1] = block[offset];
    }
}


/*  function: zigzag_component()
    Params:
        Component* comp         : frame的y/u/v其中一個component
        JpegBlockCoeffs* blocks : 儲存component做完zigzag scan後的DC/AC

    Return:
        得到zigzag scan後的DC/AC資料

    Result:
 */
void zigzag_component(Component* comp, JpegBlockCoeffs* blocks)
{
    /* 對整張frame的一個component切成blocks，對每個block做zigzag scan */
    int blocks_per_row = comp->padded_width / comp->block_info.width;

    for (int row = 0; row < comp->padded_height; row += comp->block_info.height) {
        for (int col = 0; col < comp->padded_width; col += comp->block_info.width) {
            int block_row_idx = row / comp->block_info.height;
            int block_col_idx = col / comp->block_info.width;

            /* 將本次zigzag scan結果放置在這個地方 */
            JpegBlockCoeffs* current_block = &blocks[block_row_idx * blocks_per_row + block_col_idx];
            
            /* 對component的padded data對應的block做zigzag scan */
            zigzag_scan(comp->padded_data + (row * comp->padded_width + col), comp->block_info.height, comp->block_info.width, comp->padded_width, current_block);
        }
    }
}


/*  function: differential_pulse_code_modulation()
    Params:
        JpegBlockCoeffs* blocks    : 儲存component做完zigzag scan後的DC/AC
        int num_blocks             : 該component有多少塊block

    Return:
        得到DPCM encoding後的DC係數

    Result:
        DPCM encoding是將current block的DC係數減掉上一個block的DC係數，最後將差值儲存
 */
void differential_pulse_code_modulation(JpegBlockCoeffs* blocks, int num_blocks)
{
    int16_t prev_dc = 0;  // 第一個block的前一個DC係數為0

    /* DPCM encoding: 當下block的DC - 上一個block的DC */
    for (int i = 0; i < num_blocks; i++) {
        int16_t current_dc = blocks[i].dc;
        blocks[i].dc = current_dc - prev_dc;
        prev_dc = current_dc;
    }
}

/*  function: get_size()
    Params:
        int16_t coeff : component做完DPCM的DC/AC係數

    Return:
        得到編碼該係數需要的bits個數 (不是Huffman coding需要的編碼bits個數!!!)
        用來對係數做分類

    Result:
 */
uint8_t get_size(int16_t coeff)
{
    int16_t abs_coeff = abs(coeff);
    uint8_t size = 0;

    while (abs_coeff > 0) {
        abs_coeff >>= 1;
        size++;
    }
    return size;
}


/*  function: get_amplitude()
    Params:
        int16_t coeff : component做完DPCM的DC/AC係數
        uint8_t size  : 該係數所需要的bits個數 或是 該係數在哪個category

    Return:
        該係數的1's complement結果

    Result:
 */
int16_t get_amplitude(int16_t coeff, uint8_t size)
{
    // 使用1's complement表示法
    // 正數：直接返回
    // 負數：amplitude = (2^size - 1) + coeff
    if (coeff >= 0) {
        return coeff;
    } else {
        return ((1 << size) - 1) + coeff;
    }
}


/*  function: jpeg_encode_dc()
    Params:
        JpegBlockCoeffs* blocks    : 儲存component做完zigzag scan後的DC/AC
        int num_blocks             : 該component有多少塊block
        JpegDcEncoded** dc_encoded : 儲存每個block將DC係數encode後的結果

    Return:
        得到DPCM encoding後的DC係數

    Result:
 */
void jpeg_encode_dc(JpegBlockCoeffs* blocks, int num_blocks, JpegDcEncoded** dc_encoded)
{
    /* DPCM encoding */
    differential_pulse_code_modulation(blocks, num_blocks);

    /* 根據block個數配置記憶體，儲存每塊block的DC係數在encode後的結果 */
    *dc_encoded = (JpegDcEncoded*)malloc(sizeof(JpegDcEncoded) * num_blocks);

    if (*dc_encoded == NULL) {
        /* 記憶體配置失敗，則回傳，不繼續做encode */
        perror("Failed to allocate memory for JPEG DC encoded data");
        return;
    }

    /*  Encode DC 差值係數
        JPEG不直接對DC差值做編碼，而是另外處理後，得到size(或是)category、ampltitude再對他們編碼
     */
    for (int i = 0; i < num_blocks; i++) {
        (*dc_encoded)[i].size = get_size(blocks[i].dc);
        (*dc_encoded)[i].amplitude = get_amplitude(blocks[i].dc, (*dc_encoded)[i].size);
    }
}

void inverse_zigzag_scan(int16_t* block, int b_height, int b_width, int padded_width, JpegBlockCoeffs* jpeg_block)
{
    // DC係數
    block[0] = jpeg_block->dc;

    // AC係數
    for (int i = 1; i < 64; i++) {
        int index = zigzag_8x8[i];

        /* 轉換座標 */
        int row = index / 8;
        int col = index % 8;
        int offset = row * padded_width + col;
        block[offset] = jpeg_block->ac[i-1];
    }
}

void inverse_zigzag_component(Component* comp, JpegBlockCoeffs* blocks)
{
    int blocks_per_row = comp->padded_width / comp->block_info.width;

    for (int row = 0; row < comp->padded_height; row += comp->block_info.height) {
        for (int col = 0; col < comp->padded_width; col += comp->block_info.width) {
            int block_row_idx = row / comp->block_info.height;
            int block_col_idx = col / comp->block_info.width;
            JpegBlockCoeffs* current_block = &blocks[block_row_idx * blocks_per_row + block_col_idx];
            inverse_zigzag_scan(comp->padded_data + (row * comp->padded_width + col), comp->block_info.height, comp->block_info.width, comp->padded_width, current_block);
        }
    }
}

void differential_pulse_code_demodulation(JpegBlockCoeffs* blocks, int num_blocks)
{
    int16_t prev_dc = 0;  // 第一個block的前一個DC係數為0

    for (int i = 0; i < num_blocks; i++) {
        int16_t current_dc = blocks[i].dc;
        blocks[i].dc = current_dc + prev_dc;
        prev_dc = blocks[i].dc;
    }
}

int16_t decode_amplitude(uint8_t size, int16_t amplitude)
{
    if (size == 0) return 0;

    /* 判斷amplitude的MSB決定原本的係數是正數或是負數 */
    int16_t thrshold = 1 << (size-1);
    if (amplitude >= thrshold) {
        return amplitude; // 正數，直接返回
    } else {
        return amplitude - (1 << size) + 1; // 負數，還原
    }
}

void jpeg_decode_dc(JpegBlockCoeffs* blocks, int num_blocks, JpegDcEncoded* dc_encoded)
{
    for (int i = 0; i < num_blocks; i++) {
        blocks[i].dc = decode_amplitude(dc_encoded[i].size, dc_encoded[i].amplitude);
    }

    // DPCM decoding
    differential_pulse_code_demodulation(blocks, num_blocks);
}


/*  function: run_length_encoding()
    Params:
        JpegBlockCoeffs* blocks    : 儲存component做完zigzag scan後的DC/AC
        JpegAcEncoded** ac_encoded : 儲存每個block將AC係數encode後的結果

    Return:
        得到run length encoding後的結果

    Result:
        1. 根據JPEG的RLE定義兩種特別的symbol
            a. EOB (End of Block): 表示後面全是0，編碼為(0,0)(0)
            b. ZRL (Zero Run Length): 表示16個連續的0, 編碼成(15,0)(0)
        2. 得到RLE編碼結果，每塊block後面都會有EOB
 */
void run_length_encoding(JpegBlockCoeffs* block, JpegAcEncoded* ac_encoded)
{   
    /* 初始化符號數量 */
    ac_encoded->num_symbols = 0;
    
    /* 避免block裡的係數都是0的情況，找到最後一個非零係數的位置 */
    int last_nonzero = -1;
    for (int i = 62; i >= 0; i--) {
        if (block->ac[i] != 0) {
            last_nonzero = i;
            break;
        }
    }

    /* 如果所有AC係數都是0，只輸出EOB (run_length, size)(amplitude) = (0,0)(0) */
    if (last_nonzero == -1) {
        ac_encoded->symbols[ac_encoded->num_symbols].run_length = 0;
        ac_encoded->symbols[ac_encoded->num_symbols].size = 0;
        ac_encoded->symbols[ac_encoded->num_symbols].amplitude = 0;
        ac_encoded->num_symbols++;
        return;
    }
    
    /* 只遍歷到最後一個非零係數 */
    int run_length = 0;
    for (int i = 0; i <= last_nonzero; i++) {
        if (block->ac[i] == 0) {
            run_length++;

            if (run_length == 16) {
                /* 遇到16個連續的0，輸出一個特殊符號 (run_length, size)(amplitude) = (15,0)(0) */
                ac_encoded->symbols[ac_encoded->num_symbols].run_length = 15;
                ac_encoded->symbols[ac_encoded->num_symbols].size = 0;
                ac_encoded->symbols[ac_encoded->num_symbols].amplitude = 0;
                ac_encoded->num_symbols++;
                run_length = 0;
            }
        } else {
            /* 遇到非0的AC係數，對係數做編碼，採用和DC係數編碼的方式 */
            ac_encoded->symbols[ac_encoded->num_symbols].run_length = run_length;
            ac_encoded->symbols[ac_encoded->num_symbols].size = get_size(block->ac[i]);
            ac_encoded->symbols[ac_encoded->num_symbols].amplitude = get_amplitude(block->ac[i], ac_encoded->symbols[ac_encoded->num_symbols].size);
            ac_encoded->num_symbols++;
            run_length = 0;
        }
    }
    
    /* 後面的係數都是0， 輸出EOB符號（表示後面的係數都是0）*/
    ac_encoded->symbols[ac_encoded->num_symbols].run_length = 0;
    ac_encoded->symbols[ac_encoded->num_symbols].size = 0;
    ac_encoded->symbols[ac_encoded->num_symbols].amplitude = 0;
    ac_encoded->num_symbols++;
}


/*  function: jpeg_encode_ac()
    Params:
        JpegBlockCoeffs* blocks    : 儲存component做完zigzag scan後的DC/AC
        int num_blocks             : 該component有多少塊block
        JpegAcEncoded** ac_encoded : 儲存每個block將AC係數encode後的結果

    Return:
        得到run length encoding後的結果

    Result:
 */
void jpeg_encode_ac(JpegBlockCoeffs* blocks, int num_blocks, JpegAcEncoded** ac_encoded)
{
    /* 根據block個數配置記憶體，儲存每塊block的AC係數在encode後的結果 */
    *ac_encoded = (JpegAcEncoded*)malloc(sizeof(JpegAcEncoded) * num_blocks);
    if (*ac_encoded == NULL) {
        /* 記憶體配置失敗，則回傳，不繼續做encode */
        perror("Failed to allocate memory for JPEG AC encoded data");
        return;
    }

    for (int i = 0; i < num_blocks; i++) {
        run_length_encoding(&blocks[i], &(*ac_encoded)[i]);
    }
}


/*  function: reverse_run_length_encoding()
    Params:
        JpegBlockCoeffs* blocks    : 儲存component做完zigzag scan後的DC/AC
        JpegAcEncoded** ac_encoded : 儲存每個block將AC係數encode後的結果

    Return:
        得到reverse run length encoding後的結果

    Result:
 */
void reverse_run_length_encoding(JpegBlockCoeffs* block, JpegAcEncoded* ac_encoded)
{
    // 初始化所有AC係數為0
    for (int i = 0; i < 63; i++) {
        block->ac[i] = 0;
    }

    int ac_index = 0;

    for (int i = 0; i < ac_encoded->num_symbols; i++) {
        JpegAcSymbol symbol = ac_encoded->symbols[i];

        // 檢查 ZRL 符號 (15, 0)
        if (symbol.run_length == 15 && symbol.size == 0) {
            // 跳過 16 個零（已經初始化為0了）
            ac_index += 16;
            continue;
        }

        // 檢查 EOB 符號 (0, 0)，後面全是0
        if (symbol.run_length == 0 && symbol.size == 0) {
            // 剩餘的已經是0了，直接結束
            break;
        }

        // 跳過 run_length 個零
        ac_index += symbol.run_length;

        // 解碼非零係數
        if (ac_index < 63) {
            block->ac[ac_index] = decode_amplitude(symbol.size, symbol.amplitude);
            ac_index++;
        }
    }
}


/*  function: jpeg_decode_ac()
    Params:
        JpegBlockCoeffs* blocks    : 儲存component做完zigzag scan後的DC/AC
        int num_blocks             : frame裡的block個數
        JpegAcEncoded** ac_encoded : 儲存每個block將AC係數encode後的結果

    Return:
        得到解碼AC係數後的結果

    Result:
 */
void jpeg_decode_ac(JpegBlockCoeffs* blocks, int num_blocks, JpegAcEncoded* ac_encoded)
{
    for (int i = 0; i < num_blocks; i++) {
        reverse_run_length_encoding(&blocks[i], &ac_encoded[i]);
    }
}

/*  function: jpeg_encode_header()
    Return:
        將解碼需要的資訊寫到header裡

    Result:
 */
void jpeg_encode_header(FILE* fp, YUVFrame* frame, QuantType quant_type, CompressionType compression_type, EntropyType entropy_type)
{
    /* YUV inforamtion */
    // yuv raw data width (2 bytes)
    fputc((frame->y.width >> 8) & 0xff, fp);
    fputc(frame->y.width & 0xff, fp);
    // yuv raw data height (2 bytes)
    fputc((frame->y.height >> 8) & 0xff, fp);
    fputc(frame->y.height & 0xff, fp);
    // yuv format (1 byte)
    fputc(frame->format & 0xff, fp);
    
    /* compression information */
    // Y : block size / block width / block height (各都1 byte)
    fputc(frame->y.block_info.b_size & 0xff, fp);
    fputc(frame->y.block_info.width & 0xff, fp);
    fputc(frame->y.block_info.height & 0xff, fp);
    // U : block size / block width / block height (各都1 byte)
    fputc(frame->u.block_info.b_size & 0xff, fp);
    fputc(frame->u.block_info.width & 0xff, fp);
    fputc(frame->u.block_info.height & 0xff, fp);
    // V : block size / block width / block height (各都1 byte)
    fputc(frame->v.block_info.b_size & 0xff, fp);
    fputc(frame->v.block_info.width & 0xff, fp);
    fputc(frame->v.block_info.height & 0xff, fp);
    // quantization type
    fputc(quant_type & 0xff, fp);
    // compression type
    fputc(compression_type & 0xff, fp);
    // entropy type
    fputc(entropy_type & 0xff, fp);
}


/*  function: jpeg_decode_header()
    Return:
        解碼出需要的設定資訊

    Result:
 */
int jpeg_decode_header(FILE* fp, YUVFrame* frame, QuantType quant_type, CompressionType compression_type, EntropyType entropy_type)
{
    /* YUV inforamtion */
    int frame_w, frame_h;
    YUVFormat format;
    // yuv raw data width (2 bytes)
    frame_w = (((uint8_t)fgetc(fp)) << 8);
    frame_w |= ((uint8_t)fgetc(fp));
    // yuv raw data height (2 bytes)
    frame_h = (((uint8_t)fgetc(fp)) << 8);
    frame_h |= (uint8_t)fgetc(fp);
    // yuv format (1 byte)
    format = (uint8_t)fgetc(fp);
    
    if (frame_w != frame->y.width || frame_h != frame->y.height || format != frame->format) {
        perror("YUV information is not set correctly.\n");
        fprintf(stderr, "Decoded header : w=%d h=%d foramt=%u \n", frame_w, frame_h, format);
        return -1;
    }

    /* compression information */
    BlockInfo y_block_info;
    BlockInfo u_block_info;
    BlockInfo v_block_info;
    QuantType q_type;
    CompressionType cmpr_type;
    EntropyType en_type;
    // Y : block size / block width / block height (各都1 byte)
    y_block_info.b_size = fgetc(fp);
    y_block_info.width = fgetc(fp);
    y_block_info.height = fgetc(fp);
    // U : block size / block width / block height (各都1 byte)
    u_block_info.b_size = fgetc(fp);
    u_block_info.width = fgetc(fp);
    u_block_info.height = fgetc(fp);
    // V : block size / block width / block height (各都1 byte)
    v_block_info.b_size = fgetc(fp);
    v_block_info.width = fgetc(fp);
    v_block_info.height = fgetc(fp);
    // quantization type
    q_type = (uint8_t)fgetc(fp);
    // compression type
    cmpr_type = (uint8_t)fgetc(fp);
    // entropy type
    en_type = (uint8_t)fgetc(fp);

    if (y_block_info.b_size != frame->y.block_info.b_size || y_block_info.width != frame->y.block_info.width || y_block_info.height != frame->y.block_info.height) {
        perror("Y block information is not set correctly.\n");
        fprintf(stderr, "Decoded y block size=%d block width=%d block height=%d\n", y_block_info.b_size, y_block_info.width, y_block_info.height);
        fprintf(stderr, "Config y block size=%d block width=%d block height=%d\n", frame->y.block_info.b_size, frame->y.block_info.width, frame->y.block_info.height);
        
        return -1;
    }

    if (u_block_info.b_size != frame->u.block_info.b_size || u_block_info.width != frame->u.block_info.width || u_block_info.height != frame->u.block_info.height) {
        perror("U block information is not set correctly.\n");
        fprintf(stderr, "Decoded u block size=%d block width=%d block height=%d\n", u_block_info.b_size, u_block_info.width, u_block_info.height);
        fprintf(stderr, "Config u block size=%d block width=%d block height=%d\n", frame->u.block_info.b_size, frame->u.block_info.width, frame->u.block_info.height);
        return -1;
    }

    if (v_block_info.b_size != frame->v.block_info.b_size || v_block_info.width != frame->v.block_info.width || v_block_info.height != frame->v.block_info.height) {
        perror("V block information is not set correctly.\n");
        fprintf(stderr, "Decoded v block size=%d block width=%d block height=%d\n", v_block_info.b_size, v_block_info.width, v_block_info.height);
        fprintf(stderr, "Config v block size=%d block width=%d block height=%d\n", frame->v.block_info.b_size, frame->v.block_info.width, frame->v.block_info.height);
        return -1;
    }

    if (q_type != quant_type) {
        perror("Quantization type is not set correctly.\n");
        fprintf(stderr, "Decoded quantization type=%d\n", q_type);
        return -1;
    }

    if (cmpr_type != compression_type) {
        perror("Compression type is not set correctly.\n");
        fprintf(stderr, "Decoded compression type=%d\n", cmpr_type);
        return -1;
    }

    if (en_type != entropy_type) {
        perror("Entropy type is not set correctly.\n");
        fprintf(stderr, "Decoded entropy type=%d\n", en_type);
        return -1;
    }

    // 表示decode出來的資訊和設定的都相同
    return 0;
}

void entropy_decode_jpeg(YUVFrame* frame, QuantType quant_type, CompressionType compression_type, EntropyType entropy_type, const char* out_bitstream_path)
{
    int ret;
    FILE* fp;
    fp = fopen(out_bitstream_path, "rb");
    if (!fp) {
        fprintf(stderr, "Failed to open file: %s\n", out_bitstream_path);
        return;
    }

    ret = jpeg_decode_header(fp, frame, quant_type, compression_type, entropy_type);

    /* Header解碼失敗 */
    if (ret != 0) {
        fclose(fp);
        return;
    }

    /* 統計每張frame有多少個block，再配置每個block裡的DC和AC需要儲存的資訊所需要的記憶體空間 */
    int y_blocks_num = (frame->y.padded_height / frame->y.block_info.height) * (frame->y.padded_width / frame->y.block_info.width);
    int u_blocks_num = (frame->u.padded_height / frame->u.block_info.height) * (frame->u.padded_width / frame->u.block_info.width);
    int v_blocks_num = (frame->v.padded_height / frame->v.block_info.height) * (frame->v.padded_width / frame->v.block_info.width);
    JpegBlockCoeffs* jpeg_y_blocks = (JpegBlockCoeffs*)malloc(sizeof(JpegBlockCoeffs) * y_blocks_num);
    JpegBlockCoeffs* jpeg_u_blocks = (JpegBlockCoeffs*)malloc(sizeof(JpegBlockCoeffs) * u_blocks_num);
    JpegBlockCoeffs* jpeg_v_blocks = (JpegBlockCoeffs*)malloc(sizeof(JpegBlockCoeffs) * v_blocks_num);
    JpegDcEncoded* jpeg_y_dc_encoded, *jpeg_u_dc_encoded, *jpeg_v_dc_encoded;
    JpegAcEncoded* jpeg_y_ac_encoded, *jpeg_u_ac_encoded, *jpeg_v_ac_encoded;

    if (jpeg_y_blocks == NULL || jpeg_u_blocks == NULL || jpeg_v_blocks == NULL) {
        /* 有任何一個component配置記憶體失敗，則不會繼續做壓縮 */
        perror("Failed to allocate memory for JPEG blocks of component(s) in a frame.");
        if (jpeg_y_blocks != NULL) free(jpeg_y_blocks);
        if (jpeg_u_blocks != NULL) free(jpeg_u_blocks);
        if (jpeg_v_blocks != NULL) free(jpeg_v_blocks);
        return;
    }

    jpeg_y_dc_encoded = (JpegDcEncoded*)malloc(sizeof(JpegDcEncoded) * y_blocks_num);
    jpeg_y_ac_encoded = (JpegAcEncoded*)malloc(sizeof(JpegAcEncoded) * y_blocks_num);
    if (jpeg_y_dc_encoded == NULL || jpeg_y_ac_encoded == NULL) {
        /* 儲存Y的DC/AC配置記憶體失敗，則不會繼續做壓縮 */
        perror("Failed to allocate memory for DC/AC of Y component in a block.");
        free(jpeg_y_blocks);
        free(jpeg_u_blocks);
        free(jpeg_v_blocks);
        if (jpeg_y_dc_encoded != NULL) free(jpeg_y_dc_encoded);
        if (jpeg_y_ac_encoded != NULL) free(jpeg_y_ac_encoded);
        return;
    }

    jpeg_u_dc_encoded = (JpegDcEncoded*)malloc(sizeof(JpegDcEncoded) * u_blocks_num);
    jpeg_u_ac_encoded = (JpegAcEncoded*)malloc(sizeof(JpegAcEncoded) * u_blocks_num);
    if (jpeg_u_dc_encoded == NULL || jpeg_u_ac_encoded == NULL) {
        /* 儲存U的DC/AC配置記憶體失敗，則不會繼續做壓縮 */
        perror("Failed to allocate memory for DC/AC of U component in a block.");
        free(jpeg_y_blocks);
        free(jpeg_u_blocks);
        free(jpeg_v_blocks);
        free(jpeg_y_dc_encoded);
        free(jpeg_y_ac_encoded);
        if (jpeg_u_dc_encoded != NULL) free(jpeg_u_dc_encoded);
        if (jpeg_u_ac_encoded != NULL) free(jpeg_u_ac_encoded);
        return;
    }

    jpeg_v_dc_encoded = (JpegDcEncoded*)malloc(sizeof(JpegDcEncoded) * v_blocks_num);
    jpeg_v_ac_encoded = (JpegAcEncoded*)malloc(sizeof(JpegAcEncoded) * v_blocks_num);
    if (jpeg_v_dc_encoded == NULL || jpeg_v_ac_encoded == NULL) {
        /* 儲存V的DC/AC配置記憶體失敗，則不會繼續做壓縮 */
        perror("Failed to allocate memory for DC/AC of V component in a block.");
        free(jpeg_y_blocks);
        free(jpeg_u_blocks);
        free(jpeg_v_blocks);
        free(jpeg_y_dc_encoded);
        free(jpeg_y_ac_encoded);
        free(jpeg_u_dc_encoded);
        free(jpeg_u_ac_encoded);
        if (jpeg_v_dc_encoded != NULL) free(jpeg_v_dc_encoded);
        if (jpeg_v_ac_encoded != NULL) free(jpeg_v_ac_encoded);
        return;
    }


    /* 取得計算好的Huffman tables */
    extern Huffman_Table* jpeg_y_dc_huffman_table, * jpeg_y_ac_huffman_table;
    extern Huffman_Table* jpeg_uv_dc_huffman_table, * jpeg_uv_ac_huffman_table;

    BitReader bit_reader;
    int minimum_coded_unit;
    int mcu_y_nums;
    int y_block_idx = 0, u_block_idx = 0, v_block_idx = 0;  // 紀錄要讀出的y/u/v block在當下frame的第幾個block

    create_bit_reader(&bit_reader, fp);

    /* 根據YUV format，計算出每個compoent寫入的情況 */
    if (frame->format == YUV444) {
        minimum_coded_unit = y_blocks_num;
        mcu_y_nums = 1;
    } else if (frame->format == YUV422) {
        minimum_coded_unit = y_blocks_num / 2;
        mcu_y_nums = 2;
    } else if (frame->format == YUV420) {
        minimum_coded_unit = y_blocks_num / 4;
        mcu_y_nums = 4;
    }

    for (int i = 0; i < minimum_coded_unit; i++) {
        for (int j = 0; j < mcu_y_nums; j++) {
            // huffman decode dc of y_block[y_block_idx+j]
            huffman_decode_dc(&bit_reader, &jpeg_y_dc_encoded[y_block_idx+j], jpeg_y_dc_huffman_table);

            // huffman decode ac of y_block[y_block_idx+j]
            huffman_decode_ac(&bit_reader, &jpeg_y_ac_encoded[y_block_idx+j], jpeg_y_ac_huffman_table);
        }

        // huffman decode dc of u_block[u_block_idx]
        huffman_decode_dc(&bit_reader, &jpeg_u_dc_encoded[u_block_idx], jpeg_uv_dc_huffman_table);

        // huffman decode ac of u_block[u_block_idx]
        huffman_decode_ac(&bit_reader, &jpeg_u_ac_encoded[u_block_idx], jpeg_uv_ac_huffman_table);

        // huffman decode dc of v_block[v_block_idx]
        huffman_decode_dc(&bit_reader, &jpeg_v_dc_encoded[v_block_idx], jpeg_uv_dc_huffman_table);

        // huffman decode ac of v_block[v_block_idx]
        huffman_decode_ac(&bit_reader, &jpeg_v_ac_encoded[v_block_idx], jpeg_uv_ac_huffman_table);


        y_block_idx += mcu_y_nums;
        u_block_idx++;
        v_block_idx++;
    }

    fclose(fp);

    // Decode DC係數
    jpeg_decode_dc(jpeg_y_blocks, y_blocks_num, jpeg_y_dc_encoded);
    jpeg_decode_dc(jpeg_u_blocks, u_blocks_num, jpeg_u_dc_encoded);
    jpeg_decode_dc(jpeg_v_blocks, v_blocks_num, jpeg_v_dc_encoded);

    // Decode AC係數
    jpeg_decode_ac(jpeg_y_blocks, y_blocks_num, jpeg_y_ac_encoded);
    jpeg_decode_ac(jpeg_u_blocks, u_blocks_num, jpeg_u_ac_encoded);
    jpeg_decode_ac(jpeg_v_blocks, v_blocks_num, jpeg_v_ac_encoded);

    inverse_zigzag_component(&frame->y, jpeg_y_blocks);
    inverse_zigzag_component(&frame->u, jpeg_u_blocks);
    inverse_zigzag_component(&frame->v, jpeg_v_blocks);

    // 將儲存係數的記憶體釋放
    free(jpeg_y_dc_encoded);
    free(jpeg_u_dc_encoded);
    free(jpeg_v_dc_encoded);
    free(jpeg_y_ac_encoded);
    free(jpeg_u_ac_encoded);
    free(jpeg_v_ac_encoded);
    free(jpeg_y_blocks);
    free(jpeg_u_blocks);
    free(jpeg_v_blocks);
}

void entropy_encode_jpeg(YUVFrame* frame, QuantType quant_type, CompressionType compression_type, EntropyType entropy_type, const char* out_bitstream_path)
{
    /* 取得計算好的Huffman tables */
    extern Huffman_Table* jpeg_y_dc_huffman_table, * jpeg_y_ac_huffman_table;
    extern Huffman_Table* jpeg_uv_dc_huffman_table, * jpeg_uv_ac_huffman_table;


    /* 統計每張frame有多少個block，再配置每個block裡的DC和AC需要儲存的資訊所需要的記憶體空間 */
    int y_blocks_num = (frame->y.padded_height / frame->y.block_info.height) * (frame->y.padded_width / frame->y.block_info.width);
    int u_blocks_num = (frame->u.padded_height / frame->u.block_info.height) * (frame->u.padded_width / frame->u.block_info.width);
    int v_blocks_num = (frame->v.padded_height / frame->v.block_info.height) * (frame->v.padded_width / frame->v.block_info.width);
    JpegBlockCoeffs* jpeg_y_blocks = (JpegBlockCoeffs*)malloc(sizeof(JpegBlockCoeffs) * y_blocks_num);
    JpegBlockCoeffs* jpeg_u_blocks = (JpegBlockCoeffs*)malloc(sizeof(JpegBlockCoeffs) * u_blocks_num);
    JpegBlockCoeffs* jpeg_v_blocks = (JpegBlockCoeffs*)malloc(sizeof(JpegBlockCoeffs) * v_blocks_num);
    JpegDcEncoded* jpeg_y_dc_encoded, *jpeg_u_dc_encoded, *jpeg_v_dc_encoded;
    JpegAcEncoded* jpeg_y_ac_encoded, *jpeg_u_ac_encoded, *jpeg_v_ac_encoded;

    if (jpeg_y_blocks == NULL || jpeg_u_blocks == NULL || jpeg_v_blocks == NULL) {
        /* 有任何一個component配置記憶體失敗，則不會繼續做壓縮 */
        perror("Failed to allocate memory for JPEG blocks of component(s) in a frame.");
        if (jpeg_y_blocks != NULL )free(jpeg_y_blocks);
        if (jpeg_u_blocks != NULL) free(jpeg_u_blocks);
        if (jpeg_v_blocks != NULL) free(jpeg_v_blocks);
        return;
    }

    
    /* 對frame的每個component做zigzag scan，再將結果儲存 */
    zigzag_component(&frame->y, jpeg_y_blocks);
    zigzag_component(&frame->u, jpeg_u_blocks);
    zigzag_component(&frame->v, jpeg_v_blocks);

    /* 對frame的每個component做DC係數DPCM encoding */
    jpeg_encode_dc(jpeg_y_blocks, y_blocks_num, &jpeg_y_dc_encoded);
    jpeg_encode_dc(jpeg_u_blocks, u_blocks_num, &jpeg_u_dc_encoded);
    jpeg_encode_dc(jpeg_v_blocks, v_blocks_num, &jpeg_v_dc_encoded);

    /* 對frame的每個component做AC係數run length encoding */
    jpeg_encode_ac(jpeg_y_blocks, y_blocks_num, &jpeg_y_ac_encoded);
    jpeg_encode_ac(jpeg_u_blocks, u_blocks_num, &jpeg_u_ac_encoded);
    jpeg_encode_ac(jpeg_v_blocks, v_blocks_num, &jpeg_v_ac_encoded);


    /*  將編碼後的DC/AC係數變成bitstream，寫入檔案 
        在entropy coding時，需要根據YUV format處理YUV data (以interleave方式)
        1. 以block為單位來處理
        2. 在一個block裡，編碼以及寫入Y的DC和AC，再來處理U的DC和AC，最後才處理V的DC和AC

        例如: 如果是YUV420，則寫入4個Y blocks，接著寫入1個U block，最後寫入1個V block
     */
    int minimum_coded_unit;
    int mcu_y_nums;
    int y_block_idx = 0, u_block_idx = 0, v_block_idx = 0;  // 紀錄要寫入的y/u/v block在當下frame的第幾個block

    /* 準備將整張frame編碼後的係數寫到檔案 */
    BitWriter bit_writer;
    FILE* fp = fopen(out_bitstream_path, "wb");
    if (!fp) {
        fprintf(stderr, "Failed to open file: %s\n", out_bitstream_path);
        return;
    }

    /* 將frame的width/height/YUV format/quantization type/ compression type/ entropy type 寫到header */
    jpeg_encode_header(fp, frame, quant_type, compression_type, entropy_type);
    

    /* 建立一個bit writer，紀錄整張frame的bitstream寫入情況 */
    create_bit_writer(&bit_writer, fp);

    /* 根據YUV format，計算出每個compoent寫入的情況 */
    if (frame->format == YUV444) {
        minimum_coded_unit = y_blocks_num;
        mcu_y_nums = 1;
    } else if (frame->format == YUV422) {
        minimum_coded_unit = y_blocks_num / 2;
        mcu_y_nums = 2;
    } else if (frame->format == YUV420) {
        minimum_coded_unit = y_blocks_num / 4;
        mcu_y_nums = 4;
    }

    for (int i = 0; i < minimum_coded_unit; i++) {
        /* 先寫入Y component的blocks (dc再來ac)，寫入mcu_y_nums個blocks 
           接著寫入U component的blocks (dc再來ac)，寫入1個block
           最後寫入V compoents的blocks (dc再來ac)，寫入1個block
         */
        for (int j = 0; j < mcu_y_nums; j++) {
            // huffman encode dc of y_block[y_block_idx + j]
            huffman_encode_dc(&bit_writer, &jpeg_y_dc_encoded[y_block_idx+j], jpeg_y_dc_huffman_table);

            // huffman encode ac of y_block[y_block_idx+j]
            huffman_encode_ac(&bit_writer, &jpeg_y_ac_encoded[y_block_idx+j], jpeg_y_ac_huffman_table);
        }
        // huffman encode dc of u_block[u_block_idx]
        huffman_encode_dc(&bit_writer, &jpeg_u_dc_encoded[u_block_idx], jpeg_uv_dc_huffman_table);

        // huffman encode ac of u_block[u_block_idx]
        huffman_encode_ac(&bit_writer, &jpeg_u_ac_encoded[u_block_idx], jpeg_uv_ac_huffman_table);
        
        // huffman encode dc of v_block[v_block_idx]
        huffman_encode_dc(&bit_writer, &jpeg_v_dc_encoded[v_block_idx], jpeg_uv_dc_huffman_table);

        // huffman encode ac of v_block[v_block_idx]
        huffman_encode_ac(&bit_writer, &jpeg_v_ac_encoded[v_block_idx], jpeg_uv_ac_huffman_table);

        /* 更新compoents的block在frame裡的index */
        y_block_idx += mcu_y_nums;
        u_block_idx++;
        v_block_idx++;
    }

    /* 一張frame裡的Y/U/V的所有block都變成bitstream也有寫到檔案
       最後處理還留在buffer裡的bits
       Flush: 處理剩下沒寫入的bits，補齊1個byte的資料
     */
    if (bit_writer.bit_count > 0) {
        /* 向右對齊 */
        bit_writer.buffer <<= (8-bit_writer.bit_count);

        /* 右邊bits補1 */
        bit_writer.buffer |= ((1 << (8-bit_writer.bit_count)) - 1);

        fputc(bit_writer.buffer, bit_writer.fp);

        /* byte stuffing: 避開JPEG檔案裡的marker */
        if (bit_writer.buffer == 0xff) {
            fputc(0x00, bit_writer.fp);
        }
    }

    /* 一張frame的DC/AC係數壓縮寫檔後，關檔 */
    fclose(fp);

    /* 將儲存係數的記憶體釋放 */
    free(jpeg_y_dc_encoded);
    free(jpeg_u_dc_encoded);
    free(jpeg_v_dc_encoded);
    free(jpeg_y_ac_encoded);
    free(jpeg_u_ac_encoded);
    free(jpeg_v_ac_encoded);
    free(jpeg_y_blocks);
    free(jpeg_u_blocks);
    free(jpeg_v_blocks);
}