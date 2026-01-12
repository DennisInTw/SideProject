
# **目標 : 熟悉不同的視訊壓縮機制**

### 輸入.yuv檔案，利用不同的壓縮機制來編碼/解碼

##
# **功能**

* 支援JPEG sequential baselilne編碼/解碼機制
    * 讀取YUV planar格式的檔案
    * Transform : DCT type-III
    * Quantization : JPEG standard quantization
    * Entropy : DPCM (DC係數)、Run-length coding (AC係數)、Huffman coding
* 流程 :
    * 編碼: 讀取.yuv檔 --> DCT --> Quantization --> Zigzag scan --> DPCM、RLE --> Huffman encode --> 將bitstream寫入檔案
    * 解碼: 讀取bitstream檔案 --> Huffman decode --> reverse DPCM、RLE --> reverse Zigzag scan --> reverse Quantization --> reverse DCT --> 儲存解碼後的yuv

##
# **程式架構**
* main.c : 程式入口
    * 讀取encode/decode設定檔
    * 執行encode/decode
* configs
    * 存放encode/decode設定檔
    * 依照yaml格式來填
* videos
    * 存放.yuv的raw檔
* src
    * yuv.c : 關於yuv資料的讀取、存取、記憶體配置的相關操作
    * transform.c : 關於DCT type-III的相關操作
    * quantization
        * quantization.c : quantization的入口，根據設定執行對應的函式
        * jpeg
            * quant_jpeg.c : 使用JPEG機制實作quantizaiton
            * quant_jpeg_table.c : JPEG定義好的量化表
    * file_io.c : 建立bitwriter和bitreader，來寫入/讀取bitstream
    * entropy
        * entropy.c : entropy的入口，根據設定執行對應的函式
        * algorithms
            * 不同entropy方法的實作
        * jpeg
            * entropy_jpeg.c : JPEG的entropy實作
* inc : 資料型態的structure定義和函式宣告


##
# **Feature Work**
* 計算PSNR
* H.264編碼/解碼

##
# **程式執行**

### 下載原始碼
git clone https://github.com/DennisInTw/SideProject.git

### 下載.YUV raw檔
可以下載[UVG dataset](https://ultravideo.fi/dataset.html)的檔案，下載後解壓縮可以得到.yuv
下載後將.yuv放到 **videos/** 底下

### 修改Encode/Decode設定
在 **configs/** 底下有兩個範例檔案，分別給encode和decode使用
依照上面的設定方式改成需要的設定
註解用 '#'

### **程式編譯**
```bash=
cd SideProject/video_compression

# 底下會產生object files，都放在 output/obj/ 底下
make

# or 

# 底下會清空所有object files，以及最後的target file (main)
make clean
```

### **程式執行**
* encode
    * 輸入: ./main enc [encode_config_file_path]
```bash=
#例如:
./main enc ./configs/enc_config.txt
```
* decode
    * 輸入: ./main dec [decode_config_file_path]
```bash=
#例如:
./main dec ./configs/dec_config.txt
```

## 參考資料
* 視訊壓縮上課的內容
* wiki內容
  https://en.wikipedia.org/wiki/JPEG#JPEG_compression
* https://en.wikibooks.org/wiki/JPEG_-_Idea_and_Practice/The_Huffman_coding
* StackOverflow
* ChatGPT
