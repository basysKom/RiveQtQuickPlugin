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

cmake_minimum_required(VERSION 3.14)

project(RiveQtQuickPlugin
    VERSION 0.1.0.0
    DESCRIPTION "Cross-platform rive animation renderer for Qt Quick."
    HOMEPAGE_URL "https://github.com/jebos/RiveQtQuickPlugin"
    LANGUAGES CXX
)

option(RQQRP_BUILD_EXAMPLES "Build demo applications." ON)
option(RQQRP_DOWNLOAD_BUILD_DEPENDENCIES "Downloads and Builds dependencies." ON)

# Set up Qt configuration and enable C++17
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt package with a minimum version of 5.15.2 and not 6
find_package(Qt5 5.15 COMPONENTS Core Gui Quick OpenGL REQUIRED)

# Set the project sources
set(PROJECT_SOURCES
    src/riveqtquickplugin.h
    src/riveqtquickplugin.cpp
    src/renderer/riveqtopenglrenderer.h
    src/renderer/riveqtopenglrenderer.cpp
    src/renderer/riveqtutils.h
    src/renderer/riveqtutils.cpp
    src/renderer/riveqtfont.h
    src/renderer/riveqtfont.cpp
    src/renderer/riveqtpainterrenderer.h
    src/renderer/riveqtpainterrenderer.cpp
    src/renderer/riveqtfactory.h
    src/renderer/riveqtfactory.cpp
    src/qtquick/riveqtquickitem.h
    src/qtquick/riveqtquickitem.cpp
    src/qtquick/riveqtstatemachineinputmap.h
    src/qtquick/riveqtstatemachineinputmap.cpp
    src/qtquick/riveqsgopenglrendernode.h
    src/qtquick/riveqsgopenglrendernode.cpp
    src/qtquick/riveqsgsoftwarerendernode.h
    src/qtquick/riveqsgsoftwarerendernode.cpp
    src/qtquick/riveqsgrendernode.h
    src/qtquick/riveqsgrendernode.cpp
    src/qmldir
    shader.qrc
)

# Add rive-cpp dependency if required
if(${RQQRP_DOWNLOAD_BUILD_DEPENDENCIES})
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/rive/cmake)
endif()

# Create the plugin as a shared library
add_library(${PROJECT_NAME} SHARED ${PROJECT_SOURCES})

# Set compile definitions for debug configurations
target_compile_definitions(${PROJECT_NAME}
  PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)

# Link the plugin with required libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
  Qt5::Core
  Qt5::CorePrivate
  Qt5::Quick
  Qt5::Gui
  Qt5::GuiPrivate
  Qt5::OpenGL
  rive_cpp
)

# Include necessary directories
include_directories(${Qt5Gui_PRIVATE_INCLUDE_DIRS} ${Qt5Core_PRIVATE_INCLUDE_DIRS} "src/renderer")

# Set the binary output directory for the plugin
set(QML_BINARY_DIR "${CMAKE_BINARY_DIR}/binary/" CACHE PATH "Binary directory for QML plugins")

# Copy the plugin and qmldir to the binary directory after building
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${QML_BINARY_DIR}/RiveQtQuickPlugin
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${QML_BINARY_DIR}/RiveQtQuickPlugin
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/src/qmldir ${QML_BINARY_DIR}/RiveQtQuickPlugin
    COMMENT "Copying plugin and qmldir to binary directory"
)

# Build examples if enabled
if(RQQRP_BUILD_EXAMPLES)
    set(EXAMPLES_BINARY_DIR "${CMAKE_BINARY_DIR}/binary/examples" CACHE PATH "Binary directory for examples")

    add_subdirectory(examples)

    if (WIN32)
        find_program(WINDEPLOYQT_EXECUTABLE NAMES windeployqt HINTS ${QTDIR} ENV QTDIR PATH_SUFFIXES bin)
        if (NOT WINDEPLOYQT_EXECUTABLE)
            message(FATAL_ERROR "Failed to find windeployqt executable. Please make sure it is in your PATH or set the QTDIR environment variable.")
        endif()

        # Copy the plugin and qmldir to the examples binary directory
        add_custom_command(
            TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory ${EXAMPLES_BINARY_DIR}/RiveQtQuickPlugin
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${EXAMPLES_BINARY_DIR}/RiveQtQuickPlugin
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/src/qmldir ${EXAMPLES_BINARY_DIR}/RiveQtQuickPlugin
            COMMENT "Deploying ${PROJECT_NAME} to ${EXAMPLES_BINARY_DIR}"
        )
    endif()
endif()
