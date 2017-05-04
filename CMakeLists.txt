cmake_minimum_required(VERSION 2.8.11)

project(axmldec C CXX)
set(AXMLDEC_VERSION_MAJOR 1)
set(AXMLDEC_VERSION_MINOR 0)
set(AXMLDEC_VERSION_PATCH 1)
string(TIMESTAMP AXMLDEC_BUILD_TIMESTAMP)

include(CheckCXXCompilerFlag)
if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
    # Avoid the variadic template bugs in MSVC.
    add_definitions(-DBOOST_NO_CXX11_VARIADIC_TEMPLATES)
else()
    check_cxx_compiler_flag("-std=c++14" COMPILER_SUPPORTS_CXX14)
    check_cxx_compiler_flag("-std=c++1y" COMPILER_SUPPORTS_CXX1Y)
    if(COMPILER_SUPPORTS_CXX14)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
    elseif(COMPILER_SUPPORTS_CXX1Y)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")
    else()
        message(FATAL_ERROR
                "The compiler ${CMAKE_CXX_COMPILER} has no C++14 support."
                "Please use a different C++ compiler.")
    endif()
    if(${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra -pedantic")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
endif()

include_directories("include")

set(BOOST_MIN_VERSION "1.54.0")
find_package(Boost ${BOOST_MIN_VERSION}
    COMPONENTS system iostreams program_options locale REQUIRED)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

configure_file (
    "${PROJECT_SOURCE_DIR}/axmldec_config.hpp.in"
    "${PROJECT_BINARY_DIR}/axmldec_config.hpp"
)
include_directories("${PROJECT_BINARY_DIR}")

add_executable(axmldec
    main.cpp
    include/jitana/util/axml_parser.hpp
    include/jitana/util/stream_reader.hpp
    lib/jitana/util/axml_parser.cpp
)
target_link_libraries(axmldec ${Boost_LIBRARIES})

install(TARGETS axmldec DESTINATION bin)
