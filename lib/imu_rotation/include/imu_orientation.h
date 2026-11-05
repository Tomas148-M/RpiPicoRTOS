#pragma once

#include <imu_rotation.h>
#include <stdbool.h>

typedef struct {
    float quaternion[4];
    bool initialized;
} ImuOrientation;

/**
 * @brief Estimate roll, pitch and yaw in degrees from SI acceleration and gyro.
 * Zero-initialize state before use. dt is elapsed seconds, in (0, 0.5].
 * First sample initializes tilt from gravity and yaw to zero; keep still then.
 * Gravity correction is used only within 20 percent of standard gravity.
 * Yaw is relative and drifts; linear acceleration can disturb tilt estimates.
 * Euler pitch is in [-90, 90]; roll/yaw in [-180, 180], with gimbal lock at
 * vertical pitch. Use independent state per thread. Output/state unchanged
 * on invalid input (-EINVAL). No gyro bias calibration is performed.
 */
int imu_orientation_update(ImuOrientation *state, const ImuMotion *motion,
                           float dt, float degrees[3]);
