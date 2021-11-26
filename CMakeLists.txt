cmake_minimum_required (VERSION 3.22)
project(libpck LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)

find_package(ZLIB REQUIRED)

if(MSVC)
    add_compile_options(/utf-8)
endif()

set(LIB_SRC
    src/pckfile.cpp
    src/pckfile_c.cpp
    src/pckitem.cpp
    src/pcktree.cpp
)

if(BUILD_SHARED_LIBS AND WIN32)
    set(DLL_SRC
        libpck.dll/libpck.def
        libpck.dll/libpck.rc
    )
endif()

if(NOT WIN32)
    set(SYS_LIB pthread)
endif()

add_library(libpck ${LIB_SRC} ${DLL_SRC})
target_include_directories(libpck PUBLIC include)
target_include_directories(libpck PRIVATE src)
target_link_libraries(libpck PRIVATE ZLIB::ZLIB ${SYS_LIB})

if(NOT(BUILD_SHARED_LIBS AND WIN32))
    add_executable(pcktool pcktool/pcktool.cpp)
    target_include_directories(pcktool PRIVATE src)
    target_link_libraries(pcktool PRIVATE libpck)
endif()
