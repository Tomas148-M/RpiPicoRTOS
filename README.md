# Pico 2 W Wi-Fi & Motion

Zephyr application for **Pico 2 W** (`rpi_pico2/rp2350a/m33_0/w`), using
Zephyr SDK **1.0.1**. Includes Wi-Fi, an HTTP API, and MPU6050 orientation logging.

## Build and run

1. Create ignored `src/wifi_env.h` with `APP_WIFI_SSID` and `APP_WIFI_PASSWORD`
   as C string literals. Use 2.4 GHz WPA2; an empty SSID disables Wi-Fi.
2. In VS Code, run **Compile Project** to build or **Run WiFi** to build and flash.
   For debugging, use **WiFi Debug (Pico 2 W)**.
3. Open UART at **115200 baud**: GP0 (TX) to probe RX, GP1 (RX) to probe TX,
   and GND to GND. The console prints the DHCP address and motion readings.

For a new Zephyr workspace, fetch Wi-Fi blobs once:

```text
west blobs fetch hal_infineon --allow-regex '^img/whd/resources/(firmware|clm)/COMPONENT_43439/'
```

Keep Wi-Fi credentials out of Git; they are embedded in the firmware.

## MPU6050 wiring and motion

Use a module compatible with **3.3 V power and I/O**.

| MPU6050 | Pico 2 W |
|---|---|
| VCC | 3V3 OUT (pin 36) |
| GND | GND (pin 38) |
| SDA | GP4 (pin 6) |
| SCL | GP5 (pin 7) |
| AD0 | GND: address `0x68` |
| INT, XDA, XCL | Unconnected |

SDA/SCL need pull-ups to 3.3 V, usually fitted on the module.
[app.overlay](app.overlay) configures I2C0, +/-2 g and +/-250 degrees/s.

The motion thread logs X/Y/Z orientation (roll/pitch/yaw) in whole degrees at
10 Hz. Keep the sensor still at startup; yaw starts at zero and drifts without
a compass. Sustained linear acceleration can disturb tilt. If the sensor is
unavailable at boot, check wiring and reboot.

Set `MOTION_ROTATION_DEGREES` in [src/main.c](src/main.c) to the mounting
rotation around X, Y, Z, then rebuild and flash. `{0, 0, 90}` maps +X to +Y.

## Libraries and HTTP API

- [mpu6050.h](lib/mpu6050/include/mpu6050.h): readiness and sample reads;
  acceleration in m/s², angular velocity in rad/s, temperature in °C.
- [imu_rotation.h](lib/imu_rotation/include/imu_rotation.h): rotate both vectors
  by known angles in radians, X → Y → Z. Use
  [mpu6050_rotation.h](lib/imu_rotation/include/mpu6050_rotation.h) for direct samples.
- [imu_orientation.h](lib/imu_rotation/include/imu_orientation.h): orientation estimation.
- [wifi_connection.h](lib/wifi/include/wifi_connection.h): connection/reconnection.
- [wifi_api_server.h](lib/wifi_api/include/wifi_api_server.h): HTTP route handlers.

Use `http://<pico-ip>:8080` with GET `/api/health`, `/api/uptime`, or
`/api/temperature` (CPU die temperature). Add routes in `src/main.c`.
The server has no authentication or TLS; use a trusted local network.

## Checks and CI

Host math tests (requires a C compiler):

```text
cmake -S tests/imu_rotation -B build/imu_rotation_tests
cmake --build build/imu_rotation_tests --config Debug
ctest --test-dir build/imu_rotation_tests -C Debug --output-on-failure
```

[GitHub Actions](.github/workflows/firmware.yml) builds on pushes and PRs and
provides ELF/UF2/BIN artifacts with Wi-Fi disabled. Hardware checks require a
board: verify motion output, the three API routes, and Wi-Fi reconnection.
See [contributing guidelines](doc/contributing.md) for coding conventions.
