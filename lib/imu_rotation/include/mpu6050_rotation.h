#pragma once

#include <imu_rotation.h>
#include <mpu6050.h>

/**
 * @brief Rotate MPU6050 acceleration and gyro vectors into output coordinates.
 * @param rotation Matrix prepared with imu_rotation_init(), angles in radians.
 * @param input Sensor sample; temperature is copied without rotation.
 * @param output Rotated sample in the original SI units. May equal input.
 * @return 0 on success, -EINVAL for NULL/invalid matrix, or -ERANGE on overflow.
 * Output remains unchanged on failure. Gravity remains in acceleration.
 */
int mpu6050_rotation_apply(const ImuRotation *rotation, const Mpu6050Sample *input,
                          Mpu6050Sample *output);
