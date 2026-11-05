#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <wifi_connection.h>
#include <wifi_api_server.h>
#include <mpu6050.h>
#include <mpu6050_rotation.h>
#include <imu_orientation.h>
#include <math.h>
#include <stdio.h>
#include <errno.h>
#include "wifi_credentials.h"

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

/* Sensor-to-output mounting rotation, in degrees around fixed X, Y, then Z. */
static const float MOTION_ROTATION_DEGREES[3] = {0.0f, 0.0f, 0.0f};

/**
 * @brief Write a JSON response confirming API availability.
 * @param body Output buffer with capacity bytes of storage.
 * @param capacity Buffer capacity, including the null terminator.
 * @param user_data Unused handler context.
 * @return Required byte count excluding the null terminator, or a negative formatting error.
 */
static int health_handler(char *body, size_t capacity, void *user_data)
{
    ARG_UNUSED(user_data);
    return snprintf(body, capacity, "{\"status\":\"ok\"}");
}

/**
 * @brief Write a JSON response containing device uptime in seconds.
 * @param body Output buffer with capacity bytes of storage.
 * @param capacity Buffer capacity, including the null terminator.
 * @param user_data Unused handler context.
 * @return Required byte count excluding the null terminator, or a negative formatting error.
 */
static int uptime_handler(char *body, size_t capacity, void *user_data)
{
    ARG_UNUSED(user_data);
    return snprintf(body, capacity, "{\"uptime_seconds\":%lld}",
                    (long long)(k_uptime_get() / 1000));
}

/**
 * @brief Read the chip temperature and write a JSON response in degrees Celsius.
 * @param body Output buffer with capacity bytes of storage.
 * @param capacity Buffer capacity, including the null terminator.
 * @param user_data Unused handler context.
 * @return Required byte count excluding the null terminator, or a negative error code.
 */
static int temperature_handler(char *body, size_t capacity, void *user_data)
{
    ARG_UNUSED(user_data);
    const struct device *sensor = DEVICE_DT_GET(DT_NODELABEL(die_temp));
    struct sensor_value temperature;

    if (!device_is_ready(sensor)) {
        LOG_ERR("Internal temperature sensor is not ready");
        return -ENODEV;
    }

    int ret = sensor_sample_fetch_chan(sensor, SENSOR_CHAN_DIE_TEMP);
    if (ret == 0) {
        ret = sensor_channel_get(sensor, SENSOR_CHAN_DIE_TEMP, &temperature);
    }
    if (ret < 0) {
        LOG_ERR("Temperature read failed (%d)", ret);
        return ret;
    }

    /* Format fixed-point Celsius without enabling floating-point printf.
     * Apply the sign separately so temperatures between -1 and 0 work too.
     */
    int64_t milli_celsius = sensor_value_to_milli(&temperature);
    const char *sign = milli_celsius < 0 ? "-" : "";
    if (milli_celsius < 0) {
        milli_celsius = -milli_celsius;
    }
    return snprintf(body, capacity, "{\"temperature_celsius\":%s%lld.%03lld}",
                    sign, (long long)(milli_celsius / 1000),
                    (long long)(milli_celsius % 1000));
}

static const struct WifiApiRoute API_ROUTES[] = {
    { .path = "/api/health", .handler = health_handler },
    { .path = "/api/uptime", .handler = uptime_handler },
    { .path = "/api/temperature", .handler = temperature_handler },
};

/**
 * @brief Run the HTTP API on port 8080 and wait 10 seconds before restarting after a failure.
 * @param arg1 Unused thread argument.
 * @param arg2 Unused thread argument.
 * @param arg3 Unused thread argument.
 * @note The thread runs indefinitely.
 */
static void api_task(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);
    while (true) {
        int ret = wifi_api_server_run(8080, API_ROUTES, ARRAY_SIZE(API_ROUTES));
        LOG_ERR("API server stopped (%d); retrying in 10 seconds", ret);
        k_sleep(K_SECONDS(10));
    }
}

/**
 * @brief Start Wi-Fi connection management using the application credentials.
 * @param arg1 Unused thread argument.
 * @param arg2 Unused thread argument.
 * @param arg3 Unused thread argument.
 */
