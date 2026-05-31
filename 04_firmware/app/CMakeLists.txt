# User-owned application modules.
# Folder is kept outside CubeMX ownership so regeneration does not overwrite
# production drivers, protocol parsers, platform helpers, control logic, or
# temporary test modules.

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/platform/dma_buffers.c

    ${CMAKE_CURRENT_LIST_DIR}/proto/proto_crsf.c

    ${CMAKE_CURRENT_LIST_DIR}/drivers/drv_crsf.c
    ${CMAKE_CURRENT_LIST_DIR}/drivers/drv_ssd1306.c

    ${CMAKE_CURRENT_LIST_DIR}/tasks/task_crsf.c
    ${CMAKE_CURRENT_LIST_DIR}/tasks/task_oled_ui.c

    ${CMAKE_CURRENT_LIST_DIR}/tests/test_crsf_oled_serial.c
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/platform
    ${CMAKE_CURRENT_LIST_DIR}/proto
    ${CMAKE_CURRENT_LIST_DIR}/drivers
    ${CMAKE_CURRENT_LIST_DIR}/tasks
    ${CMAKE_CURRENT_LIST_DIR}/tests
)