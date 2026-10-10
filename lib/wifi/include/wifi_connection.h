#ifndef WIFI_CONNECTION_H
#define WIFI_CONNECTION_H

/* Run the WPA2/DHCP connection and reconnection loop in the calling thread.
 * Blocks while configured; call once. Strings must remain valid for this call.
 * An empty SSID disables Wi-Fi connection attempts.
 */
int wifi_run(const char *ssid, const char *password);

#endif
