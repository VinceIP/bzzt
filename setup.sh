#!/usr/bin/env bash
set -euo pipefail

# --------- CONFIG -----------------------------------------------------------
PREFIX="$HOME/.local"            # Where we’ll install third-party libs
JOBS="$(nproc || echo 4)"        # Parallel build cores
RAYLIB_TAG="5.0"                 # Pinned to the 5.0 release
LIBCYAML_TAG="1.4.2"             # Match latest Debian/Ubuntu package :contentReference[oaicite:0]{index=0}
BUILD_DIR="build"                # Final artefacts end up here
# ---------------------------------------------------------------------------

echo "==> Updating package index & installing build prerequisites"
sudo apt-get update -y
sudo apt-get install -y \
  build-essential cmake git pkg-config \
  libasound2-dev libpulse-dev libaudio-dev libx11-dev libxext-dev \
  libxcursor-dev libxrandr-dev libxinerama-dev libxi-dev libxfixes-dev \
  libgl1-mesa-dev libegl1-mesa-dev libudev-dev libdrm-dev \
  libyaml-dev  # hard dependency for libcyaml

##############################################################################
# Build raylib (static, Desktop platform, no system install needed)
##############################################################################
if [ ! -d "external/raylib" ]; then
  echo "==> Cloning raylib ${RAYLIB_TAG}"
  mkdir -p external
  git clone --depth 1 --branch "${RAYLIB_TAG}" https://github.com/raysan5/raylib.git external/raylib
fi

echo "==> Compiling raylib"
cmake -S external/raylib -B external/raylib/build \
      -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
      -DBUILD_SHARED_LIBS=OFF \
      -DPLATFORM=Desktop \
      -DUSE_WAYLAND=OFF >/dev/null
cmake --build external/raylib/build -j"${JOBS}" >/dev/null
cmake --install external/raylib/build >/dev/null

##############################################################################
# Build libcyaml (needs libyaml, already installed)
##############################################################################
if [ ! -d "external/libcyaml" ]; then
  echo "==> Cloning libcyaml ${LIBCYAML_TAG}"
  git clone --depth 1 --branch "v${LIBCYAML_TAG}" https://github.com/tlsa/libcyaml.git external/libcyaml
fi

echo "==> Compiling libcyaml"
make -C external/libcyaml VARIANT=release -j"${JOBS}" >/dev/null
make -C external/libcyaml VARIANT=release PREFIX="${PREFIX}" install >/dev/null

##############################################################################
# Patch Makefile paths (Windows-specific flags → POSIX)
##############################################################################
echo "==> Patching project Makefile for local install"
sed -i.bak \
  -e "s|^RAYLIB_INC.*|RAYLIB_INC    := ${PREFIX}/include|" \
  -e "s|^RAYLIB_LIB.*|RAYLIB_LIB    := ${PREFIX}/lib|" \
  -e "s|-lwinmm ||g" \
  -e "s|-lgdi32 ||g" \
  -e "s|LDFLAGS.*|LDFLAGS       := -L\$(RAYLIB_LIB) -lraylib -lcyaml -lm -lpthread -ldl|" \
  Makefile

##############################################################################
# Build the project itself
##############################################################################
echo "==> Building bzzt"
make clean >/dev/null || true
make -j"${JOBS}"

echo "==> Build complete! Executable is at ${BUILD_DIR}/bzzt.exe (or bzzt)"
echo "    Add ${PREFIX}/lib to LD_LIBRARY_PATH if you plan to run from another shell:"
echo "      export LD_LIBRARY_PATH=\"${PREFIX}/lib:\$LD_LIBRARY_PATH\""
