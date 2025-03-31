cmake_minimum_required(VERSION 3.15)
project(pk4_projekt)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 COMPONENTS Widgets Network Concurrent REQUIRED HINTS "C:/.vcpkg-clion/pk4/packages/qt5-base_x64-windows")

add_executable(${PROJECT_NAME} main.cpp
        resources.qrc
        wavelength/ui/navigation/navbar.h
        wavelength/ui/navigation/navbar.cpp
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
        wavelength/messages/handler/message_handler.cpp
        wavelength/messages/handler/message_handler.h
        wavelength/auth/authentication_manager.cpp
        wavelength/auth/authentication_manager.h
        wavelength/registry/wavelength_registry.cpp
        wavelength/registry/wavelength_registry.h
        wavelength/network/network_utilities.cpp
        wavelength/network/network_utilities.h
        wavelength/session/events/creator/wavelength_creator.cpp
        wavelength/session/events/creator/wavelength_creator.h
        wavelength/session/events/joiner/wavelength_joiner.cpp
        wavelength/session/events/joiner/wavelength_joiner.h
        wavelength/session/events/leaver/wavelength_leaver.cpp
        wavelength/session/events/leaver/wavelength_leaver.h
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
        wavelength/messages/formatter/message_formatter.cpp
        wavelength/messages/formatter/message_formatter.h
        blob/core/blob_absorption.cpp
        blob/core/blob_absorption.h
        blob/core/blob_transition_manager.cpp
        blob/core/blob_transition_manager.h
        blob/core/blob_event_handler.cpp
        blob/core/blob_event_handler.h
        font_manager.cpp
        font_manager.h
        wavelength/messages/wavelength_text_display.cpp
        wavelength/messages/wavelength_text_display.h
        wavelength/files/video/decoder/video_decoder.cpp
        wavelength/files/video/decoder/video_decoder.h
        wavelength/files/video/player/inline_video_player.cpp
        wavelength/files/video/player/inline_video_player.h
        wavelength/files/audio/decoder/audio_decoder.cpp
        wavelength/files/audio/decoder/audio_decoder.h
        wavelength/files/audio/player/inline_audio_player.cpp
        wavelength/files/audio/player/inline_audio_player.h
        wavelength/files/gif/decoder/gif_decoder.cpp
        wavelength/files/gif/decoder/gif_decoder.h
        wavelength/files/gif/player/inline_gif_player.cpp
        wavelength/files/gif/player/inline_gif_player.h
        wavelength/files/image/decoder/image_decoder.cpp
        wavelength/files/image/decoder/image_decoder.h
        wavelength/files/image/displayer/image_viewer.cpp
        wavelength/files/image/displayer/image_viewer.h
        wavelength/files/attachment_placeholder.cpp
        wavelength/files/attachment_placeholder.h
        wavelength/files/video/player/video_player_overlay.cpp
        wavelength/files/video/player/video_player_overlay.h
        wavelength/files/attachment_queue_manager.h
        wavelength/files/attachment_queue_manager.cpp
        wavelength/util/wavelength_utilities.cpp
        wavelength/util/wavelength_utilities.h
        wavelength/ui/widgets/animated_stacked_widget.cpp
        wavelength/ui/widgets/animated_stacked_widget.h
        wavelength/ui/dialogs/animated_dialog.cpp
        wavelength/ui/dialogs/animated_dialog.h
        wavelength/ui/messages/message_bubble.cpp
        wavelength/ui/messages/message_bubble.h
        wavelength/ui/chat/chat_style.cpp
        wavelength/ui/chat/chat_style.h
        wavelength/ui/messages/radio_message.cpp
        wavelength/ui/messages/radio_message.h
        wavelength/messages/wavelength_radio_display.cpp
        wavelength/messages/wavelength_radio_display.h
        wavelength/ui/messages/system_radio_message.cpp
        wavelength/ui/messages/system_radio_message.h
        wavelength/ui/chat/communication_stream.cpp
        wavelength/ui/chat/communication_stream.h
        wavelength/messages/wavelength_stream_display.cpp
        wavelength/messages/wavelength_stream_display.h
        wavelength/ui/chat/effects/disintegration_effect.cpp
        wavelength/ui/chat/effects/disintegration_effect.h
        wavelength/ui/chat/stream_message.cpp
        wavelength/ui/chat/stream_message.h
        wavelength/ui/chat/effects/opengl_disintegration_effect.cpp
        wavelength/ui/chat/effects/opengl_disintegration_effect.h
        wavelength/ui/cyber_attachment_viewer.cpp
        wavelength/ui/cyber_attachment_viewer.h
        wavelength/ui/chat/cyber_text_display.cpp
        wavelength/ui/chat/cyber_text_display.h
        wavelength/files/auto_scaling_attachment.cpp
        wavelength/files/auto_scaling_attachment.h
        wavelength/ui/chat/effects/electronic_shutdown_effect.cpp
        wavelength/ui/chat/effects/electronic_shutdown_effect.h
        wavelength/ui/chat/cyber_long_text_display.cpp
        wavelength/ui/chat/cyber_long_text_display.h
        wavelength/ui/cyberpunk_style.cpp
        wavelength/ui/cyberpunk_style.h
        wavelength/view/main_window/cyberpunk_text_effect.cpp
        wavelength/view/main_window/cyberpunk_text_effect.h
)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets Qt5::Network Qt5::Concurrent)

