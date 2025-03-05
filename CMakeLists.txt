cmake_minimum_required(VERSION 3.15)
project(pk4_projekt)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED HINTS "C:/Users/szymo/.vcpkg-clion/pk4/packages/qt5-base_x64-windows")

add_executable(${PROJECT_NAME} main.cpp
        navbar.h
        navbar.cpp
        blobanimation.cpp
        blobanimation.h)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets)