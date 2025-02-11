#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "dshlib.h"

int build_cmd_list(char *cmd_line, command_list_t *clist)
{
    if (cmd_line == NULL || strlen(cmd_line) == 0) {
        return WARN_NO_CMDS;  // No input provided
    }

    char *token;
    char *saveptr;
    int count = 0;

    // Tokenize input using '|' as delimiter
    token = strtok_r(cmd_line, PIPE_STRING, &saveptr);
    while (token != NULL) {
        // Trim leading and trailing spaces
        while (*token == SPACE_CHAR) token++; // Trim leading spaces
        char *end = token + strlen(token) - 1;
        while (end > token && *end == SPACE_CHAR) *end-- = '\0'; // Trim trailing spaces

        if (strlen(token) == 0) {
            return WARN_NO_CMDS;  // No valid commands found
        }

        // Check if too many commands
        if (count >= CMD_MAX) {
            return ERR_TOO_MANY_COMMANDS;
        }

        // Extract command and arguments
        char *arg_ptr = strchr(token, SPACE_CHAR);
        if (arg_ptr) {
            *arg_ptr = '\0';  // Split command and arguments
            arg_ptr++;
            strncpy(clist->commands[count].args, arg_ptr, ARG_MAX - 1);
        } else {
            clist->commands[count].args[0] = '\0';  // Ensure empty args string
        }

        strncpy(clist->commands[count].exe, token, EXE_MAX - 1);
        count++;

        token = strtok_r(NULL, PIPE_STRING, &saveptr);
    }

    clist->num = count;
    return OK;
}

