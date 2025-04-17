#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <uv.h>

#include "../include/ipc.h"

#ifdef MOCK_DEBUG
#include "../tests/mock_debug.h"
#else
#include "../include/debug.h"
#endif

#include "../include/config.h"

#define IPC_SOCKET_PATH "/tmp/belvedere.sock"
#define MAX_CMD_LENGTH 256

// Global variables
static uv_pipe_t server;
static uv_pipe_t client;
static bool server_running = false;

// Forward declarations
static void on_new_connection(uv_stream_t* server, int status);
static void on_read(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf);
static void on_write(uv_write_t* req, int status);
static void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
static void on_close(uv_handle_t* handle);

// Initialize IPC server
int ipc_init(ipc_server_t* server, uv_loop_t* loop) {
    if (!server || !loop) {
        return -1;
    }

    server->loop = loop;
    server->is_running = 0;

    // Initialize the pipe server
    int result = uv_pipe_init(loop, &server->server, 0);
    if (result != 0) {
        return -1;
    }

    return 0;
}

// Start IPC server
int ipc_start(ipc_server_t* server) {
    if (!server || !server->loop) {
        return -1;
    }

    // Remove existing socket file if it exists
    unlink(IPC_SOCKET_PATH);

    // Bind to the socket path
    int result = uv_pipe_bind(&server->server, IPC_SOCKET_PATH);
    if (result != 0) {
        return -1;
    }

    // Start listening for connections
    result = uv_listen((uv_stream_t*)&server->server, 128, on_new_connection);
    if (result != 0) {
        return -1;
    }

    server->is_running = 1;
    return 0;
}

// Stop IPC server
void ipc_stop(ipc_server_t* server) {
    if (!server) {
        return;
    }

    if (server->is_running) {
        uv_close((uv_handle_t*)&server->server, NULL);
        server->is_running = 0;
    }
}

// Cleanup IPC server
void ipc_cleanup(ipc_server_t* server) {
    if (!server) {
        return;
    }

    ipc_stop(server);
    unlink(IPC_SOCKET_PATH);
}

// Process IPC command
int ipc_process_command(ipc_server_t* server, const char* command, char* response, size_t response_size) {
    if (!command || !response || response_size == 0) {
        return -1;
    }

    // For now, just return "OK" for any command
    strncpy(response, "OK", response_size);
    response[response_size - 1] = '\0';
    return 0;
}

// Callback for new connections
static void on_new_connection(uv_stream_t* server, int status) {
    if (status < 0) {
        debugf(stderr, "New connection error: %s\n", uv_strerror(status));
        return;
    }

    uv_pipe_t* client = (uv_pipe_t*)malloc(sizeof(uv_pipe_t));
    uv_pipe_init(server->loop, client, 0);

    if (uv_accept(server, (uv_stream_t*)client) == 0) {
        uv_read_start((uv_stream_t*)client, alloc_buffer, on_read);
    } else {
        uv_close((uv_handle_t*)client, on_close);
    }
}

// Callback for reading data
static void on_read(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf) {
    if (nread < 0) {
        if (nread != UV_EOF) {
            debugf(stderr, "Read error: %s\n", uv_strerror(nread));
        }
        uv_close((uv_handle_t*)client, on_close);
        free(buf->base);
        return;
    }

    if (nread == 0) {
        free(buf->base);
        return;
    }

    // Process command
    char cmd[MAX_CMD_LENGTH];
    strncpy(cmd, buf->base, nread);
    cmd[nread] = '\0';

    // Get response
    char response[512];
    int result = ipc_process_command(NULL, cmd, response, sizeof(response));

    // Prepare response message
    char response_msg[512];
    snprintf(response_msg, sizeof(response_msg), "%s\n%s\n",
             result == 0 ? "OK" : "ERROR",
             response);

    // Send response
    uv_buf_t response_buf = uv_buf_init(strdup(response_msg), strlen(response_msg));
    uv_write_t* write_req = (uv_write_t*)malloc(sizeof(uv_write_t));
    uv_write(write_req, client, &response_buf, 1, on_write);

    free(buf->base);
}

// Callback for writing data
static void on_write(uv_write_t* req, int status) {
    if (status < 0) {
        debugf(stderr, "Write error: %s\n", uv_strerror(status));
    }
    free(req->data);
    free(req);
}

// Buffer allocation callback
static void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    buf->base = (char*)malloc(suggested_size);
    buf->len = suggested_size;
}

// Callback for closing handles
static void on_close(uv_handle_t* handle) {
    free(handle);
}