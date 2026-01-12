/*  Date: 20260105 by Dennis Wong
 *  資料結構和流程設計參考來源:
 *     1. 視訊壓縮課程內容
 *     2. wiki內容
 *        https://en.wikipedia.org/wiki/JPEG#JPEG_compression
 *     3. https://en.wikibooks.org/wiki/JPEG_-_Idea_and_Practice/The_Huffman_coding
 *     4. StackOverflow
 *     5. ChatGPT
*/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<errno.h>
#include<ctype.h>
#include"yuv.h"
#include"transform.h"
#include"block.h"
#include"quantization/quantization.h"
#include"entropy/entropy.h"
#include"main.h"



int create_output_dirs(const char* bs_file_dir, int save_yuv_raw_frame, const char* output_yuv_raw_dir, int save_idct_yuv, const char* output_yuv_idct_dir);


/*  function: trim()
    Params:
        char* s : 輸入的字串

    Return:
        將字串前後的space去除的結果

    Result:
 */
void trim(char* s)
{
    if (s == NULL) return;

    /* 移除字串後面的space */
    char* end = s + strlen(s) - 1;
    while (end != s && isspace(*end)) {
        *end = '\0';
        end--;
    }

    /* 移除字串前面的space */
    char* start = s;
    while (*start && isspace(*start)) start++;

    /* 將不是space的部分搬回原來的字串 */
    if (start != s) memmove(s, start, strlen(start)+1);
}

/*  function: load_encode_config()
    Params:
        AppEncodeConfig* config      : 保存yuv raw data和壓縮的相關設定
        const char* config_file_path : encoding的相關設定檔案

    Return:
        None

    Result:
        1. 得到yuv raw data大小和foramt的資訊
        2. 紀錄使用哪一種壓縮方式
        3. 取得壓縮後的路徑資訊
 */
void load_encode_config(AppEncodeConfig* config, const char* config_file_path)
{   
    char line[4096];
    FILE* fp = fopen(config_file_path, "rb");
    if (!fp) {
        fprintf(stderr, "Failed to open file: %s\n", config_file_path);
        return;
    }

    while (fgets(line, sizeof(line), fp)) {
        // 遇到註解行，跳過，讀取下一行
        if (strchr(line, '#')) continue;

        char* delim = strchr(line, ':');
        // 遇到空白行
        if (delim == NULL) continue;
        
        /* truncate key的字串，只取':'前面內容當作key */
        *delim = '\0';

        char* key = line;
        char* value = delim+1;

        /* 去除字串前後的space */
        trim(key);
        trim(value);

        if (strcmp(key, "input_path") == 0) {
            strncpy(config->input_path, value, MAX_PATH_LEN);
        } else if (strcmp(key, "width") == 0) {
            config->yuv_raw_info.width = atoi(value);
        } else if (strcmp(key, "height") == 0) {
            config->yuv_raw_info.height = atoi(value);
        } else if (strcmp(key, "format") == 0) {
            if (strcmp(value, "YUV444") == 0) config->yuv_raw_info.format = YUV444;
            else if (strcmp(value, "YUV422") == 0) config->yuv_raw_info.format = YUV422;
            else if (strcmp(value, "YUV420") == 0) config->yuv_raw_info.format = YUV420;
        } else if (strcmp(key, "block_size") == 0) {
            if (strcmp(value, "BLOCK_4x4") == 0) config->compress_info.block_info.b_size = BLOCK_4x4;
            else if (strcmp(value, "BLOCK_8x8") == 0) config->compress_info.block_info.b_size = BLOCK_8x8;
        } else if (strcmp(key, "block_width") == 0) {
            config->compress_info.block_info.width = atoi(value);
        } else if (strcmp(key, "block_height") == 0) {
            config->compress_info.block_info.height = atoi(value);
        } else if (strcmp(key, "compress_type") == 0) {
            if (strcmp(value, "JPEG_SEQUENTIAL") == 0) config->compress_info.comprss_type = JPEG_SEQUENTIAL;
        } else if (strcmp(key, "quant_type") == 0) {
            if (strcmp(value, "JPEG_QUANT_STANDARD") == 0) config->compress_info.quant_type = JPEG_QUANT_STANDARD;
        } else if (strcmp(key, "entropy_type") == 0) {
            if (strcmp(value, "HUFFMAN") == 0) config->compress_info.entropy_type = HUFFMAN;
        } else if (strcmp(key, "output_yuv_raw_dir") == 0) {
            strncpy(config->output_yuv_raw_dir, value, MAX_PATH_LEN);
        } else if (strcmp(key, "output_bitstream_dir") == 0) {
            strncpy(config->output_bitstream_dir, value, MAX_PATH_LEN);
        } else if (strcmp(key, "save_yuv_raw_frame") == 0) {
            config->option_info.save_yuv_raw_frame = atoi(value);
        } else if (strcmp(key, "truncate_yuv_frame") == 0) {
            config->option_info.truncate_yuv_frame = atoi(value);
        } else if (strcmp(key, "truncate_yuv_index") == 0) {
            config->option_info.truncate_yuv_index = atoi(value);
        }
    }
    fclose(fp);
}


