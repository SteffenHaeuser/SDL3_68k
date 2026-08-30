# Classical AmigaOS 68k Makefile for SDL3_gfx
#
# Builds separate clib2 and libnix variants, including the shipped tests.
# Assumes SDL3 headers and the corresponding SDL3 libraries are installed
# in the compiler's normal include/library paths.

CC      := m68k-amigaos-gcc
AR      := m68k-amigaos-ar
RANLIB  := m68k-amigaos-ranlib
RM      := rm -f
MKDIR   := mkdir -p
CP      := cp

CPUFLAGS  := -mcpu=68040 -mhard-float
OPTFLAGS  := -O3
WARNFLAGS := -Wall -Wextra

MATH_BUILTIN_FLAGS := \
	-fno-builtin-floor \
	-fno-builtin-ceil \
	-fno-builtin-sin \
	-fno-builtin-cos \
	-fno-builtin-log10

CRTFLAGS :=

CFLAGS   := $(CPUFLAGS) $(OPTFLAGS) $(WARNFLAGS) $(MATH_BUILTIN_FLAGS) $(CRTFLAGS)
CPPFLAGS := -I.
LDFLAGS  := $(CPUFLAGS) $(CRTFLAGS)

SDL3_CFLAGS :=
SDL3_LIBS := -lSDL3
SDL3_TEST_LIBS := -lSDL3_test
MATH_LIBS := -lm -lminigl -latomic

BUILD_DIR := build/default
OBJ_DIR   := $(BUILD_DIR)/obj
TEST_OBJ_DIR := $(BUILD_DIR)/testobj
BIN_DIR   := $(BUILD_DIR)/bin
LIB_DIR   := $(BUILD_DIR)/lib

LIB := $(LIB_DIR)/libSDL3_gfx.a

LIB_SOURCES := \
	SDL3_framerate.c \
	SDL3_gfxPrimitives.c \
	SDL3_imageFilter.c \
	SDL3_rotozoom.c

LIB_OBJECTS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(LIB_SOURCES))

TEST_PROGRAMS := \
	$(BIN_DIR)/TestGfx \
	$(BIN_DIR)/TestImageFilter \
	$(BIN_DIR)/TestRotozoom \
	$(BIN_DIR)/TestFramerate

TEST_ASSETS := \
	test/sample2x2.bmp \
	test/sample3x3.bmp \
	test/sample8.bmp \
	test/sample8-box.bmp \
	test/sample16x16.bmp \
	test/sample24.bmp \
	test/sample24-box.bmp

.PHONY: all clib2 libnix build library tests assets clean install

all: clib2 libnix

clib2:
	$(MAKE) build \
		BUILD_DIR=build/clib2 \
		CRTFLAGS=-mcrt=clib2 \
		SDL3_LIBS=-lSDL3 \
		SDL3_TEST_LIBS=-lSDL3_test

libnix:
	$(MAKE) build \
		BUILD_DIR=build/libnix \
		CRTFLAGS=-noixemul \
		SDL3_LIBS=-lSDL3_libnix \
		SDL3_TEST_LIBS=-lSDL3_test_libnix

build: library tests

library: $(LIB)

tests: $(TEST_PROGRAMS) assets

$(LIB): $(LIB_OBJECTS) | $(LIB_DIR)
	$(AR) rcs $@ $(LIB_OBJECTS)
	$(RANLIB) $@

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(SDL3_CFLAGS) $(CFLAGS) -c $< -o $@

$(TEST_OBJ_DIR)/TestGfx.o: test/TestGfx.c | $(TEST_OBJ_DIR)
	$(CC) $(CPPFLAGS) $(SDL3_CFLAGS) $(CFLAGS) -c $< -o $@

$(TEST_OBJ_DIR)/testimagefilter.o: test/testimagefilter.c | $(TEST_OBJ_DIR)
	$(CC) $(CPPFLAGS) $(SDL3_CFLAGS) $(CFLAGS) -c $< -o $@

$(TEST_OBJ_DIR)/testrotozoom.o: test/testrotozoom.c | $(TEST_OBJ_DIR)
	$(CC) $(CPPFLAGS) $(SDL3_CFLAGS) $(CFLAGS) -c $< -o $@

$(TEST_OBJ_DIR)/testframerate.o: test/testframerate.c | $(TEST_OBJ_DIR)
	$(CC) $(CPPFLAGS) $(SDL3_CFLAGS) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/TestGfx: $(TEST_OBJ_DIR)/TestGfx.o $(LIB) | $(BIN_DIR)
	$(CC) $(LDFLAGS) -o $@ $< $(LIB) $(SDL3_TEST_LIBS) $(SDL3_LIBS) $(MATH_LIBS)

$(BIN_DIR)/TestImageFilter: $(TEST_OBJ_DIR)/testimagefilter.o $(LIB) | $(BIN_DIR)
	$(CC) $(LDFLAGS) -o $@ $< $(LIB) $(SDL3_LIBS) $(MATH_LIBS)

$(BIN_DIR)/TestRotozoom: $(TEST_OBJ_DIR)/testrotozoom.o $(LIB) | $(BIN_DIR)
	$(CC) $(LDFLAGS) -o $@ $< $(LIB) $(SDL3_TEST_LIBS) $(SDL3_LIBS) $(MATH_LIBS)

$(BIN_DIR)/TestFramerate: $(TEST_OBJ_DIR)/testframerate.o $(LIB) | $(BIN_DIR)
	$(CC) $(LDFLAGS) -o $@ $< $(LIB) $(SDL3_TEST_LIBS) $(SDL3_LIBS) $(MATH_LIBS)

assets: | $(BIN_DIR)
	$(CP) $(TEST_ASSETS) $(BIN_DIR)/

$(OBJ_DIR) $(TEST_OBJ_DIR) $(BIN_DIR) $(LIB_DIR):
	$(MKDIR) $@

PREFIX := /usr/local
install: $(LIB)
	$(MKDIR) $(DESTDIR)$(PREFIX)/lib
	$(MKDIR) $(DESTDIR)$(PREFIX)/include/SDL3_gfx
	$(CP) $(LIB) $(DESTDIR)$(PREFIX)/lib/
	$(CP) SDL3_framerate.h SDL3_gfxPrimitives.h SDL3_gfxPrimitives_font.h \
	      SDL3_imageFilter.h SDL3_rotozoom.h $(DESTDIR)$(PREFIX)/include/SDL3_gfx/

clean:
	rm -rf build
