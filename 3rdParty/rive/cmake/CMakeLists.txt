#[[
  SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
  SPDX-FileCopyrightText: 2023 basysKom GmbH

  SPDX-License-Identifier: LGPL-3.0-or-later
]]

cmake_minimum_required(VERSION 3.14)
project(rive_cpp VERSION 1.0.0 LANGUAGES CXX)

set(BUILD_SHARED_LIBS OFF)

set(RIVE_CPP_SOURCE_DIR  "${CMAKE_SOURCE_DIR}/3rdParty/rive/cmake/rive-cpp")
set(RIVE_CPP_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/3rdParty/rive/cmake/rive-cpp/include")

file(GLOB RIVE_CPP_SOURCE_DIR_SUBDIRS LIST_DIRECTORIES true "${RIVE_CPP_SOURCE_DIR}/*")
list(GET RIVE_CPP_SOURCE_DIR_SUBDIRS 0 RIVE_CPP_SOURCE_DIR)

set(HARFBUZZ_SOURCE_DIR "${CMAKE_SOURCE_DIR}/3rdParty/rive/cmake/harfbuzz")
set(SHEENBIDI_SOURCE_DIR "${CMAKE_SOURCE_DIR}/3rdParty/rive/cmake/sheenbidi/SheenBidi")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-D_DEBUG)
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    add_definitions(-D_RELEASE)
endif()

include_directories("${RIVE_CPP_INCLUDE_DIR}")

option(BUILD_SHARED_LIBS "Build shared libraries" ON)

add_subdirectory("${HARFBUZZ_SOURCE_DIR}/${HARFBUZZ_CPP_TARGET_DIR}")

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

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/sheenbidi")

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

include("${CMAKE_CURRENT_LIST_DIR}/src-CMakeLists.txt")

