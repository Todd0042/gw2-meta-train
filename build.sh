#!/usr/bin/env bash
# MetaTrain Nexus Addon — build script
# Cross-compiles a Windows DLL from Linux using mingw-w64.
# Requires: cmake, x86_64-w64-mingw32-g++, and imgui/ from the Nexus SDK.
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "=== MetaTrain Nexus Addon Builder ==="

# ── prerequisite checks ───────────────────────────────────────────────────────
for dep in cmake x86_64-w64-mingw32-g++; do
    if ! command -v "$dep" &>/dev/null; then
        echo "ERROR: $dep not found." >&2
        echo "  Arch/CachyOS: sudo pacman -S mingw-w64-gcc cmake" >&2
        echo "  Debian/Ubuntu: sudo apt install mingw-w64 cmake" >&2
        exit 1
    fi
done

if [[ ! -f "imgui/imgui.cpp" ]]; then
    echo "ERROR: imgui/ directory not found." >&2
    echo "  Copy the imgui/ folder from the Nexus SDK:" >&2
    echo "  https://github.com/RaidcoreGG/RCGG-lib-nexus-api" >&2
    echo "  (look in the 'include' or 'imgui' subdirectory of the SDK)" >&2
    exit 1
fi

# ── optional --clean flag ─────────────────────────────────────────────────────
CLEAN=0
for arg in "$@"; do [[ "$arg" == "--clean" ]] && CLEAN=1; done
if [[ $CLEAN -eq 1 ]]; then
    echo "Cleaning build directory..."
    rm -rf build
fi

# ── configure + build ─────────────────────────────────────────────────────────
cmake -B build \
      -DCMAKE_TOOLCHAIN_FILE=toolchain-mingw.cmake \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_VERBOSE_MAKEFILE=OFF \
      -Wno-dev

cmake --build build --config Release -- -j"$(nproc)"

echo ""
echo "Build complete: build/MetaTrain.dll"
echo ""
echo "Deploy to:"
echo "  C:\\Program Files\\Guild Wars 2\\addons\\MetaTrain.dll"
echo ""

# ── dependency audit — must not list libwinpthread, libgcc_s, libstdc++ (§4) ──
echo "DLL dependencies (should only be KERNEL32, USER32, api-ms-win-crt-*):"
x86_64-w64-mingw32-objdump -p build/MetaTrain.dll | grep "DLL Name" || true
