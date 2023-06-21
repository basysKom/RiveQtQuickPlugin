#[[
  SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
  SPDX-FileCopyrightText: 2023 basysKom GmbH

  SPDX-License-Identifier: LGPL-3.0-or-later
]]

cmake_minimum_required(VERSION 3.14)

project(RiveQtQuickSimpleViewer
    VERSION 0.1.0.0
    DESCRIPTION "Simple Viewer Example to demonstrate the rive qt quick plugin"
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

set(SOURCES
    main.cpp
    main.qml
    qml.qrc
)

add_executable(${PROJECT_NAME} ${SOURCES})
add_dependencies(${PROJECT_NAME} RiveQtQuickPlugin)

target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::Gui
)

#if (WIN32)
#    find_program(WINDEPLOYQT_EXECUTABLE NAMES windeployqt HINTS ${QTDIR} ENV QTDIR PATH_SUFFIXES bin)
#    if (NOT WINDEPLOYQT_EXECUTABLE)
#        message(FATAL_ERROR "Failed to find windeployqt executable. Please make sure it is in your PATH or set the QTDIR environment variable.")
#    endif()

#    # Copy the plugin and qmldir to the examples binary directory
#    add_custom_command(
#        TARGET ${PROJECT_NAME} POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E make_directory ${EXAMPLES_BINARY_DIR}/RiveQtQuickPlugin
#        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${EXAMPLES_BINARY_DIR}/RiveQtQuickPlugin
#        COMMAND ${CMAKE_COMMAND} -E copy ${PLUGIN_TARGET_PATH} ${EXAMPLES_BINARY_DIR}/RiveQtQuickPlugin
#        COMMAND ${CMAKE_COMMAND} -E copy ${PLUGIN_SOURCE_DIR}/qmldir ${EXAMPLES_BINARY_DIR}/RiveQtQuickPlugin
#        COMMENT "Deploying ${PROJECT_NAME} to ${EXAMPLES_BINARY_DIR}"
#    )
#endif()
