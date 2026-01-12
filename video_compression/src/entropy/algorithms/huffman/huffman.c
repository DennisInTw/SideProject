#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include"entropy/algorithms/huffman.h"
#include"file_io.h"

/* 建立Huffman tree上面每個symbol對應的code word */
void huffman_create_lookup_table(const uint8_t* bits_table, const uint8_t* hufval_table, Huffman_Table* huffman_table)
{
    uint16_t codeword = 0;
    int hufval_index = 0;

    for (int bit_len = 1; bit_len <= 16; bit_len++) {
        uint8_t symbol_nums = bits_table[bit_len-1];

        for (int i = 0; i < symbol_nums; i++) {
            uint8_t symbol = hufval_table[hufval_index];
            huffman_table->codeword[symbol] = codeword;
            huffman_table->code_length[symbol] = bit_len;
            codeword++;
            hufval_index++;
        }
        // 新增一個bit長度
        codeword <<= 1;
    }
}

int huffman_decode_dc(BitReader* bit_reader, JpegDcEncoded* dc_encoded, Huffman_Table* huffman_table)
{
    uint16_t codeword = 0, amplitude = 0;
    int bit_len = 0;
    uint8_t bit;
    const int max_symbols_num = 16;  // DC 最多16種symbols
    int found_symbol = 0;

    // Decode DC codeword
    while (!found_symbol) {
        // 讀取bit by bit
        if (bit_reader->bit_left == 0) {
            int data = fgetc(bit_reader->fp);

            /* 不應該出現底下情況 */
            if (data == EOF) {
                perror("[Error] Unexpected End of File!");
                return -1;
            }

            bit_reader->buffer = (uint8_t)data;
            bit_reader->bit_left = 8;

            /* byte stuffing情況: 0xff 0x00 */
            if (bit_reader->buffer == 0xff) {
                // 丟掉0x00
                fgetc(bit_reader->fp);
            }
        }

        bit = (bit_reader->buffer >> (bit_reader->bit_left-1)) & 0x01;
        codeword = (codeword << 1) | bit;
        bit_len++;
        bit_reader->bit_left--;

        // 匹配目前的codeword有沒有在huffman table裡
        for (int i = 0; i < max_symbols_num; i++) {
            if (huffman_table->codeword[i] == codeword && huffman_table->code_length[i] == bit_len) {
                found_symbol = 1; // 找到DC係數的codeword長度
                dc_encoded->size = i;  // amplitude的bit長度 (不是codeword本身的長度)
                break;
            }
        }
    }

    // Decode DC amplitude
    for (int i = 0; i < dc_encoded->size; i++) {
        if (bit_reader->bit_left == 0) {
            bit_reader->buffer = (uint8_t)fgetc(bit_reader->fp);
            bit_reader->bit_left = 8;
            
            /* byte stuffing情況: 0xff 0x00 */
            if (bit_reader->buffer == 0xff) {
                // 丟掉0x00
                fgetc(bit_reader->fp);
            }
        }

        bit = (bit_reader->buffer >> (bit_reader->bit_left-1)) & 0x01;
        amplitude = (amplitude << 1) | bit;
        bit_reader->bit_left--;
    }
    dc_encoded->amplitude = amplitude;

    return 0;
}


/*  function: huffman_encode_dc()
    Params:
        BitWriter* bit_writer        : 紀錄寫檔的情況
        JpegDcEncoded* dc_encoded    : 儲存每個block將DC係數encode後的結果
        Huffman_Table* huffman_table : 根據symbolt產生對應的codeword

    Return:
        將DC編碼後的係數(size, ampltitude)利用Huffman encode後的bitstream

    Result:
        編碼size後，再編碼ampltitude
 */
