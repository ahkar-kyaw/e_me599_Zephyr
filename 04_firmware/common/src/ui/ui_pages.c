#include "ui/ui_pages.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define UI_RAD_TO_DEG 57.2957795f

#define UI_COLOR_BLACK       ((ui_color_t)0x0000u)
#define UI_COLOR_WHITE       ((ui_color_t)0xFFFFu)
#define UI_COLOR_MUTED       ((ui_color_t)0x7BEFu)
#define UI_COLOR_PANEL       ((ui_color_t)0x0821u)
#define UI_COLOR_PANEL_LIGHT ((ui_color_t)0x1082u)
#define UI_COLOR_CYAN        ((ui_color_t)0x07FFu)
#define UI_COLOR_GREEN       ((ui_color_t)0x07E0u)
#define UI_COLOR_AMBER       ((ui_color_t)0xFD20u)
#define UI_COLOR_RED         ((ui_color_t)0xF800u)
#define UI_COLOR_MAGENTA     ((ui_color_t)0xF81Fu)

#define UI_HEADER_HEIGHT 21u
#define UI_FOOTER_Y      114u
#define UI_FOOTER_HEIGHT 14u

static void ui_pages_draw_header(ui_canvas_t *canvas,
                                 const ui_state_t *state);
static void ui_pages_draw_footer(ui_canvas_t *canvas,
                                 const ui_state_t *state,
                                 const ui_page_model_t *model);
static void ui_pages_render_status(ui_canvas_t *canvas,
                                   const ui_page_model_t *model);
static void ui_pages_render_rc(ui_canvas_t *canvas,
                               const ui_state_t *state,
                               const ui_page_model_t *model);
static void ui_pages_render_imu(ui_canvas_t *canvas,
                                const ui_state_t *state,
                                const ui_page_model_t *model);
static const char *ui_pages_rc_state(const safety_rc_status_t *status);
static const char *ui_pages_imu_state(const imu_snapshot_t *snapshot,
                                      const safety_imu_status_t *status);
static ui_color_t ui_pages_rc_color(const safety_rc_status_t *status);
static ui_color_t ui_pages_imu_color(
    const imu_snapshot_t *snapshot,
    const safety_imu_status_t *status);
static uint32_t ui_pages_age_ms(uint64_t age_us);
static void ui_pages_format_age(char *text,
                                size_t text_size,
                                uint64_t age_us,
                                bool available);
static void ui_pages_draw_right_text(ui_canvas_t *canvas,
                                     uint16_t right,
                                     uint16_t y,
                                     const char *text,
                                     ui_color_t color,
                                     uint8_t scale);

void ui_pages_render(ui_canvas_t *canvas,
                     const ui_state_t *state,
                     const ui_page_model_t *model)
{
    if ((canvas == NULL) || (state == NULL) || (model == NULL) ||
        (model->rc_snapshot == NULL) || (model->rc_status == NULL) ||
        (model->imu_snapshot == NULL) || (model->imu_status == NULL))
    {
        return;
    }

    ui_canvas_clear(canvas, UI_COLOR_BLACK);
    ui_pages_draw_header(canvas, state);

    switch (state->page)
    {
        case UI_PAGE_RC:
            ui_pages_render_rc(canvas, state, model);
            break;

        case UI_PAGE_IMU:
            ui_pages_render_imu(canvas, state, model);
            break;

        case UI_PAGE_STATUS:
        default:
            ui_pages_render_status(canvas, model);
            break;
    }

    ui_pages_draw_footer(canvas, state, model);
}

static void ui_pages_draw_header(ui_canvas_t *canvas,
                                 const ui_state_t *state)
{
    static const char *const page_name[UI_PAGE_COUNT] =
    {
        "STATUS",
        "CRSF",
        "IMU",
    };
    static const ui_color_t page_color[UI_PAGE_COUNT] =
    {
        UI_COLOR_CYAN,
        UI_COLOR_GREEN,
        UI_COLOR_MAGENTA,
    };
    const char *mode;
    ui_color_t mode_color;

    if (!state->input_enabled)
    {
        mode = "LOCK";
        mode_color = UI_COLOR_AMBER;
    }
    else if (state->mode == UI_MODE_INTERACT)
    {
        mode = "INTERACT";
        mode_color = UI_COLOR_GREEN;
    }
    else
    {
        mode = "BROWSE";
        mode_color = UI_COLOR_CYAN;
    }

    ui_canvas_fill(canvas,
                   0u,
                   0u,
                   canvas->width,
                   UI_HEADER_HEIGHT - 1u,
                   UI_COLOR_PANEL);
    ui_canvas_fill(canvas,
                   0u,
                   UI_HEADER_HEIGHT - 1u,
                   canvas->width,
                   1u,
                   page_color[state->page]);
    ui_canvas_draw_text(canvas,
                        4u,
                        3u,
                        page_name[state->page],
                        page_color[state->page],
                        2u);
    ui_pages_draw_right_text(canvas,
                             (uint16_t)(canvas->width - 4u),
                             7u,
                             mode,
                             mode_color,
                             1u);
}

