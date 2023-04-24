#[[
  SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
  SPDX-FileCopyrightText: 2023 basysKom GmbH

  SPDX-License-Identifier: LGPL-3.0-or-later
]]

project(sheenbidi VERSION 1.0.0 LANGUAGES CXX)

set(SHEENBIDI_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../3rdParty/rive/cmake/rive-cpp-src/zip/sheenbidi/zip")

file(GLOB_RECURSE SHEENBIDI_SOURCES
     "${SHEENBIDI_SOURCE_DIR}/*.cpp"
     "${SHEENBIDI_SOURCE_DIR}/*.h"
)

add_library(sheenbidi STATIC ${SHEENBIDI_SOURCES})

set_target_properties(sheenbidi PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output/bin"
)
target_include_directories(sheenbidi PUBLIC ${SHEENBIDI_SOURCE_DIR}/Headers)
