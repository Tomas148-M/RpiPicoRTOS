#include <imu_rotation.h>
#include <imu_orientation.h>
#include <assert.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void expect_vector(const float actual[3], float x, float y, float z)
{
    assert(fabsf(actual[0] - x) < 0.00002f);
    assert(fabsf(actual[1] - y) < 0.00002f);
    assert(fabsf(actual[2] - z) < 0.00002f);
}

static float norm_squared(const float vector[3])
{
    return vector[0] * vector[0] + vector[1] * vector[1] + vector[2] * vector[2];
}

int main(void)
{
    ImuOrientation orientation = {0};
    ImuMotion stationary = {{0, 0, 9.80665f}, {0, 0, 0}};
    float angles[3];
    assert(imu_orientation_update(&orientation, &stationary, 0.01f, angles) == 0);
    expect_vector(angles, 0, 0, 0);
    /* One second of 90 degrees/s yaw, with gravity unchanged. */
    stationary.angular_velocity[2] = 1.57079632679f;
    for (int i = 0; i < 100; ++i) {
        assert(imu_orientation_update(&orientation, &stationary, 0.01f, angles) == 0);
    }
    assert(fabsf(angles[2] - 90.0f) < 0.01f);
    assert(fabsf(angles[0]) < 0.001f && fabsf(angles[1]) < 0.001f);
    ImuOrientation saved = orientation;
    assert(imu_orientation_update(&orientation, &stationary, 1.0f, angles) == -EINVAL);
    assert(memcmp(&saved, &orientation, sizeof(saved)) == 0);
    orientation = (ImuOrientation){0};
    stationary = (ImuMotion){{0, 9.80665f, 0}, {0, 0, 0}};
    assert(imu_orientation_update(&orientation, &stationary, 0.01f, angles) == 0);
    assert(fabsf(angles[0] - 90.0f) < 0.001f);

    const float half_pi = 1.5707963267948966f;
    const ImuMotion input = {{1.0f, 2.0f, 3.0f}, {-4.0f, 5.0f, -6.0f}};
    ImuRotation rotation;
    ImuMotion output;

    assert(imu_rotation_init(&rotation, 0, 0, 0) == 0);
    assert(imu_rotation_apply(&rotation, &input, &output) == 0);
    expect_vector(output.acceleration, 1, 2, 3);
    expect_vector(output.angular_velocity, -4, 5, -6);

    assert(imu_rotation_init(&rotation, half_pi, 0, 0) == 0);
    assert(imu_rotation_apply(&rotation, &input, &output) == 0);
    expect_vector(output.acceleration, 1, -3, 2);
    expect_vector(output.angular_velocity, -4, 6, 5);

    assert(imu_rotation_init(&rotation, 0, half_pi, 0) == 0);
    assert(imu_rotation_apply(&rotation, &input, &output) == 0);
    expect_vector(output.acceleration, 3, 2, -1);
    expect_vector(output.angular_velocity, -6, 5, 4);

    assert(imu_rotation_init(&rotation, 0, 0, half_pi) == 0);
    output = input;
    assert(imu_rotation_apply(&rotation, &output, &output) == 0);
    expect_vector(output.acceleration, -2, 1, 3);
    expect_vector(output.angular_velocity, -5, -4, -6);

    /* Noncommuting rotations verify fixed X -> Y -> Z order. */
    assert(imu_rotation_init(&rotation, half_pi, half_pi, half_pi) == 0);
    assert(imu_rotation_apply(&rotation, &input, &output) == 0);
    expect_vector(output.acceleration, 3, 2, -1);
    expect_vector(output.angular_velocity, -6, 5, 4);

    assert(imu_rotation_init(&rotation, 0.37f, -0.63f, 1.12f) == 0);
    assert(imu_rotation_apply(&rotation, &input, &output) == 0);
    assert(fabsf(norm_squared(output.acceleration) - norm_squared(input.acceleration)) < 0.0001f);
    assert(fabsf(norm_squared(output.angular_velocity) - norm_squared(input.angular_velocity)) < 0.0001f);

    /* Transpose is the inverse for a proper rotation. */
    ImuRotation inverse;
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            inverse.matrix[row][column] = rotation.matrix[column][row];
        }
    }
    assert(imu_rotation_apply(&inverse, &output, &output) == 0);
    expect_vector(output.acceleration, 1, 2, 3);
    expect_vector(output.angular_velocity, -4, 5, -6);

    ImuRotation saved_rotation = rotation;
    assert(imu_rotation_init(NULL, 0, 0, 0) == -EINVAL);
    assert(imu_rotation_init(&rotation, NAN, 0, 0) == -EINVAL);
    assert(imu_rotation_init(&rotation, 0, INFINITY, 0) == -EINVAL);
    assert(memcmp(&rotation, &saved_rotation, sizeof(rotation)) == 0);
    assert(imu_rotation_apply(NULL, &input, &output) == -EINVAL);
    assert(imu_rotation_apply(&rotation, NULL, &output) == -EINVAL);
    assert(imu_rotation_apply(&rotation, &input, NULL) == -EINVAL);

    ImuMotion saved_output = output;
    ImuMotion invalid = input;
    invalid.angular_velocity[2] = NAN;
    assert(imu_rotation_apply(&rotation, &invalid, &output) == -EINVAL);
    rotation.matrix[2][1] = INFINITY;
    assert(imu_rotation_apply(&rotation, &input, &output) == -EINVAL);
    assert(memcmp(&output, &saved_output, sizeof(output)) == 0);

    assert(imu_rotation_init(&rotation, 0, 0, half_pi / 2) == 0);
    ImuMotion huge = {{FLT_MAX, -FLT_MAX, 0}, {0, 0, 0}};
    assert(imu_rotation_apply(&rotation, &huge, &output) == -ERANGE);
    assert(memcmp(&output, &saved_output, sizeof(output)) == 0);

    puts("IMU rotation tests passed");
    return 0;
}
