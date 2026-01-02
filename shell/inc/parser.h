#ifndef PARSER_H
#define PARSER_H

#define MAX_ARGS  (64)
#define MAX_PIPES (10)

typedef struct {
    char* args[MAX_ARGS];   // 命令參數
    char* input_file;       // 輸入重新導向的檔案
    char* output_file;      // 輸出重新導向的檔案
    int append_output;      // 使用 ">>"
    int background;         // 使用 "&"
} command_t;

typedef struct {
    command_t commands[MAX_PIPES];
    int num_commands;
} pipeline_t;

int is_builtins(command_t* cmd);
command_t parse_command(char* cmd_str);
pipeline_t parse_pipeline(char* input);

#endif