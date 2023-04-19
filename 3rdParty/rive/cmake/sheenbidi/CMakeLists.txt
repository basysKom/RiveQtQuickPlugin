#[[
  MIT License

  Copyright (C) 2023 by Jeremias Bosch

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
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
