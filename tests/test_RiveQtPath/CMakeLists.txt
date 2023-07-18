project(test_RiveQtPath LANGUAGES CXX)

cmake_minimum_required(VERSION 3.14)

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test Core)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

enable_testing(true)


add_executable(${PROJECT_NAME} test_triangulation.cpp)
include_directories(
    ${Qt${QT_VERSION_MAJOR}Gui_PRIVATE_INCLUDE_DIRS}
    ${Qt${QT_VERSION_MAJOR}Core_PRIVATE_INCLUDE_DIRS}
    "${CMAKE_CURRENT_SOURCE_DIR}/../../src/RiveQtQuickItem/"
)

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})

if (${QT_VERSION_MAJOR} EQUAL 5)
    target_link_libraries(${PROJECT_NAME}
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Test

        Qt${QT_VERSION_MAJOR}::CorePrivate
        Qt${QT_VERSION_MAJOR}::Quick
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::GuiPrivate
        Qt${QT_VERSION_MAJOR}::OpenGL
        Qt${QT_VERSION_MAJOR}::Qml
        Qt${QT_VERSION_MAJOR}::QuickPrivate
        rive_cpp

        RiveQtQuickPlugin
    )
elseif(${QT_VERSION_MAJOR} EQUAL 6)
    target_link_libraries(${PROJECT_NAME}
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Test
        RiveQtQuickPlugin_TEST
    )
endif()
