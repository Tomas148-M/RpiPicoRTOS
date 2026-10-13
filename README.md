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

Replace the example IP with your Pico's address. Add entries to `api_routes`
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
