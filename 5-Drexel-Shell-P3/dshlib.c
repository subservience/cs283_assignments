#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "dshlib.h"

int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff) {
    if (!cmd_line || !cmd_buff) return ERR_MEMORY;

    cmd_buff->argc = 0;
    char *token = strtok(cmd_line, " ");
    while (token && cmd_buff->argc < CMD_ARGV_MAX - 1) {
        cmd_buff->argv[cmd_buff->argc++] = token;
        token = strtok(NULL, " ");
    }
    cmd_buff->argv[cmd_buff->argc] = NULL;
    return (cmd_buff->argc > 0) ? OK : WARN_NO_CMDS;
}

int build_cmd_list(char *cmd_line, command_list_t *clist) {
    if (!cmd_line || !clist) return ERR_MEMORY;

    clist->num = 0;  

    char *saveptr;
    char *token = strtok_r(cmd_line, PIPE_STRING, &saveptr);

    while (token && clist->num < CMD_MAX) {
        
        while (*token == ' ') token++;  
        char *end = token + strlen(token) - 1;
        while (end > token && *end == ' ') end--;  
        *(end + 1) = '\0';

        if (strlen(token) == 0) {
            
            token = strtok_r(NULL, PIPE_STRING, &saveptr);
            continue;
        }

        if (build_cmd_buff(token, &clist->commands[clist->num]) != OK) {
            return ERR_CMD_ARGS_BAD;
        }

        clist->num++;
        token = strtok_r(NULL, PIPE_STRING, &saveptr);
    }

    return (clist->num > 0) ? OK : WARN_NO_CMDS;
}

int execute_pipeline(command_list_t *clist) {
    int num_commands = clist->num;
    int pipefds[2 * (num_commands - 1)];
    pid_t pids[num_commands];

   
    for (int i = 0; i < num_commands - 1; i++) {
        if (pipe(pipefds + 2 * i) < 0) {
            perror("pipe failed");
            return ERR_EXEC_CMD;
        }
    }

   
    for (int i = 0; i < num_commands; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork failed");
            return ERR_EXEC_CMD;
        }

        if (pids[i] == 0) {
           
            if (i > 0) {
                dup2(pipefds[2 * (i - 1)], STDIN_FILENO);
            }

           
            if (i < num_commands - 1) {
                dup2(pipefds[2 * i + 1], STDOUT_FILENO);
            }

            
            for (int j = 0; j < 2 * (num_commands - 1); j++) {
                close(pipefds[j]);
            }

            
            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            perror("execvp failed");
            exit(250);
        }
    }

    
    for (int i = 0; i < 2 * (num_commands - 1); i++) {
        close(pipefds[i]);
    }

    
    for (int i = 0; i < num_commands; i++) {
        int status;
        waitpid(pids[i], &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            return WEXITSTATUS(status);
        }
    }

    return OK;
}

int exec_cmd(cmd_buff_t *cmd) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return ERR_EXEC_CMD;
    } else if (pid == 0) {
        execvp(cmd->argv[0], cmd->argv);
        perror("execvp failed");
        exit(250);
    } else {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else {
            return -1;
        }
    }
}

int exec_local_cmd_loop() {
    char cmd_buff[SH_CMD_MAX];
    command_list_t clist;

    while (1) {
        
        printf("%s", SH_PROMPT);
        fflush(stdout);

        if (fgets(cmd_buff, SH_CMD_MAX, stdin) == NULL) {
            printf("\n");
            break;
        }

        
        cmd_buff[strcspn(cmd_buff, "\n")] = '\0';

        if (strcmp(cmd_buff, EXIT_CMD) == 0) {
            break;
        }

        
        memset(&clist, 0, sizeof(clist));

        
        if (build_cmd_list(cmd_buff, &clist) != OK) {
            printf("%s\n", CMD_WARN_NO_CMD);
            continue;
        }

        
        if (clist.num > 1) {
            execute_pipeline(&clist);
        } else {
            exec_cmd(&clist.commands[0]);
        }
    }

    return OK;
}
