cmake_minimum_required(VERSION 2.8.4)
project(yagb)

set(CMAKE_BUILD_TYPE DEBUG)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -O0 -gdwarf-2 -Wall -Wextra -Woverloaded-virtual -Werror -Wno-unused-parameter -Wno-unknown-pragmas")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_AUTOMOC ON)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Multimedia REQUIRED)

file(GLOB_RECURSE srcs ${CMAKE_CURRENT_SOURCE_DIR}/emu/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/gui/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/emu/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/gui/*.hpp)
qt5_wrap_ui(ui_headers ${CMAKE_CURRENT_SOURCE_DIR}/gui/MainWindow.ui)

add_executable(yagb ${srcs} ${ui_headers})
qt5_use_modules(yagb Widgets Multimedia)
