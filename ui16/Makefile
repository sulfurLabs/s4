#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Copyright (c) 2026 sulfurLabs
#
# PROJECT: s4
# FILE: Makefile
#

CC   := x86_64-elf-gcc
AR   := x86_64-elf-ar
LIBC := ../include/libc
LIBBMP := ../libbmp

CFLAGS := -ffreestanding -nostdlib -fno-builtin -fno-stack-protector  \
          -fno-PIE -fno-pic -m64 -march=x86-64 -mno-red-zone          \
          -Wall -Wextra -std=gnu11 -D__sulfur__                       \
          -I$(LIBC)/include                                           \
          -Iinclude                                                   \
          -Isrc                                                       \
          -I$(LIBBMP)

SRCS := src/tree.c src/style.c src/layout.c src/render.c src/frame.c src/widgets.c src/widgets_button.c src/font.c src/softrender.c src/generic.c src/input.c src/keymap.c src/bmp.c
OBJS := $(SRCS:src/%.c=build/%.o)

all: build/ui16.a

build/%.o: src/%.c include/ui16.h include/ui16buttons.h src/ui16_priv.h | build
	$(CC) $(CFLAGS) -c $< -o $@

build/ui16.a: $(OBJS) | build
	$(AR) rcs $@ $(OBJS)

build:
	mkdir -p build

clean:
	rm -rf build

.PHONY: all clean