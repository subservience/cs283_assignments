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

int stop_server(int svr_socket) {
    return close(svr_socket);
}

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
            free(io_buff);
            return OK;
        }

        if (strcmp(io_buff, "stop-server") == 0) {
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
	printf("Command output: %s\n", io_buff);
        send_message_eof(cli_socket);
    }

    free(io_buff);
    return OK;
}

int send_message_eof(int cli_socket) {
    int send_len = (int)sizeof(RDSH_EOF_CHAR);
    int sent_len = send(cli_socket, &RDSH_EOF_CHAR, send_len, 0);

    if (sent_len != send_len) {
        return ERR_RDSH_COMMUNICATION;
    }
    return OK;
}

int send_message_string(int cli_socket, char *buff) {
    int send_len = strlen(buff) + 1;
    int sent_len = send(cli_socket, buff, send_len, 0);

    if (sent_len != send_len) {
        return ERR_RDSH_COMMUNICATION;
    }
    return OK;
}

int rsh_execute_pipeline(int cli_sock, command_list_t *clist) {
    (void)cli_sock; // Explicitly mark the parameter as unused (temporary)
    int pipes[clist->num - 1][2];
    pid_t pids[clist->num];
    int exit_code;

    // Create pipes
    for (int i = 0; i < clist->num - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return ERR_RDSH_COMMUNICATION;
        }
    }

    // Fork and execute commands
    for (int i = 0; i < clist->num; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            return ERR_RDSH_COMMUNICATION;
        }

        if (pids[i] == 0) { // Child process
            // Redirect stdin for the first command
            if (i == 0) {
                // TODO: Redirect stdin to cli_sock
            }

            // Redirect stdout for the last command
            if (i == clist->num - 1) {
                // TODO: Redirect stdout and stderr to cli_sock
            }

            // Close all pipe ends
            for (int j = 0; j < clist->num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Execute command
            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            perror("execvp");
            exit(250);
        }
    }

    // Parent process: close all pipe ends
    for (int i = 0; i < clist->num - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all child processes
    for (int i = 0; i < clist->num; i++) {
        waitpid(pids[i], &exit_code, 0);
    }

    return WEXITSTATUS(exit_code);
}
