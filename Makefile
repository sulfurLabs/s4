#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Copyright (c) 2026 sulfurLabs
#
# PROJECT: s4
# FILE: Makefile
#

CC     := x86_64-elf-gcc
LD     := x86_64-elf-ld
LIBC   ?= include/libc

OS_PATH     ?= ~/sulfurLabs/sulfurOS/
OS_DSK_PATH ?= $(OS_PATH)dsk/
ROOTFS_PATH ?= $(OS_DSK_PATH)rdh/user_id/applications/desktop/
OS_LIBS     ?= $(OS_PATH)user/libs/
LIBDESKTOP  := libdesktop/
UI16 := ui16/
LIBPSF := libpsf/
LIBBMP := libbmp/

CFLAGS := -ffreestanding -nostdlib -fno-builtin -fno-stack-protector       \
          -fno-PIE -fno-pic -m64 -march=x86-64                             \
          -mno-mmx -mno-red-zone -Wall -Wextra -std=gnu11 -D__sulfur__ -O2 \
          -I$(LIBC)/include                                                \
          -I$(LIBDESKTOP)                                                  \
          -I$(LIBPSF)                                                      \
          -I$(LIBBMP)

LDFLAGS := -nostdlib -static -no-pie -T user.ld

LIBC_DIR := include/libc

OBJS := build/desktop.o \
        build/dt_ipc.o \
        build/ipc/ipc.o \
        build/wm/wm.o \
        build/compositor/comp.o \
        build/compositor/fb_backend.o \
        build/compositor/surface.o \
        build/compositor/scale.o \
        build/render/render_target.o \
        build/cmd/cmd.o \
        build/bg/tga/tga.o \
        build/bg/bg.o \
        build/win/win.o \
        build/cursor/cursor.o \
        build/render/render.o \
        build/input/input.o \
        build/fonts/fonts.o \
        build/taskbar/taskbar.o \
        build/taskbar/entries.o \
        build/taskbar/startmenu.o \
        build/shm/shm_host.o

all: clean $(LIBC)/build/crt0.o dirs build/s4.elf run

fetchDeps:
	mkdir -p include
	if [ ! -d "$(LIBC_DIR)" ]; then \
		git clone --depth=1 https://github.com/sulfurLabs/sulfurOS.git include/sulfurOS; \
		mv include/sulfurOS/user/libc $(LIBC_DIR); \
		rm -rf include/sulfurOS; \
	fi

dirs:
	mkdir -p libdesktop/build
	mkdir -p ui16/build
	mkdir -p libpsf/build

	mkdir -p build
	mkdir -p build/ipc
	mkdir -p build/wm
	mkdir -p build/compositor
	mkdir -p build/bg
	mkdir -p build/bg/tga
	mkdir -p build/win
	mkdir -p build/cursor
	mkdir -p build/render
	mkdir -p build/input
	mkdir -p build/cmd
	mkdir -p build/fonts
	mkdir -p build/taskbar
	mkdir -p build/shm
run:
	@echo "running OS..."
	cd $(OS_PATH) && make run

build/s4.elf: dirs $(OBJS) $(LIBC)/build/crt0.o $(LIBC)/build/libc.a $(LIBDESKTOP)/build/libdesktop.a $(LIBPSF)/build/psf.a
	@echo "building libpsf now..."
	@$(MAKE) -C libpsf
	@echo "libpsf was succesfully built!"

	@echo "building libbmp now..."
	@$(MAKE) -C libbmp
	@echo "libbmp was succesfully built!"

	@echo "Building s4 now..."
	$(LD) $(LDFLAGS) $(LIBC)/build/crt0.o $(OBJS)  \
		$(LIBDESKTOP)/build/libdesktop.a           \
		$(LIBPSF)/build/psf.a                      \
		$(LIBBMP)/build/libbmp.a                   \
		$(LIBC)/build/libc.a                       \
		-o $@

	@echo "s4 was succesfully built!"

	@echo "building libdesktop now..."
	@$(MAKE) -C libdesktop
	@echo "libdesktop was succesfully built!"

	@echo "building ui16 now..."
	@$(MAKE) -C ui16
	@echo "ui16 was succesfully built!"

	@echo "building s4 resources now..."
	@chmod +x tools/buildimage.sh
	@./tools/buildimage.sh

	@rm -f $(ROOTFS_PATH)/s4.elf
	@mkdir -p $(OS_LIBS)$(UI16)
	@mkdir -p $(OS_LIBS)$(LIBDESKTOP)
	@mkdir -p $(OS_LIBS)$(LIBPSF)
	@mkdir -p $(OS_LIBS)$(LIBBMP)
	@cp build/s4.elf $(ROOTFS_PATH)desktop.elf
	@cp libdesktop/build/libdesktop.a $(OS_LIBS)$(LIBDESKTOP)libdesktop.a
	@cp libdesktop/libdesktop.h $(OS_LIBS)$(LIBDESKTOP)libdesktop.h
	@cp libdesktop/dt_ipc.h $(OS_LIBS)$(LIBDESKTOP)dt_ipc.h
	@cp ui16/build/ui16.a $(OS_LIBS)$(UI16)ui16.a
	@cp ui16/include/ui16.h $(OS_LIBS)$(UI16)ui16.h
	@cp ui16/include/ui16buttons.h $(OS_LIBS)$(UI16)ui16buttons.h
	@cp libpsf/psf.h $(OS_LIBS)$(LIBPSF)psf.h
	@cp libpsf/build/psf.a $(OS_LIBS)$(LIBPSF)psf.a
	@cp libbmp/bmp.h $(OS_LIBS)$(LIBBMP)bmp.h
	@cp libbmp/build/libbmp.a $(OS_LIBS)$(LIBBMP)libbmp.a
	@cp build/s4.cpio $(OS_DSK_PATH)rdh/user_id/extend/s4.cpio


