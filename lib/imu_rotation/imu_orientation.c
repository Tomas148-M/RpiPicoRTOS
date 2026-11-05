#include <imu_orientation.h>
#include <errno.h>
#include <math.h>
#include <stddef.h>

int imu_orientation_update(ImuOrientation *state, const ImuMotion *motion,
                           float dt, float degrees[3])
{
    if (state == NULL || motion == NULL || degrees == NULL ||
        !isfinite(dt) || dt <= 0.0f || dt > 0.5f) {
        return -EINVAL;
    }
    for (int i = 0; i < 3; ++i) {
        if (!isfinite(motion->acceleration[i]) ||
            !isfinite(motion->angular_velocity[i])) {
            return -EINVAL;
        }
    }
    float ax = motion->acceleration[0];
    float ay = motion->acceleration[1];
    float az = motion->acceleration[2];
    float norm = sqrtf(ax * ax + ay * ay + az * az);
    bool gravity_valid = isfinite(norm) && norm > 7.84532f && norm < 11.76798f;
    float w, x, y, z;
    if (!state->initialized) {
        if (!gravity_valid) {
            return -EINVAL;
        }
        float roll = atan2f(ay, az);
        float pitch = atan2f(-ax, sqrtf(ay * ay + az * az));
        w = cosf(roll / 2) * cosf(pitch / 2);
        x = sinf(roll / 2) * cosf(pitch / 2);
        y = cosf(roll / 2) * sinf(pitch / 2);
        z = -sinf(roll / 2) * sinf(pitch / 2);
    } else {
        w = state->quaternion[0];
        x = state->quaternion[1];
        y = state->quaternion[2];
        z = state->quaternion[3];
        float gx = motion->angular_velocity[0];
        float gy = motion->angular_velocity[1];
        float gz = motion->angular_velocity[2];
        if (gravity_valid) {
            ax /= norm;
            ay /= norm;
            az /= norm;
            /* Measured gravity cross predicted gravity, proportional gain 2/s. */
            float vx = 2 * (x * z - w * y);
            float vy = 2 * (w * x + y * z);
            float vz = 1 - 2 * (x * x + y * y);
            gx += 2 * (ay * vz - az * vy);
            gy += 2 * (az * vx - ax * vz);
            gz += 2 * (ax * vy - ay * vx);
        }
        float half_dt = 0.5f * dt;
        float nw = w + (-x * gx - y * gy - z * gz) * half_dt;
        float nx = x + (w * gx + y * gz - z * gy) * half_dt;
        float ny = y + (w * gy - x * gz + z * gx) * half_dt;
        float nz = z + (w * gz + x * gy - y * gx) * half_dt;
        w = nw;
        x = nx;
        y = ny;
        z = nz;
    }
    norm = sqrtf(w * w + x * x + y * y + z * z);
    if (!isfinite(norm) || norm < 0.000001f) {
        return -EINVAL;
    }
    w /= norm;
    x /= norm;
    y /= norm;
    z /= norm;
    float pitch_sine = fmaxf(-1.0f, fminf(1.0f, 2 * (w * y - z * x)));
    const float degrees_per_radian = 57.295779513f;
    degrees[0] = atan2f(2 * (w * x + y * z), 1 - 2 * (x * x + y * y)) * degrees_per_radian;
    degrees[1] = asinf(pitch_sine) * degrees_per_radian;
    degrees[2] = atan2f(2 * (w * z + x * y), 1 - 2 * (y * y + z * z)) * degrees_per_radian;
    *state = (ImuOrientation){.quaternion = {w, x, y, z}, .initialized = true};
    return 0;
}
