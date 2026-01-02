#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "parser.h"

int builtin_exit(char** args)
{
    printf("Goodbye!\n");
    exit(0);
}

int builtin_cd(char** args)
{
    if (args[1] == NULL) {
        fprintf(stderr, "cd: missing argument\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("cd");
        }
    }
    return 1;
}

int builtin_pwd(char** args)
{
    char pwd[1024];
    if (getcwd(pwd, sizeof(pwd)) != NULL) {
        printf("%s\n", pwd);
    } else {
        perror("pwd");
    }
    return 1;
}

    int builtin_help(char** args)
{
    printf("myshell --- a simple shell implementation\n");
    printf("builtin commands:\n");
    printf("    cd, pwd, exit, help\n");
    return 1;
}

int is_builtins(command_t* cmd)
{
    // 刷一排space的情況
    if (cmd->args[0] == NULL) return 0;

    if (strcmp(cmd->args[0], "exit") == 0) {
        return builtin_exit(cmd->args);
    } else if (strcmp(cmd->args[0], "cd") == 0) {
        return builtin_cd(cmd->args);
    } else if (strcmp(cmd->args[0], "pwd") == 0) {
        return builtin_pwd(cmd->args);
    } else if (strcmp(cmd->args[0], "help") == 0) {
        return builtin_help(cmd->args);
    }

    // 不是builtin命令
    return 0;
}
