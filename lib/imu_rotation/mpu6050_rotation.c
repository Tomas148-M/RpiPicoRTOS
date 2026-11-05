#include <mpu6050_rotation.h>
#include <errno.h>
#include <limits.h>
#include <math.h>

int mpu6050_rotation_apply(const ImuRotation *rotation, const Mpu6050Sample *input,
                          Mpu6050Sample *output)
{
    if (input == NULL || output == NULL) {
        return -EINVAL;
    }

    ImuMotion motion;
    ImuMotion rotated;
    for (int axis = 0; axis < 3; ++axis) {
        motion.acceleration[axis] = (float)sensor_value_to_double(&input->acceleration[axis]);
        motion.angular_velocity[axis] =
            (float)sensor_value_to_double(&input->angular_velocity[axis]);
    }
    int ret = imu_rotation_apply(rotation, &motion, &rotated);
    if (ret < 0) {
        return ret;
    }

    Mpu6050Sample next = *input;
    for (int axis = 0; axis < 3; ++axis) {
        double acceleration = rotated.acceleration[axis];
        double gyro = rotated.angular_velocity[axis];
        if (acceleration <= INT_MIN || acceleration >= INT_MAX ||
            gyro <= INT_MIN || gyro >= INT_MAX) {
            return -ERANGE;
        }
        ret = sensor_value_from_double(&next.acceleration[axis], acceleration);
        if (ret < 0) {
            return ret;
        }
        ret = sensor_value_from_double(&next.angular_velocity[axis], gyro);
        if (ret < 0) {
            return ret;
        }
    }
    *output = next;
    return 0;
}