find_package(Qt5 COMPONENTS WebSockets REQUIRED HINTS "C:/.vcpkg-clion/pk4/packages/qt5-websockets_x64-windows")
target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::WebSockets)

find_package(Qt5 COMPONENTS Multimedia MultimediaWidgets REQUIRED HINTS "C:/.vcpkg-clion/pk4/packages/qt5-multimedia_x64-windows")
target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Multimedia Qt5::MultimediaWidgets)

find_package(libpqxx REQUIRED HINTS "C:/.vcpkg-clion/pk4/packages/libpqxx_x64-windows")
target_link_libraries(${PROJECT_NAME} PRIVATE libpqxx::pqxx)

find_package(mongocxx REQUIRED HINTS "C:/.vcpkg-clion/pk4/packages/mongo-cxx-driver_x64-windows")
find_package(bsoncxx REQUIRED HINTS "C:/.vcpkg-clion/pk4/packages/mongo-cxx-driver_x64-windows")

find_package(bsoncxx CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE $<IF:$<TARGET_EXISTS:mongo::bsoncxx_static>,mongo::bsoncxx_static,mongo::bsoncxx_shared>)

find_package(mongocxx CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE $<IF:$<TARGET_EXISTS:mongo::mongocxx_static>,mongo::mongocxx_static,mongo::mongocxx_shared>)

list(APPEND CMAKE_PREFIX_PATH "C:/.vcpkg-clion/pk4/packages/ffmpeg_x64-windows")

# Bezpośrednie linkowanie bibliotek FFmpeg
find_library(AVCODEC_LIBRARY avcodec HINTS "C:/.vcpkg-clion/pk4/packages/ffmpeg_x64-windows/lib")
find_library(AVFORMAT_LIBRARY avformat HINTS "C:/.vcpkg-clion/pk4/packages/ffmpeg_x64-windows/lib")
find_library(AVUTIL_LIBRARY avutil HINTS "C:/.vcpkg-clion/pk4/packages/ffmpeg_x64-windows/lib")
find_library(SWSCALE_LIBRARY swscale HINTS "C:/.vcpkg-clion/pk4/packages/ffmpeg_x64-windows/lib")
find_library(SWRESAMPLE_LIBRARY swresample HINTS "C:/.vcpkg-clion/pk4/packages/ffmpeg_x64-windows/lib")

# Dodaj ścieżkę do plików nagłówkowych
find_path(FFMPEG_INCLUDE_DIR libavcodec/avcodec.h HINTS "C:/.vcpkg-clion/pk4/packages/ffmpeg_x64-windows/include")

# Wyświetl znalezione ścieżki
message(STATUS "FFmpeg libraries:")
message(STATUS "  avcodec: ${AVCODEC_LIBRARY}")
message(STATUS "  avformat: ${AVFORMAT_LIBRARY}")
message(STATUS "  avutil: ${AVUTIL_LIBRARY}")
message(STATUS "  swscale: ${SWSCALE_LIBRARY}")
message(STATUS "  include: ${FFMPEG_INCLUDE_DIR}")
message(STATUS "  swresample: ${SWRESAMPLE_LIBRARY}")

# Podlinkuj biblioteki
target_link_libraries(${PROJECT_NAME} PRIVATE
        ${AVCODEC_LIBRARY}
        ${AVFORMAT_LIBRARY}
        ${AVUTIL_LIBRARY}
        ${SWSCALE_LIBRARY}
        ${SWRESAMPLE_LIBRARY}
)

# Dodaj ścieżki do plików nagłówkowych
target_include_directories(${PROJECT_NAME} PRIVATE ${FFMPEG_INCLUDE_DIR})