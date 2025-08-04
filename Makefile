# ---------- project layout ----------------------------------
SRC_ROOT      := src
BUILD_DIR     := build
TARGET        := $(BUILD_DIR)/bzzt.exe

# ---------- toolchain & flags -------------------------------
CC            := gcc

# Raylib
RAYLIB_INC    := /ucrt64/include
RAYLIB_LIB    := /ucrt64/lib
RAYLIB_LIBS   := -lraylib -lwinmm -lgdi32

# CYAML
CYAML_INC     := /ucrt64/include
CYAML_LIB     := /ucrt64/lib
CYAML_LIBS    := -lcyaml -lyaml

# ---------- warning control ---------------------------------
# Pass WARN=0 to silence warnings:   make WARN=0
# Leave blank or set to 1 to keep them: make (or make WARN=1)
WARN        ?= 1
ifeq ($(WARN),1)
  WARN_FLAGS := -Wall -Wextra
else
  WARN_FLAGS := -w          # completely silence GCC
endif

# Recurse through src/ to build the include-path list
INC_DIRS      := $(shell find $(SRC_ROOT) -type d)
CFLAGS        := -std=c17 $(WARN_FLAGS) \
                 -I$(RAYLIB_INC) -I$(CYAML_INC) \
                 $(addprefix -I,$(INC_DIRS))

LDFLAGS       := -L$(RAYLIB_LIB) $(RAYLIB_LIBS) \
                 -L$(CYAML_LIB)  $(CYAML_LIBS)

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
