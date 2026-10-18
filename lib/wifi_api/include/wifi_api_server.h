#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WIFI_API_RESPONSE_SIZE 512

/**
 * @brief Route handler type that writes a JSON response into the output buffer.
 * @param body Output buffer for null-terminated JSON.
 * @param capacity Buffer capacity in bytes, including the null terminator.
 * @param user_data Context passed from the corresponding WifiApiRoute entry.
 * @return JSON length excluding the null terminator, or a negative error code.
 * @note Handlers run sequentially in the server thread. An error or a length reaching
 *       the buffer capacity produces HTTP 500; success produces HTTP 200.
 */
typedef int (*WifiApiHandler)(char *body, size_t capacity, void *user_data);

struct WifiApiRoute {
    const char *path;
    WifiApiHandler handler;
    void *user_data;
};

/**
 * @brief Run a blocking IPv4 HTTP server for GET requests and JSON responses.
 * @param port Nonzero TCP port to listen on.
 * @param routes Route table; the table, strings, and contexts must remain valid for the entire call.
 * @param route_count Nonzero number of entries in the table.
 * @return Negative errno code for invalid configuration or a listener socket failure.
 * @note Call in a dedicated thread. The server handles one client at a time,
 *       using bounded buffers and a 5-second deadline for reading request headers.
 *       Closes the connection after responding. HTTP without authentication is intended for a trusted local network.
 */
int wifi_api_server_run(uint16_t port, const struct WifiApiRoute *routes,
                        size_t route_count);

#ifdef __cplusplus
}
#endif
