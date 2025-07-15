# ---------- project layout ----------------------------------
SRC_ROOT := src                                 # recurse only here

# ---------- libraries & toolchain ---------------------------
RAYLIB_INC  := /ucrt64/include
RAYLIB_LIB  := /ucrt64/lib
RAYLIB_DLLS := -lraylib -lwinmm -lgdi32

CC      := gcc
CFLAGS  := -std=c17 -Wall -Wextra -I$(RAYLIB_INC) \
           $(addprefix -I,$(shell find $(SRC_ROOT) -type d))
LDFLAGS := -L$(RAYLIB_LIB) $(RAYLIB_DLLS) -lcyaml

# ---------- sources / objects -------------------------------
SRC := $(shell find $(SRC_ROOT) -type f -name '*.c')
OBJ := $(patsubst %.c,build/%.o,$(SRC))

TARGET := build/bzzt.exe

# ---------- rules -------------------------------------------
.PHONY: all clean
all: $(TARGET)

# Make sure the base build dir exists
$(TARGET) $(OBJ): | build
build:
	mkdir -p $@

# Link
$(TARGET): $(OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

# Compile; mirrors source tree under build/
build/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build
