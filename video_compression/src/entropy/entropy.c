#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include"yuv.h"
#include"entropy/entropy.h"
#include"entropy/algorithms/huffman.h"

Huffman_Table* jpeg_y_dc_huffman_table, * jpeg_y_ac_huffman_table;
Huffman_Table* jpeg_uv_dc_huffman_table, * jpeg_uv_ac_huffman_table;


/*  function: entropy_initialization()
    Params:
        EntropyType entropy_type         : entropy的方式

    Return:
        得到entropy coding需要的資源. 例如: Huffman coding的tables

    Result:
 */
void entropy_initialization(EntropyType entropy_type)
{
    if (entropy_type == HUFFMAN) {
        printf("Initialize Huffman tables\n");
        jpeg_y_dc_huffman_table = (Huffman_Table*)malloc(sizeof(Huffman_Table));
        jpeg_y_ac_huffman_table = (Huffman_Table*)malloc(sizeof(Huffman_Table));
        jpeg_uv_dc_huffman_table = (Huffman_Table*)malloc(sizeof(Huffman_Table));
        jpeg_uv_ac_huffman_table = (Huffman_Table*)malloc(sizeof(Huffman_Table));
        
        if (jpeg_y_dc_huffman_table == NULL || jpeg_y_ac_huffman_table == NULL || jpeg_uv_dc_huffman_table == NULL || jpeg_uv_ac_huffman_table == NULL) {
            perror("Failed to allocate memory for Huffman table");
            if (jpeg_y_dc_huffman_table != NULL) free(jpeg_y_dc_huffman_table);
            if (jpeg_y_ac_huffman_table != NULL) free(jpeg_y_ac_huffman_table);
            if (jpeg_uv_dc_huffman_table != NULL) free(jpeg_uv_dc_huffman_table);
            if (jpeg_uv_ac_huffman_table != NULL) free(jpeg_uv_ac_huffman_table);
            return;
        }

        // Y component
        huffman_create_lookup_table(jpeg_dc_luminance_huffman_bits_table, jpeg_dc_luminance_huffman_hufval_table, jpeg_y_dc_huffman_table);
        huffman_create_lookup_table(jpeg_ac_luminance_huffman_bits_table, jpeg_ac_luminance_huffman_hufval_table, jpeg_y_ac_huffman_table);
    
        // UV component
        huffman_create_lookup_table(jpeg_dc_chrominance_huffman_bits_table, jpeg_dc_chrominance_huffman_hufval_table, jpeg_uv_dc_huffman_table);
        huffman_create_lookup_table(jpeg_ac_chrominance_huffman_bits_table, jpeg_ac_chrominance_huffman_hufval_table, jpeg_uv_ac_huffman_table);
    }
}


/*  function: entropy_destropy()
    Params:
        EntropyType entropy_type         : entropy的方式

    Return:
        將entropy取得的資源釋放. 例如: 將Huffman coding tables釋放

    Result:
 */
void entropy_destropy(EntropyType entropy_type)
{
    if (entropy_type == HUFFMAN) {
        free(jpeg_y_dc_huffman_table);
        free(jpeg_y_ac_huffman_table);
        free(jpeg_uv_dc_huffman_table);
        free(jpeg_uv_ac_huffman_table);
    }
}

/*  function: entropy_encode()
    Params:
        YUVFrame* frame                  : yuv raw data frame
        CompressionType compression_type : 壓縮的方式
        const char* out_bitstream_path   : 儲存bitstream的path

    Return:
        對frame的padded data做完壓縮，再將bitstream儲存起來

    Result:
        1. 依照壓縮的方式對frame的padded y/u/v data各自做壓縮
        2. 依照壓縮的方式將bitstream儲存
 */
void entropy_encode(YUVFrame* frame, QuantType quant_type, CompressionType compression_type, EntropyType entropy_type, const char* out_bitstream_path)
{
    if (compression_type == JPEG_SEQUENTIAL) {
        entropy_encode_jpeg(frame, quant_type, compression_type, entropy_type, out_bitstream_path);
    }
}


/*  function: entropy_decode()
    Params:
        YUVFrame* frame                  : yuv raw data frame
        CompressionType compression_type : 壓縮的方式
        const char* out_bitstream_path   : 讀取bitstream的path

    Return:
        將bitstream檔案內容讀取出來，再將DC/AC係數擺放到正確的位置

    Result:
        1. 解碼後的DC/AC係數
        2. reverse zigzag scan的結果
 */
void entropy_decode(YUVFrame* frame, QuantType quant_type, CompressionType compression_type, EntropyType entropy_type, const char* out_bitstream_path)
{
    if (compression_type == JPEG_SEQUENTIAL) {
        entropy_decode_jpeg(frame, quant_type, compression_type, entropy_type, out_bitstream_path);
    }
}