int huffman_encode_dc(BitWriter* bit_writer, JpegDcEncoded* dc_encoded, Huffman_Table* huffman_table)
{
    /* Huffman coding將DPCM編碼後的DC係數得到的size (也就是category)當作symbol，寫入它的codeword */
    uint8_t symbol = dc_encoded->size;
    uint16_t codeword = huffman_table->codeword[symbol];
    uint8_t code_len = huffman_table->code_length[symbol]; // codeword編碼需要的bits個數，由Huffman table定義好，只需要做mapping
    int16_t amplitude = dc_encoded->amplitude;
    uint8_t amp_len = dc_encoded->size;

    /* 將codeword轉成bitstream，寫入檔案
       使用MSB->LSB順序讀取codeword
     */
    for (int offset = code_len-1; offset >= 0; offset--) {
        // 讀取codeword的bit
        uint8_t bit = (codeword >> offset) & 0x01;

        /* 寫入bit到buffer */
        bit_writer->buffer <<= 1;
        bit_writer->buffer |= bit;
        bit_writer->bit_count++;

        /* 讀到8個bits時，將一個byte的資料寫入檔案*/
        if (bit_writer->bit_count == 8) {
            fputc(bit_writer->buffer, bit_writer->fp);

            /* byte stuffing: 避開JPEG檔案裡的marker */
            if (bit_writer->buffer == 0xff) {
                fputc(0x00, bit_writer->fp);
            }
            
            /* 清空buffer */
            bit_writer->buffer = 0;
            bit_writer->bit_count = 0;
        }
    }

    /* 將ampltitude轉成bitstream，寫入檔案 
       使用MSB->LSB順序讀取ampltitude
     */
    for (int offset = amp_len-1; offset >= 0; offset--) {
        // 讀取ampltitude的bit
        uint8_t bit = (amplitude >> offset) & 0x01;

        /* 寫入bit到buffer */
        bit_writer->buffer <<= 1;
        bit_writer->buffer |= bit;
        bit_writer->bit_count++;

        /* 讀到8個bits時，將一個byte的資料寫入檔案*/
        if (bit_writer->bit_count == 8) {
            fputc(bit_writer->buffer, bit_writer->fp);

            /* byte stuffing: 避開JPEG檔案裡的marker */
            if (bit_writer->buffer == 0xff) {
                fputc(0x00, bit_writer->fp);
            }

            bit_writer->buffer = 0;
            bit_writer->bit_count = 0;
        }
    }


    return 0;
}

int huffman_decode_ac(BitReader* bit_reader, JpegAcEncoded* ac_encoded, Huffman_Table* huffman_table)
{
    int ac_index = 0;
    int found_eob = 0;

    while (!found_eob) {
        uint16_t codeword = 0, amplitude = 0;
        int bit_len = 0;
        uint8_t bit;
        const int max_symbols_num = 256;  // AC 最多256種symbols
        int found_symbol = 0;

        // Decode AC codeword
        while (!found_symbol) {
            if (bit_reader->bit_left == 0) {
                int data = fgetc(bit_reader->fp);

                /* 不應該出現底下情況 */
                if (data == EOF) {
                    perror("[Error] Unexpected End of File!");
                    return -1;
                
                }

                bit_reader->buffer = (uint8_t)data;
                bit_reader->bit_left = 8;

                /* byte stuffing情況: 0xff 0x00 */
                if (bit_reader->buffer == 0xff) {
                    // 丟掉0x00
                    fgetc(bit_reader->fp);
                }
            }

            bit = (bit_reader->buffer >> (bit_reader->bit_left-1)) & 0x01;
            codeword = (codeword << 1) | bit;
            bit_len++;
            bit_reader->bit_left--;

            // 匹配目前的codeword有沒有在huffman table裡
            for (int i = 0; i < max_symbols_num; i++) {
                if (huffman_table->codeword[i] == codeword && huffman_table->code_length[i] == bit_len) {
                    ac_encoded->symbols[ac_index].run_length = (i >> 4) & 0x0f;
                    ac_encoded->symbols[ac_index].size = i & 0x0f;
                    found_symbol = 1; // 找到AC係數的codeword長度
                    break;
                }
            }
        }


        if (ac_encoded->symbols[ac_index].run_length == 0 && ac_encoded->symbols[ac_index].size == 0) {
            /* 檢查是不是EOB */
            found_eob = 1; // 找到EOB符號
            ac_encoded->symbols[ac_index].amplitude = 0;
            ac_index++;
        } else if (ac_encoded->symbols[ac_index].run_length == 15 && ac_encoded->symbols[ac_index].size == 0) {
            /* 檢查是不是ZRL*/
            ac_encoded->symbols[ac_index].amplitude = 0;
            ac_index++;
        } else {
            // Decode AC amplitude
            for (int i = 0; i < ac_encoded->symbols[ac_index].size; i++) {
                if (bit_reader->bit_left == 0) {
                    bit_reader->buffer = (uint8_t)fgetc(bit_reader->fp);
                    bit_reader->bit_left = 8;

                    /* byte stuffing情況: 0xff 0x00 */
                    if (bit_reader->buffer == 0xff) {
                        // 丟掉0x00
                        fgetc(bit_reader->fp);
                    }
                }

                bit = (bit_reader->buffer >> (bit_reader->bit_left-1)) & 0x01;
                amplitude = (amplitude << 1) | bit;
                bit_reader->bit_left--;
            }
            ac_encoded->symbols[ac_index].amplitude = amplitude;
            ac_index++;
        }
    }

    ac_encoded->num_symbols = ac_index;
    return 0;
}


