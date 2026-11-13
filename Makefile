CC := gcc
SRC_DIR = src
OBJ_DIR = obj
DOT_DIR = dot_of_dumps
PNG_DIR = png_dumps
BINPRO_DIR = bin_pograms

CFLAGS := -Wall -Wextra -Wshadow -g -O2 -MMD -MP -I$(SRC_DIR)
LDFLAGS = -O2

SRCS = $(wildcard src/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

ifeq ($(OS), Windows_NT)
	TARGET := $(addprefix $(OBJ_DIR)/, akinator.exe)
	RUN    := ./$(TARGET)
	RM     := del /Q /F
else
	TARGET := $(addprefix $(OBJ_DIR)/, akinator)
	RUN    := ./$(TARGET)
	RM     := rm -f
endif

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ -mconsole

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/tree.h
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	-rd /S /Q bin_programs obj dot_of_dumps png_dumps
	-$(RM) .png .dot errors.log
	mkdir bin_programs obj dot_of_dumps png_dumps

.PHONY: all run clean