# Zephyr RTOS Project for Raspberry Pi Pico 2 W

This is a basic Zephyr real-time operating system application for the Raspberry Pi Pico 2 W (RP2350, Cortex-M33).

## Project Structure

- `src/`: Source code files
- `include/`: Header files
- `boards/`: Custom board definitions (if any)
- `dts/`: Device tree source files and overlays
- `CMakeLists.txt`: CMake build configuration
- `prj.conf`: Zephyr configuration file

## Prerequisites

1. Install the Zephyr SDK from https://docs.zephyrproject.org/latest/getting_started/index.html
2. Use the local Zephyr workspace in `zephyrproject/` and Python environment in `rtos/`.
3. From the project directory, install the Python build dependencies:

```powershell
.\rtos\Scripts\python.exe -m pip install -r .\zephyrproject\zephyr\scripts\requirements-base.txt
```

## Building the Project

Run these commands from the project directory. West commands run inside the local workspace:

```powershell
Set-Location .\zephyrproject
..\rtos\Scripts\west.exe blobs fetch hal_infineon --allow-regex '.*43439A0\.(bin|clm_blob)$'
..\rtos\Scripts\west.exe build -b rpi_pico2/rp2350a/m33/w .. -d ..\build_pico2w --pristine
```

Fetch the firmware blobs once per workspace. The application currently only prints a greeting; it does not enable Wi-Fi networking.

The board target selects the processor, clock setup, and UART pins. Do not set `CONFIG_BOARD` in `prj.conf`.

The local Zephyr checkout has an onboard LED driver fix preserved in `patches/cyw43-gpioout.patch`. It is already applied here. After creating a fresh checkout of the same Zephyr revision, apply it from the project root before building:

```powershell
git -C .\zephyrproject\zephyr apply ..\..\patches\cyw43-gpioout.patch
```

## Flashing

Hold BOOTSEL while connecting the Pico 2 W over USB, then copy `build_pico2w/zephyr/zephyr.uf2` from the project directory to the board's USB mass-storage drive.

## Usage

After flashing, the Pico 2 W prints "Hello World from Zephyr on Raspberry Pi Pico 2 W!" to UART0 at 115200 baud. Use a 3.3 V USB-to-UART adapter with its RX connected to GP0 (board TX), its TX connected to GP1 (board RX), and a shared GND. The current configuration uses the UART pins for the console, not USB serial.

## Troubleshooting

- Ensure the Zephyr SDK is properly installed and the environment is set up
- Check that the Raspberry Pi Pico 2 W is in BOOTSEL mode for UF2 flashing
- Build with `-b rpi_pico2/rp2350a/m33/w`
- If Python modules are missing, install the dependencies listed above
- If CMake reports a missing `GNUARMEMB_TOOLCHAIN_PATH`, install the Zephyr SDK and select it in the current PowerShell session before rebuilding:

```powershell
$env:ZEPHYR_TOOLCHAIN_VARIANT = "zephyr"
Remove-Item Env:GNUARMEMB_TOOLCHAIN_PATH -ErrorAction SilentlyContinue
```

If the SDK is installed outside the standard discovery locations, set `ZEPHYR_SDK_INSTALL_DIR` to its installation directory as well.
