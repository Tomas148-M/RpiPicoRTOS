#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <wifi_api_server.h>

LOG_MODULE_REGISTER(wifi_api, CONFIG_LOG_DEFAULT_LEVEL);

/**
 * @brief Send the entire buffer, handling partial sends and EINTR interruptions.
 * @param client Connected client socket.
 * @param data Data to send.
 * @param length Number of bytes to send.
 * @return 0 when all bytes are sent; -EIO if a send fails or returns zero.
 */
static int send_all(int client, const char *data, size_t length)
{
    while (length) {
        int sent = zsock_send(client, data, length, 0);
        if (sent < 0 && errno == EINTR) {
            continue;
        }
        if (sent <= 0) {
            return -EIO;
        }
        data += sent;
        length -= sent;
    }
    return 0;
}

/**
 * @brief Build the HTTP response header and send the JSON response body to the client.
 * @param client Connected client socket.
 * @param status HTTP status code.
 * @param reason HTTP status reason phrase.
 * @param body JSON response body.
 * @param length Body length in bytes, excluding the null terminator.
 * @note The body is not sent if header formatting or transmission fails.
 */
static void respond(int client, int status, const char *reason,
                    const char *body, size_t length)
{
    char header[224];
    int size = snprintf(header, sizeof(header),
                        "HTTP/1.1 %d %s\r\nContent-Type: application/json\r\n"
                        "Content-Length: %zu\r\nConnection: close\r\n"
                        "%s\r\n",
                        status, reason, length,
                        status == 405 ? "Allow: GET\r\n" : "");
    if (size > 0 && (size_t)size < sizeof(header) &&
        send_all(client, header, (size_t)size) == 0) {
        (void)send_all(client, body, length);
    }
}

#define ERROR_RESPONSE(code, reason, json) \
    respond(client, code, reason, json, sizeof(json) - 1)

/**
 * @brief Read one HTTP GET request and invoke the handler for the matching route.
 * @param client Connected client socket; the caller is responsible for closing it.
 * @param routes Table of available routes and their handlers.
 * @param count Number of entries in the table.
 * @note Wait up to 5 seconds for request headers and send an HTTP error for invalid requests.
 */
static void serve_client(int client, const struct WifiApiRoute *routes,
                         size_t count)
{
    char request[1024];
    size_t used = 0;
    int64_t deadline = k_uptime_get() + 5000;

    request[0] = '\0';
    while (!strstr(request, "\r\n\r\n")) {
        int64_t remaining = deadline - k_uptime_get();
        if (remaining <= 0) {
            ERROR_RESPONSE(408, "Request Timeout", "{\"error\":\"timeout\"}");
            return;
        }
        struct zsock_pollfd fd = {.fd = client, .events = ZSOCK_POLLIN};
        int ready = zsock_poll(&fd, 1, (int)remaining);
        if (ready < 0 && errno == EINTR) {
            continue;
        }
        if (ready <= 0) {
            ERROR_RESPONSE(408, "Request Timeout", "{\"error\":\"timeout\"}");
            return;
        }
        int received = zsock_recv(client, request + used,
                                  sizeof(request) - used - 1, 0);
        if (received <= 0) {
            return;
        }
        if (memchr(request + used, '\0', (size_t)received)) {
            ERROR_RESPONSE(400, "Bad Request", "{\"error\":\"bad request\"}");
            return;
        }
        used += received;
        request[used] = '\0';
        if (used == sizeof(request) - 1 && !strstr(request, "\r\n\r\n")) {
            ERROR_RESPONSE(431, "Request Header Fields Too Large",
                           "{\"error\":\"headers too large\"}");
            return;
        }
    }

    char *line_end = strstr(request, "\r\n");
    *line_end = '\0';
    char *path = strchr(request, ' ');
    if (!path) {
        ERROR_RESPONSE(400, "Bad Request", "{\"error\":\"bad request\"}");
        return;
    }
    *path++ = '\0';
    char *version = strchr(path, ' ');
    if (!version || path[0] != '/') {
        ERROR_RESPONSE(400, "Bad Request", "{\"error\":\"bad request\"}");
        return;
    }
    *version++ = '\0';
    if (strcmp(version, "HTTP/1.1") && strcmp(version, "HTTP/1.0")) {
        ERROR_RESPONSE(400, "Bad Request", "{\"error\":\"bad version\"}");
        return;
    }
    if (strcmp(request, "GET")) {
        ERROR_RESPONSE(405, "Method Not Allowed", "{\"error\":\"GET required\"}");
        return;
    }
    char *query = strchr(path, '?');
    if (query) {
        *query = '\0';
    }
    for (size_t i = 0; i < count; ++i) {
        if (!strcmp(path, routes[i].path)) {
            char body[WIFI_API_RESPONSE_SIZE];
            int length = routes[i].handler(body, sizeof(body), routes[i].user_data);
            if (length < 0 || (size_t)length >= sizeof(body)) {
                ERROR_RESPONSE(500, "Internal Server Error",
                               "{\"error\":\"handler failed\"}");
            } else {
                respond(client, 200, "OK", body, (size_t)length);
            }
            return;
        }
    }
    ERROR_RESPONSE(404, "Not Found", "{\"error\":\"not found\"}");
}

/**
 * @brief Run a blocking IPv4 HTTP server that handles clients sequentially.
 * @param port Nonzero TCP port to listen on.
 * @param routes Route table that remains valid for the entire server lifetime.
 * @param route_count Nonzero number of routes in the table.
 * @return Negative errno code for invalid configuration or a listener socket failure.
 * @note Run in a dedicated thread; each client connection is closed after handling.
 */
int wifi_api_server_run(uint16_t port, const struct WifiApiRoute *routes,
                        size_t route_count)
{
    if (!port || !routes || !route_count) {
        return -EINVAL;
    }
    for (size_t i = 0; i < route_count; ++i) {
        if (!routes[i].path || routes[i].path[0] != '/' || !routes[i].handler) {
            return -EINVAL;
        }
    }
    int listener = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener < 0) {
        return -errno;
    }
    struct sockaddr_in address = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };
    int reuse = 1;
    (void)zsock_setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    if (zsock_bind(listener, (struct sockaddr *)&address, sizeof(address)) < 0 ||
        zsock_listen(listener, 2) < 0) {
        int error = -errno;
        zsock_close(listener);
        return error;
    }
    LOG_INF("HTTP API listening on port %u", port);
    while (true) {
        int client = zsock_accept(listener, NULL, NULL);
        if (client < 0) {
            if (errno == EINTR) {
                continue;
            }
            int error = -errno;
            zsock_close(listener);
            return error;
        }
        struct zsock_timeval timeout = {.tv_sec = 5};
        if (zsock_setsockopt(client, SOL_SOCKET, SO_SNDTIMEO,
                             &timeout, sizeof(timeout)) < 0) {
            LOG_ERR("Failed to set send timeout: errno=%d", errno);
            zsock_close(client);
            continue;
        }
        if (zsock_setsockopt(client, SOL_SOCKET, SO_RCVTIMEO,
                             &timeout, sizeof(timeout)) < 0) {
            LOG_ERR("Failed to set receive timeout: errno=%d", errno);
            zsock_close(client);
            continue;
        }
        serve_client(client, routes, route_count);
        zsock_close(client);
    }
}
