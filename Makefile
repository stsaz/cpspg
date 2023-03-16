# Cross-Platform System Programming Guide: Makefile for building sample files

C := clang
DOT_EXE :=

ifeq "$(OS)" "windows"
	C := x86_64-w64-mingw32-gcc
	DOT_EXE := .exe
endif

CFLAGS := -O0 -g -Wno-main-return-type

BINS := \
	heap-mem$(DOT_EXE) \
	std-echo$(DOT_EXE) \
	err$(DOT_EXE) \
	file-echo$(DOT_EXE) \
	file-echo-trunc$(DOT_EXE) \
	file-man$(DOT_EXE) \
	file-props$(DOT_EXE) \
	dir-list$(DOT_EXE) \
	pipe$(DOT_EXE) \
	ps-exec$(DOT_EXE) \
	ps-exec-out$(DOT_EXE)

all: $(BINS)

clean:
	rm -f $(BINS)

%$(DOT_EXE): $(DIR)/%.c
	$(C) $(CFLAGS) $< -o $@
