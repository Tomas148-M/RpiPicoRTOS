#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

int main(void)
{
    int ret;

    printk("Hello World from Zephyr on Raspberry Pi Pico 2 W!\n");

    if (!device_is_ready(DEVICE_DT_GET(DT_PARENT(DT_NODELABEL(cyw43_gpio))))) {
        printk("Wi-Fi chip initialization failed; onboard LED is unavailable\n");
        return 0;
    }

    if (!gpio_is_ready_dt(&led)) {
        printk("LED device is not ready\n");
        return 0;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        printk("Failed to configure LED: %d\n", ret);
        return 0;
    }

    printk("Onboard LED blinking: 500 ms on, 500 ms off\n");

    while (1) {
        ret = gpio_pin_toggle_dt(&led);
        if (ret < 0) {
            printk("Failed to toggle LED: %d\n", ret);
            return 0;
        }
        k_msleep(500);
    }
}
