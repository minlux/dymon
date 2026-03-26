# Cross-compilation toolchain for Windows x64 (win64) on Linux
# Requires: sudo apt-get install mingw-w64
#
# Usage:
#   cmake -B build_win64 -DCMAKE_TOOLCHAIN_FILE=cmake/mingw64-linux-cross-toolchain.cmake
#   cmake --build build_win64

set(CMAKE_SYSTEM_NAME Windows)

set(CMAKE_C_COMPILER   x86_64-w64-mingw32-gcc)
set(CMAKE_ASM_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER  x86_64-w64-mingw32-windres)

set(CMAKE_OBJCOPY   x86_64-w64-mingw32-objcopy CACHE INTERNAL "objcopy tool")
set(CMAKE_SIZE_UTIL x86_64-w64-mingw32-size    CACHE INTERNAL "size tool")

set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
