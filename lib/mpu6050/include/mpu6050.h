#pragma once

#include <zephyr/drivers/sensor.h>

/** One complete sample in SI units, without floating-point conversion. */
typedef struct {
    struct sensor_value acceleration[3]; /**< X, Y, Z in m/s^2. */
    struct sensor_value angular_velocity[3]; /**< X, Y, Z in rad/s. */
    struct sensor_value temperature; /**< Sensor die temperature in Celsius. */
} Mpu6050Sample;

/**
 * @brief Check the MPU6050 configured by the mpu6050 devicetree alias.
 * @return 0 if ready, -ENODEV if absent or driver initialization failed.
 * @note Zephyr initializes and configures the hardware before main().
 */
int app_mpu6050_init(void);

/**
 * @brief Fetch acceleration, angular velocity and temperature together.
 * @param sample Output; unchanged on error. Must not be NULL.
 * @return 0 on success, otherwise a negative errno value.
 * @note Thread context only. Calls are serialized. Access this sensor only
 * through this library so that fetch/get sequences cannot interleave.
 */
int app_mpu6050_read(Mpu6050Sample *sample);
