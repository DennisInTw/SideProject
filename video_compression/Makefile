CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lm
TARGET = main
SRCS = $(shell find . -name "*.c" -type f)
#SRCS = main.c src/yuv.c src/transform.c src/quantization/quantization.c src/quantization/jpeg/quant_jpeg.c src/entropy/entropy.c src/entropy/jpeg/entropy_jpeg.c

# 將所有的object檔放在output/obj下
OBJ_DIR = output/obj
OBJS = $(addprefix $(OBJ_DIR)/,$(SRCS:.c=.o))

CFLAGS += -Iinc

all: $(OBJ_DIR) $(TARGET)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all clean
