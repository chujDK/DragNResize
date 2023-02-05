cmake_minimum_required(VERSION 3.14)
project(DragNResize)
set(CMAKE_CXX_STANDARD 17)
include_directories(${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/res)
aux_source_directory(${CMAKE_SOURCE_DIR}/src/ ALL_SRC)
set_source_files_properties(res/tray.ico ./res/tray.rc PROPERTIES LANGUAGE RC)
add_executable(DragNResize WIN32 ${ALL_SRC} ./res/tray.rc)

target_compile_definitions(DragNResize
  PRIVATE "UNICODE;_UNICODE"
)