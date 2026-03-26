# Native Windows x64 toolchain using MinGW-w64
# Download: https://github.com/niXman/mingw-builds-binaries/releases
#   x86_64-*-release-*-ucrt-*.7z
#
# Usage:
#   cmake -B build_win64 -DCMAKE_TOOLCHAIN_FILE=build/win64/mingw64-gcc-toolchain.cmake
#   cmake --build build_win64
#
# Set MINGW64_ROOT env variable if MinGW is not installed at the default path (C:/mingw64).

if(DEFINED ENV{MINGW64_ROOT})
  set(TOOLCHAIN_ROOT "$ENV{MINGW64_ROOT}")
else()
  set(TOOLCHAIN_ROOT "C:/mingw64")
endif()

# Set up the compiler and assembler
set(CMAKE_C_COMPILER   ${TOOLCHAIN_ROOT}/bin/gcc.exe)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_ROOT}/bin/gcc.exe)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_ROOT}/bin/g++.exe)

# Set up GCC binutils
set(CMAKE_OBJCOPY   ${TOOLCHAIN_ROOT}/bin/objcopy.exe CACHE INTERNAL "objcopy tool")
set(CMAKE_SIZE_UTIL ${TOOLCHAIN_ROOT}/bin/size.exe    CACHE INTERNAL "size tool")