/*  function: huffman_encode_ac()
    Params:
        BitWriter* bit_writer        : 紀錄寫檔的情況
        JpegAcEncoded* ac_encoded    : 儲存每個block將AC係數encode後的結果
        Huffman_Table* huffman_table : 根據symbolt產生對應的codeword

    Return:
        將AC編碼後的係數((run_length, size), ampltitude)利用Huffman encode後的bitstream

    Result:
        編碼(run_length, size)後，再編碼ampltitude
 */
int huffman_encode_ac(BitWriter* bit_writer, JpegAcEncoded* ac_encoded, Huffman_Table* huffman_table)
{
    /* 編碼Block裡的所有RLE後的AC係數 (每個AC係數編碼流程和DC係數類似) */
    for (int i = 0; i < ac_encoded->num_symbols; i++) {
        /* Huffman coding將RLE encode後的(run length, size)組成一個byte當作symbol */
        uint8_t symbol = (ac_encoded->symbols[i].run_length << 4 | ac_encoded->symbols[i].size);
        uint16_t codeword = huffman_table->codeword[symbol];
        uint8_t code_len = huffman_table->code_length[symbol];
        uint16_t amplitude = ac_encoded->symbols[i].amplitude;
        uint8_t amp_len = ac_encoded->symbols[i].size;

        
        /* 將codeword轉成bitstream，寫入檔案
           使用MSB->LSB順序讀取codeword
         */
        for (int offset = code_len-1; offset >= 0; offset--) {
            // 讀取codeword的bit
            uint8_t bit = (codeword >> offset) & 0x01;

            /* 寫入bit到buffer */
            bit_writer->buffer <<= 1;
            bit_writer->buffer |= bit;
            bit_writer->bit_count++;

            /* 讀到8個bits時，將一個byte的資料寫入檔案*/
            if (bit_writer->bit_count == 8) {
                fputc(bit_writer->buffer, bit_writer->fp);

                /* byte stuffing: 避開JPEG檔案裡的marker */
                if (bit_writer->buffer == 0xff) {
                    fputc(0x00, bit_writer->fp);
                }

                bit_writer->buffer = 0;
                bit_writer->bit_count = 0;
            }
        }


        /* 將ampltitude轉成bitstream，寫入檔案 
           使用MSB->LSB順序讀取ampltitude
         */
        for (int offset = amp_len-1; offset >= 0; offset--) {
            // 讀取ampltitude的bit
            uint8_t bit = (amplitude >> offset) & 0x01;

            /* 寫入bit到buffer */
            bit_writer->buffer <<= 1;
            bit_writer->buffer |= bit;
            bit_writer->bit_count++;

            /* 讀到8個bits時，將一個byte的資料寫入檔案*/
            if (bit_writer->bit_count == 8) {
                fputc(bit_writer->buffer, bit_writer->fp);

                /* byte stuffing: 避開JPEG檔案裡的marker */
                if (bit_writer->buffer == 0xff) {
                    fputc(0x00, bit_writer->fp);
                }

                bit_writer->buffer = 0;
                bit_writer->bit_count = 0;
            }
        }
    }

    return 0;

}