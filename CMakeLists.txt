cmake_minimum_required(VERSION 2.8.11)

project(axmldec C CXX)
option(ENABLE_APK_LOADING "enable loading from an APK file" ON)
set(AXMLDEC_VERSION_MAJOR 1)
set(AXMLDEC_VERSION_MINOR 2)
set(AXMLDEC_VERSION_PATCH 0)
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
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
endif()

add_definitions(
    -DAXMLDEC_VERSION_MAJOR="${AXMLDEC_VERSION_MAJOR}"
    -DAXMLDEC_VERSION_MINOR="${AXMLDEC_VERSION_MINOR}"
    -DAXMLDEC_VERSION_PATCH="${AXMLDEC_VERSION_PATCH}"
    -DAXMLDEC_BUILD_TIMESTAMP="${AXMLDEC_BUILD_TIMESTAMP}"
)

include_directories("${PROJECT_BINARY_DIR}")
include_directories("include")

#-------------------------------------------------------------------------------
# axmldec
#-------------------------------------------------------------------------------

add_executable(axmldec
    main.cpp
    include/jitana/util/axml_parser.hpp
    include/jitana/util/stream_reader.hpp
    lib/jitana/util/axml_parser.cpp
)

set(BOOST_MIN_VERSION "1.53.0")
find_package(Boost ${BOOST_MIN_VERSION}
    COMPONENTS system iostreams program_options locale REQUIRED)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
add_definitions(
    -DBOOST_MAJOR_VERSION=${Boost_MAJOR_VERSION}
    -DBOOST_MINOR_VERSION=${Boost_MINOR_VERSION}
)
target_link_libraries(axmldec ${Boost_LIBRARIES})

if(ENABLE_APK_LOADING)
    find_package(ZLIB REQUIRED)
    include_directories(${ZLIB_INCLUDE_DIR})

    add_definitions(
        -DENABLE_APK_LOADING=1
    )
    add_library(minizip STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/external/minizip/unzip.c
        ${CMAKE_CURRENT_SOURCE_DIR}/external/minizip/ioapi.c
    )
    set_target_properties(minizip PROPERTIES
        COMPILE_DEFINITIONS "NOUNCRYPT;NOCRYPT")
    target_link_libraries(minizip ${ZLIB_LIBRARIES})
    set(MINIZIP_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/external/minizip)
    set(MINIZIP_LIBRARIES minizip)
    include_directories(SYSTEM ${MINIZIP_INCLUDE_DIR})
    target_link_libraries(axmldec ${MINIZIP_LIBRARIES})
endif()

#-------------------------------------------------------------------------------
# Install
#-------------------------------------------------------------------------------

install(TARGETS axmldec DESTINATION bin)

#-------------------------------------------------------------------------------
# clang-format
#-------------------------------------------------------------------------------

add_custom_target(format
    find main.cpp include lib \\\( -name "*.hpp" -or -name "*.cpp" \\\)
            -type f -exec clang-format -i {} \\+
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

#-------------------------------------------------------------------------------
# Line Count
#-------------------------------------------------------------------------------

add_custom_target(count
    find main.cpp include lib \\\( -name "*.hpp" -or -name -name "*.cpp" \\\)
            -print0 | xargs -0 wc -l
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

#-------------------------------------------------------------------------------
# Doxygen
#-------------------------------------------------------------------------------

find_package(Doxygen)
if(DOXYGEN_FOUND)
    if(COMPILER_SUPPORTS_CXX14)
        set(CMAKE_DOXYGEN_CLANG_OPTIONS
            "${CMAKE_DOXYGEN_CLANG_OPTIONS} -std=c++14")
    else()
        set(CMAKE_DOXYGEN_CLANG_OPTIONS
            "${CMAKE_DOXYGEN_CLANG_OPTIONS} -std=c++1y")
    endif()
    set(CMAKE_DOXYGEN_CLANG_OPTIONS
        "${CMAKE_DOXYGEN_CLANG_OPTIONS} -stdlib=libc++")
    set(CMAKE_DOXYGEN_CLANG_OPTIONS
        "${CMAKE_DOXYGEN_CLANG_OPTIONS} --verbose")

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/doxygen/Doxyfile.in
        ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY
    )
    add_custom_target(doxygen
        ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
endif()
