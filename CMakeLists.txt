cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(myproject LANGUAGES CXX)
add_executable(rtfreadr rtf/rtfreadr.cpp rtf/rtfactn.cpp)
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
            target_compile_options(rtfreadr
                -Wduplicated-branches
            )
        # GCC >= 6.0.0
        if (NOT(${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 6.0.0))
            target_compile_options(rtfreadr
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
