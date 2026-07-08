# Firmware Framework Overview

## Project

Self-balancing robot with four collinear Mecanum wheels.

The firmware is organized for safe bring-up, modular development, hardware swapping, control testing, UI operation, and documentation.

## Main design idea

```text
UI requests actions
Supervisor changes robot mode
Control computes desired commands
Safety limits or blocks commands
Motor task sends the final allowed command
```

Tasks are named by subsystem responsibility.

Drivers are named by hardware.

Backends connect abstract interfaces to real hardware.

BSP files wrap STM32 peripherals.

Protocol files encode and decode messages.

Test files stay separate from production logic.

## Folder structure

```text
04_firmware/
  cube/
    Core/
    Drivers/
    Middlewares/

  app/
    config/
      config_robot.h
      config_control.h
      config_kinematics.h
      config_motor.h
      config_imu.h
      config_power.h
      config_safety.h
      config_ui.h
      config_logging.h
      config_tests.h

    platform/
      platform_time.c
      platform_assert.c
      platform_error.c
      platform_version.c

    bsp/
      bsp_gpio.c
      bsp_spi.c
      bsp_i2c.c
      bsp_uart.c
      bsp_fdcan.c
      bsp_adc.c
      bsp_timer.c
      bsp_dma.c
      bsp_flash.c
      bsp_watchdog.c

    interfaces/
      if_imu.h
      if_motor.h
      if_display.h
      if_power_monitor.h
      if_storage.h
      if_logger_backend.h

    drivers/
      drv_ism330dhcx.c
      drv_ak40_can.c
      drv_crsf_uart.c
      drv_oled_ssd1306.c
      drv_battery_adc.c
      drv_bq76930.c
      drv_tps25751a.c
      drv_lt4256.c
      drv_ltc3779.c
      drv_lt8640a.c
      drv_flash_storage.c

    backends/
      imu_backend_ism330dhcx_spi.c
      motor_backend_ak40_can.c
      display_backend_ssd1306_oled.c
      power_backend_board_v1.c
      log_backend_uart.c
      storage_backend_flash.c

    proto/
      proto_crsf.c
      proto_ak40_can.c
      proto_log_frame.c

    services/
      svc_mode_manager.c
      svc_health.c
      svc_events.c
      svc_params.c
      svc_calibration.c
      svc_power.c
      svc_faults.c
      svc_commands.c

    estimation/
      est_attitude.c
      est_imu_calibration.c
      est_odometry.c
      est_filters.c

    kinematics/
      kin_collinear_mecanum.c
      kin_wheel_signs.c

    control/
      ctrl_balance.c
      ctrl_velocity.c
      ctrl_lateral.c
      ctrl_yaw.c
      ctrl_motor_mixer.c
      ctrl_limits.c

    safety/
      safety_monitor.c
      safety_faults.c
      safety_limits.c
      safety_watchdog.c
      safety_precheck.c

    power/
      power_state_machine.c
      power_battery_model.c
      power_charger_manager.c
      power_path_manager.c

    ui/
      ui_model.c
      ui_input.c
      ui_pages.c
      ui_navigation.c
      ui_render.c
      ui_render_oled.c
      ui_page_home.c
      ui_page_arm.c
      ui_page_status.c
      ui_page_imu.c
      ui_page_crsf.c
      ui_page_motor.c
      ui_page_power.c
      ui_page_calibration.c
      ui_page_tuning.c
      ui_page_tests.c
      ui_page_logging.c
      ui_page_faults.c
      ui_page_about.c

    logging/
      log_manager.c
      log_ring_buffer.c
      log_binary.c
      log_csv.c
      log_fault_snapshot.c

    tasks/
      task_supervisor.c
      task_safety.c
      task_control.c
      task_motor.c
      task_imu.c
      task_crsf.c
      task_power.c
      task_ui.c
      task_log.c
      task_debug.c

    tests/
      app_test.c
      test_imu.c
      test_crsf.c
      test_motor.c
      test_power.c
      test_adc.c
      test_oled.c
      test_logging.c
      test_kinematics.c

    app_types/
      robot_state.h
      imu_types.h
      motor_types.h
      power_types.h
      control_types.h
      safety_types.h
      ui_types.h
      log_types.h
      fault_types.h
      parameter_types.h
```

