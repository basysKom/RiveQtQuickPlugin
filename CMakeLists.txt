#[[
SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
SPDX-FileCopyrightText: 2023 Jonas Kalinka <jonas.kalinka@basyskom.com>
SPDX-FileCopyrightText: 2023 basysKom GmbH

SPDX-License-Identifier: LGPL-3.0-or-later
]]

cmake_minimum_required(VERSION 3.16)

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

if (${QT_VERSION_MAJOR} EQUAL 6)
    # TODO use new policy
    # https://doc.qt.io/qt-6/qt-cmake-policy-qtp0001.html
    qt_policy(SET QTP0001 OLD)
endif()

# Add rive-cpp dependency if required
if(${RQQRP_DOWNLOAD_BUILD_DEPENDENCIES})
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/rive/cmake)
endif()

add_subdirectory(src/RiveQtQuickItem)

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
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/qmldir ${EXAMPLES_BINARY_DIR}/RiveQtQuickPlugin
            COMMENT "Deploying ${PROJECT_NAME} to ${EXAMPLES_BINARY_DIR}"
        )
    endif()
endif()

