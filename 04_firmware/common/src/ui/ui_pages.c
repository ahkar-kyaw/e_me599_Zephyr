#include "ui/ui_pages.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define UI_RAD_TO_DEG 57.2957795f

static void ui_pages_draw_header(drv_ssd1306_t *display,
                                 const ui_state_t *state);
static void ui_pages_draw_footer(drv_ssd1306_t *display,
                                 const ui_state_t *state);
static void ui_pages_render_status(drv_ssd1306_t *display,
                                   const ui_page_model_t *model);
static void ui_pages_render_rc(drv_ssd1306_t *display,
                               const ui_state_t *state,
                               const ui_page_model_t *model);
static void ui_pages_render_imu(drv_ssd1306_t *display,
                                const ui_state_t *state,
                                const ui_page_model_t *model);
static const char *ui_pages_rc_state(const safety_rc_status_t *status);
static const char *ui_pages_imu_state(const imu_snapshot_t *snapshot,
                                      const safety_imu_status_t *status);
static uint32_t ui_pages_age_ms(uint64_t age_us);

void ui_pages_render(drv_ssd1306_t *display,
                     const ui_state_t *state,
                     const ui_page_model_t *model)
{
    if ((display == NULL) || (state == NULL) || (model == NULL) ||
        (model->rc_snapshot == NULL) || (model->rc_status == NULL) ||
        (model->imu_snapshot == NULL) || (model->imu_status == NULL))
    {
        return;
    }

    drv_ssd1306_clear(display);
    ui_pages_draw_header(display, state);

    switch (state->page)
    {
        case UI_PAGE_RC:
            ui_pages_render_rc(display, state, model);
            break;

        case UI_PAGE_IMU:
            ui_pages_render_imu(display, state, model);
            break;

        case UI_PAGE_STATUS:
        default:
            ui_pages_render_status(display, model);
            break;
    }

    ui_pages_draw_footer(display, state);
}

static void ui_pages_draw_header(drv_ssd1306_t *display,
                                 const ui_state_t *state)
{
    static const char *const page_name[UI_PAGE_COUNT] =
    {
        "STATUS",
        "CRSF",
        "IMU",
    };
    char line[24];

    (void)snprintf(line,
                   sizeof(line),
                   "%s %c %u",
                   page_name[state->page],
                   (state->mode == UI_MODE_INTERACT) ? 'I' : 'B',
                   (unsigned int)(state->subpage + 1u));
    drv_ssd1306_draw_text(display, 0u, 0u, line);
}

static void ui_pages_draw_footer(drv_ssd1306_t *display,
                                 const ui_state_t *state)
{
    const char *text = (state->mode == UI_MODE_INTERACT)
        ? "AIL NAV SE ENTER"
        : "AIL PAGE SD INTERACT";

    drv_ssd1306_draw_text(display, 0u, 56u, text);
}

static void ui_pages_render_status(drv_ssd1306_t *display,
                                   const ui_page_model_t *model)
{
    char line[24];

    (void)snprintf(line,
                   sizeof(line),
                   "RC %s AGE %lu",
                   ui_pages_rc_state(model->rc_status),
                   (unsigned long)ui_pages_age_ms(
                       model->rc_status->age_us));
    drv_ssd1306_draw_text(display, 0u, 8u, line);

    (void)snprintf(line,
                   sizeof(line),
                   "IMU %s AGE %lu",
                   ui_pages_imu_state(model->imu_snapshot,
                                      model->imu_status),
                   (unsigned long)ui_pages_age_ms(
                       model->imu_status->age_us));
    drv_ssd1306_draw_text(display, 0u, 16u, line);

    (void)snprintf(line,
                   sizeof(line),
                   "ROLL %7.2f DEG",
                   (double)(model->imu_snapshot->attitude.roll_rad *
                            UI_RAD_TO_DEG));
    drv_ssd1306_draw_text(display, 0u, 24u, line);

    (void)snprintf(line,
                   sizeof(line),
                   "PITCH %6.2f DEG",
                   (double)(model->imu_snapshot->attitude.pitch_rad *
                            UI_RAD_TO_DEG));
    drv_ssd1306_draw_text(display, 0u, 32u, line);

    (void)snprintf(line,
                   sizeof(line),
                   "SAMPLES %lu ERR %lu",
                   (unsigned long)model->imu_snapshot->sample_count,
                   (unsigned long)model->imu_snapshot->read_error_count);
    drv_ssd1306_draw_text(display, 0u, 40u, line);

    (void)snprintf(line,
                   sizeof(line),
                   "RF %lX IF %lX",
                   (unsigned long)model->rc_status->fault_flags,
                   (unsigned long)model->imu_status->fault_flags);
    drv_ssd1306_draw_text(display, 0u, 48u, line);
}

