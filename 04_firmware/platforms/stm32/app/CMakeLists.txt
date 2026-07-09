# User-owned application sources.
# This file is intentionally outside CubeMX generated code.

option(APP_ENABLE_BRINGUP_TESTS "Build temporary bring-up tests" ON)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/app_main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/board/board_led.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tasks/task_debug.c
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/board
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${CMAKE_CURRENT_SOURCE_DIR}/tasks
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../common/include
)

if(APP_ENABLE_BRINGUP_TESTS)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_debug_led.c
    )

    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
        APP_ENABLE_BRINGUP_TESTS=1
    )
else()
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
        APP_ENABLE_BRINGUP_TESTS=0
    )
endif()