## Runtime flow

```text
Boot hardware
Initialize app services
Create RTOS objects
Start RTOS tasks
Enter SYS_BOOT
Move to SYS_SAFE_IDLE after startup checks
Wait for user command
Run calibration, tests, tuning, or balance mode as requested
Log important data and faults
Return to safe idle or fault state when required
```

## Main tasks

```text
task_supervisor
  owns whole robot mode

task_safety
  owns permission to move

task_control
  computes balance, forward, lateral, and yaw commands

task_motor
  sends final allowed motor commands and reads motor feedback

task_imu
  reads IMU data and publishes attitude

task_crsf
  reads RC receiver data and creates user commands

task_power
  monitors battery, charger, power path, and regulators

task_ui
  handles OLED pages and user interaction

task_log
  records experiment data and faults

task_debug
  prints non-critical diagnostics
```

## Communication model

```text
Snapshots
  attitude_snapshot
  user_command_snapshot
  power_snapshot
  motor_feedback_snapshot
  control_output_snapshot
  safety_snapshot
  system_status_snapshot
  log_status_snapshot

Queues
  q_ui_events
  q_fault_events
  q_log_frames
  q_motor_rx_frames
  q_service_requests
```

## UI pages

```text
UI_PAGE_HOME
  quick robot status

UI_PAGE_ARM
  precheck, ready, arm, start balance, stop

UI_PAGE_STATUS
  task health and data freshness

UI_PAGE_IMU
  raw IMU and attitude data

UI_PAGE_CRSF
  RC channels and link state

UI_PAGE_MOTOR
  motor feedback and motor faults

UI_PAGE_POWER
  battery, charger, USB-C, power path, regulators

UI_PAGE_CALIBRATION
  IMU bias, motor direction, wheel signs

UI_PAGE_TUNING
  gains and limits

UI_PAGE_TESTS
  subsystem tests

UI_PAGE_LOGGING
  logging control and status

UI_PAGE_FAULTS
  active and latched faults

UI_PAGE_ABOUT
  firmware and hardware information
```

## System mode states

```text
SYS_BOOT
SYS_HARDWARE_INIT
SYS_SELF_TEST
SYS_SAFE_IDLE
SYS_CALIBRATION
SYS_TEST
SYS_READY
SYS_ARMED
SYS_BALANCING
SYS_CONTROLLED_STOP
SYS_FAULT
SYS_ESTOP
SYS_SHUTDOWN
```

## System mode pseudocode

```text
SYS_BOOT
  wait for RTOS and services
  go to SYS_HARDWARE_INIT

SYS_HARDWARE_INIT
  wait for required subsystems
  if ready go to SYS_SELF_TEST
  if timeout go to SYS_FAULT

SYS_SELF_TEST
  run non-moving checks
  if pass go to SYS_SAFE_IDLE
  if fail go to SYS_FAULT

SYS_SAFE_IDLE
  keep motor torque disabled
  allow UI, calibration, and safe tests
  if ready requested and prechecks pass go to SYS_READY

SYS_CALIBRATION
  run selected calibration
  save only after confirmation
  return to SYS_SAFE_IDLE

SYS_TEST
  run selected test
  return to SYS_SAFE_IDLE

SYS_READY
  keep torque disabled
  wait for valid arm request
  if armed go to SYS_ARMED

SYS_ARMED
  allow zero torque motor state
  wait for start balance request
  if safe go to SYS_BALANCING

SYS_BALANCING
  run balance and drive control
  if stop requested go to SYS_CONTROLLED_STOP
  if fault go to SYS_FAULT
  if kill switch go to SYS_ESTOP

SYS_CONTROLLED_STOP
  ramp commands to zero
  return to SYS_SAFE_IDLE

SYS_FAULT
  apply fault reaction
  log fault
  wait for safe recovery

SYS_ESTOP
  disable torque
  require reset or power cycle

SYS_SHUTDOWN
  disable torque
  flush logs
  save data if needed
```