static void ui_pages_render_rc(drv_ssd1306_t *display,
                               const ui_state_t *state,
                               const ui_page_model_t *model)
{
    char line[24];
    const uint32_t first_channel = (uint32_t)state->subpage * 4u;

    (void)snprintf(line,
                   sizeof(line),
                   "LINK %s AGE %lu",
                   ui_pages_rc_state(model->rc_status),
                   (unsigned long)ui_pages_age_ms(
                       model->rc_status->age_us));
    drv_ssd1306_draw_text(display, 0u, 8u, line);

    for (uint32_t row = 0u; row < 4u; row++)
    {
        const uint32_t channel = first_channel + row;

        (void)snprintf(line,
                       sizeof(line),
                       "CH%02lu %04u",
                       (unsigned long)(channel + 1u),
                       (unsigned int)model->rc_snapshot->channel[channel]);
        drv_ssd1306_draw_text(display,
                              0u,
                              (uint8_t)(16u + (row * 8u)),
                              line);
    }

    (void)snprintf(line,
                   sizeof(line),
                   "CRC %lu PARSE %lu",
                   (unsigned long)model->rc_snapshot->crc_error_count,
                   (unsigned long)model->rc_snapshot->parse_error_count);
    drv_ssd1306_draw_text(display, 0u, 48u, line);
}

static void ui_pages_render_imu(drv_ssd1306_t *display,
                                const ui_state_t *state,
                                const ui_page_model_t *model)
{
    char line[24];

    (void)snprintf(line,
                   sizeof(line),
                   "%s AGE %lu",
                   ui_pages_imu_state(model->imu_snapshot,
                                      model->imu_status),
                   (unsigned long)ui_pages_age_ms(
                       model->imu_status->age_us));
    drv_ssd1306_draw_text(display, 0u, 8u, line);

    if (state->subpage == 0u)
    {
        (void)snprintf(line,
                       sizeof(line),
                       "ROLL %7.2f DEG",
                       (double)(model->imu_snapshot->attitude.roll_rad *
                                UI_RAD_TO_DEG));
        drv_ssd1306_draw_text(display, 0u, 16u, line);

        (void)snprintf(line,
                       sizeof(line),
                       "PITCH %6.2f DEG",
                       (double)(model->imu_snapshot->attitude.pitch_rad *
                                UI_RAD_TO_DEG));
        drv_ssd1306_draw_text(display, 0u, 24u, line);

        (void)snprintf(line,
                       sizeof(line),
                       "YAW %8.2f DEG",
                       (double)(model->imu_snapshot->attitude.yaw_rad *
                                UI_RAD_TO_DEG));
        drv_ssd1306_draw_text(display, 0u, 32u, line);

        (void)snprintf(line,
                       sizeof(line),
                       "PRATE %6.3f",
                       (double)model->imu_snapshot->attitude.pitch_rate_rps);
        drv_ssd1306_draw_text(display, 0u, 40u, line);

        (void)snprintf(line,
                       sizeof(line),
                       "CAL %s ERR %lu",
                       model->imu_snapshot->calibrated ? "YES" : "NO",
                       (unsigned long)model->imu_snapshot->read_error_count);
        drv_ssd1306_draw_text(display, 0u, 48u, line);
    }
    else
    {
        (void)snprintf(line,
                       sizeof(line),
                       "AX %6.2f AY %6.2f",
                       (double)model->imu_snapshot->data.accel_mps2[0],
                       (double)model->imu_snapshot->data.accel_mps2[1]);
        drv_ssd1306_draw_text(display, 0u, 16u, line);

        (void)snprintf(line,
                       sizeof(line),
                       "AZ %6.2f N %6.2f",
                       (double)model->imu_snapshot->data.accel_mps2[2],
                       (double)model->imu_status->accel_norm_mps2);
        drv_ssd1306_draw_text(display, 0u, 24u, line);

        (void)snprintf(line,
                       sizeof(line),
                       "GX %6.3f GY %6.3f",
                       (double)model->imu_snapshot->data.gyro_rps[0],
                       (double)model->imu_snapshot->data.gyro_rps[1]);
        drv_ssd1306_draw_text(display, 0u, 32u, line);

        (void)snprintf(line,
                       sizeof(line),
                       "GZ %6.3f TEMP %4.1f",
                       (double)model->imu_snapshot->data.gyro_rps[2],
                       (double)model->imu_snapshot->data.temp_c);
        drv_ssd1306_draw_text(display, 0u, 40u, line);

        (void)snprintf(line,
                       sizeof(line),
                       "RAW T %d",
                       (int)model->imu_snapshot->raw.temp_raw);
        drv_ssd1306_draw_text(display, 0u, 48u, line);
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

static uint32_t ui_pages_age_ms(uint64_t age_us)
{
    const uint64_t age_ms = age_us / 1000u;

    return (age_ms > UINT32_MAX) ? UINT32_MAX : (uint32_t)age_ms;
}