/*  function: load_decode_config()
    Params:
        AppDecodeConfig* config      : 保存yuv raw data和解碼的相關設定
        const char* config_file_path : encoding的相關設定檔案

    Return:
        None

    Result:
        1. 得到yuv raw data大小和foramt的資訊
        2. 紀錄使用哪一種壓縮方式
        3. 取得壓縮檔的路徑資訊
 */
void load_decode_config(AppDecodeConfig* config, const char* config_file_path)
{   
    char line[4096];
    FILE* fp = fopen(config_file_path, "rb");
    if (!fp) {
        fprintf(stderr, "Failed to open file: %s\n", config_file_path);
        return;
    }

    while (fgets(line, sizeof(line), fp)) {
        // 遇到註解行，跳過，讀取下一行
        if (strchr(line, '#')) continue;

        char* delim = strchr(line, ':');
        // 遇到空白行
        if (delim == NULL) continue;
        
        /* truncate key的字串，只取':'前面內容當作key */
        *delim = '\0';

        char* key = line;
        char* value = delim+1;

        /* 去除字串前後的space */
        trim(key);
        trim(value);

        if (strcmp(key, "width") == 0) {
            config->yuv_raw_info.width = atoi(value);
        } else if (strcmp(key, "height") == 0) {
            config->yuv_raw_info.height = atoi(value);
        } else if (strcmp(key, "format") == 0) {
            if (strcmp(value, "YUV444") == 0) config->yuv_raw_info.format = YUV444;
            else if (strcmp(value, "YUV422") == 0) config->yuv_raw_info.format = YUV422;
            else if (strcmp(value, "YUV420") == 0) config->yuv_raw_info.format = YUV420;
        } else if (strcmp(key, "block_size") == 0) {
            if (strcmp(value, "BLOCK_4x4") == 0) config->compress_info.block_info.b_size = BLOCK_4x4;
            else if (strcmp(value, "BLOCK_8x8") == 0) config->compress_info.block_info.b_size = BLOCK_8x8;
        } else if (strcmp(key, "block_width") == 0) {
            config->compress_info.block_info.width = atoi(value);
        } else if (strcmp(key, "block_height") == 0) {
            config->compress_info.block_info.height = atoi(value);
        } else if (strcmp(key, "compress_type") == 0) {
            if (strcmp(value, "JPEG_SEQUENTIAL") == 0) config->compress_info.comprss_type = JPEG_SEQUENTIAL;
        } else if (strcmp(key, "quant_type") == 0) {
            if (strcmp(value, "JPEG_QUANT_STANDARD") == 0) config->compress_info.quant_type = JPEG_QUANT_STANDARD;
        } else if (strcmp(key, "entropy_type") == 0) {
            if (strcmp(value, "HUFFMAN") == 0) config->compress_info.entropy_type = HUFFMAN;
        } else if (strcmp(key, "output_yuv_idct_dir") == 0) {
            strncpy(config->output_yuv_idct_dir, value, MAX_PATH_LEN);
        } else if (strcmp(key, "input_bitstream_dir") == 0) {
            strncpy(config->input_bitstream_dir, value, MAX_PATH_LEN);
        } else if (strcmp(key, "save_idct_yuv_frame") == 0) {
            config->option_info.save_idct_yuv_frame = atoi(value);
        }
    }
    fclose(fp);
}

