# Cross-Platform System Programming Guide: Makefile for building sample files

C := gcc
DOT_EXE :=
SO := so
SRC_DIR := .

ifeq "$(OS)" "windows"
	C := x86_64-w64-mingw32-gcc
	DOT_EXE := .exe
	SO := dll
else ifeq "$(OS)" "macos"
	SO := dylib
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
	ps-exec-out$(DOT_EXE) \
	\
	ps-exec-wait$(DOT_EXE) \
	ps-info$(DOT_EXE) \
	signal-interrupt$(DOT_EXE) \
	signal-cpu-exception$(DOT_EXE) \
	semaphore$(DOT_EXE) \
	pipe-named$(DOT_EXE) \
	file-mapping$(DOT_EXE) \
	dylib.$(SO) dylib-load$(DOT_EXE)

all: $(BINS)

clean:
	rm -f $(BINS)

%.$(SO): $(SRC_DIR)/%.c
	$(C) -shared $(CFLAGS) $< -o $@

%$(DOT_EXE): $(SRC_DIR)/%.c
	$(C) $(CFLAGS) $< -o $@
