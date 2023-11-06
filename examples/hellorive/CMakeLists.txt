#[[
  SPDX-FileCopyrightText: 2023 basysKom GmbH
  SPDX-License-Identifier: LGPL-3.0-or-later
]]

cmake_minimum_required(VERSION 3.14)
set(PROJECT_NAME "hellorive")

project(${PROJECT_NAME}
    VERSION 0.1.0.0
    DESCRIPTION "Hello Rive example"
    HOMEPAGE_URL "https://github.com/jebos/RiveQtQuickPlugin"
    LANGUAGES CXX
)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Quick OpenGL )
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Quick OpenGL)

qt_add_executable(${PROJECT_NAME}
    main.cpp
)

add_dependencies(${PROJECT_NAME} RiveQtQuickPlugin)
target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::Gui
)

qt6_add_resources(${PROJECT_NAME} "hello-rive"
    PREFIX
        "/"
    FILES
        "../rive/travel-icons-pack.riv"
        "main.qml"
)
