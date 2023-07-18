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
    HOMEPAGE_URL "https://github.com/basysKom/RiveQtQuickPlugin"
    LANGUAGES CXX
)

option(RQQRP_BUILD_EXAMPLES "Build demo applications." ON)
option(RQQRP_DOWNLOAD_BUILD_DEPENDENCIES "Downloads and builds dependencies." ON)
option(RQQRP_BUILD_TESTS "Build unit tests." ON)

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

add_compile_options(-gdwarf-4)

# Add rive-cpp dependency if required
if(${RQQRP_DOWNLOAD_BUILD_DEPENDENCIES})
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/rive/cmake)
endif()

set(PLUGIN_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/RiveQtQuickItem)

add_subdirectory(${PLUGIN_SOURCE_DIR})

# Build examples if enabled
if(RQQRP_BUILD_EXAMPLES)
    set(EXAMPLES_BINARY_DIR "${CMAKE_BINARY_DIR}/binary/examples" CACHE PATH "Binary directory for examples")
    set(PLUGIN_TARGET_PATH "${CMAKE_BINARY_DIR}/binary/${PROJECT_NAME}/")

    add_subdirectory(examples)
endif()

if(RQQRP_BUILD_TESTS)
    add_subdirectory(tests)
endif()

