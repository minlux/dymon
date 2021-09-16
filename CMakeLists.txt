cmake_minimum_required (VERSION 2.6)
project(dymon)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=gnu++17")

include_directories(
   libs/cjson
   libs/cpp-httplib
   src/barcode
   src/bitmap
   src/bitmap/fonts
   src/dymon
   src
)


if(WIN32)
  set(dymon_x dymon_win32.cpp)
else()
  set(dymon_x dymon_linux.cpp)
endif()


#commandline tool
add_executable(dymon_cli
   src/bitmap/bitmap.cpp
   src/bitmap/glyphIterator.cpp
   src/bitmap/utf8decoder.cpp
   src/barcode/barcodeEan8.cpp
   src/dymon/dymon.cpp
   src/dymon/${dymon_x}
   src/main_dymon_cli.cpp
)
if(WIN32)
  target_link_libraries(dymon_cli ws2_32)
endif()


#print bitmap-files from command line
add_executable(dymon_pbm
   src/bitmap/bitmap.cpp
   src/bitmap/glyphIterator.cpp
   src/bitmap/utf8decoder.cpp
   src/barcode/barcodeEan8.cpp
   src/dymon/dymon.cpp
   src/dymon/${dymon_x}
   src/main_dymon_pbm.cpp
)
if(WIN32)
  target_link_libraries(dymon_pbm ws2_32)
endif()


#printserver/webserver
add_executable(dymon_srv
   libs/cjson/cJSON.c
   src/bitmap/bitmap.cpp
   src/bitmap/glyphIterator.cpp
   src/bitmap/utf8decoder.cpp
   src/barcode/barcodeEan8.cpp
   src/dymon/dymon.cpp
   src/dymon/${dymon_x}
   src/print.cpp
   src/main_dymon_srv.cpp
   src/index_html.c
)
target_link_libraries(dymon_srv pthread)
if(WIN32)
  target_link_libraries(dymon_srv ws2_32)
endif()


