#include <mpu6050.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <errno.h>

K_MUTEX_DEFINE(mpu6050_mutex);

static const struct device *get_sensor(void)
{
#if DT_NODE_HAS_STATUS(DT_ALIAS(mpu6050), okay)
    return DEVICE_DT_GET(DT_ALIAS(mpu6050));
#else
    return NULL;
#endif
}

int app_mpu6050_init(void)
{
    const struct device *sensor = get_sensor();

    return sensor != NULL && device_is_ready(sensor) ? 0 : -ENODEV;
}

int app_mpu6050_read(Mpu6050Sample *sample)
{
    if (sample == NULL) {
        return -EINVAL;
    }

    const struct device *sensor = get_sensor();
    if (sensor == NULL || !device_is_ready(sensor)) {
        return -ENODEV;
    }

    int ret = k_mutex_lock(&mpu6050_mutex, K_FOREVER);
    if (ret < 0) {
        return ret;
    }

    Mpu6050Sample next;
    ret = sensor_sample_fetch(sensor);
    if (ret == 0) {
        ret = sensor_channel_get(sensor, SENSOR_CHAN_ACCEL_XYZ, next.acceleration);
    }
    if (ret == 0) {
        ret = sensor_channel_get(sensor, SENSOR_CHAN_GYRO_XYZ, next.angular_velocity);
    }
    if (ret == 0) {
        ret = sensor_channel_get(sensor, SENSOR_CHAN_DIE_TEMP, &next.temperature);
    }
    if (ret == 0) {
        *sample = next;
    }

    k_mutex_unlock(&mpu6050_mutex);
    return ret;
}
