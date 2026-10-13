#ifndef WIFI_CONNECTION_H
#define WIFI_CONNECTION_H

/**
 * @brief Manage the WPA2 connection, DHCP, and reconnection after a disconnect.
 * @param ssid Network name containing 1 to 32 bytes; an empty string disables connection attempts.
 * @param password WPA2 passphrase containing 8 to 63 bytes.
 * @return -EINVAL for a NULL argument; 0 for invalid credentials or a missing interface.
 * @note Call only once in a dedicated thread. Blocks with a valid configuration;
 *       both strings must remain valid for the entire call.
 */
int wifi_run(const char *ssid, const char *password);

#endif
