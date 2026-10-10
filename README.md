# Pico 2 W Wi-Fi

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
