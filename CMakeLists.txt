# 2.8.12 because we use target_compile_options introduced there.
cmake_minimum_required(VERSION 2.8.12)
project(myproject CXX)
if (CMAKE_VERSION VERSION_LESS "3.1")
  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  endif()
else()
  set(CMAKE_CXX_STANDARD 11)
endif()
add_executable(rtfreadr rtf/rtfreadr.cpp rtf/rtfparser.h)
add_executable(sb-sloka-counter sb-sloka-counter.cpp rtf/rtfparser.h)
add_executable(sb-itx-sloka-counter sb-itx-sloka-counter.cpp)
target_include_directories(sb-sloka-counter PRIVATE rtf)
if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS) # for fopen()
    set(WARN_FLAGS ${WARN_FLAGS} /permissive- /W4
        /WX
        /w14242 /w14254 /w14263 /w14265 /w14287 /we4289 /w14296 /w14311
        /w14545 /w14546 /w14547 /w14549 /w14555 /w14619 /w14640 /w14826
        /w14905 /w14906 /w14928)
else()
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # GCC >= 4.3
        if (NOT(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.3))
            set(WARN_FLAGS ${WARN_FLAGS} -Wsign-conversion)
        # GCC >= 4.6
        elseif (NOT(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.6))
            set(WARN_FLAGS ${WARN_FLAGS} -Wdouble-promotion)
        # GCC >= 4.8
        elseif (NOT(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8))
            set(WARN_FLAGS ${WARN_FLAGS} -Wuseless-cast)
        # GCC >= 6.0
        elseif (NOT(${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 6.0))
            set(WARN_FLAGS ${WARN_FLAGS}
                -Wmisleading-indentation
                -Wduplicated-cond
                -Wnull-dereference
            )
        # GCC >= 7.0
        elseif (NOT(${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 7.0))
            set(WARN_FLAGS ${WARN_FLAGS} -Wduplicated-branches)
        endif()
        # GCC, all versions
        set(WARN_FLAGS ${WARN_FLAGS}
            -Wlogical-op
            -Wpedantic)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # CLANG >= 3.2
        if (NOT(${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 3.2))
            set(WARN_FLAGS ${WARN_FLAGS} -Wpedantic)
        # CLANG >= 3.8
        elseif (NOT(${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 3.8))
            set(WARN_FLAGS ${WARN_FLAGS} -Wdouble-promotion)
        # CLANG >= 8.0
        elseif (NOT(${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 8.0))
            # As of now (Nov 2018) -Wlifetime is only present in the
            # special branch of clang (not even in trunk!).
            # So, assume future release 8 (but we don't really know).
            set(WARN_FLAGS ${WARN_FLAGS} -Wlifetime)
        endif()
        # Clang, all versions
        set(WARN_FLAGS ${WARN_FLAGS} -Wsign-conversion)
    endif()
    # Common warnings understood by both GCC and Clang, including old versions.
    if (CMAKE_CXX_COMPILER_ID MATCHES "^GNU|Clang$")
        set(WARN_FLAGS ${WARN_FLAGS}
            -Wall -Wextra -Wshadow
            -Werror
            -Wnon-virtual-dtor -pedantic
            -Wold-style-cast -Wcast-align -Wunused -Woverloaded-virtual
            -Wconversion
            -Wformat=2)
    endif()
endif()
target_compile_options(rtfreadr PRIVATE ${WARN_FLAGS})
target_compile_options(sb-sloka-counter PRIVATE ${WARN_FLAGS})
target_compile_options(sb-itx-sloka-counter PRIVATE ${WARN_FLAGS})
