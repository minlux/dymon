cmake_minimum_required (VERSION 2.6)
project(apoll)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
include_directories(fonts)

add_executable(dymon main.cpp barcodeEan8.cpp bitmap.cpp glyphIterator.cpp )
