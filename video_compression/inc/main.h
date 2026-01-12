#ifndef MAIN_H
#define MAIN_H

#include"yuv.h"
#include"block.h"
#include"quantization/quantization.h"
#include"entropy/entropy.h"

#define MAX_PATH_LEN (1024)

typedef struct {
    int save_yuv_raw_frame;  // 是否儲存yuv raw data. 0: 不儲存 1: 儲存
    int save_idct_yuv_frame; // 是否儲存idct後的yuv frame. 0: 不儲存 1: 儲存
    int truncate_yuv_frame;  // 是否只使用前面部分的yuv data. 0: 使用全部的frames 1: 使用前面部分的frames
    int truncate_yuv_index;  // 如果有truncate，則指定從哪一張frame做truncate
}OptionInfo;

typedef struct {
    int width;
    int height;
    YUVFormat format;
}YUVInInfo;

typedef struct {
    BlockInfo block_info;
    CompressionType comprss_type;
    QuantType quant_type;
    EntropyType entropy_type;
}CompressionInfo;

/* 定義編碼需要的參數 */
typedef struct {
    char input_path[MAX_PATH_LEN];           // YUV raw data路徑. ex: ./cur_dir/videos/yuv_file_name.yuv
    char output_yuv_raw_dir[MAX_PATH_LEN];   // 儲存yuv raw data的路徑
    char output_bitstream_dir[MAX_PATH_LEN]; // 儲存entropy後的bitstream路徑
    OptionInfo option_info;                  // 儲存需要開放的功能
    YUVInInfo yuv_raw_info;                  // 讀取yuv raw data需要的資訊
    CompressionInfo compress_info;           // 壓縮(quant.和entropy)yuv data需要的設定
}AppEncodeConfig;

/* 定義解碼需要的參數 */
typedef struct {
    char output_yuv_idct_dir[MAX_PATH_LEN];  // 儲存idct後的yuv data的路徑
    char input_bitstream_dir[MAX_PATH_LEN];  // Bitstream路徑. ex: ./output/bitstream/xxx.bin
    OptionInfo option_info;                  // 儲存需要開放的功能
    YUVInInfo yuv_raw_info;                  // 從bitstream讀取yuv info.後，存放在這裡
    CompressionInfo compress_info;           // 壓縮(quant.和entropy)yuv data需要的設定
}AppDecodeConfig;


#endif