void app_encode_process(AppEncodeConfig* appencconfig)
{
    YUVVideo* yuv_video;
    char bs_file_path[MAX_PATH_LEN];
    int ret = 0;
    
    
    // 目前只支援YUV planar格式
    yuv_video = read_yuv_file(appencconfig->input_path, appencconfig->yuv_raw_info.width, \
                              appencconfig->yuv_raw_info.height, appencconfig->yuv_raw_info.format, \
                              appencconfig->option_info.truncate_yuv_frame, appencconfig->option_info.truncate_yuv_index);
    if (yuv_video != NULL) {
        /* 建立存放yuv raw frame的資料夾 */
        ret =create_output_dirs(appencconfig->output_bitstream_dir, appencconfig->option_info.save_yuv_raw_frame, \
                                appencconfig->output_yuv_raw_dir, 0, NULL);
        if (ret != 1) {
            /* 存放的壓縮data的資料夾建立失敗，則釋放frame的記憶體，不繼續做後續的壓縮 */
            for (int i = 0; i < yuv_video->total_frames; i++) {
                if (yuv_video->frames[i] != NULL) {
                    free_yuv_frame(yuv_video->frames[i]);
                }
            }
            free(yuv_video);
            return -1;
        }
        /* 將讀取後的yuv raw data儲存 */
        if (appencconfig->option_info.save_yuv_raw_frame) {
            char raw_filename[256];
            for (int i = 0; i < yuv_video->total_frames; i++) {
                sprintf(raw_filename, "%sframe_%04d.yuv", appencconfig->output_yuv_raw_dir, i);
                save_raw_frame_to_yuv_file(raw_filename, yuv_video->frames[i]);
            }
        }

        /* 先處理好entropy coding需要的資源 */
        entropy_initialization(appencconfig->compress_info.entropy_type);

        /* 以sequential方式處理video裡的每一張frame */
        for (int i = 0; i < yuv_video->total_frames; i++) {
            yuv_video->frames[i]->y.block_info = appencconfig->compress_info.block_info;
            yuv_video->frames[i]->u.block_info = appencconfig->compress_info.block_info;
            yuv_video->frames[i]->v.block_info = appencconfig->compress_info.block_info;

            /* DCT forward */
            transform_frame(yuv_video->frames[i]);

            /* Quantization forward */
            quantize_frame(yuv_video->frames[i], appencconfig->compress_info.quant_type);

            /* Entropy encoding */
            memset(bs_file_path, 0x0, sizeof(bs_file_path));
            sprintf(bs_file_path, "%sframe_%04d_bs.bin", appencconfig->output_bitstream_dir, i);
            entropy_encode(yuv_video->frames[i], appencconfig->compress_info.quant_type, appencconfig->compress_info.comprss_type, \
                           appencconfig->compress_info.entropy_type, bs_file_path);
        }

        /* 釋放entropy coding的資源 */
        entropy_destropy(appencconfig->compress_info.entropy_type);

        /* 將video使用到的memory釋放 */
        for (int i = 0; i < yuv_video->total_frames; i++) {
            if (yuv_video->frames[i] != NULL) {
                free_yuv_frame(yuv_video->frames[i]);
            }
        }

        printf("Encoding %d frames has successfully done.\n", yuv_video->total_frames);
        free(yuv_video);
    }
}


int app_decode_process(AppDecodeConfig* appdecconfig)
{
    char bs_file_path[MAX_PATH_LEN];
    char idct_filename[MAX_PATH_LEN];
    YUVFrame* frame;
    FILE* fp;
    int ret;
    /* 建立存放解碼後的frame的資料夾 */
    ret =create_output_dirs(appdecconfig->input_bitstream_dir, 0, NULL, \
                            appdecconfig->option_info.save_idct_yuv_frame, appdecconfig->output_yuv_idct_dir);

    if (ret != 1) {
        /* 存放的解碼後的data的資料夾建立失敗，不繼續做後續的壓縮 */
        return -1;
    }

    int frame_idx = 0;
    
    int alignment = 64;  // u/v的height要align 8倍， MCU下的Y要align 16倍，取64-alignment

    /* 先處理好entropy coding需要的資源 */
    entropy_initialization(appdecconfig->compress_info.entropy_type);

    /* 根據decode設定，取得需要的記憶體空間 */
    init_yuv_frame(&frame, appdecconfig->yuv_raw_info.format, appdecconfig->yuv_raw_info.width, appdecconfig->yuv_raw_info.height, alignment);
    
    /* 設定y、u、v的block資訊 */
    frame->y.block_info = appdecconfig->compress_info.block_info;
    frame->u.block_info = appdecconfig->compress_info.block_info;
    frame->v.block_info = appdecconfig->compress_info.block_info;
    
    printf("Y w:%d h:%d pad_w:%d pad_h:%d\n", frame->y.width, frame->y.height, frame->y.padded_width, frame->y.padded_height);
    printf("U w:%d h:%d pad_w:%d pad_h:%d\n", frame->u.width, frame->u.height, frame->u.padded_width, frame->u.padded_height);
    printf("V w:%d h:%d pad_w:%d pad_h:%d\n", frame->v.width, frame->v.height, frame->v.padded_width, frame->v.padded_height);


    /* 讀取以及解碼所有的bitstream檔案 */
    while (1) {
        /* 設定bitstream檔案名稱 */
        memset(bs_file_path, 0x0, MAX_PATH_LEN);
        sprintf(bs_file_path, "%sframe_%04d_bs.bin", appdecconfig->input_bitstream_dir, frame_idx);

        fp = fopen(bs_file_path, "rb");
        if (fp == NULL) {
            printf("Decoding %d frames has successfully done.\n", frame_idx);
            break;
        }

        /* entropy decoding :
            檢查bitstream解碼出來的資訊是不是和設定檔相同
            不相同則不會繼續解碼
         */
        entropy_decode(frame, appdecconfig->compress_info.quant_type, appdecconfig->compress_info.comprss_type, \
                       appdecconfig->compress_info.entropy_type, bs_file_path);

        /* de-quantization */
        dequantize_frame(frame, appdecconfig->compress_info.quant_type);

        /* transform backward */
        reverse_transform_frame(frame);

        /* 將idct後的yuv data儲存下來 */
        memset(idct_filename, 0x0, sizeof(idct_filename));
        sprintf(idct_filename, "%sframe_%04d.yuv", appdecconfig->output_yuv_idct_dir, frame_idx);
        save_idct_frame_to_yuv_file(idct_filename, frame);

        /* 將當下解碼後的raw data buffer清空，給下一張frame使用 */
        memset(frame->y.raw_data, 0x0, sizeof(frame->y.width * frame->y.height));
        memset(frame->u.raw_data, 0x0, sizeof(frame->u.width * frame->u.height));
        memset(frame->y.raw_data, 0x0, sizeof(frame->y.width * frame->y.height));
        memset(frame->y.padded_data, 0x0, sizeof(frame->y.padded_width * frame->y.padded_height));
        memset(frame->u.padded_data, 0x0, sizeof(frame->u.padded_width * frame->u.padded_height));
        memset(frame->v.padded_data, 0x0, sizeof(frame->v.padded_width * frame->v.padded_height));

        frame_idx++;
    }
    
    /* 釋放entropy coding的資源 */
    entropy_destropy(appdecconfig->compress_info.entropy_type);

    free_yuv_frame(frame);
    return 0;
}