## Subsystem state names

```text
SAFETY_SAFE_IDLE
SAFETY_PRECHECK
SAFETY_ARMED_NO_MOTION
SAFETY_MOTION_ALLOWED
SAFETY_LIMITED_OPERATION
SAFETY_CONTROLLED_STOP
SAFETY_FAULT_REACTION
SAFETY_ESTOP_LATCHED

CONTROL_DISABLED
CONTROL_ZERO_TORQUE
CONTROL_STANDSTILL_BALANCE
CONTROL_DRIVE_BALANCE
CONTROL_CONTROLLED_STOP
CONTROL_FAULT

MOTOR_INIT
MOTOR_DISABLED
MOTOR_PREARM
MOTOR_ZERO_TORQUE_ENABLED
MOTOR_COMMAND_ENABLED
MOTOR_CONTROLLED_STOP
MOTOR_FAULT

IMU_INIT
IMU_CONFIGURE
IMU_CALIBRATE_STARTUP
IMU_RUNNING
IMU_DEGRADED
IMU_FAULT

POWER_INIT
POWER_PACK_DETECTED
POWER_PATH_CHECK
POWER_ROBOT_POWER_READY
POWER_USB_C_ATTACHED
POWER_CHARGING
POWER_CHARGE_COMPLETE
POWER_LOW_BATTERY_LIMITED
POWER_CRITICAL_BATTERY_STOP
POWER_FAULT

UI_BOOT
UI_HOME
UI_MENU
UI_PAGE_ACTIVE
UI_EDIT_FIELD
UI_CONFIRM_ACTION
UI_MESSAGE
UI_FAULT_LOCK

LOG_IDLE
LOG_READY
LOG_RECORDING
LOG_FLUSHING
LOG_ERROR
```

## Task pseudocode

```text
task_supervisor
  read subsystem readiness
  read faults
  read user requests
  update system mode
  publish system status
  service watchdog if healthy
```

```text
task_safety
  read snapshots
  check freshness and limits
  classify faults
  update safety state
  publish safety-limited command
```

```text
task_control
  read attitude, command, motor feedback, and parameters
  update control state
  compute balance and drive command if allowed
  publish desired command
```

```text
task_motor
  read motor feedback
  read safety-limited command
  update motor state
  send allowed command or safe zero command
```

```text
task_imu
  read IMU sample
  validate sample
  apply calibration
  update attitude estimate
  publish IMU and attitude snapshots
```

```text
task_crsf
  read UART data
  parse CRSF frames
  publish RC command snapshot
  send UI events to queue
  detect link loss
```

```text
task_power
  read battery and power ICs
  update power state
  publish power snapshot
  raise or clear power faults
```

```text
task_ui
  read snapshots
  process UI events
  update current page
  render display
```

```text
task_log
  collect log frames
  write log data
  record faults and parameter changes
```

## Bring-up order

```text
1  Debug UART, timestamp timer, heartbeat LED
2  CRSF receiver and channel display
3  IMU readout and attitude estimate
4  ADC battery and current measurement
5  FDCAN motor feedback
6  zero torque and low torque motor jog
7  wheel direction and kinematic signs
8  balance on stand or tether
9  drive, lateral, and yaw tests
10 logging and thesis experiments
```

## Final summary

This framework separates the robot firmware into clear layers.

FreeRTOS tasks decide when code runs.

State machines decide what each subsystem is allowed to do.

Snapshots share latest state.

Queues carry events.

The supervisor owns whole robot mode.

The safety task owns permission to move.

The motor task is the only task that sends actuator commands.

The UI helps the user operate, test, calibrate, tune, and debug the robot without bypassing safety.
