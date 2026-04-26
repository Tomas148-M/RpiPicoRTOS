# Zephyr RTOS Project for Raspberry Pi Pico

This is a basic Zephyr real-time operating system application set up for the Raspberry Pi Pico microcontroller.

## Project Structure

- `src/`: Source code files
- `include/`: Header files
- `boards/`: Custom board definitions (if any)
- `dts/`: Device tree source files and overlays
- `CMakeLists.txt`: CMake build configuration
- `prj.conf`: Zephyr configuration file

## Prerequisites

1. Install the Zephyr SDK from https://docs.zephyrproject.org/latest/getting_started/index.html
2. Set up the Zephyr development environment
3. Ensure `ZEPHYR_BASE` environment variable points to your Zephyr installation

## Building the Project

1. Navigate to the project directory
2. Run `west build` to build the application
3. Run `west flash` to flash it to the Raspberry Pi Pico

## Usage

After flashing, the Pico will run the main function which prints "Hello World from Zephyr on Raspberry Pi Pico!" to the console.

## Troubleshooting

- Ensure the Zephyr SDK is properly installed and the environment is set up
- Check that the Raspberry Pi Pico is in bootloader mode for flashing
- Verify the board configuration in `prj.conf`