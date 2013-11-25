cmake_minimum_required(VERSION 2.6)

link_directories("c:/Program Files (x86)/Intel/OpenCL SDK/3.0/lib")
link_directories("c:/Program Files (x86)/AMD APP/lib")
link_directories("c:/Program Files/Intel/OpenCL SDK/3.0/lib")
link_directories("c:/Program Files/AMD APP/lib")

link_directories("c:/Program Files (x86)/Intel/OpenCL SDK/3.0/lib/x86")
link_directories("c:/Program Files (x86)/AMD APP/lib/x86")
link_directories("c:/Program Files/Intel/OpenCL SDK/3.0/lib/x86")
link_directories("c:/Program Files/AMD APP/lib/x86")


include_directories("c:/Program Files (x86)/Intel/OpenCL SDK/3.0/include")
include_directories("c:/Program Files (x86)/AMD APP/include")
include_directories("c:/Program Files/Intel/OpenCL SDK/3.0/include")
include_directories("c:/Program Files/AMD APP/include")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(clinstbench WIN32 w32-app.cpp resource.rc ../bench/bench.c ../port.c)

target_link_libraries(clinstbench OpenCL.lib comctl32.lib)

set(CMAKE_CXX_FLAGS_RELEASE "/MT /O2 /Ob2 /D NDEBUG")
