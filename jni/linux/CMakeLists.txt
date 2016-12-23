cmake_minimum_required(VERSION 2.6)

find_package(OpenCL REQUIRED)

include_directories(${OpenCL_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

link_directories(${OpenCL_LIB_DIRS})

add_executable(clinstbench
  cli-app.cpp ../bench/bench.c ../port.c)

target_link_libraries(clinstbench OpenCL)
