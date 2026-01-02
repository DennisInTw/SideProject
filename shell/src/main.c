#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "parser.h"
#include "executor.h"


void execute_command(char* input)
{
    // 檢查空的command
    if (input == NULL || strlen(input) == 0) {
        return;
    }

    if (strchr(input, '|') != NULL) {
        pipeline_t pipeline = parse_pipeline(input);

        if (pipeline.num_commands == 1) {
            if (is_builtins(&pipeline.commands[0]) == 0) {
                execute_single_command(&pipeline.commands[0]);
            }
        } else {
            execute_pipeline(&pipeline);
        }
    } else { // 只有單一個命令
        command_t cmd = parse_command(input);

        // 0: 不是內建的指令
        if (is_builtins(&cmd) == 0) {
            execute_single_command(&cmd);
        }

        /*
        printf("Input:\n");
        for (int i = 0; cmd.args[i] != NULL; i++) {
            printf("%s\n", cmd.args[i]);
        }
        */
        
        // 釋放記憶體
        for (int i = 0; cmd.args[i] != NULL; i++) {
            free(cmd.args[i]);
        }
        if (cmd.input_file) free(cmd.input_file);
        if (cmd.output_file) free(cmd.output_file);
    }
}

int main()
{
    char* input = NULL;
    size_t len = 0;

    while (1) {
        printf("myshell> ");

        // getline 會自動分配/重新分配記憶體來儲存輸入的字串
        if (getline(&input, &len, stdin) == -1) {
            // 遇到Ctrl-C則跳出
            break;
        }

        // 移除最後的換行'\n'
        // 遇到 '\n'時停止並返回此長度
        input[strcspn(input, "\n")] = 0;

        execute_command(input);
    }

    free(input);
    return 0;
}