cmake_minimum_required( VERSION 3.20 )
project( la_fenshen LANGUAGES CXX C )

set( CMAKE_CXX_STANDARD 20 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static" )
set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++" )

add_library( minhook STATIC
    third_party/minhook/src/buffer.c
    third_party/minhook/src/hook.c
    third_party/minhook/src/trampoline.c
    third_party/minhook/src/hde/hde32.c
    third_party/minhook/src/hde/hde64.c )

target_include_directories( minhook PUBLIC third_party/minhook/include )

add_library( payload SHARED
    src/payload/dllmain.cpp
    src/payload/mutant.cpp
    src/payload/share.cpp
    src/payload/profile.cpp
    src/payload/stealth.cpp
    src/payload/hwid.cpp
    src/payload/render.cpp )

target_include_directories( payload PRIVATE src/common src/payload )
target_link_libraries( payload PRIVATE minhook ntdll iphlpapi d3d11 dxgi )
set_target_properties( payload PROPERTIES OUTPUT_NAME "64" PREFIX "" )

add_library( driverinject STATIC
    src/driverinject/loader.cpp )

target_include_directories( driverinject PUBLIC src/driverinject src/common )

add_executable( launcher
    src/launcher/main.cpp
    src/launcher/inject.cpp
    src/launcher/layout.cpp
    src/launcher/cleanup.cpp )

target_include_directories( launcher PRIVATE src/common )
target_link_libraries( launcher PRIVATE driverinject psapi shell32 )
