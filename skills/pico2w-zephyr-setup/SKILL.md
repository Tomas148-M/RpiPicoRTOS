---
name: pico2w-zephyr-setup
description: Build and troubleshoot this Windows Zephyr project for Raspberry Pi Pico 2 W, including workspace discovery, Python dependencies, SDK and 7-Zip setup, UF2 flashing, and UART console access. Use for this project's Pico 2 W setup or the matching errors recorded here.
---

# Raspberry Pi Pico 2 W: Setup and Troubleshooting

Use this guide when supporting this project's Windows PowerShell build and flashing workflow. Inspect the current environment before repeating installation or changing configuration. Adapt paths if the project has moved. Preserve unrelated user changes; do not install tools or flash hardware solely because this skill was loaded.

## Project Context

- Project root: `E:\_UdemyProjectLearning\RpiPicoRTOS`
- Zephyr workspace: `zephyrproject/` (contains `.west/`)
- Python environment and West executable: `rtos/Scripts/`
- Intended SDK directory: `zephyr-sdk/` in the project root
- Build directory: `build_pico2w/` in the project root
- Board: Raspberry Pi Pico 2 W, RP2350 Cortex-M33
- Zephyr board target: `rpi_pico2/rp2350a/m33/w`
- Application: `src/main.c`, currently prints a greeting through UART0

The user reported a successful upload during this conversation. Console output and operation on hardware have not been independently verified. Do not claim a successful build or hardware test without evidence from the current run.

## Configuration Corrections Applied

The original `prj.conf` used Pico 1 settings and obsolete symbols. These were removed:

| Original setting | Correction and reason |
| --- | --- |
| `CONFIG_BOARD=rpi_pico` | Select Pico 2 W with the West `-b` argument instead. |
| `CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC=125000000` | Let the RP2350 board and SoC configuration provide the clock settings. |
| `CONFIG_UART_0=y` | Remove the obsolete symbol; UART0 is enabled in the board devicetree. |
| `CONFIG_KERNEL_LOGGER=y` | Remove the obsolete symbol; retain `CONFIG_LOG=y`. |

Retain the application's heap, thread priority, GPIO, logging, and UART console options unless the task requires changing them. The greeting and README were updated to name Pico 2 W.

## Error: West Does Not Recognize `build`

Observed error:

```text
west: unknown command "build"; do you need to run this inside a workspace?
```

The project root is outside the local West workspace. Run West inside `zephyrproject`. From the project root:

```powershell
Set-Location .\zephyrproject
..\rtos\Scripts\west.exe build -b rpi_pico2/rp2350a/m33/w .. -d ..\build_pico2w --pristine
```

Inside `zephyrproject`, the executable path starts with `..\rtos`, not `.\rtos`. The application path `..` points back to the project root. Using `-z` alone from outside the workspace did not resolve extension-command discovery in this setup.

## Error: Missing `jsonschema` or Other Python Dependencies

Observed configuration failure: `Missing jsonschema dependency`.

Install dependencies into the project's Python environment. From the project root:

```powershell
.\rtos\Scripts\python.exe -m pip install -r .\zephyrproject\zephyr\scripts\requirements-base.txt
```

Use this interpreter rather than a different system Python. During the chat, installation first failed with network access denied (`WinError 10013`) inside the agent sandbox and succeeded after network access was approved. If that failure recurs during an agent run, use the normal escalation mechanism; it is not evidence that the package is unavailable.

## Error: Missing GNU Arm Toolchain Directory

Observed error:

```text
Nothing found at GNUARMEMB_TOOLCHAIN_PATH:
'C:\Program Files (x86)\GNU Arm Embedded Toolchain\10 2021.10'
```

The environment selected `gnuarmemb` and referenced a nonexistent compiler directory. Check for an existing valid SDK or compiler first. If installing the Zephyr SDK is needed, run from `zephyrproject`:

```powershell
..\rtos\Scripts\west.exe sdk install --install-dir ..\zephyr-sdk --gnu-toolchains arm-zephyr-eabi
```

This selects the SDK version required by the local Zephyr checkout and installs the Arm compiler. Avoid hardcoding an SDK version from this conversation.

