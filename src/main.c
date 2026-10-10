#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <wifi_connection.h>
#include "wifi_credentials.h"

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

static void wifi_task(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    int ret = wifi_run(APP_WIFI_SSID, APP_WIFI_PASSWORD);
    LOG_INF("Wi-Fi task finished (%d)", ret);
}

static void console_task(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    while (true)
    {
        LOG_INF(" Uptime %lld seconds",
                (long long)(k_uptime_get() / 1000));
        k_sleep(K_SECONDS(1));
    }
}

/* Lower numeric priority runs first. Both tasks sleep or wait between work. */
K_THREAD_DEFINE(wifi_task_id, 5200, wifi_task,
                NULL, NULL, NULL, 5, 0, SYS_FOREVER_MS);
K_THREAD_DEFINE(console_task_id, 1024, console_task,
                NULL, NULL, NULL, 7, 0, SYS_FOREVER_MS);

int main(void)
{
    k_thread_name_set(wifi_task_id, "wifi");
    k_thread_name_set(console_task_id, "console");
    k_thread_start(wifi_task_id);
    k_thread_start(console_task_id);
    return 0;
}
