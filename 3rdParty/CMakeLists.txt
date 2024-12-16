#[[
  SPDX-FileCopyrightText: 2024 Jeremias Bosch <jeremias.bosch@basyskom.com>
  SPDX-FileCopyrightText: 2023 basysKom GmbH

  SPDX-License-Identifier: LGPL-3.0-or-later
]]

cmake_minimum_required(VERSION 3.14)
project(rive_cpp VERSION 1.0.0 LANGUAGES CXX C)

set(BUILD_SHARED_LIBS OFF)
set(RIVE_CPP_DEFINITIONS "_RIVE_INTERNAL_")
set(RIVE_CPP_SOURCE_DIR  "${CMAKE_SOURCE_DIR}/3rdParty/rive-runtime")
set(RIVE_CPP_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/3rdParty/rive-runtime/include")

file(GLOB RIVE_CPP_SOURCE_DIR_SUBDIRS LIST_DIRECTORIES true "${RIVE_CPP_SOURCE_DIR}/*")
list(GET RIVE_CPP_SOURCE_DIR_SUBDIRS 0 RIVE_CPP_SOURCE_DIR)

set(HARFBUZZ_SOURCE_DIR "${CMAKE_SOURCE_DIR}/3rdParty/cmake/harfbuzz")
set(SHEENBIDI_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/SheenBidi")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-D_DEBUG)
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    add_definitions(-D_RELEASE)
endif()

include_directories("${RIVE_CPP_INCLUDE_DIR}")

option(BUILD_SHARED_LIBS "Build shared libraries" ON)

add_subdirectory("harfbuzz")

set(HARFBUZZ_BUILD_TYPE "${CMAKE_BUILD_TYPE}")
set(HARFBUZZ_DEFINITIONS "HAVE_OT;HB_NO_FALLBACK_SHAPE;HB_NO_WIN1256;_HAS_EXCEPTIONS=0")

if(HARFBUZZ_BUILD_TYPE STREQUAL "Debug")
  list(APPEND HARFBUZZ_DEFINITIONS "DEBUG")
  set(HARFBUZZ_SYMBOLS ON)
  set(HARFBUZZ_OPTIMIZE OFF)
elseif(HARFBUZZ_BUILD_TYPE STREQUAL "Release")
  list(APPEND HARFBUZZ_DEFINITIONS "NDEBUG")
  set(HARFBUZZ_SYMBOLS ON)
  set(HARFBUZZ_OPTIMIZE ON)
elseif(HARFBUZZ_BUILD_TYPE STREQUAL "Dist")
  list(APPEND HARFBUZZ_DEFINITIONS "NDEBUG")
  set(HARFBUZZ_SYMBOLS OFF)
  set(HARFBUZZ_OPTIMIZE ON)
endif()

target_compile_definitions(harfbuzz PRIVATE ${HARFBUZZ_DEFINITIONS})

if(HARFBUZZ_BUILD_TYPE STREQUAL "Debug")
  target_compile_options(harfbuzz PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-g> $<$<COMPILE_LANGUAGE:C>:-g>)
elseif(HARFBUZZ_BUILD_TYPE STREQUAL "Release" OR HARFBUZZ_BUILD_TYPE STREQUAL "Dist")
  target_compile_options(harfbuzz PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-O2> $<$<COMPILE_LANGUAGE:C>:-O2>)
endif()

if (WIN32)
    # Set the warning level
    add_compile_options(/W3)
else()
    add_compile_options(-Wall -fPIC)
endif()

# Define _UNICODE and UNICODE
add_definitions(-D_UNICODE -DUNICODE)

# Define _USE_MATH_DEFINES
add_definitions(-D_USE_MATH_DEFINES)

#
#
#
############## SHEENBIDI

file(COPY ${RIVE_CPP_INCLUDE_DIR} DESTINATION ${CMAKE_BINARY_DIR}/output)


file(GLOB_RECURSE SHEENBIDI_SOURCES
        "${SHEENBIDI_SOURCE_DIR}/Source/*.c"
     "${SHEENBIDI_SOURCE_DIR}/Headers/*.h"
)

add_library(sheenbidi STATIC ${SHEENBIDI_SOURCES})

set_target_properties(sheenbidi PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output/bin"
)

#
#
#
############## RIVE RUNTIME
file(GLOB_RECURSE RIVE_CPP_SOURCES "${CMAKE_SOURCE_DIR}/3rdParty/rive-runtime/src/*.cpp")


if(BUILD_SHARED_LIBS)
  add_library(rive_cpp SHARED ${RIVE_CPP_SOURCES})
else()
  add_library(rive_cpp STATIC ${RIVE_CPP_SOURCES})
endif()

target_compile_definitions(rive_cpp PRIVATE ${RIVE_CPP_DEFINITIONS})

set_target_properties(rive_cpp PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output/lib"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output/lib"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output/bin"
)

target_include_directories(rive_cpp PUBLIC
    "${RIVE_CPP_INCLUDE_DIR}"
    "${HARFBUZZ_SOURCE_DIR}/${HARFBUZZ_CPP_TARGET_DIR}/src"
    "${SHEENBIDI_SOURCE_DIR}/${SHEENBIDI_CPP_TARGET_DIR}/Headers"
)

if(${RQQRP_RIVE_TEXT_RENDERING})
    target_link_libraries(rive_cpp PRIVATE harfbuzz)
    target_link_libraries(rive_cpp PRIVATE sheenbidi)
endif()

target_include_directories(sheenbidi PUBLIC ${SHEENBIDI_SOURCE_DIR}/Headers)
