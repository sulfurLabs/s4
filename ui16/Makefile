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
LIBPSF := ../libpsf

CFLAGS := -ffreestanding -nostdlib -fno-builtin -fno-stack-protector  \
          -fno-PIE -fno-pic -m64 -march=x86-64 -mno-red-zone          \
          -Wall -Wextra -std=gnu11 -D__sulfur__                       \
          -I$(LIBC)/include                                           \
          -Iinclude                                                   \
          -Isrc                                                       \
          -I$(LIBBMP)                                                 \
          -I$(LIBPSF)

SRCS := src/tree.c src/style.c src/layout.c src/render.c src/frame.c  \
        src/widgets.c src/widgets_button.c src/font.c                 \
        src/softrender.c src/generic.c src/input.c src/keymap.c       \
        src/bmp.c src/widgets_image.c src/cache.c
OBJS := $(SRCS:src/%.c=build/%.o)

all: build/ui16.a

$(LIBBMP)/build/libbmp.a:
	$(MAKE) -C $(LIBBMP)

$(LIBPSF)/build/psf.a:
	$(MAKE) -C $(LIBPSF)

build/%.o: src/%.c include/ui16.h include/ui16buttons.h src/ui16_priv.h | build
	$(CC) $(CFLAGS) -c $< -o $@

build/ui16.a: $(OBJS) $(LIBBMP)/build/libbmp.a $(LIBPSF)/build/psf.a | build
	rm -rf build/libobjs
	mkdir -p build/libobjs
	cd build/libobjs && $(AR) x ../../$(LIBBMP)/build/libbmp.a
	cd build/libobjs && $(AR) x ../../$(LIBPSF)/build/psf.a
	$(AR) rcs $@ $(OBJS) build/libobjs/*.o
	rm -rf build/libobjs

build:
	mkdir -p build

clean:
	rm -rf build

.PHONY: all clean