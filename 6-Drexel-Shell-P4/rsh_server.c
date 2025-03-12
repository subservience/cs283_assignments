#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <fcntl.h>

#include "dshlib.h"
#include "rshlib.h"

/*
 * start_server(ifaces, port, is_threaded)
 *      ifaces:        The network interface address to bind to.
 *      port:          The port number to listen on.
 *      is_threaded:   Whether to run in threaded mode (not used in this version).
 * 
 * This function starts the remote shell server.
 */
int start_server(char *ifaces, int port, int is_threaded) {
    (void)is_threaded; // Explicitly mark the parameter as unused
    int svr_socket;
    int rc;

    svr_socket = boot_server(ifaces, port);
    if (svr_socket < 0) {
        return svr_socket; // Return error code
    }

    rc = process_cli_requests(svr_socket);

    stop_server(svr_socket);

    return rc;
}

/*
 * stop_server(svr_socket)
 *      svr_socket: The server socket to close.
 * 
 * This function stops the server by closing its socket.
 */
int stop_server(int svr_socket) {
    return close(svr_socket);
}

/*
 * boot_server(ifaces, port)
 *      ifaces:  The network interface address to bind to.
 *      port:    The port number to listen on.
 * 
 * This function initializes and starts the server socket.
 */
int boot_server(char *ifaces, int port) {
    int svr_socket;
    int ret;
    struct sockaddr_in addr;

    svr_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (svr_socket < 0) {
        perror("socket");
        return ERR_RDSH_COMMUNICATION;
    }

    int enable = 1;
    setsockopt(svr_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ifaces);

    ret = bind(svr_socket, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        perror("bind");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    ret = listen(svr_socket, 20);
    if (ret < 0) {
        perror("listen");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    return svr_socket;
}

/*
 * process_cli_requests(svr_socket)
 *      svr_socket: The main server socket.
 * 
 * This function accepts incoming client connections and processes their requests.
 */
int process_cli_requests(int svr_socket) {
    int cli_socket;
    int rc = OK;

    while (1) {
        cli_socket = accept(svr_socket, NULL, NULL);
        if (cli_socket < 0) {
            perror("accept");
            return ERR_RDSH_COMMUNICATION;
        }

        rc = exec_client_requests(cli_socket);
        if (rc == OK_EXIT) {
            break;
        }
    }

    stop_server(svr_socket);
    return rc;
}

/*
 * exec_client_requests(cli_socket)
 *      cli_socket: The client socket.
 * 
 * This function reads commands from the client, executes them, and returns results.
 */
int exec_client_requests(int cli_socket) {
    char *io_buff = malloc(RDSH_COMM_BUFF_SZ);
    command_list_t cmd_list;
    int rc;

    if (!io_buff) {
        return ERR_RDSH_SERVER;
    }

    while (1) {
        rc = recv(cli_socket, io_buff, RDSH_COMM_BUFF_SZ, 0);
        if (rc <= 0) {
            free(io_buff);
            return ERR_RDSH_COMMUNICATION;
        }

        io_buff[rc] = '\0';
        printf("Received command: %s\n", io_buff);

        if (strcmp(io_buff, EXIT_CMD) == 0) {
    	    printf("Client exiting...\n");  // Log server-side exit message
    	    send_message_string(cli_socket, "Client exiting...\n"); // Notify client
    	    send_message_eof(cli_socket); // Signal end of message
    	    free(io_buff);
    	    return OK;
        }

        if (strcmp(io_buff, "stop-server") == 0) {
            printf("Stopping server...\n");
            send_message_string(cli_socket, "Stopping server\n");
            send_message_eof(cli_socket);
            free(io_buff);
            return OK_EXIT;
        }

        rc = build_cmd_list(io_buff, &cmd_list);
        if (rc != OK) {
            send_message_string(cli_socket, CMD_WARN_NO_CMD);
            send_message_eof(cli_socket);
            continue;
        }

        rc = rsh_execute_pipeline(cli_socket, &cmd_list);
        if (rc != OK) {
            send_message_string(cli_socket, "error: command execution failed\n");
            send_message_eof(cli_socket);
            continue;
        }

        send_message_eof(cli_socket);
    }

    free(io_buff);
    return OK;
}

/*
 * send_message_eof(cli_socket)
 *      cli_socket: The client socket.
 * 
 * This function sends the EOF character to the client to indicate the end of a response.
 */
int send_message_eof(int cli_socket) {
    int send_len = (int)sizeof(RDSH_EOF_CHAR);
    int sent_len = send(cli_socket, &RDSH_EOF_CHAR, send_len, 0);

    if (sent_len != send_len) {
        return ERR_RDSH_COMMUNICATION;
    }
    return OK;
}

/*
 * send_message_string(cli_socket, buff)
 *      cli_socket: The client socket.
 *      buff:       The message to send.
 * 
 * This function sends a string message to the client and ensures it ends with a newline.
 */
int send_message_string(int cli_socket, char *buff) {
    strcat(buff, "\n");
    int send_len = strlen(buff) + 1;
    int sent_len = send(cli_socket, buff, send_len, 0);

    if (sent_len != send_len) {
        return ERR_RDSH_COMMUNICATION;
    }
    return OK;
}

/*
 * rsh_execute_pipeline(cli_socket, clist)
 *      cli_socket: The client socket.
 *      clist:      The list of commands to execute.
 * 
 * This function executes a command pipeline and sends results to the client.
 */
int rsh_execute_pipeline(int cli_sock, command_list_t *clist) {
    int pipes[clist->num - 1][2];
    pid_t pids[clist->num];
    int exit_code;

    // Create pipes for communication between commands
    for (int i = 0; i < clist->num - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return ERR_RDSH_COMMUNICATION;
        }
    }

    // Fork and execute each command in the pipeline
    for (int i = 0; i < clist->num; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            return ERR_RDSH_COMMUNICATION;
        }

        if (pids[i] == 0) { // Child process
            // Redirect input for commands after the first
            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO); // Read from previous pipe
            }

            // Redirect output for commands before the last
            if (i < clist->num - 1) {
                dup2(pipes[i][1], STDOUT_FILENO); // Write to next pipe
            } else {
                // For the last command, send output to the client socket
                dup2(cli_sock, STDOUT_FILENO);  // Send final output to client
                dup2(cli_sock, STDERR_FILENO);  // Ensure errors go to client
            }

            // Close all pipe file descriptors in the child process
            for (int j = 0; j < clist->num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Execute the command
            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            perror("execvp failed"); // If execvp fails
            exit(250); // Exit with an error code
        }
    }

    // Parent process: close all pipe file descriptors
    for (int i = 0; i < clist->num - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all child processes to complete
    for (int i = 0; i < clist->num; i++) {
        waitpid(pids[i], &exit_code, 0);
    }

    // Return the exit code of the last command in the pipeline
    return WEXITSTATUS(exit_code);
}
