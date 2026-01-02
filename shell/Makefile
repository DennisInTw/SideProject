# 變數定義
CC = gcc                # 定義編譯器為 gcc
CFLAGS = -Wall -g       # 編譯選項：顯示所有警告、加入除錯資訊
TARGET = main           # 執行檔名稱
SRCS = src/main.c src/parser.c src/builtins.c src/executor.c    # 原始碼文件路徑
OBJS = $(SRCS:.c=.o)    # 使用字串替換語法, 自動將 .c 替換為 .o

CFLAGS += -Iinc         # 加入標頭檔的路徑

# 目標定義: all, clean
all: $(TARGET)


# 對所有object檔做link
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# $<：第一個依賴項 (這裡是.c)
# $@：目標文件名稱 (這裡是.o)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJS) $(TARGET)

# 告訴 make：all 和 clean 不是真正的「檔案」
.PHONY: all clean