static void ui_pages_draw_footer(ui_canvas_t *canvas,
                                 const ui_state_t *state,
                                 const ui_page_model_t *model)
{
    char line[16];

    ui_canvas_fill(canvas,
                   0u,
                   UI_FOOTER_Y,
                   canvas->width,
                   UI_FOOTER_HEIGHT,
                   UI_COLOR_PANEL);

    ui_canvas_draw_text(canvas,
                        4u,
                        UI_FOOTER_Y + 4u,
                        "RC",
                        UI_COLOR_MUTED,
                        1u);
    ui_canvas_fill(canvas,
                   18u,
                   UI_FOOTER_Y + 5u,
                   5u,
                   5u,
                   ui_pages_rc_color(model->rc_status));
    ui_canvas_draw_text(canvas,
                        30u,
                        UI_FOOTER_Y + 4u,
                        "IMU",
                        UI_COLOR_MUTED,
                        1u);
    ui_canvas_fill(canvas,
                   50u,
                   UI_FOOTER_Y + 5u,
                   5u,
                   5u,
                   ui_pages_imu_color(model->imu_snapshot,
                                      model->imu_status));

    if ((state->mode == UI_MODE_INTERACT) &&
        (ui_state_selection_count(state->page) > 1u))
    {
        (void)snprintf(line,
                       sizeof(line),
                       "%u/%u",
                       (unsigned int)(state->selection + 1u),
                       (unsigned int)ui_state_selection_count(
                           state->page));
        ui_pages_draw_right_text(canvas,
                                 92u,
                                 UI_FOOTER_Y + 4u,
                                 line,
                                 UI_COLOR_GREEN,
                                 1u);
    }

    (void)snprintf(line,
                   sizeof(line),
                   "%u/%u",
                   (unsigned int)(state->page + 1u),
                   (unsigned int)UI_PAGE_COUNT);
    ui_pages_draw_right_text(canvas,
                             (uint16_t)(canvas->width - 4u),
                             UI_FOOTER_Y + 4u,
                             line,
                             UI_COLOR_WHITE,
                             1u);
}

static void ui_pages_render_status(ui_canvas_t *canvas,
                                   const ui_page_model_t *model)
{
    char line[24];

    ui_canvas_fill(canvas, 4u, 28u, 120u, 20u, UI_COLOR_PANEL_LIGHT);
    ui_canvas_draw_text(canvas, 9u, 34u, "RC", UI_COLOR_MUTED, 1u);
    ui_canvas_draw_text(canvas,
                        31u,
                        34u,
                        ui_pages_rc_state(model->rc_status),
                        ui_pages_rc_color(model->rc_status),
                        1u);
    ui_pages_format_age(
        line,
        sizeof(line),
        model->rc_status->age_us,
        (model->rc_status->fault_flags &
         SAFETY_RC_FAULT_NO_DATA) == 0u);
    ui_pages_draw_right_text(canvas,
                             119u,
                             34u,
                             line,
                             UI_COLOR_MUTED,
                             1u);

    ui_canvas_fill(canvas, 4u, 53u, 120u, 20u, UI_COLOR_PANEL_LIGHT);
    ui_canvas_draw_text(canvas, 9u, 59u, "IMU", UI_COLOR_MUTED, 1u);
    ui_canvas_draw_text(canvas,
                        37u,
                        59u,
                        ui_pages_imu_state(model->imu_snapshot,
                                           model->imu_status),
                        ui_pages_imu_color(model->imu_snapshot,
                                           model->imu_status),
                        1u);
    ui_pages_format_age(line,
                        sizeof(line),
                        model->imu_status->age_us,
                        model->imu_snapshot->valid);
    ui_pages_draw_right_text(canvas,
                             119u,
                             59u,
                             line,
                             UI_COLOR_MUTED,
                             1u);

    (void)snprintf(line,
                   sizeof(line),
                   "R %+5.1f",
                   (double)(model->imu_snapshot->attitude.roll_rad *
                            UI_RAD_TO_DEG));
    ui_canvas_draw_text(canvas, 7u, 79u, line, UI_COLOR_WHITE, 2u);

    (void)snprintf(line,
                   sizeof(line),
                   "P %+5.1f",
                   (double)(model->imu_snapshot->attitude.pitch_rad *
                            UI_RAD_TO_DEG));
    ui_canvas_draw_text(canvas, 7u, 97u, line, UI_COLOR_WHITE, 2u);
}

