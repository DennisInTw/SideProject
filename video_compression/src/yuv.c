#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"yuv.h"

void get_yuv_size_info(YUVFormat format, int width, int height, size_t* frame_size, size_t* y_size, size_t* u_size, size_t* v_size)
{
    *y_size = width * height;

    switch(format) {
        case YUV444:
            *u_size = width * height;
            *v_size = width * height;
            break;
        case YUV422:
            *u_size = (width / 2) * height;
            *v_size = (width / 2) * height;
            break;
        case YUV420:
            *u_size = (width / 2) * (height / 2);
            *v_size = (width / 2) * (height / 2);
            break;
        default:
            perror("Unknown YUV format");
            return;
    }

    *frame_size = *y_size + *u_size + *v_size;
}


/*  function: set_yuv_frame_info()
    Params:
        YUVFrame** frame : y/u/v data的資訊
        YUVFormat format : yuv raw data的yuv format
        int width        : yuv raw data的width
        int height       : yuv raw data的height
        int alignment    : 對y/u/v做padding的alignment

    Return:
        根據YUV format設定好的raw data Y width/height、U width/height、V width/height
        padded data Y width/height、U width/height、V width/height

    Result:
        根據YUV format決定的YUV data width/height
 */
void set_yuv_frame_info(YUVFrame* frame, YUVFormat format, int width, int height, int alignment)
{
    frame->format = format;
    frame->y.width = width;
    frame->y.height = height;

    switch(format) {
        case YUV444:
            frame->u.width = width;
            frame->u.height = height;
            frame->v.width = width;
            frame->v.height = height;
            break;
        case YUV422:
            frame->u.width = width / 2;
            frame->u.height = height;
            frame->v.width = width / 2;
            frame->v.height = height;
            break;
        case YUV420:
            frame->u.width = width / 2;
            frame->u.height = height / 2;
            frame->v.width = width / 2;
            frame->v.height = height / 2;
            break;
        default:
            perror("Unknown YUV format");
            return;
    }

    /* 取round-up : (ori_val + alignment-1) & ~(alignment-1) */
    frame->y.padded_width = (width + (alignment-1)) & ~(alignment-1);
    frame->y.padded_height = (height + (alignment-1)) & ~(alignment-1);
    frame->u.padded_width = (frame->u.width + (alignment-1)) & ~(alignment-1);
    frame->u.padded_height = (frame->u.height + (alignment-1)) & ~(alignment-1);
    frame->v.padded_width = (frame->v.width + (alignment-1)) & ~(alignment-1);
    frame->v.padded_height = (frame->v.height + (alignment-1)) & ~(alignment-1);
}


/*  function: init_yuv_frame()
    Params:
        YUVFrame** frame : y/u/v data的資訊
        YUVFormat format : yuv raw data的yuv format
        int width        : yuv raw data的width
        int height       : yuv raw data的height
        int alignment    : 對y/u/v做padding的alignment

    Return:
        NULL : 配置給frame的記憶體失敗

        YUVFrame :
            配置記憶體存放一張frame的資訊
            包含 raw data的width/height/address/data size
            以及 padded data的width/height/data size

    Result:
        1. 得到yuv video的所有frame內容，將每張frame單獨存放在buffer裡
        2. 取得yuv video一共有多少張frames
        3. 每張frame有raw data的width/height，以及aligment後的padded width/height資訊
 */
