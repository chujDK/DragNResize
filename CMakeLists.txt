cmake_minimum_required(VERSION 3.14)
project(DragNResize)
set(CMAKE_CXX_STANDARD 17)
include_directories(${CMAKE_SOURCE_DIR}/include)
aux_source_directory(${CMAKE_SOURCE_DIR}/src/ ALL_SRC)
add_executable(DragNResize WIN32 ${ALL_SRC})

target_compile_definitions(DragNResize
  PRIVATE "UNICODE;_UNICODE"
)