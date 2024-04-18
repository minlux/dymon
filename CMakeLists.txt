cmake_minimum_required (VERSION 2.6)
project(dymon)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=gnu++17")

include_directories(
   libs/cjson
   src/barcode
   src/bitmap
   src/bitmap/fonts
   src/dymon
   src
)


if(WIN32)
  set(dymon_x
    src/dymon/dymon_usb_win32.cpp
    src/usbprint_win32.c
  )
else()
  set(dymon_x

    src/dymon/dymon_usb_linux.cpp
  )
endif()

#print bitmap-files from command line
add_executable(dymon_pbm
  libs/cjson/cJSON.c
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



