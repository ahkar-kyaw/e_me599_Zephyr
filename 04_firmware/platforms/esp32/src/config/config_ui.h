#ifndef CONFIG_UI_H
#define CONFIG_UI_H

#define APP_OLED_I2C_ADDRESS       0x3Cu
#define APP_OLED_WIDTH             128u
#define APP_OLED_HEIGHT            64u
#define APP_OLED_CONTRAST          0x7Fu
#define APP_OLED_ROTATE_180        0
#define APP_OLED_INVERT            0

#define APP_UI_RC_AXIS_CHANNEL         1u
#define APP_UI_RC_INTERACT_CHANNEL     6u
#define APP_UI_RC_ENTER_CHANNEL        7u

#define APP_UI_RC_AXIS_LEFT_THRESHOLD     600u
#define APP_UI_RC_AXIS_NEUTRAL_LOW        850u
#define APP_UI_RC_AXIS_NEUTRAL_HIGH      1134u
#define APP_UI_RC_AXIS_RIGHT_THRESHOLD   1384u

#define APP_UI_RC_SWITCH_OFF_THRESHOLD    600u
#define APP_UI_RC_SWITCH_ON_THRESHOLD    1384u

#define APP_UI_RC_INTERACT_ACTIVE_HIGH 1
#define APP_UI_RC_ENTER_ACTIVE_HIGH    1

#define APP_UI_TASK_PERIOD_MS       50u
#define APP_UI_RENDER_PERIOD_MS    200u
#define APP_UI_RETRY_MS            1000u

#endif
