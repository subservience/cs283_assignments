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

Built_In_Cmds match_command(const char *input) {
    if (strcmp(input, "cd") == 0) return BI_CMD_CD;
    if (strcmp(input, "exit") == 0) return BI_CMD_EXIT;
    return BI_NOT_BI;
}

int exec_local_cmd_loop() {
    char cmd_buff[SH_CMD_MAX];  
    cmd_buff_t cmd;

    while (1) {
        printf("%s", SH_PROMPT);
        fflush(stdout);  

        if (fgets(cmd_buff, SH_CMD_MAX, stdin) == NULL) {
            printf("\n");
            break; 
        }

        cmd_buff[strcspn(cmd_buff, "\n")] = '\0'; 

        if (strcmp(cmd_buff, EXIT_CMD) == 0) {
            exit(OK);
        }

        memset(&cmd, 0, sizeof(cmd));
        if (build_cmd_buff(cmd_buff, &cmd) != OK) {
            printf("%s\n", CMD_WARN_NO_CMD);
            continue;
        }

        Built_In_Cmds bi_cmd = match_command(cmd.argv[0]);
        if (bi_cmd == BI_CMD_CD) {
            if (cmd.argc > 1) {
                if (chdir(cmd.argv[1]) != 0) {
                    perror("cd");
                }
            }
            continue;
        } else if (bi_cmd == BI_CMD_EXIT) {
            exit(OK);
        }

        int exit_status = exec_cmd(&cmd);

	if (exit_status != 0) {
            exit(exit_status);
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