static void ui_pages_render_rc(ui_canvas_t *canvas,
                               const ui_state_t *state,
                               const ui_page_model_t *model)
{
    char line[24];
    const uint32_t first_channel =
        (uint32_t)state->selection * 8u;

    ui_canvas_draw_text(canvas,
                        4u,
                        27u,
                        "LINK",
                        UI_COLOR_MUTED,
                        1u);
    ui_canvas_draw_text(canvas,
                        34u,
                        27u,
                        ui_pages_rc_state(model->rc_status),
                        ui_pages_rc_color(model->rc_status),
                        1u);
    ui_pages_format_age(
        line,
        sizeof(line),
        model->rc_status->age_us,
        (model->rc_status->fault_flags &
         SAFETY_RC_FAULT_NO_DATA) == 0u);
    ui_pages_draw_right_text(canvas,
                             124u,
                             27u,
                             line,
                             UI_COLOR_MUTED,
                             1u);

    for (uint32_t row = 0u; row < 4u; row++)
    {
        const uint32_t left_channel = first_channel + row;
        const uint32_t right_channel = first_channel + row + 4u;
        const uint16_t y = (uint16_t)(44u + (row * 13u));

        (void)snprintf(line,
                       sizeof(line),
                       "%02lu %04u",
                       (unsigned long)(left_channel + 1u),
                       (unsigned int)model->rc_snapshot->channel[
                           left_channel]);
        ui_canvas_draw_text(canvas, 4u, y, line, UI_COLOR_WHITE, 1u);

        (void)snprintf(line,
                       sizeof(line),
                       "%02lu %04u",
                       (unsigned long)(right_channel + 1u),
                       (unsigned int)model->rc_snapshot->channel[
                           right_channel]);
        ui_canvas_draw_text(canvas, 70u, y, line, UI_COLOR_WHITE, 1u);
    }

    (void)snprintf(line,
                   sizeof(line),
                   "CRC %lu  PARSE %lu",
                   (unsigned long)model->rc_snapshot->crc_error_count,
                   (unsigned long)model->rc_snapshot->parse_error_count);
    ui_canvas_draw_text(canvas, 4u, 101u, line, UI_COLOR_MUTED, 1u);
}

static void ui_pages_render_imu(ui_canvas_t *canvas,
                                const ui_state_t *state,
                                const ui_page_model_t *model)
{
    char line[24];

    if (state->selection == 0u)
    {
        (void)snprintf(line,
                       sizeof(line),
                       "R %+5.1f",
                       (double)(model->imu_snapshot->attitude.roll_rad *
                                UI_RAD_TO_DEG));
        ui_canvas_draw_text(canvas, 7u, 28u, line, UI_COLOR_WHITE, 2u);

        (void)snprintf(line,
                       sizeof(line),
                       "P %+5.1f",
                       (double)(model->imu_snapshot->attitude.pitch_rad *
                                UI_RAD_TO_DEG));
        ui_canvas_draw_text(canvas, 7u, 50u, line, UI_COLOR_WHITE, 2u);

        (void)snprintf(line,
                       sizeof(line),
                       "Y %+5.1f",
                       (double)(model->imu_snapshot->attitude.yaw_rad *
                                UI_RAD_TO_DEG));
        ui_canvas_draw_text(canvas, 7u, 72u, line, UI_COLOR_WHITE, 2u);

        (void)snprintf(line,
                       sizeof(line),
                       "RATE %+6.3f  CAL %s",
                       (double)model->imu_snapshot->attitude.pitch_rate_rps,
                       model->imu_snapshot->calibrated ? "YES" : "NO");
        ui_canvas_draw_text(canvas, 4u, 101u, line, UI_COLOR_MUTED, 1u);
    }
    else
    {
        ui_canvas_draw_text(canvas,
                            4u,
                            28u,
                            "ACCEL",
                            UI_COLOR_CYAN,
                            1u);
        ui_canvas_draw_text(canvas,
                            70u,
                            28u,
                            "GYRO",
                            UI_COLOR_MAGENTA,
                            1u);

        for (uint32_t axis = 0u; axis < 3u; axis++)
        {
            static const char axis_name[] = {'X', 'Y', 'Z'};
            const uint16_t y = (uint16_t)(43u + (axis * 15u));

            (void)snprintf(line,
                           sizeof(line),
                           "A%c %+5.2f",
                           axis_name[axis],
                           (double)model->imu_snapshot->data.accel_mps2[
                               axis]);
            ui_canvas_draw_text(canvas, 4u, y, line, UI_COLOR_WHITE, 1u);

            (void)snprintf(line,
                           sizeof(line),
                           "G%c %+5.2f",
                           axis_name[axis],
                           (double)model->imu_snapshot->data.gyro_rps[
                               axis]);
            ui_canvas_draw_text(canvas, 70u, y, line, UI_COLOR_WHITE, 1u);
        }

        (void)snprintf(line,
                       sizeof(line),
                       "TEMP %+4.1fC",
                       (double)model->imu_snapshot->data.temp_c);
        ui_canvas_draw_text(canvas, 4u, 92u, line, UI_COLOR_MUTED, 1u);

        (void)snprintf(line,
                       sizeof(line),
                       "NORM %5.2f",
                       (double)model->imu_status->accel_norm_mps2);
        ui_canvas_draw_text(canvas, 4u, 103u, line, UI_COLOR_MUTED, 1u);
    }
}

