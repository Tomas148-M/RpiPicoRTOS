#include <imu_rotation.h>
#include <errno.h>
#include <math.h>
#include <stddef.h>

int imu_rotation_init(ImuRotation *rotation, float roll, float pitch, float yaw)
{
    if (rotation == NULL || !isfinite(roll) || !isfinite(pitch) || !isfinite(yaw)) {
        return -EINVAL;
    }

    float cr = cosf(roll);
    float sr = sinf(roll);
    float cp = cosf(pitch);
    float sp = sinf(pitch);
    float cy = cosf(yaw);
    float sy = sinf(yaw);
    ImuRotation next = {
        .matrix = {
            {cy * cp, cy * sp * sr - sy * cr, cy * sp * cr + sy * sr},
            {sy * cp, sy * sp * sr + cy * cr, sy * sp * cr - cy * sr},
            {-sp, cp * sr, cp * cr},
        },
    };

    *rotation = next;
    return 0;
}

int imu_rotation_apply(const ImuRotation *rotation, const ImuMotion *input,
                       ImuMotion *output)
{
    if (rotation == NULL || input == NULL || output == NULL) {
        return -EINVAL;
    }

    for (int axis = 0; axis < 3; ++axis) {
        if (!isfinite(input->acceleration[axis]) ||
            !isfinite(input->angular_velocity[axis])) {
            return -EINVAL;
        }
        for (int column = 0; column < 3; ++column) {
            if (!isfinite(rotation->matrix[axis][column])) {
                return -EINVAL;
            }
        }
    }

    ImuMotion next = {0};
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            next.acceleration[row] += rotation->matrix[row][column] *
                                      input->acceleration[column];
            next.angular_velocity[row] += rotation->matrix[row][column] *
                                          input->angular_velocity[column];
        }
        if (!isfinite(next.acceleration[row]) || !isfinite(next.angular_velocity[row])) {
            return -ERANGE;
        }
    }

    *output = next;
    return 0;
}
