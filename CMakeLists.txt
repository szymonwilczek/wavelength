cmake_minimum_required(VERSION 3.15)
project(pk4_projekt)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets Network Concurrent REQUIRED HINTS "C:/Users/szymo/.vcpkg-clion/pk4/packages/qt5-base_x64-windows")

add_executable(${PROJECT_NAME} main.cpp
        navbar.h
        navbar.cpp
        blob/core/blob_animation.cpp
        blob/core/blob_animation.h
        blob/core/blob_config.h
        blob/physics/blob_physics.cpp
        blob/physics/blob_physics.h
        blob/rendering/blob_render.cpp
        blob/rendering/blob_render.h
        blob/states/blob_state.h
        blob/states/idle_state.cpp
        blob/states/idle_state.h
        blob/states/moving_state.cpp
        blob/states/moving_state.h
        blob/states/resizing_state.cpp
        blob/states/resizing_state.h
        blob/utils/blob_math.cpp
        blob/utils/blob_math.h
        blob/utils/blob_path.cpp
        blob/utils/blob_path.h
        blob/core/app_instance_manager.cpp
        blob/core/app_instance_manager.h
        wavelength/wavelength_dialog.cpp
        wavelength/wavelength_dialog.h
        wavelength/wavelength_manager.cpp
        wavelength/wavelength_manager.h
        wavelength/wavelength_chat_view.cpp
        wavelength/wavelength_chat_view.h
        wavelength/join_wavelength_dialog.cpp
        wavelength/join_wavelength_dialog.h
)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets Qt5::Network Qt5::Concurrent)

find_package(Qt5 COMPONENTS WebSockets REQUIRED HINTS "C:/Users/szymo/.vcpkg-clion/pk4/packages/qt5-websockets_x64-windows")
target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::WebSockets)

find_package(mongocxx REQUIRED HINTS "C:/Users/szymo/.vcpkg-clion/pk4/packages/mongo-cxx-driver_x64-windows")
find_package(bsoncxx REQUIRED HINTS "C:/Users/szymo/.vcpkg-clion/pk4/packages/mongo-cxx-driver_x64-windows")

find_package(bsoncxx CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE $<IF:$<TARGET_EXISTS:mongo::bsoncxx_static>,mongo::bsoncxx_static,mongo::bsoncxx_shared>)

find_package(mongocxx CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE $<IF:$<TARGET_EXISTS:mongo::mongocxx_static>,mongo::mongocxx_static,mongo::mongocxx_shared>)