build/desktop.o:                    s4/desktop.c                   ; $(CC) $(CFLAGS) -c $< -o $@
build/dt_ipc.o:                     libdesktop/dt_ipc.c            ; $(CC) $(CFLAGS) -c $< -o $@
build/ipc/ipc.o:                    s4/ipc/ipc.c                   ; $(CC) $(CFLAGS) -c $< -o $@
build/wm/wm.o:                      s4/wm/wm.c                     ; $(CC) $(CFLAGS) -c $< -o $@
build/compositor/comp.o:            s4/compositor/comp.c           ; $(CC) $(CFLAGS) -c $< -o $@
build/compositor/fb_backend.o:      s4/compositor/fb_backend.c     ; $(CC) $(CFLAGS) -c $< -o $@
build/compositor/surface.o:         s4/compositor/surface.c        ; $(CC) $(CFLAGS) -c $< -o $@
build/compositor/scale.o:           s4/compositor/scale.c          ; $(CC) $(CFLAGS) -c $< -o $@
build/render/render_target.o:       s4/render/render_target.c      ; $(CC) $(CFLAGS) -c $< -o $@
build/bg/tga/tga.o:                 s4/bg/tga/tga.c                ; $(CC) $(CFLAGS) -c $< -o $@
build/bg/bg.o:                      s4/bg/bg.c                     ; $(CC) $(CFLAGS) -c $< -o $@
build/win/win.o:                    s4/win/win.c                   ; $(CC) $(CFLAGS) -c $< -o $@
build/cursor/cursor.o:              s4/cursor/cursor.c             ; $(CC) $(CFLAGS) -c $< -o $@
build/render/render.o:              s4/render/render.c             ; $(CC) $(CFLAGS) -c $< -o $@
build/input/input.o:                s4/input/input.c               ; $(CC) $(CFLAGS) -c $< -o $@
build/fonts/fonts.o:                s4/fonts/fonts.c               ; $(CC) $(CFLAGS) -c $< -o $@
build/taskbar/taskbar.o:            s4/taskbar/taskbar.c           ; $(CC) $(CFLAGS) -c $< -o $@
build/taskbar/entries.o:            s4/taskbar/entries.c           ; $(CC) $(CFLAGS) -c $< -o $@
build/taskbar/startmenu.o:          s4/taskbar/startmenu.c         ; $(CC) $(CFLAGS) -c $< -o $@
build/shm/shm_host.o:               s4/shm/shm_host.c              ; $(CC) $(CFLAGS) -c $< -o $@
build/cmd/cmd.o:                    s4/cmd/cmd.c                   ; $(CC) $(CFLAGS) -c $< -o $@

$(LIBC)/build/crt0.o $(LIBC)/build/libc.a:
	$(MAKE) -C $(LIBC)

$(LIBDESKTOP)/build/libdesktop.a:
	$(MAKE) -C $(LIBDESKTOP)

$(LIBPSF)/build/psf.a:
	$(MAKE) -C $(LIBPSF)

$(UI16)/build/ui16.a:
	$(MAKE) -C $(UI16)

clean:
	rm -f build/*.o build/ipc/*.o build/wm/*.o build/compositor/*.o build/bg/*.o build/bg/bmp/*.o build/win/*.o build/cursor/*.o build/render/*.o build/input/*.o build/cmd/*.o build/fonts/* build/taskbar/*.o build/desktop.elf
	@$(MAKE) -C ui16 clean
	@$(MAKE) -C libdesktop clean
	@$(MAKE) -C libpsf clean
	@$(MAKE) -C $(LIBC) clean

.PHONY: all clean install run
