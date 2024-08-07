cmake_minimum_required (VERSION 2.6)
project(dymon)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=gnu++17")

include_directories(
   libs/cjson
   libs/cpp-httplib
   libs/argtable3
   src/barcode
   src/bitmap
   src/bitmap/fonts
   src/dymon
   src
)


if(WIN32)
  set(dymon_x
    src/dymon/dymon_net_win32.cpp
    src/dymon/dymon_usb_win32.cpp
    src/usbprint_win32.c
  )
else()
  set(dymon_x
    src/dymon/dymon_net_linux.cpp
    src/dymon/dymon_usb_linux.cpp
  )
endif()


#print bitmap-files from command line
add_executable(dymon_pbm
  libs/cjson/cJSON.c
  libs/argtable3/argtable3.c
  src/bitmap/bitmap.cpp
  src/bitmap/glyphIterator.cpp
  src/bitmap/utf8decoder.cpp
  src/barcode/barcodeEan8.cpp
  src/dymon/dymon.cpp
  ${dymon_x}
  src/main_dymon_pbm.cpp
)
if(WIN32)
  target_link_libraries(dymon_pbm
    ws2_32
    setupapi
  )
endif()


#printserver/webserver
add_executable(dymon_srv
  libs/cjson/cJSON.c
  libs/argtable3/argtable3.c
  src/bitmap/bitmap.cpp
  src/bitmap/bitmap_fromText.cpp
  src/bitmap/glyphIterator.cpp
  src/bitmap/utf8decoder.cpp
  src/barcode/barcodeEan8.cpp
  src/dymon/dymon.cpp
  ${dymon_x}
  src/print_json.cpp
  src/main_dymon_srv.cpp
  src/index_html.c
)
target_link_libraries(dymon_srv
  pthread
)
if(WIN32)
  target_link_libraries(dymon_srv
    ws2_32
    setupapi
  )
endif()


#text to pbm tool
add_executable(txt2pbm
  libs/argtable3/argtable3.c
  src/bitmap/bitmap.cpp
  src/bitmap/bitmap_fromText.cpp
  src/bitmap/glyphIterator.cpp
  src/bitmap/utf8decoder.cpp
  src/barcode/barcodeEan8.cpp
  txt2pbm/main.cpp
)
