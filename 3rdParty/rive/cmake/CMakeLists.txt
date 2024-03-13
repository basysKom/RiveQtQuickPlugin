#[[
  SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
  SPDX-FileCopyrightText: 2023 basysKom GmbH

  SPDX-License-Identifier: LGPL-3.0-or-later
]]

cmake_minimum_required(VERSION 3.14)
project(rive_cpp VERSION 1.0.0 LANGUAGES CXX)

set(BUILD_SHARED_LIBS OFF)

function(extract_zip zip_file output_dir target_name)
  if (WIN32)
    # Remove the output directory if it already exists
    set(PS_REMOVE_DIR_SCRIPT "if (Test-Path '${output_dir}') { Remove-Item '${output_dir}' -Recurse -Force }")
    execute_process(COMMAND powershell -NoProfile -NonInteractive -ExecutionPolicy Bypass -Command "${PS_REMOVE_DIR_SCRIPT}")

		message("Add-Type -A System.IO.Compression.FileSystem; [System.IO.Compression.ZipFile]::ExtractToDirectory('${zip_file}', '${output_dir}')")
    # Extract the zip file
		set(PS_SCRIPT "Add-Type -A System.IO.Compression.FileSystem; [System.IO.Compression.ZipFile]::ExtractToDirectory('${zip_file}', '${output_dir}')")
    execute_process(COMMAND powershell -NoProfile -NonInteractive -ExecutionPolicy Bypass -Command "${PS_SCRIPT}")
  else()
    # Remove the output directory if it already exists
    execute_process(COMMAND ${CMAKE_COMMAND} -E remove_directory "${output_dir}")

    # Extract the zip file
    message(STATUS "EXTRACT directory content: ${zip_file} to ${output_dir}")
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory "${output_dir}")
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf "${zip_file}" WORKING_DIRECTORY "${output_dir}")
  endif()

  if(target_name)
      file(GLOB SUBDIR LIST_DIRECTORIES true "${output_dir}/*")

      # Add diagnostic messages
      message(STATUS "Output directory content: ${output_dir}")
      message(STATUS "SUBDIR variable content: ${SUBDIR}")

      # List all files and directories in the output_dir
      file(GLOB ALL_CONTENT "${output_dir}/*")
      message(STATUS "All content inside output_dir: ${ALL_CONTENT}")

      if (SUBDIR)
        list(GET SUBDIR 0 FIRST_SUBDIR)
        file(RENAME "${FIRST_SUBDIR}" "${output_dir}/${target_name}")
      else()
        message(FATAL_ERROR "No subdirectories found in ${output_dir}")
      endif()
    endif()
endfunction()

# use the version that fixes gcc compilation 
set(RIVE_CPP_DOWNLOAD_URL "https://github.com/rive-app/rive-cpp/archive/16d32220e76f40fe6492cbc575c80e9545707873.zip")

set(RIVE_CPP_DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/rive-cpp-src-download")
set(RIVE_CPP_SOURCE_DIR "${CMAKE_SOURCE_DIR}/3rdParty/rive/cmake/rive-cpp-src")
set(RIVE_CPP_TARGET_DIR "zip")

file(DOWNLOAD ${RIVE_CPP_DOWNLOAD_URL} "${RIVE_CPP_DOWNLOAD_DIR}/rive-cpp.zip" SHOW_PROGRESS)
extract_zip("${RIVE_CPP_DOWNLOAD_DIR}/rive-cpp.zip" "${RIVE_CPP_SOURCE_DIR}" "${RIVE_CPP_TARGET_DIR}")

file(GLOB RIVE_CPP_SOURCE_DIR_SUBDIRS LIST_DIRECTORIES true "${RIVE_CPP_SOURCE_DIR}/*")
list(GET RIVE_CPP_SOURCE_DIR_SUBDIRS 0 RIVE_CPP_SOURCE_DIR)

set(HARFBUZZ_DOWNLOAD_URL "https://github.com/harfbuzz/harfbuzz/archive/refs/tags/8.3.0.zip")
set(HARFBUZZ_DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/harfbuzz-src-download")
set(HARFBUZZ_SOURCE_DIR "${RIVE_CPP_SOURCE_DIR}/harfbuzz/")
set(HARFBUZZ_CPP_TARGET_DIR "zip")

file(DOWNLOAD ${HARFBUZZ_DOWNLOAD_URL} "${HARFBUZZ_DOWNLOAD_DIR}/harfbuzz.zip" SHOW_PROGRESS)
extract_zip("${HARFBUZZ_DOWNLOAD_DIR}/harfbuzz.zip" "${HARFBUZZ_SOURCE_DIR}" "${HARFBUZZ_CPP_TARGET_DIR}")

set(SHEENBIDI_DOWNLOAD_URL "https://codeload.github.com/Tehreer/SheenBidi/zip/refs/tags/v2.6")
set(SHEENBIDI_DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/sheenbidi-src-download")
set(SHEENBIDI_SOURCE_DIR "${RIVE_CPP_SOURCE_DIR}/sheenbidi")
set(SHEENBIDI_CPP_TARGET_DIR "zip")

file(DOWNLOAD ${SHEENBIDI_DOWNLOAD_URL} "${SHEENBIDI_DOWNLOAD_DIR}/sheenbidi.zip" SHOW_PROGRESS)
extract_zip("${SHEENBIDI_DOWNLOAD_DIR}/sheenbidi.zip" "${SHEENBIDI_SOURCE_DIR}" "${SHEENBIDI_CPP_TARGET_DIR}")


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-D_DEBUG)
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    add_definitions(-D_RELEASE)
endif()

include_directories("${RIVE_CPP_SOURCE_DIR}/include")

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