YUVFrame* init_yuv_frame(YUVFrame** frame, YUVFormat format, int width, int height, int alignment)
{
    size_t y_raw_size, u_raw_size, v_raw_size;
    size_t y_padded_size, u_padded_size, v_padded_size;

    /* 配置記憶體給一張frame */
    *frame = (YUVFrame*)malloc(sizeof(YUVFrame));
    if (*frame == NULL) {
        perror("Allocate YUVFrame failed");
        return NULL;
    } else {
        /* 根據YUV format，設定frame裡的YUV raw data的width/height，和padded data的width/height */
        set_yuv_frame_info(*frame, format, width, height, alignment);

        y_raw_size = (*frame)->y.width * (*frame)->y.height;
        u_raw_size = (*frame)->u.width * (*frame)->u.height;
        v_raw_size = (*frame)->v.width * (*frame)->v.height;
        y_padded_size = (*frame)->y.padded_width * (*frame)->y.padded_height;
        u_padded_size = (*frame)->u.padded_width * (*frame)->u.padded_height;
        v_padded_size = (*frame)->v.padded_width * (*frame)->v.padded_height;

        (*frame)->y.raw_data = (uint8_t*)malloc(sizeof(uint8_t) * y_raw_size);
        (*frame)->u.raw_data = (uint8_t*)malloc(sizeof(uint8_t) * u_raw_size);
        (*frame)->v.raw_data = (uint8_t*)malloc(sizeof(uint8_t) * v_raw_size);

        (*frame)->y.padded_data = (int16_t*)malloc(sizeof(int16_t) * y_padded_size);
        (*frame)->u.padded_data = (int16_t*)malloc(sizeof(int16_t) * u_padded_size);
        (*frame)->v.padded_data = (int16_t*)malloc(sizeof(int16_t) * v_padded_size);

        /* 將多餘的部分都初始化成0, 不能使用128當作初始值,不然底部會看到奇怪線條 */
        memset((*frame)->y.padded_data, 0, sizeof(int16_t) * y_padded_size);
        memset((*frame)->u.padded_data, 0, sizeof(int16_t) * u_padded_size);
        memset((*frame)->v.padded_data, 0, sizeof(int16_t) * v_padded_size);

        // 一但有任一個記憶體配置失敗,這張frame就不能使用,釋放data記憶體
        if ((*frame)->y.raw_data == NULL || (*frame)->u.raw_data == NULL || (*frame)->v.raw_data == NULL ||
            (*frame)->y.padded_data == NULL || (*frame)->u.padded_data == NULL || (*frame)->v.padded_data == NULL) {
            perror("Allocate YUV data memory failed");
            if ((*frame)->y.raw_data) free((*frame)->y.raw_data);
            if ((*frame)->u.raw_data) free((*frame)->u.raw_data);
            if ((*frame)->v.raw_data) free((*frame)->v.raw_data);
            if ((*frame)->y.padded_data) free((*frame)->y.padded_data);
            if ((*frame)->u.padded_data) free((*frame)->u.padded_data);
            if ((*frame)->v.padded_data) free((*frame)->v.padded_data);
            
            return NULL;
        }
    }
    return *frame;
}

/*  function: read_yuv_frame_data()
    Params:
        FILE* fp         : yuv raw data的file descriptor (fd)
        YUVFrame* frame  : 存放frame的資訊
        YUVFormat format : yuv raw data的yuv format

    Return:
        NULL : 讀取yuv raw data失敗

        YUVFrame* :
            讀取檔案裡的y/u/v raw data，將raw data放在frame的y/u/v buffer裡
            目前只支援YUV planar格式

    Result:
        取得yuv raw data裡的y/u/v data，將它們放在frame上各自的buffer裡
 */
YUVFrame* read_yuv_frame_data(FILE* fp, YUVFrame* frame, YUVFormat format)
{
    size_t y_raw_size, u_raw_size, v_raw_size;

    y_raw_size = frame->y.width * frame->y.height;
    u_raw_size = frame->u.width * frame->u.height;
    v_raw_size = frame->v.width * frame->v.height;

    /* YUV planar格式: Y/U/V三個平面分開依序存放 [y1,y2,...,yn][u1,u2,...,un][v1,v2,...,vn] */
    if (fread(frame->y.raw_data, 1, y_raw_size, fp) != y_raw_size ||
        fread(frame->u.raw_data, 1, u_raw_size, fp) != u_raw_size ||
        fread(frame->v.raw_data, 1, v_raw_size, fp) != v_raw_size) {

        /* 任何一個data讀取失敗就回傳NULL */
        perror("Read YUV raw data failed");
        return NULL;
    }
    return frame;
}