After installation succeeds, select it in the PowerShell session used for building:

```powershell
$env:ZEPHYR_TOOLCHAIN_VARIANT = "zephyr"
$env:ZEPHYR_SDK_INSTALL_DIR = (Resolve-Path ..\zephyr-sdk).Path
Remove-Item Env:GNUARMEMB_TOOLCHAIN_PATH -ErrorAction SilentlyContinue
```

Use the actual SDK directory if the installer selected a different existing installation. These changes affect only the current terminal. A new terminal may inherit the old persistent environment settings again.

## Error: SDK Cannot Extract `.7z` or Cannot Find `7z`

Observed messages:

```text
patoolib.util.PatoolError: could not find an executable program to extract format 7z
Zephyr SDK setup requires '7z' to be installed and available in the PATH.
```

The download can succeed while extraction fails. The warning about a missing `file` executable is harmless when patool can detect the archive format from its extension.

Check whether 7-Zip is installed:

```powershell
Test-Path "C:\Program Files\7-Zip\7z.exe"
Get-Command 7z -ErrorAction SilentlyContinue
```

If missing, install it using Windows Package Manager, or the official installer at https://www.7-zip.org/download.html:

```powershell
winget install --id 7zip.7zip --exact --source winget
```

After installation succeeds, add its actual installation directory to the current terminal's PATH. For the standard location:

```powershell
$env:Path = "C:\Program Files\7-Zip;$env:Path"
Get-Command 7z
```

The command must resolve `7z.exe` in the same terminal used to run SDK setup. Then repeat the SDK installation command above. Install 7-Zip before retrying if `Test-Path` returns `False`; adding a nonexistent directory to PATH cannot fix extraction.

## Message: Windows Host Tools Are Skipped

```text
SKIPPED: Windows host tools are not available yet.
```

This is informational for the SDK setup used here. It does not by itself mean the Arm compiler installation failed. Check the final installation result, select the SDK environment, and try the application build. Investigate any later missing-tool errors separately.

## Firmware Setup and Build

From `zephyrproject`, fetch the CYW43439 firmware blobs once per workspace, then build:

```powershell
..\rtos\Scripts\west.exe blobs fetch hal_infineon --allow-regex '.*43439A0\.(bin|clm_blob)$'
..\rtos\Scripts\west.exe build -b rpi_pico2/rp2350a/m33/w .. -d ..\build_pico2w --pristine
```

The regex starts with `.*` so it matches the directory prefixes in blob paths. This downloads the CYW43439 firmware and matching CLM files without fetching every Infineon blob. The local board documentation requests these files for Pico 2 W builds. Fetching firmware does not enable networking in the current greeting application.

Check that the build finishes successfully and produces `build_pico2w/zephyr/zephyr.uf2` before flashing. An SDK installation finishing is not equivalent to an application build finishing.

## Flash the Board with UF2

1. Disconnect the Pico 2 W from USB.
2. Hold BOOTSEL while reconnecting USB with a data-capable cable.
3. Release BOOTSEL when the `RP2350` mass-storage drive appears.
4. Copy `E:\_UdemyProjectLearning\RpiPicoRTOS\build_pico2w\zephyr\zephyr.uf2` onto that drive.

The board should restart automatically after the copy. Use the Pico 2 W build's UF2 file, not an SDK archive or an artifact for another board. Do not prescribe `west flash` for this BOOTSEL workflow; it may require a separately configured debugger or runner.

## Read the UART Greeting

The current console is UART0 on the header pins, not USB serial. Use a 3.3 V USB-to-UART adapter with the board powered through USB:

| Pico 2 W connection | Adapter connection |
| --- | --- |
| GP0, physical pin 1, board TX | RX |
| GP1, physical pin 2, board RX | TX |
| GND, physical pin 3 | GND |

Open the adapter's COM port at 115200 baud, 8 data bits, no parity, 1 stop bit, and no flow control. Open the terminal before restarting the board because the greeting prints only once:

```text
Hello World from Zephyr on Raspberry Pi Pico 2 W!
```

Do not infer a failed upload solely from the absence of a USB serial port or from missing a one-time greeting.
