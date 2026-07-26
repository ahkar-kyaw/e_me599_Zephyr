#ifndef EST_ATTITUDE_H
#define EST_ATTITUDE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    EST_ATTITUDE_ALGORITHM_COMPLEMENTARY = 0,
    EST_ATTITUDE_ALGORITHM_MADGWICK,
    EST_ATTITUDE_ALGORITHM_KALMAN
} est_attitude_algorithm_t;

typedef enum
{
    EST_ATTITUDE_SENSOR_MODE_6DOF = 0,
    EST_ATTITUDE_SENSOR_MODE_9DOF
} est_attitude_sensor_mode_t;

typedef struct
{
    float accel_mps2[3];
    float gyro_rps[3];

    bool mag_valid;
    float mag_uT[3];

    float dt_s;
} est_attitude_input_t;

typedef struct
{
    bool valid;

    float roll_rad;
    float pitch_rad;
    float yaw_rad;

    float roll_rate_rps;
    float pitch_rate_rps;
    float yaw_rate_rps;
} est_attitude_output_t;

typedef struct
{
    est_attitude_algorithm_t algorithm;
    est_attitude_sensor_mode_t sensor_mode;

    float complementary_alpha;
    float complementary_mag_alpha;
} est_attitude_config_t;

typedef struct
{
    est_attitude_algorithm_t algorithm;
    est_attitude_sensor_mode_t sensor_mode;

    bool initialized;

    float complementary_alpha;
    float complementary_mag_alpha;

    float roll_rad;
    float pitch_rad;
    float yaw_rad;
} est_attitude_t;

est_attitude_config_t est_attitude_default_config(void);

void est_attitude_init(est_attitude_t *est,
                       const est_attitude_config_t *config);

void est_attitude_reset(est_attitude_t *est);

bool est_attitude_update(est_attitude_t *est,
                         const est_attitude_input_t *input,
                         est_attitude_output_t *output);

#ifdef __cplusplus
}
#endif

#endif