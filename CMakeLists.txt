cmake_minimum_required(VERSION 2.8)

project(dublicates)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5Widgets REQUIRED)

add_executable(dublicates scanner.h scanner.cpp main.cpp mainwindow.h mainwindow.cpp)
qt5_use_modules(dublicates Widgets)
