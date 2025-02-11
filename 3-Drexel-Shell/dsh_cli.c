#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dshlib.h"

int main()
{
    char cmd_buff[SH_CMD_MAX];  // Buffer to hold user input
    int rc;
    command_list_t clist;

    while (1) {
        printf("%s", SH_PROMPT);
        fflush(stdout);  // Ensures prompt appears correctly

        // Read input from user
        if (fgets(cmd_buff, SH_CMD_MAX, stdin) == NULL) {
            printf("\n");
            break; // Handle EOF (Ctrl+D)
        }

        // Remove trailing newline
        cmd_buff[strcspn(cmd_buff, "\n")] = '\0';

        // Check for exit command
        if (strcmp(cmd_buff, EXIT_CMD) == 0) {
            exit(OK);
        }

        // Parse input into commands
        memset(&clist, 0, sizeof(clist));  // Clear previous command list
        rc = build_cmd_list(cmd_buff, &clist);

        // Handle parsing errors
        if (rc == WARN_NO_CMDS) {
            printf("%s\n", CMD_WARN_NO_CMD);
        } else if (rc == ERR_TOO_MANY_COMMANDS) {
            printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
        } else if (rc == OK) {
            // Print parsed commands
            printf(CMD_OK_HEADER, clist.num);
            for (int i = 0; i < clist.num; i++) {
                printf("<%d> %s", i + 1, clist.commands[i].exe);
                if (strlen(clist.commands[i].args) > 0) {
                    printf(" [%s]", clist.commands[i].args);
                }
                printf("\n");
            }
        }
    }

    return OK;
}

