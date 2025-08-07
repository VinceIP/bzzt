# ---------------- project layout ----------------------------
SRC_ROOT  := src
BUILD_DIR := build

# ------------------------------------------------------------
# Host detection
# ------------------------------------------------------------
ifeq ($(OS),Windows_NT)          # MSYS/MinGW, Git-Bash, etc.
    HOST := WINDOWS
else                              # Everything else we treat as Linux
    HOST := LINUX
endif

# ---------------- toolchain & target ------------------------
CC   := gcc
EXE  := $(if $(filter $(HOST),WINDOWS),.exe,)   # bzzt.exe on Win, bzzt on Linux
TARGET := $(BUILD_DIR)/bzzt$(EXE)

# ---------------- optional warnings -------------------------
WARN ?= 1
ifeq ($(WARN),1)
  WARN_FLAGS := -Wall -Wextra
else
  WARN_FLAGS := -w
endif



# ------------------------------------------------------------
# Library configuration per host
# ------------------------------------------------------------
ifeq ($(HOST),WINDOWS)
    # ---- Windows / MinGW -----------------------------------
    RAYLIB_INC  := /ucrt64/include
    RAYLIB_LIB  := /ucrt64/lib
    RAYLIB_LIBS := -lraylib -lwinmm -lgdi32

    CYAML_INC   := /ucrt64/include
    CYAML_LIB   := /ucrt64/lib
    CYAML_LIBS  := -lcyaml -lyaml
else
    # ---- Linux ---------------------------------------------
    # Rely on system-installed packages (apt install libraylib-dev libcyaml-dev)
    RAYLIB_INC  := /usr/include
    RAYLIB_LIB  := /usr/lib
    RAYLIB_LIBS := -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

    CYAML_INC   := /usr/include
    CYAML_LIB   := /usr/lib
    CYAML_LIBS  := -lcyaml -lyaml
endif

# ------------------------------------------------------------
# Generic compiler & linker flags
# ------------------------------------------------------------
INC_DIRS := $(shell find $(SRC_ROOT) -type d)
CFLAGS   := -std=c99 -D_POSIX_C_SOURCE=200809L $(WARN_FLAGS)            \
            -I$(RAYLIB_INC) -I$(CYAML_INC)    \
            $(addprefix -I,$(INC_DIRS))

LDFLAGS  := -L$(RAYLIB_LIB) $(RAYLIB_LIBS)    \
            -L$(CYAML_LIB)  $(CYAML_LIBS)

# -- Address sanitizer
SAN ?= 0            # make SAN=1 to enable AddressSanitizer
ifeq ($(SAN),1)
  SAN_FLAGS := -fsanitize=address -fno-omit-frame-pointer
  CFLAGS   += $(SAN_FLAGS) -g        # -g for usable stack traces
  LDFLAGS  += $(SAN_FLAGS)
endif

# ---------------- source & objects --------------------------
SRC := $(shell find $(SRC_ROOT) -name '*.c')
OBJ := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRC))

# ---------------- targets -----------------------------------
.PHONY: all clean
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) -r $(BUILD_DIR)
