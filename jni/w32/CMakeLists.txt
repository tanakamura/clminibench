cmake_minimum_required(VERSION 2.6)

include_directories("c:/Program Files (x86)/Intel/OpenCL SDK/3.0/include")
include_directories("c:/Program Files (x86)/Intel/OpenCL SDK/4.6/include")
include_directories("c:/Program Files (x86)/AMD APP/include")
include_directories("c:/Program Files/Intel/OpenCL SDK/3.0/include")
include_directories("c:/Program Files/Intel/OpenCL SDK/4.6/include")
include_directories("c:/Intel/INDE/code_builder_5.0.0.43/include")
include_directories("c:/Program Files/AMD APP/include")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

if ($(CMAKE_SIZEOF_VOID_P) MATCHES 4)

link_directories("c:/Program Files (x86)/Intel/OpenCL SDK/3.0/lib")
link_directories("c:/Program Files (x86)/AMD APP/lib")
link_directories("c:/Program Files/Intel/OpenCL SDK/3.0/lib")
link_directories("c:/Program Files/AMD APP/lib")

link_directories("c:/Program Files (x86)/Intel/OpenCL SDK/3.0/lib/x86")
link_directories("c:/Program Files (x86)/AMD APP/lib/x86")
link_directories("c:/Program Files/Intel/OpenCL SDK/3.0/lib/x86")
link_directories("c:/Program Files/AMD APP/lib/x86")

else()

link_directories("c:/Program Files (x86)/Intel/OpenCL SDK/4.6/lib/x64")
link_directories("c:/Intel/INDE/code_builder_5.0.0.43/lib/x64/")

endif()

add_executable(clinstbench WIN32 w32-app.cpp resource.rc ../bench/bench.c ../port.c)

target_link_libraries(clinstbench OpenCL.lib comctl32.lib)

set(CMAKE_CXX_FLAGS_RELEASE "/MT /O2 /Ob2 /D NDEBUG")
