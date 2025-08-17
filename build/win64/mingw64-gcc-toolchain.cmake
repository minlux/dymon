# https://github.com/niXman/mingw-builds-binaries/releases
#  https://github.com/niXman/mingw-builds-binaries/releases/download/15.2.0-rt_v13-rev0/x86_64-15.2.0-release-mcf-seh-ucrt-rt_v13-rev0.7z
set(CMAKE_SYSTEM_NAME Windows)
set(TOOLCHAIN_ROOT c:/mingw64)

# Set up the compiler and assembler
set(CMAKE_C_COMPILER   ${TOOLCHAIN_ROOT}/bin/gcc.exe)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_ROOT}/bin/gcc.exe)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_ROOT}/bin/g++.exe)

# Set up GCC binutils
set(CMAKE_OBJCOPY   ${TOOLCHAIN_ROOT}/bin/objcopy.exe CACHE INTERNAL "objcopy tool")
set(CMAKE_SIZE_UTIL ${TOOLCHAIN_ROOT}/bin/size.exe    CACHE INTERNAL "size tool")

# Set the path where to search for programs, libraries and incudes
#  (cf. https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/CrossCompiling)
set(CMAKE_FIND_ROOT_PATH ${TOOLCHAIN_ROOT}/bin)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

