#ifndef HUFFMAN_H
#define HUFFMAN_H

#include<stdint.h>
#include"entropy/jpeg/entropy_jpeg.h"
#include"file_io.h"


extern const uint8_t jpeg_dc_luminance_huffman_bits_table[];
extern const uint8_t jpeg_dc_luminance_huffman_hufval_table[];
extern const uint8_t jpeg_dc_chrominance_huffman_bits_table[];
extern const uint8_t jpeg_dc_chrominance_huffman_hufval_table[];
extern const uint8_t jpeg_ac_luminance_huffman_bits_table[];
extern const uint8_t jpeg_ac_luminance_huffman_hufval_table[];
extern const uint8_t jpeg_ac_chrominance_huffman_bits_table[];
extern const uint8_t jpeg_ac_chrominance_huffman_bits_table[];
extern const uint8_t jpeg_ac_chrominance_huffman_hufval_table[];


typedef struct {
  // Huffman coding的symbol最多就是256種
  uint16_t codeword[256];
  uint8_t code_length[256];
}Huffman_Table;

void huffman_create_lookup_table(const uint8_t* bits_table, const uint8_t* hufval_table, Huffman_Table* huffman_table);
int huffman_encode_dc(BitWriter* bit_writer, JpegDcEncoded* dc_encoded, Huffman_Table* huffman_table);
int huffman_decode_dc(BitReader* bit_reader, JpegDcEncoded* dc_encoded, Huffman_Table* huffman_table);
int huffman_encode_ac(BitWriter* bit_writer, JpegAcEncoded* ac_encoded, Huffman_Table* huffman_table);
int huffman_decode_ac(BitReader* bit_reader, JpegAcEncoded* ac_encoded, Huffman_Table* huffman_table);

#endif // HUFFMAN_H
