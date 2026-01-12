#ifndef BLOCK_H
#define BLOCK_H

typedef enum {
    BLOCK_4x4,
    BLOCK_8x8
}BlockSize;

typedef struct {
    BlockSize b_size;
    int width;
    int height;
}BlockInfo;



#endif /* BLOCK_H */