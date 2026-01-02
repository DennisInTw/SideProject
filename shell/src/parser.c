#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "parser.h"


/*  將輸入的指令字串進行前處理,分割出token
    輸入: cmd_str
    輸出: 分割好的token
*/
command_t parse_command(char* cmd_str)
{
    int arg_index = 0;
    command_t cmd = {0};
    char* token;

    // 複製字串到另一塊記憶體空間
    char* cmd_copy = strdup(cmd_str);

    // 使用分隔符[' ','\t','\n']將字串切割
    token = strtok(cmd_copy, " \t\n");

    while (token != NULL && arg_index < MAX_ARGS-1) {
        if (strcmp(token, "<") == 0) {
            token = strtok(NULL, " \t\n");
            if (token) cmd.input_file = strdup(token);
        } else if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " \t\n");
            if (token) {
                cmd.output_file = strdup(token);
                cmd.append_output = 0;
            }
        } else if (strcmp(token, ">>") == 0) {
            token = strtok(NULL, " \t\n");
            if (token) {
                cmd.output_file = strdup(token);
                cmd.append_output = 1;
            }
        } else if (strcmp(token, "&") == 0) {
            cmd.background = 1;
        } else {
            cmd.args[arg_index++] = strdup(token);
        }

        // 取得下一個token
        token = strtok(NULL, " \t\n");
    }

    cmd.args[arg_index] = NULL;   // execvp要求以NULL結尾
    free(cmd_copy);
    return cmd;
}

pipeline_t parse_pipeline(char* input)
{
    pipeline_t pipeline = {0};
    char* cmd_str;
    char* saveptr;

    cmd_str = strtok_r(input, "|", &saveptr);
    while (cmd_str != NULL && pipeline.num_commands < MAX_PIPES) {
        // 去除頭尾的space
        while (isspace(*cmd_str)) cmd_str++;
        char* end = cmd_str + strlen(cmd_str) - 1;
        while (end > cmd_str && isspace(*end)) *end-- = 0;

        if (strlen(cmd_str) > 0) {
            pipeline.commands[pipeline.num_commands++] = parse_command(cmd_str);
        }
        cmd_str = strtok_r(NULL, "|", &saveptr);
    }
    return pipeline;
}