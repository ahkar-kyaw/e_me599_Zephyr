idf_component_register(
    SRCS
        "main.c"
        "board/board_led.c"
        "bsp/bsp_imu_spi_esp32.c"
        "tasks/task_imu.c"
        "tests/test_debug_led.c"
        "tests/test_imu_snapshot.c"
        "../../../common/src/drivers/drv_ism330dhcx.c"
        "../../../common/src/estimation/est_imu_calibration.c"
        "../../../common/src/estimation/est_attitude.c"
        "../../../common/src/safety/safety_imu.c"
        "../../../common/src/control/ctrl_balance_types.c"
    INCLUDE_DIRS
        "../include"
        "../../../common/include"
        "board"
        "bsp"
        "config"
        "tasks"
        "tests"
)

target_link_libraries(${COMPONENT_LIB} PRIVATE m)