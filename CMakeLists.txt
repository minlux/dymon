cmake_minimum_required (VERSION 2.6)
project(dymon)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")

include_directories(
   libs/cjson
   libs/cpp-httplib
   src/barcode
   src/bitmap
   src/bitmap/fonts
   src/dymon
   src
)

#commandline tool
add_executable(dymon_cli
   src/bitmap/bitmap.cpp
   src/bitmap/glyphIterator.cpp
   src/bitmap/utf8decoder.cpp
   src/barcode/barcodeEan8.cpp
   src/dymon/dymon.cpp
   src/dymon/dymon_linux.cpp
   src/main_dymon_cli.cpp
)

#print bitmap-files from command line
add_executable(dymon_pbm
   src/bitmap/bitmap.cpp
   src/bitmap/glyphIterator.cpp
   src/bitmap/utf8decoder.cpp
   src/barcode/barcodeEan8.cpp
   src/dymon/dymon.cpp
   src/dymon/dymon_linux.cpp
   src/main_dymon_pbm.cpp
)

#printserver/webserver
add_executable(dymon_srv
   libs/cjson/cJSON.c
   src/bitmap/bitmap.cpp
   src/bitmap/glyphIterator.cpp
   src/bitmap/utf8decoder.cpp
   src/barcode/barcodeEan8.cpp
   src/dymon/dymon.cpp
   src/dymon/dymon_linux.cpp
   src/print.cpp
   src/main_dymon_srv.cpp
   src/index_html.c
)
target_link_libraries(dymon_srv pthread)



#debugging and development
add_executable(dev
   src/dymon/dymon.cpp
   src/dymon/dymon_linux.cpp
   src/main_dev.cpp
)
target_compile_definitions(dev PUBLIC
   DYMON_DEBUG
)