static const char *ui_pages_rc_state(const safety_rc_status_t *status)
{
    if ((status->fault_flags & SAFETY_RC_FAULT_NO_DATA) != 0u)
    {
        return "WAIT";
    }

    if ((status->fault_flags & SAFETY_RC_FAULT_STALE) != 0u)
    {
        return "STALE";
    }

    if ((status->fault_flags & SAFETY_RC_FAULT_CHANNEL_RANGE) != 0u)
    {
        return "RANGE";
    }

    return "OK";
}

static const char *ui_pages_imu_state(const imu_snapshot_t *snapshot,
                                      const safety_imu_status_t *status)
{
    if (!snapshot->valid)
    {
        return snapshot->calibrated ? "WAIT" : "CAL";
    }

    if ((status->fault_flags & SAFETY_IMU_FAULT_STALE) != 0u)
    {
        return "STALE";
    }

    if (status->fault_flags != SAFETY_IMU_FAULT_NONE)
    {
        return "FAULT";
    }

    return "OK";
}

static ui_color_t ui_pages_rc_color(const safety_rc_status_t *status)
{
    if (status->fault_flags == SAFETY_RC_FAULT_NONE)
    {
        return UI_COLOR_GREEN;
    }

    if ((status->fault_flags & SAFETY_RC_FAULT_NO_DATA) != 0u)
    {
        return UI_COLOR_AMBER;
    }

    return UI_COLOR_RED;
}

static ui_color_t ui_pages_imu_color(
    const imu_snapshot_t *snapshot,
    const safety_imu_status_t *status)
{
    if (status->fault_flags == SAFETY_IMU_FAULT_NONE)
    {
        return snapshot->valid ? UI_COLOR_GREEN : UI_COLOR_AMBER;
    }

    return UI_COLOR_RED;
}

static uint32_t ui_pages_age_ms(uint64_t age_us)
{
    const uint64_t age_ms = age_us / 1000u;

    return (age_ms > UINT32_MAX) ? UINT32_MAX : (uint32_t)age_ms;
}

static void ui_pages_format_age(char *text,
                                size_t text_size,
                                uint64_t age_us,
                                bool available)
{
    if ((text == NULL) || (text_size == 0u))
    {
        return;
    }

    if (!available)
    {
        (void)snprintf(text, text_size, "--");
        return;
    }

    (void)snprintf(text,
                   text_size,
                   "%luMS",
                   (unsigned long)ui_pages_age_ms(age_us));
}

static void ui_pages_draw_right_text(ui_canvas_t *canvas,
                                     uint16_t right,
                                     uint16_t y,
                                     const char *text,
                                     ui_color_t color,
                                     uint8_t scale)
{
    const uint16_t width = ui_canvas_text_width(text, scale);
    const uint16_t x = (width < right) ? (uint16_t)(right - width) : 0u;

    ui_canvas_draw_text(canvas, x, y, text, color, scale);
}
