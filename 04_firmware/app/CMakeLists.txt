# User-owned application modules.
# Folder is kept outside CubeMX ownership so regeneration does not overwrite
# production drivers, protocol parsers, platform helpers, control logic, or
# temporary test modules.

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/platform/dma_buffers.c

    ${CMAKE_CURRENT_LIST_DIR}/proto/proto_crsf.c

    ${CMAKE_CURRENT_LIST_DIR}/drivers/drv_crsf.c
    ${CMAKE_CURRENT_LIST_DIR}/drivers/drv_ssd1306.c
    ${CMAKE_CURRENT_LIST_DIR}/drivers/drv_ism330dhcx.c

    ${CMAKE_CURRENT_LIST_DIR}/tasks/task_log.c
    ${CMAKE_CURRENT_LIST_DIR}/tasks/task_crsf.c
    ${CMAKE_CURRENT_LIST_DIR}/tasks/task_oled_ui.c
    ${CMAKE_CURRENT_LIST_DIR}/tasks/task_imu.c
    ${CMAKE_CURRENT_LIST_DIR}/tasks/task_app_ui.c
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/platform
    ${CMAKE_CURRENT_LIST_DIR}/proto
    ${CMAKE_CURRENT_LIST_DIR}/drivers
    ${CMAKE_CURRENT_LIST_DIR}/tasks
)

target_link_libraries(${CMAKE_PROJECT_NAME} m)