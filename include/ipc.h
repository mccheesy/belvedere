#ifndef IPC_H
#define IPC_H

#include <uv.h>

// IPC server structure
typedef struct {
    uv_pipe_t server;
    uv_loop_t* loop;
    int is_running;
} ipc_server_t;

// Initialize the IPC server
int ipc_init(ipc_server_t* server, uv_loop_t* loop);

// Start the IPC server
int ipc_start(ipc_server_t* server);

// Stop the IPC server
void ipc_stop(ipc_server_t* server);

// Clean up the IPC server
void ipc_cleanup(ipc_server_t* server);

// Process a command received from a client
int ipc_process_command(ipc_server_t* server, const char* command, char* response, size_t response_size);

#endif // IPC_H