/*  function: read_yuv_file()
    Params:
        const char* file : yuv raw data的路徑
        int width        : yuv raw data的width
        int height       : yuv raw data的height
        YUVFormat format : yuv raw data的yuv format

    Return:
        NULL : 讀取yuv檔案失敗 或是 配置給frame的記憶體失敗

        YUVVideo :
            讀取影片裡的所有frames，將frame放在buffer裡
            目前只支援YUV planar格式

    Result:
        1. 得到yuv video的所有frame內容，將每張frame單獨存放在buffer裡
        2. 取得yuv video一共有多少張frames
        3. 每張frame有raw data的width/height，以及aligment後的padded width/height資訊
 */
YUVVideo* read_yuv_file(const char* file, int width, int height, YUVFormat format, int truncate_yuv_frame, int truncate_yuv_index)
{
    size_t frame_size, y_size, u_size, v_size;
    long video_file_size;
    int total_frames;
    YUVFrame** frames;

    FILE* fp = fopen(file, "rb");
    if (!fp) {
        fprintf(stderr, "Failed to open file: %s\n", file);
        return NULL;
    }

    // 計算每張frame的資訊
    get_yuv_size_info(format, width, height, &frame_size, &y_size, &u_size, &v_size);

    // 取得frame數量
    fseek(fp, 0, SEEK_END);
    video_file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    total_frames = video_file_size / frame_size;
    printf("Total frames: %d   frame size: %zu bytes\n", total_frames, frame_size);
    printf("Truncated frame index: %d\n", truncate_yuv_index);


    /* 根據truncated frames個數，配置需要的指標型態的記憶體 */
    frames = (YUVFrame**)malloc(sizeof(YUVFrame*) * truncate_yuv_index);
    if (frames == NULL) {
        perror("Allocate frames pointer failed");
        fclose(fp);
        return NULL;
    }


    /*  對前面配置好的記憶體配置完整的frame記憶體空間，再將yuv raw data搬到記憶體空間 
        一旦有任何一張frame的記憶體配置失敗，都不能再前進，需要釋放空間然後return
        只有每張frame都可以拿到記憶體才能夠繼續前進
     */
    for (int i = 0; i < truncate_yuv_index; i++) {
        /* 為了u/v切成block後，width和height可能不足整數，因此需要做8-alignment，但是考慮到MCU的關係，直接使用64-alignment */
        int alignment = 64;

        /* 確認是不是能夠配置完整記憶體給一張frame */
        if (init_yuv_frame(&frames[i], format, width, height, alignment) != NULL) {
            /* 讀取yuv raw data，再放到buffer裡 */
            if (read_yuv_frame_data(fp, frames[i], format) == NULL) {
                fprintf(stderr, "Failed to read yuv frame %d\n", i);
                // 將前面的所有frames記憶體釋放
                for (int j = 0; j < i; j++) {
                    free(frames[j]->y.raw_data);
                    free(frames[j]->u.raw_data);
                    free(frames[j]->v.raw_data);
                    free(frames[j]->y.padded_data);
                    free(frames[j]->u.padded_data);
                    free(frames[j]->v.padded_data);
                    free(frames[j]);
                }
                free(frames);
                fclose(fp);
                return NULL;
            }
        } else {
            /* 配置失敗則將前面配置好的frame空間都釋放 */
            for (int j = 0; j < i; j++) {
                free(frames[j]->y.raw_data);
                free(frames[j]->u.raw_data);
                free(frames[j]->v.raw_data);
                free(frames[j]->y.padded_data);
                free(frames[j]->u.padded_data);
                free(frames[j]->v.padded_data);
                free(frames[j]);
            }
            free(frames);
            fclose(fp);
            return NULL;
        }
    }
    fclose(fp);

    /* 將video裡所有frame的內容統一用YUVVideo包起來 */
    YUVVideo* video = (YUVVideo*)malloc(sizeof(YUVVideo));
    if (video == NULL) {
        perror("Allocate YUVVideo failed");
        for (int i = 0; i < total_frames; i++) {
            free(frames[i]->y.raw_data);
            free(frames[i]->u.raw_data);
            free(frames[i]->v.raw_data);
            free(frames[i]->y.padded_data);
            free(frames[i]->u.padded_data);
            free(frames[i]->v.padded_data);
            free(frames[i]);
        }
        free(frames);
        return NULL;
    }
    video->total_frames = truncate_yuv_index;
    video->frames = frames;

    return video;

}

