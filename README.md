# Pico 2 W Wi-Fi

## MPU6050 motion sensor

`lib/mpu6050` wraps Zephyr's MPU6050 driver. Include `mpu6050.h` and call
`app_mpu6050_init()` to check readiness, then `app_mpu6050_read()` from a thread.
Zephyr performs hardware initialization at boot. Reads are serialized by a
mutex and leave the output unchanged on failure; use this library exclusively
when accessing this sensor. Errors are negative errno values (`-EINVAL` for
a null output, `-ENODEV` for an unavailable device, or a driver error).

Wire SDA to **GP4** (physical pin 6), SCL to **GP5** (pin 7), GND to GND,
and use a module compatible with **3.3 V power and I/O**. SDA/SCL need pull-ups
to 3.3 V (usually provided on the module). Tie AD0 low for address `0x68`.
INT is unused. `app.overlay` configures I2C0 with ranges of +/-2 g and
+/-250 degrees/s. For AD0 high, change both `mpu6050@68` and `reg` to `69`
and `0x69`. Configuration is based on the
[Zephyr MPU6050 binding](https://docs.zephyrproject.org/latest/build/dts/api/bindings/sensor/invensense%2Cmpu6050.html).

Example inside an application thread (add `#include <mpu6050.h>`):

```c
Mpu6050Sample sample;
int ret = app_mpu6050_init();
if (ret == 0) {
    ret = app_mpu6050_read(&sample);
}
if (ret == 0) {
    int64_t acceleration_z_micro = sensor_value_to_micro(&sample.acceleration[2]);
    int64_t gyro_z_micro = sensor_value_to_micro(&sample.angular_velocity[2]);
    int64_t temperature_milli = sensor_value_to_milli(&sample.temperature);
    /* Use values in micro m/s^2, micro rad/s and milli degrees Celsius. */
    ARG_UNUSED(acceleration_z_micro);
    ARG_UNUSED(gyro_z_micro);
    ARG_UNUSED(temperature_milli);
}
```

The sample stores X/Y/Z acceleration in m/s^2, angular velocity in rad/s,
and sensor die temperature in Celsius as `struct sensor_value` fixed-point
values. No floating-point printf is required. The library does not estimate
orientation or calibrate bias. The existing HTTP endpoints are unchanged.

Build using **Compile Project**. If an existing build predates `app.overlay`,
perform a pristine build (`west build -p always` with the board and paths
described below). On hardware, check that readiness succeeds, a stationary
sensor measures approximately 9.81 m/s^2 along gravity and near-zero angular
velocity, and rotation changes the corresponding gyro axis. Disconnect the
sensor to verify error handling; reconnect and reboot if boot initialization
failed. These physical checks require a connected MPU6050.

## Rotating acceleration and angular velocity

`lib/imu_rotation` is a separate, hardware-independent C library. It rotates
both XYZ vectors by known angles, preserving units and vector lengths (within
floating-point rounding). Prepare an `ImuRotation` once, then reuse it for each
sample. Angles are **radians** and follow the right-hand rule. The convention
is `output = Rz(yaw) * Ry(pitch) * Rx(roll) * input`: fixed X, then Y, then Z
rotations from sensor coordinates into the desired output coordinates.
A +90 degree yaw maps sensor +X to output +Y and +Y to -X.

Example in a thread, after checking `app_mpu6050_init()` succeeds:

```c
#include <mpu6050.h>
#include <imu_rotation.h>

/* Inside a function: a known +90 degree rotation around Z. */
ImuRotation rotation;
Mpu6050Sample sample;
ImuMotion input;
ImuMotion rotated;
int ret = imu_rotation_init(&rotation, 0.0f, 0.0f, 1.57079632679f);
if (ret == 0) {
    ret = app_mpu6050_read(&sample);
}
if (ret == 0) {
    for (int axis = 0; axis < 3; ++axis) {
        input.acceleration[axis] = (float)sensor_value_to_double(&sample.acceleration[axis]);
        input.angular_velocity[axis] = (float)sensor_value_to_double(&sample.angular_velocity[axis]);
    }
    ret = imu_rotation_apply(&rotation, &input, &rotated);
}
if (ret == 0) {
    /* rotated.acceleration: m/s^2; rotated.angular_velocity: rad/s. */
}
```

Use `radians = degrees * 0.01745329252f` when starting with degrees.
Choose angles describing the sensor-to-output transformation. To undo a
combined rotation, use the transpose of its matrix; negating all three Euler
angles in the same order does not generally produce the inverse.

`imu_rotation_apply()` supports using the same input and output pointer.
It returns `-EINVAL` for null pointers or nonfinite values and `-ERANGE` for
overflow; output remains unchanged on error. Share an initialized matrix
between threads only while it remains immutable. The library uses single
precision math, keeps gravity in the acceleration, and performs no orientation
estimation, integration or calibration. Temperature is not rotated.

Standalone math tests need a host C compiler and do not require hardware or
Zephyr. From a compiler-enabled terminal (e.g. Visual Studio Developer Command
Prompt), run:

```text
cmake -S tests/imu_rotation -B build/imu_rotation_tests
cmake --build build/imu_rotation_tests --config Debug
ctest --test-dir build/imu_rotation_tests -C Debug --output-on-failure
```

## Continuous integration

[The GitHub Actions workflow](.github/workflows/firmware.yml) builds the Pico 2 W
firmware on pushes, pull requests, and manual runs from the **Actions** tab.
It pins Zephyr to revision `a915b2f12675b48b16d56653ea145dcd8f20cbbf`
and uses Zephyr SDK 1.0.1. Update the workflow pins deliberately when upgrading
the local toolchain. CI fetches that exact commit with Git and then runs
`west init -l zephyr`; west 1.5.0's remote `--mr` option accepts branch or tag
names and cannot initialize directly from a commit SHA.

CI creates its own `src/wifi_env.h` with empty credentials; no secrets are needed.
Downloaded CI firmware therefore has Wi-Fi connection attempts disabled. A
successful run provides the ELF, UF2, and BIN files as an artifact retained for
14 days. CI checks compilation; device and Wi-Fi checks still require hardware.
Formatting hooks described in `doc/contributing.md` are not yet configured in CI.

## Local development

Targets `rpi_pico2/rp2350a/m33_0/w` with Zephyr SDK 1.0.1.

Wi-Fi lives in the Zephyr library `lib/wifi`, with public header
`wifi_connection.h`. A dedicated Zephyr thread named `wifi` calls
`wifi_run(ssid, password)` to run the connection/reconnection loop.
A second thread named `console` logs uptime every second independently of
Wi-Fi connection attempts. `main()` starts both threads and returns.
The Wi-Fi thread has priority 5 and a 5200-byte stack; the console thread
has priority 7 and a 1024-byte stack. Thread names are visible in the debugger.
Credentials live in `src/wifi_env.h`, which is ignored by Git and included
by `src/wifi_credentials.h`.

VS Code tasks: **Build WiFi** builds the application and library;
**Run WiFi** builds, flashes, verifies and starts the firmware using OpenOCD.
For interactive debugging, select **WiFi Debug (Pico 2 W)** and press F5.

1. Create `src/wifi_env.h` if needed, then define `APP_WIFI_SSID` and
   `APP_WIFI_PASSWORD` as C string literals in that file.
   Use a 2.4 GHz WPA2-Personal network with an 8-63 byte passphrase.
2. Run **Compile Project** in VS Code, then flash or start debugging.
3. Read the UART console at 115200 baud for connection status and the DHCP IPv4 address.
   Connect Pico GP0 (TX) to probe RX, GP1 (RX) to probe TX, and GND.
   OpenOCD's terminal does not carry application UART output.

Failed connections are retried every 10 seconds; disconnections trigger reconnection.
An empty SSID disables connection attempts. Credentials are embedded in firmware;
keep real passwords out of source control. Passwords are never logged.

For a new Zephyr workspace, run `west blobs fetch hal_infineon` once to obtain
the CYW43439 firmware. The `/w` board supplies the Wi-Fi devicetree and driver;
no application overlay is needed. Set `CONFIG_WHD_WIFI_COUNTRY` in `prj.conf`
to the appropriate driver country constant for your deployment if needed.

## Wi-Fi HTTP API server

The reusable `lib/wifi_api` library exposes `wifi_api_server_run(port, routes,
route_count)` through `wifi_api_server.h`. The application starts it in the
`wifi_api` thread (priority 6, 4096-byte stack), listening on IPv4 port 8080.
It becomes reachable once Wi-Fi has a DHCP address; the Wi-Fi reconnection
thread continues running independently. Listener failures retry after 10 seconds.

Using the address printed on the UART console, try:

```powershell
curl.exe http://192.168.1.100:8080/api/health
# {"status":"ok"}
curl.exe http://192.168.1.100:8080/api/uptime
# {"uptime_seconds":123}
curl.exe http://192.168.1.100:8080/api/temperature
# {"temperature_celsius":32.125}
```

`/api/temperature` takes a fresh reading from the internal RP2350 temperature
sensor for each request and returns Celsius as a JSON number. It measures the
CPU die temperature, rather than ambient air temperature. The three decimal
places describe the response format, not sensor accuracy. If the sensor is
unavailable or reading fails, the server returns HTTP 500 with
`{"error":"handler failed"}` and logs the cause on the console.

Replace the example IP with your Pico's address. Add entries to `API_ROUTES`
in `src/main.c` to expose more GET endpoints. Each handler receives a response
buffer, its capacity, and the route's `user_data`; write JSON and return the
byte count excluding the terminating NUL. Negative results or results exceeding
the buffer produce HTTP 500. Route storage must outlive the server call.

The server handles one client at a time, closes each connection after its
response, limits request headers to 1023 bytes and JSON responses to 511 bytes,
and applies a five-second header deadline and socket I/O timeouts. Query strings
are ignored for route matching. Unknown routes return 404, unsupported methods
return 405 with `Allow: GET`, and oversized headers return 431.
It does not support request bodies, streaming, TLS, or authentication; use it
on a trusted local network. `/api/health` reports that the API is responding.

After building and flashing, verify the three endpoints above, an unknown path
(404), and `curl.exe -X POST http://<pico-ip>:8080/api/health` (405). Disconnect
and reconnect Wi-Fi and repeat the health request to check recovery on hardware.