int main(int argc, char* argv[])
{
    YUVVideo* yuv_video;
    int ret = 0;
    char config_file_path[MAX_PATH_LEN];
    AppEncodeConfig appencconfig = {0};
    AppDecodeConfig appdecconfig = {0};

    if (argc == 1) {
        perror("Please input encode or decode you want to do, and also config file path.\n");
        return -1;
    }

    if (strcmp(argv[1], "enc") == 0) {
        strncpy(config_file_path, argv[2], MAX_PATH_LEN);
        trim(config_file_path);
        load_encode_config(&appencconfig, config_file_path);
        app_encode_process(&appencconfig);
    } else if (strcmp(argv[1], "dec") == 0) {
        strncpy(config_file_path, argv[2], MAX_PATH_LEN);
        trim(config_file_path);
        load_decode_config(&appdecconfig, config_file_path);
        ret = app_decode_process(&appdecconfig);
    } else {
        perror("Please input \"enc\" or \"dec\" as the sencond argument.\n");
        return -1;
    }
    return ret;
}


int create_output_dirs(const char* bs_file_dir, int save_yuv_raw_frame, const char* output_yuv_raw_dir, int save_idct_yuv, const char* output_yuv_idct_dir)
{
    int ret = 0;
    char cmd[1024];

    /* 建立儲存bitstream的資料夾 */
    if (bs_file_dir != NULL) {
        struct stat raw_st = {0};
        if (stat(bs_file_dir, &raw_st) == -1) {
            memset(cmd, 0, sizeof(cmd));
            snprintf(cmd, sizeof(cmd), "mkdir -p %s", bs_file_dir);
            if (system(cmd) != 0) {
                perror("Create output directory failed");
                return -1;
            }
        }
        ret |= 1;
    }
    

    if (save_yuv_raw_frame) {
        /* 建立儲存yuv raw data的資料夾 */
        struct stat raw_st = {0};
        if (stat(output_yuv_raw_dir, &raw_st) == -1) {
            memset(cmd, 0, sizeof(cmd));
            snprintf(cmd, sizeof(cmd), "mkdir -p %s", output_yuv_raw_dir);
            if (system(cmd) != 0) {
                perror("Create output directory failed");
                return -1;
            }
        }
        ret |= 1;
    }

    /* 建立idct後的yuv data的資料夾 */
    if (save_idct_yuv) {
        struct stat idct_st = {0};
        if (stat(output_yuv_idct_dir, &idct_st) == -1) {
            memset(cmd, 0, sizeof(cmd));
            snprintf(cmd, sizeof(cmd), "mkdir -p %s", output_yuv_idct_dir);
            if (system(cmd) != 0) {
                perror("Create output directory failed");
                return -1;
            }
        }
        ret |= 1;
    }
    return ret;
}