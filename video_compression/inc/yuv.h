#ifndef YUV_H
#define YUV_H

#include<stdint.h>
#include"block.h"

typedef enum {
    YUV444,
    YUV422,
    YUV420
}YUVFormat;

typedef struct {
    int width;
    int height;
    int padded_width;
    int padded_height;
    uint8_t* raw_data;
    int16_t* padded_data;
    BlockInfo block_info;
}Component;

typedef struct {
    YUVFormat format;
    Component y;
    Component u;
    Component v;
}YUVFrame;

typedef struct {
    int total_frames;
    YUVFrame** frames;
}YUVVideo;

YUVFrame* init_yuv_frame(YUVFrame** frame, YUVFormat format, int width, int height, int alignment);
void get_yuv_size_info(YUVFormat format,int width, int height, size_t* frame_size, size_t* y_size, size_t* u_size, size_t* v_size);
YUVFrame* read_yuv_frame_data(FILE* fp, YUVFrame* frame, YUVFormat format);
YUVVideo* read_yuv_file(const char* file, int width, int height, YUVFormat format, int truncate_yuv_frame, int truncate_yuv_index);
void save_raw_frame_to_yuv_file(const char* file, YUVFrame* frame);
void save_idct_frame_to_yuv_file(const char* file, YUVFrame* frame);
void free_yuv_frame(YUVFrame* frame);

#endif /* YUV_H */

