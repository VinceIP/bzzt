# ---------- project layout ----------------------------------
SRC_ROOT      := src
BUILD_DIR     := build
TARGET        := $(BUILD_DIR)/bzzt.exe

# ---------- toolchain & flags -------------------------------
CC            := gcc
RAYLIB_INC    := /ucrt64/include
RAYLIB_LIB    := /ucrt64/lib
RAYLIB_LIBS   := -lraylib -lwinmm -lgdi32

# Recurse through src/ to build the include-path list
INC_DIRS      := $(shell find $(SRC_ROOT) -type d)
CFLAGS        := -std=c17 -Wall -Wextra \
                 -I$(RAYLIB_INC) \
                 $(addprefix -I,$(INC_DIRS))

LDFLAGS       := -L$(RAYLIB_LIB) $(RAYLIB_LIBS)

# ---------- source & object lists ---------------------------
SRC           := $(shell find $(SRC_ROOT) -name '*.c')
OBJ           := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRC))

# ---------- phony targets -----------------------------------
.PHONY: all clean
all: $(TARGET)

# ---------- link step ---------------------------------------
$(TARGET): $(OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

# ---------- compile step ------------------------------------
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# ---------- clean -------------------------------------------
clean:
	$(RM) -r $(BUILD_DIR)
