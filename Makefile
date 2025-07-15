RAYLIB_INC  := /ucrt64/include
RAYLIB_LIB  := /ucrt64/lib
RAYLIB_DLLS := -lraylib -lwinmm -lgdi32

CC      := gcc
CFLAGS  := -std=c17 -Wall -Wextra \
           -I$(RAYLIB_INC) \
           -Isrc -Isrc/core -Isrc/editor -Isrc/ui -Isrc/utils
LDFLAGS := -L$(RAYLIB_LIB) $(RAYLIB_DLLS) -lcyaml

SRC := $(shell find . -name '*.c')
OBJ := $(SRC:%.c=build/%.o)

TARGET := build/bzzt.exe

# Create the build directory if it doesn't exist
$(TARGET): | build
$(OBJ): | build

build:
	mkdir -p build
	# also make subfolders as needed for objects (recursive, safe)
	mkdir -p $(sort $(dir $(OBJ)))

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

# Pattern rule: place .o files in build/ mirroring their source paths
build/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build
