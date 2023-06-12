#[[
  SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
  SPDX-FileCopyrightText: 2023 basysKom GmbH

  SPDX-License-Identifier: LGPL-3.0-or-later
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

# Find Qt package
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Qml Quick OpenGL )
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Qml Gui Quick OpenGL)

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
    src/qtquick/datatypes.h
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

if (${QT_VERSION_MAJOR} EQUAL 6)
    set(QT6_PROJECT_SOURCES
        #qt6
        src/qtquick/rhi/texturetargetnode.h
        src/qtquick/rhi/texturetargetnode.cpp
        src/qtquick/riveqsgrhirendernode.h
        src/qtquick/riveqsgrhirendernode.cpp
        src/renderer/riveqtrhirenderer.h
        src/renderer/riveqtrhirenderer.cpp
    )
#    qt_add_qml_module(
#        ${PROJECT_NAME}
#        URI
#            ${PROJECT_NAME}
#        VERSION
#            1.0
#        PLUGIN_TARGET
#            ${PROJECT_NAME}
#        OUTPUT_DIRECTORY
#            ${QML_BINARY_DIR}
#        SOURCES
#            ${PROJECT_SOURCES}
#        NO_GENERATE_PLUGIN_SOURCE
#            1
#    )
endif()

# Add rive-cpp dependency if required
if(${RQQRP_DOWNLOAD_BUILD_DEPENDENCIES})
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/rive/cmake)
endif()

if (${QT_VERSION_MAJOR} EQUAL 6)
    add_library(${PROJECT_NAME} SHARED ${PROJECT_SOURCES} ${QT6_PROJECT_SOURCES})
else()
    add_library(${PROJECT_NAME} SHARED ${PROJECT_SOURCES})
endif()

# Set compile definitions for debug configurations
target_compile_definitions(${PROJECT_NAME}
    PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)

# Link the plugin with required libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::CorePrivate
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::GuiPrivate
    Qt${QT_VERSION_MAJOR}::OpenGL
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::QuickPrivate
    rive_cpp
)

# Include necessary directories
if (${QT_VERSION_MAJOR} EQUAL 5)
    include_directories(${Qt5Gui_PRIVATE_INCLUDE_DIRS} ${Qt5Core_PRIVATE_INCLUDE_DIRS} "src/renderer")
else()
    include_directories(${Qt6Gui_PRIVATE_INCLUDE_DIRS} ${Qt6Core_PRIVATE_INCLUDE_DIRS} "src/renderer")
endif()

include_directories(${Qt${QT_VERSION_MAJOR}Gui_PRIVATE_INCLUDE_DIRS})

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

if (${QT_VERSION_MAJOR} EQUAL 6)
    find_package(Qt6 COMPONENTS ShaderTools)
    qt6_add_shaders(${PROJECT_NAME} "graph-shaders"
        BATCHABLE
        PRECOMPILE
        OPTIMIZED
        PREFIX
            "/"
        FILES
            "shaders/qt6/drawRiveTextureNode.frag"
            "shaders/qt6/drawRiveTextureNode.vert"
            "shaders/qt6/finalDraw.frag"
            "shaders/qt6/finalDraw.vert"
            "shaders/qt6/blendRiveTextureNode.frag"
            "shaders/qt6/blendRiveTextureNode.vert"
      )

    set(APP_RESOURCES
        "shaders.qrc"
        "${CMAKE_CURRENT_BINARY_DIR}/shaders/qt6/drawRiveTextureNode.vert.qsb"
        "${CMAKE_CURRENT_BINARY_DIR}/shaders/qt6/drawRiveTextureNode.frag.qsb"
        "${CMAKE_CURRENT_BINARY_DIR}/shaders/qt6/finalDraw.vert.qsb"
        "${CMAKE_CURRENT_BINARY_DIR}/shaders/qt6/finalDraw.frag.qsb"
        "${CMAKE_CURRENT_BINARY_DIR}/shaders/qt6/blendRiveTextureNode.vert.qsb"
        "${CMAKE_CURRENT_BINARY_DIR}/shaders/qt6/blendRiveTextureNode.frag.qsb"
    )
endif()

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

