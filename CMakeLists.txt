cmake_minimum_required(VERSION 3.15)
project(pk4_projekt)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 COMPONENTS Widgets Network Concurrent REQUIRED)

add_executable(
        ${PROJECT_NAME}
        #        WIN32
        main.cpp
        resources.qrc
        src/ui/navigation/navbar.h
        src/ui/navigation/navbar.cpp
        src/blob/core/blob_animation.cpp
        src/blob/core/blob_animation.h
        src/blob/blob_config.h
        src/blob/physics/blob_physics.cpp
        src/blob/physics/blob_physics.h
        src/blob/rendering/blob_render.cpp
        src/blob/rendering/blob_render.h
        src/blob/states/blob_state.h
        src/blob/states/idle_state.cpp
        src/blob/states/idle_state.h
        src/blob/states/moving_state.cpp
        src/blob/states/moving_state.h
        src/blob/states/resizing_state.cpp
        src/blob/states/resizing_state.h
        src/blob/utils/blob_math.cpp
        src/blob/utils/blob_math.h
        src/blob/utils/blob_path.cpp
        src/blob/utils/blob_path.h
        src/app/managers/app_instance_manager.cpp
        src/app/managers/app_instance_manager.h
        src/ui/dialogs/wavelength_dialog.cpp
        src/ui/dialogs/wavelength_dialog.h
        src/ui/views/wavelength_chat_view.cpp
        src/ui/views/wavelength_chat_view.h
        src/ui/dialogs/join_wavelength_dialog.cpp
        src/ui/dialogs/join_wavelength_dialog.h
        src/storage/database_manager.cpp
        src/storage/database_manager.h
        src/chat/messages/handler/message_handler.cpp
        src/chat/messages/handler/message_handler.h
        src/auth/authentication_manager.cpp
        src/auth/authentication_manager.h
        src/storage/wavelength_registry.cpp
        src/storage/wavelength_registry.h
        src/session/events/creator/wavelength_creator.cpp
        src/session/events/creator/wavelength_creator.h
        src/session/events/joiner/wavelength_joiner.cpp
        src/session/events/joiner/wavelength_joiner.h
        src/session/events/leaver/wavelength_leaver.cpp
        src/session/events/leaver/wavelength_leaver.h
        src/chat/messages/services/wavelength_message_service.cpp
        src/chat/messages/services/wavelength_message_service.h
        src/chat/messages/services/wavelength_message_processor.cpp
        src/chat/messages/services/wavelength_message_processor.h
        src/services/wavelength_state_manager.cpp
        src/services/wavelength_state_manager.h
        src/services/wavelength_event_broker.cpp
        src/services/wavelength_event_broker.h
        src/app/wavelength_config.cpp
        src/app/wavelength_config.h
        src/session/wavelength_session_coordinator.cpp
        src/session/wavelength_session_coordinator.h
        src/chat/messages/formatter/message_formatter.cpp
        src/chat/messages/formatter/message_formatter.h
        src/blob/core/dynamics/blob_transition_manager.cpp
        src/blob/core/dynamics/blob_transition_manager.h
        src/blob/core/dynamics/blob_event_handler.cpp
        src/blob/core/dynamics/blob_event_handler.h
        src/chat/files/video/decoder/video_decoder.cpp
        src/chat/files/video/decoder/video_decoder.h
        src/chat/files/audio/decoder/audio_decoder.cpp
        src/chat/files/audio/decoder/audio_decoder.h
        src/chat/files/audio/player/inline_audio_player.cpp
        src/chat/files/audio/player/inline_audio_player.h
        src/chat/files/gif/decoder/gif_decoder.cpp
        src/chat/files/gif/decoder/gif_decoder.h
        src/chat/files/gif/player/inline_gif_player.cpp
        src/chat/files/gif/player/inline_gif_player.h
        src/chat/files/image/decoder/image_decoder.cpp
        src/chat/files/image/decoder/image_decoder.h
        src/chat/files/image/displayer/image_viewer.cpp
        src/chat/files/image/displayer/image_viewer.h
        src/chat/files/attachments/attachment_placeholder.cpp
        src/chat/files/attachments/attachment_placeholder.h
        src/chat/files/video/player/video_player.cpp
        src/chat/files/video/player/video_player.h
        src/chat/files/attachments/attachment_queue_manager.h
        src/chat/files/attachments/attachment_queue_manager.cpp
        src/util/wavelength_utilities.cpp
        src/util/wavelength_utilities.h
        src/ui/widgets/animated_stacked_widget.cpp
        src/ui/widgets/animated_stacked_widget.h
        src/ui/dialogs/animated_dialog.cpp
        src/ui/dialogs/animated_dialog.h
        src/ui/chat/style/chat_style.cpp
        src/ui/chat/style/chat_style.h
        src/ui/chat/communication_stream.cpp
        src/ui/chat/communication_stream.h
        src/ui/chat/wavelength_stream_display.cpp
        src/ui/chat/wavelength_stream_display.h
        src/ui/chat/effects/disintegration_effect.cpp
        src/ui/chat/effects/disintegration_effect.h
        src/ui/chat/stream_message.cpp
        src/ui/chat/stream_message.h
        src/ui/chat/effects/opengl_disintegration_effect.cpp
        src/ui/chat/effects/opengl_disintegration_effect.h
        src/ui/files/cyber_attachment_viewer.cpp
        src/ui/files/cyber_attachment_viewer.h
        src/ui/chat/effects/cyber_text_display.cpp
        src/ui/chat/effects/cyber_text_display.h
        src/chat/files/attachments/auto_scaling_attachment.cpp
        src/chat/files/attachments/auto_scaling_attachment.h
        src/ui/chat/effects/electronic_shutdown_effect.cpp
        src/ui/chat/effects/electronic_shutdown_effect.h
        src/ui/chat/effects/cyber_long_text_display.cpp
        src/ui/chat/effects/cyber_long_text_display.h
        src/app/style/cyberpunk_style.cpp
        src/app/style/cyberpunk_style.h
        src/ui/effects/cyberpunk_text_effect.cpp
        src/ui/effects/cyberpunk_text_effect.h
        src/ui/buttons/navbar_button.cpp
        src/ui/buttons/navbar_button.h
        src/blob/rendering/path_markers_manager.cpp
        src/blob/rendering/path_markers_manager.h
        src/ui/navigation/network_status_widget.cpp
        src/ui/navigation/network_status_widget.h
        src/app/managers/font_manager.cpp
        src/app/managers/font_manager.h
        src/ui/checkbox/cyber_checkbox.cpp
        src/ui/checkbox/cyber_checkbox.h
        src/ui/buttons/cyber_button.cpp
        src/ui/buttons/cyber_button.h
        src/ui/input/cyber_line_edit.cpp
        src/ui/input/cyber_line_edit.h
        src/ui/views/settings_view.cpp
        src/ui/views/settings_view.h
        src/ui/widgets/gl_transition_widget.cpp
        src/ui/widgets/gl_transition_widget.h
        src/ui/settings/tabs/classified/layers/security_layer.h
        src/ui/settings/tabs/classified/layers/fingerprint/fingerprint_layer.cpp
        src/ui/settings/tabs/classified/layers/fingerprint/fingerprint_layer.h
        src/ui/settings/tabs/classified/layers/handprint/handprint_layer.cpp
        src/ui/settings/tabs/classified/layers/handprint/handprint_layer.h
        src/ui/settings/tabs/classified/layers/code/security_code_layer.cpp
        src/ui/settings/tabs/classified/layers/code/security_code_layer.h
        src/ui/settings/tabs/classified/layers/question/security_question_layer.cpp
        src/ui/settings/tabs/classified/layers/question/security_question_layer.h
        src/ui/settings/tabs/classified/layers/retina_scan/retina_scan_layer.cpp
        src/ui/settings/tabs/classified/layers/retina_scan/retina_scan_layer.h
        src/ui/settings/tabs/classified/layers/voice_recognition/voice_recognition_layer.cpp
        src/ui/settings/tabs/classified/layers/voice_recognition/voice_recognition_layer.h
        src/ui/settings/tabs/classified/layers/typing_test/typing_test_layer.cpp
        src/ui/settings/tabs/classified/layers/typing_test/typing_test_layer.h
        src/ui/settings/tabs/classified/layers/snake_game/snake_game_layer.cpp
        src/ui/settings/tabs/classified/layers/snake_game/snake_game_layer.h
        src/ui/settings/tabs/classified/components/floating_energy_sphere_widget.cpp
        src/ui/settings/tabs/classified/components/floating_energy_sphere_widget.h
        src/ui/settings/tabs/classified/components/system_override_manager.cpp
        src/ui/settings/tabs/classified/components/system_override_manager.h
        src/ui/buttons/tab_button.cpp
        src/ui/buttons/tab_button.h
        src/ui/settings/tabs/appearance/appearance_settings_widget.cpp
        src/ui/settings/tabs/appearance/appearance_settings_widget.h
        src/ui/settings/tabs/appearance/components/clickable_color_preview.cpp
        src/ui/settings/tabs/appearance/components/clickable_color_preview.h
        src/ui/settings/tabs/wavelength/wavelength_settings_widget.cpp
        src/ui/settings/tabs/wavelength/wavelength_settings_widget.h
        src/ui/settings/tabs/network/network_settings_widget.cpp
        src/ui/settings/tabs/network/network_settings_widget.h
        src/ui/chat/effects/mask_overlay.cpp
        src/ui/chat/effects/mask_overlay.h
        src/ui/labels/user_info_label.cpp
        src/ui/labels/user_info_label.h
        src/app/managers/shortcut_manager.cpp
        src/app/managers/shortcut_manager.h
        src/ui/settings/tabs/shortcuts/shortcuts_settings_widget.cpp
        src/ui/settings/tabs/shortcuts/shortcuts_settings_widget.h
        src/ui/sliders/cyber_audio_slider.cpp
        src/ui/sliders/cyber_audio_slider.h
        src/ui/buttons/cyber_audio_button.cpp
        src/ui/buttons/cyber_audio_button.h
        src/ui/sliders/cyber_slider.cpp
        src/ui/sliders/cyber_slider.h
        src/ui/buttons/cyber_push_button.cpp
        src/ui/buttons/cyber_push_button.h
        src/chat/files/attachments/attachment_data_store.cpp
        src/chat/files/attachments/attachment_data_store.h
        src/ui/buttons/cyber_chat_button.cpp
        src/ui/buttons/cyber_chat_button.h
        src/ui/widgets/overlay_widget.cpp
        src/ui/widgets/overlay_widget.h
        src/util/resize_event_filter.cpp
        src/util/resize_event_filter.h
        src/app/managers/translation_manager.cpp
        src/app/managers/translation_manager.h
)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets Qt5::Network Qt5::Concurrent)

find_package(Qt5 COMPONENTS WebSockets REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::WebSockets)

find_package(Qt5 COMPONENTS Multimedia MultimediaWidgets REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Multimedia Qt5::MultimediaWidgets)

find_package(Qt5 COMPONENTS Svg REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Svg)

find_package(libpqxx REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE libpqxx::pqxx)

find_package(FFMPEG REQUIRED)
target_include_directories(${PROJECT_NAME} PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PRIVATE ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${FFMPEG_LIBRARIES})
