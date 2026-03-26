set MINGW64_ROOT=C:\mingw64
cmake -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=../../cmake/mingw64-windows-toolchain.cmake ../..
echo "call 'cmake --build . --parallel' to build the project"