static void wifi_task(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    int ret = wifi_run(APP_WIFI_SSID, APP_WIFI_PASSWORD);
    LOG_INF("Wi-Fi task finished (%d)", ret);
}

/**
 * @brief Log device uptime once per second.
 * @param arg1 Unused thread argument.
 * @param arg2 Unused thread argument.
 * @param arg3 Unused thread argument.
 * @note The thread runs indefinitely.
 */
static void console_task(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    while (true) {
        LOG_INF(" Uptime %lld seconds",
                (long long)(k_uptime_get() / 1000));
        k_sleep(K_SECONDS(1));
    }
}

/** @brief Estimate and log X/Y/Z orientation in degrees at 10 Hz. */
static void motion_task(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    ImuRotation rotation;
    const float radians_per_degree = 0.01745329252f;
    int ret = imu_rotation_init(&rotation,
                                MOTION_ROTATION_DEGREES[0] * radians_per_degree,
                                MOTION_ROTATION_DEGREES[1] * radians_per_degree,
                                MOTION_ROTATION_DEGREES[2] * radians_per_degree);
    if (ret < 0) {
        LOG_ERR("Invalid motion rotation (%d)", ret);
        return;
    }
    ret = app_mpu6050_init();
    if (ret < 0) {
        LOG_ERR("MPU6050 unavailable (%d); check wiring and reboot", ret);
        return;
    }

    ImuOrientation orientation = {0};
    int64_t previous_ms = 0;
    while (true) {
        Mpu6050Sample sample;
        ret = app_mpu6050_read(&sample);
        if (ret == 0) {
            ret = mpu6050_rotation_apply(&rotation, &sample, &sample);
        }
        if (ret < 0) {
            LOG_ERR("MPU6050 read/rotation failed (%d)", ret);
            k_sleep(K_SECONDS(1));
            continue;
        }

        int64_t now_ms = k_uptime_get();
        float dt = previous_ms == 0 ? 0.1f : (now_ms - previous_ms) / 1000.0f;
        previous_ms = now_ms;
        if (dt <= 0.0f || dt > 0.5f) {
            /* Do not integrate a stale sample across a read failure/debug pause. */
            k_sleep(K_MSEC(100));
            continue;
        }
        ImuMotion motion;
        for (int axis = 0; axis < 3; ++axis) {
            motion.acceleration[axis] =
                (float)sensor_value_to_double(&sample.acceleration[axis]);
            motion.angular_velocity[axis] =
                (float)sensor_value_to_double(&sample.angular_velocity[axis]);
        }
        float degrees[3];
        ret = imu_orientation_update(&orientation, &motion, dt, degrees);
        if (ret == 0) {
            LOG_INF("X: %d, Y: %d, Z: %d [deg]",
                    (int)lroundf(degrees[0]), (int)lroundf(degrees[1]),
                    (int)lroundf(degrees[2]));
        } else {
            LOG_WRN("Orientation unavailable (%d); keep sensor still", ret);
        }
        k_sleep(K_MSEC(100));
    }
}

/* Lower numeric priority runs first. Tasks sleep or wait between work. */
K_THREAD_DEFINE(wifi_task_id, 5200, wifi_task,
                NULL, NULL, NULL, 5, 0, SYS_FOREVER_MS);
K_THREAD_DEFINE(console_task_id, 1024, console_task,
                NULL, NULL, NULL, 7, 0, SYS_FOREVER_MS);
K_THREAD_DEFINE(api_task_id, 4096, api_task,
                NULL, NULL, NULL, 6, 0, SYS_FOREVER_MS);
K_THREAD_DEFINE(motion_task_id, 2048, motion_task,
                NULL, NULL, NULL, 7, 0, SYS_FOREVER_MS);

/**
 * @brief Name and start the Wi-Fi, console, HTTP API, and motion threads.
 * @return Always 0 after starting the threads.
 */
int main(void)
{
    k_thread_name_set(wifi_task_id, "wifi");
    k_thread_name_set(console_task_id, "console");
    k_thread_name_set(api_task_id, "wifi_api");
    k_thread_name_set(motion_task_id, "motion");
    k_thread_start(wifi_task_id);
    k_thread_start(console_task_id);
    k_thread_start(api_task_id);
    k_thread_start(motion_task_id);
    return 0;
}
