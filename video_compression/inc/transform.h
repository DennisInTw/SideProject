#ifndef TRANSFORM_H
#define TRANSFORM_H

#include"yuv.h"

void shift_128(YUVFrame* frame);
void transform_frame(YUVFrame* frame);
void reverse_transform_frame(YUVFrame* frame);

#endif