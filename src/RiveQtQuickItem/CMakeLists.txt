 #[[
SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
SPDX-FileCopyrightText: 2023 Jonas Kalinka <jonas.kalinka@basyskom.com>
SPDX-FileCopyrightText: 2023 basysKom GmbH

SPDX-License-Identifier: LGPL-3.0-or-later
]]

cmake_minimum_required(VERSION 3.16)

message("Build Type: ${CMAKE_BUILD_TYPE}")

set(PROJECT_SOURCES
    renderer/riveqtopenglrenderer.h
    renderer/riveqtopenglrenderer.cpp
    renderer/riveqtutils.h
    renderer/riveqtutils.cpp
    renderer/riveqtfont.h
    renderer/riveqtfont.cpp
    renderer/riveqtpainterrenderer.h
    renderer/riveqtpainterrenderer.cpp
    renderer/riveqtfactory.h
    renderer/riveqtfactory.cpp
    datatypes.h
    riveqtquickitem.h
    riveqtquickitem.cpp
    riveqtstatemachineinputmap.h
    riveqtstatemachineinputmap.cpp
    riveqsgopenglrendernode.h
    riveqsgopenglrendernode.cpp
    riveqsgsoftwarerendernode.h
    riveqsgsoftwarerendernode.cpp
    riveqsgrendernode.h
    riveqsgrendernode.cpp
    riveqtpath.cpp
    riveqtpath.h
    rqqplogging.h
    rqqplogging.cpp
    qmldir
    shader.qrc
)

if (${QT_VERSION_MAJOR} EQUAL 5)
    set(QT5_PROJECT_SOURCES
        #qt5
        riveqtquickplugin.h
        riveqtquickplugin.cpp
    )
elseif (${QT_VERSION_MAJOR} EQUAL 6)
    set(QT6_PROJECT_SOURCES
        #qt6
        rhi/texturetargetnode.h
        rhi/texturetargetnode.cpp
        riveqsgrhirendernode.h
        riveqsgrhirendernode.cpp
        renderer/riveqtrhirenderer.h
        renderer/riveqtrhirenderer.cpp
    )
endif()

set(DEPENDENCIES
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

# Set the binary output directory for the plugin
set(QML_BINARY_DIR "${CMAKE_BINARY_DIR}/binary" CACHE PATH "Binary directory for QML plugins")

if (${QT_VERSION_MAJOR} EQUAL 5)
    add_library(${PROJECT_NAME} SHARED ${PROJECT_SOURCES} ${QT5_PROJECT_SOURCES})
elseif (${QT_VERSION_MAJOR} EQUAL 6)
    qt_add_qml_module(
        ${PROJECT_NAME}
        URI
            ${PROJECT_NAME}
        VERSION
            1.0
        SHARED
        PLUGIN_TARGET
            ${PROJECT_NAME}
        OUTPUT_DIRECTORY
           ${QML_BINARY_DIR}/${PROJECT_NAME}
        SOURCES
            ${PROJECT_SOURCES}
            ${QT6_PROJECT_SOURCES}
    )
#if(BUILD_TESTS)
    qt_add_qml_module(
        ${PROJECT_NAME}_TEST
        URI
            ${PROJECT_NAME}_TEST
        VERSION
            1.0
        SHARED
        OUTPUT_DIRECTORY
           ${QML_BINARY_DIR}/${PROJECT_NAME}_TEST
        SOURCES
            ${PROJECT_SOURCES}
            ${QT6_PROJECT_SOURCES}
    )
    target_link_libraries(${PROJECT_NAME}_TEST PUBLIC ${DEPENDENCIES})
endif()

# Set compile definitions for debug configurations
target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)

# Link the plugin with required libraries
target_link_libraries(${PROJECT_NAME} PRIVATE ${DEPENDENCIES})

# Include necessary directories
include_directories(${Qt${QT_VERSION_MAJOR}Gui_PRIVATE_INCLUDE_DIRS} ${Qt${QT_VERSION_MAJOR}Core_PRIVATE_INCLUDE_DIRS} "renderer")


# Add shaders
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



if (${QT_VERSION_MAJOR} EQUAL 6)
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        # from binary to source dir
        COMMAND ${CMAKE_COMMAND} -E copy "${QML_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}.qmltypes" ${CMAKE_CURRENT_SOURCE_DIR}
    )
elseif(${QT_VERSION_MAJOR} EQUAL 5)
    # Copy the plugin and qmldir to the binary directory after building
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${QML_BINARY_DIR}/${PROJECT_NAME}"
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> "${QML_BINARY_DIR}/${PROJECT_NAME}"
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/qmldir" "${QML_BINARY_DIR}/${PROJECT_NAME}"
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.qmltypes" "${QML_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}.qmltypes"

        COMMENT "Copying plugin and qmldir to binary directory"
    )
endif()

# Generating the qmltypes file for Qt5.
# qmplugindump only works with release builds.
#if (${QT_VERSION_MAJOR} EQUAL 5 AND CMAKE_BUILD_TYPE STREQUAL "Release")
#    if (WIN32)
#        cmake_path(SET qmlplugindump "${CMAKE_PREFIX_PATH}/bin/qmlplugindump.exe")
#        add_custom_command(
#            TARGET ${PROJECT_NAME} POST_BUILD
#            COMMAND
#                ${qmlplugindump} -output ${PROJECT_NAME}/RiveQtQuickPlugin.qmltypes RiveQtQuickPlugin 1.0 .
#            WORKING_DIRECTORY
#                ${QML_BINARY_DIR}
#        )
#    else()
#        add_custom_command(
#            TARGET ${PROJECT_NAME} POST_BUILD
#            COMMAND
#                qmlplugindump -output ${PROJECT_NAME}/RiveQtQuickPlugin.qmltypes RiveQtQuickPlugin 1.0 .
#            WORKING_DIRECTORY
#                ${QML_BINARY_DIR}
#        )
#    endif()
#endif()