void save_raw_frame_to_yuv_file(const char* file, YUVFrame* frame)
{
    FILE* fp = fopen(file, "wb");
    if (fp == NULL) {
        fprintf(stderr, "Failed to save %s YUV file\n", file);
        return;
    }

    size_t y_raw_size, u_raw_size, v_raw_size;

    y_raw_size = frame->y.width * frame->y.height;
    u_raw_size = frame->u.width * frame->u.height;
    v_raw_size = frame->v.width * frame->v.height;

    if (fwrite(frame->y.raw_data, 1, y_raw_size, fp) != y_raw_size ||
        fwrite(frame->u.raw_data, 1, u_raw_size, fp) != u_raw_size ||
        fwrite(frame->v.raw_data, 1, v_raw_size, fp) != v_raw_size) {
        perror("Write YUV raw data failed");
    }

    fclose(fp);
}

void save_idct_frame_to_yuv_file(const char* file, YUVFrame* frame)
{
    FILE* fp = fopen(file, "wb");
    if (fp == NULL) {
        fprintf(stderr, "Failed to save %s YUV file\n", file);
        return;
    }

    size_t y_size = frame->y.width * frame->y.height;
    size_t u_size = frame->u.width * frame->u.height;
    size_t v_size = frame->v.width * frame->v.height;
    uint8_t* y_buffer = (uint8_t*)malloc(sizeof(uint8_t) * y_size);
    uint8_t* u_buffer = (uint8_t*)malloc(sizeof(uint8_t) * u_size);
    uint8_t* v_buffer = (uint8_t*)malloc(sizeof(uint8_t) * v_size);

    // 將padded data轉回uint8_t buffer
    for (int row = 0; row < frame->y.height; row++) {
        for (int col = 0; col < frame->y.width; col++) {
            y_buffer[row * frame->y.width + col] = (uint8_t)(frame->y.padded_data[row * frame->y.padded_width + col]);
        }
    }
    for (int row = 0; row < frame->u.height; row++) {
        for (int col = 0; col < frame->u.width; col++) {
            u_buffer[row * frame->u.width + col] = (uint8_t)(frame->u.padded_data[row * frame->u.padded_width + col]);
        }
    }
    for (int row = 0; row < frame->v.height; row++) {
        for (int col = 0; col < frame->v.width; col++) {
            v_buffer[row * frame->v.width + col] = (uint8_t)(frame->v.padded_data[row * frame->v.padded_width + col]);
        }
    }

    if (fwrite(y_buffer, 1, y_size, fp) != y_size ||
        fwrite(u_buffer, 1, u_size, fp) != u_size ||
        fwrite(v_buffer, 1, v_size, fp) != v_size) {
        perror("Write YUV padded data failed");
    }

    free(y_buffer);
    free(u_buffer);
    free(v_buffer);

    fclose(fp);
}

void free_yuv_frame(YUVFrame* frame)
{
    free(frame->y.raw_data);
    free(frame->u.raw_data);
    free(frame->v.raw_data);
    free(frame->y.padded_data);
    free(frame->u.padded_data);
    free(frame->v.padded_data);
    free(frame);
}

