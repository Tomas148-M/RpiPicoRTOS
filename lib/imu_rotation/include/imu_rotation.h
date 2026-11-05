#pragma once

/** Rotation from sensor coordinates to the desired output coordinates. */
typedef struct {
    float matrix[3][3];
} ImuRotation;

/** XYZ vectors; rotation preserves the input units. */
typedef struct {
    float acceleration[3];
    float angular_velocity[3];
} ImuMotion;

/**
 * @brief Prepare a rotation matrix once for repeated sample transformations.
 * @param rotation Output matrix; unchanged on failure.
 * @param roll Rotation about X, in radians.
 * @param pitch Rotation about Y, in radians.
 * @param yaw Rotation about Z, in radians.
 * @return 0 on success, -EINVAL for NULL or nonfinite angles.
 * @note Uses right-handed column vectors: output = Rz(yaw) * Ry(pitch) *
 * Rx(roll) * input. Rotations act about fixed axes in X, Y, Z order.
 * A positive pi/2 yaw maps input +X to output +Y. These angles describe
 * sensor-to-output rotation; they are not estimated from measurements.
 */
int imu_rotation_init(ImuRotation *rotation, float roll, float pitch, float yaw);

/**
 * @brief Apply the same rotation to acceleration and angular velocity.
 * @param rotation Matrix prepared by imu_rotation_init(); do not modify it.
 * @param input Input vectors, e.g. m/s^2 and rad/s from an MPU6050.
 * @param output Rotated vectors, in the same units. May equal input.
 * @return 0 on success, -EINVAL for NULL/nonfinite input or matrix, -ERANGE
 * if a result overflows. Output is unchanged on failure.
 * @note Reentrant for independent output buffers and an immutable matrix.
 * Gravity remains part of acceleration; no filtering or bias removal occurs.
 */
int imu_rotation_apply(const ImuRotation *rotation, const ImuMotion *input,
                       ImuMotion *output);
