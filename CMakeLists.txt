cmake_minimum_required(VERSION 2.8.4 FATAL_ERROR)
project(myproject CXX)
if (CMAKE_VERSION VERSION_LESS "3.1")
  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  endif()
else()
  set(CMAKE_CXX_STANDARD 11)
endif()
add_executable(rtfreadr rtf/rtfreadr.cpp)
if(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS) # for fopen()
    target_compile_options(rtfreadr PRIVATE /permissive- /W4 /w14640
        /WX
        /w14242 /w14254 /w14263 /w14265 /w14287 /we4289 /w14296 /w14311
        /w14545 /w14546 /w14547 /w14549 /w14555 /w14619 /w14640 /w14826
        /w14905 /w14906 /w14928)
else(MSVC)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        # GCC >= 7.0.0
        if (NOT(${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 7.0.0))
            target_compile_options(rtfreadr PRIVATE
                -Wduplicated-branches
            )
        # GCC >= 6.0.0
        elseif (NOT(${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 6.0.0))
            target_compile_options(rtfreadr PRIVATE
                -Wmisleading-indentation
                -Wduplicated-cond
                -Wnull-dereference
            )
        endif()
    endif()
    target_compile_options(rtfreadr PRIVATE -Wall -Wextra -Wshadow
        -Werror
        -Wnon-virtual-dtor -pedantic
        -Wold-style-cast -Wcast-align -Wunused -Woverloaded-virtual
        -Wpedantic -Wconversion -Wsign-conversion
        -Wlogical-op
        -Wuseless-cast -Wdouble-promotion -Wformat=2)
    if(CLANG)
        target_compile_options(rtfreadr PRIVATE -Wlifetime)
    endif(CLANG)
endif(MSVC)
