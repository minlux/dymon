cmake_minimum_required (VERSION 2.6)
project(dymon)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")
include_directories(src libs libs/fonts libs/cjson)

add_executable(dymon
   src/main.cpp
   src/barcodeEan8.cpp
   src/bitmap.cpp
   src/glyphIterator.cpp
   src/utf8decoder.cpp
   libs/cjson/cJSON.c
)
