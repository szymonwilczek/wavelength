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
        wavelength/dialogs/wavelength_dialog.cpp
        wavelength/dialogs/wavelength_dialog.h
        wavelength/view/wavelength_chat_view.cpp
        wavelength/view/wavelength_chat_view.h
        wavelength/dialogs/join_wavelength_dialog.cpp
        wavelength/dialogs/join_wavelength_dialog.h
        wavelength/database/database_manager.cpp
        wavelength/database/database_manager.h
        wavelength/messages/message_handler.cpp
        wavelength/messages/message_handler.h
        wavelength/auth/authentication_manager.cpp
        wavelength/auth/authentication_manager.h
        wavelength/registry/wavelength_registry.cpp
        wavelength/registry/wavelength_registry.h
        wavelength/network/network_utilities.cpp
        wavelength/network/network_utilities.h
        wavelength/session/wavelength_creator.cpp
        wavelength/session/wavelength_creator.h
        wavelength/session/wavelength_joiner.cpp
        wavelength/session/wavelength_joiner.h
        wavelength/session/wavelength_leaver.cpp
        wavelength/session/wavelength_leaver.h
        wavelength/socket/wavelength_socket_handler.cpp
        wavelength/socket/wavelength_socket_handler.h
        wavelength/messages/wavelength_message_service.cpp
        wavelength/messages/wavelength_message_service.h
        wavelength/messages/wavelength_message_processor.cpp
        wavelength/messages/wavelength_message_processor.h
        wavelength/managers/wavelength_state_manager.cpp
        wavelength/managers/wavelength_state_manager.h
        wavelength/managers/wavelength_event_broker.cpp
        wavelength/managers/wavelength_event_broker.h
        wavelength/util/wavelength_config.cpp
        wavelength/util/wavelength_config.h
        wavelength/session/wavelength_session_coordinator.cpp
        wavelength/session/wavelength_session_coordinator.h
        wavelength/messages/message_formatter.cpp
        wavelength/messages/message_formatter.h
        blob/core/blob_absorption.cpp
        blob/core/blob_absorption.h
        blob/core/blob_transition_manager.cpp
        blob/core/blob_transition_manager.h
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