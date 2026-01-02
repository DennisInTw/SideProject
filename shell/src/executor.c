#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include "parser.h"


int execute_single_command(command_t* cmd)
{
    pid_t pid;
    int status;

    pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return -1;
    }

    if (pid == 0) {   // child process
        // 調整指令的輸入和輸出
        if (cmd->input_file) {
            int fd = open(cmd->input_file, O_RDONLY);
            if (fd < 0) {
                perror("Open input file");
                exit(1);
            }

            // 將child process的stdin改變(不影響parent process的stdin指向的情況,依然是從鍵盤輸入)
            dup2(fd, STDIN_FILENO);

            close(fd);
        }
        if (cmd->output_file) {
            int flags = O_WRONLY | O_CREAT;
            flags |= (cmd->append_output ? O_APPEND : O_TRUNC);
            int fd = open(cmd->output_file, flags, 0644);
            if (fd < 0) {
                perror("Open output file");
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        // 調整好輸入和輸出來源,執行本次的指令
        execvp(cmd->args[0], cmd->args);

        // 看到底下訊息表示出現問題
        fprintf(stderr, "Command not found: %s\n", cmd->args[0]);
        exit(1);
    } else {   // parent process
        if (cmd->background) {
            // 背景執行,不需要等待child
            return status;
        } else {
            // 等待child執行完畢
            waitpid(pid, &status, 0);
        }
    }
    return status;
}

int execute_pipeline(pipeline_t* pipeline)
{
    int num_cmds = pipeline->num_commands;
    int pipes[2];  // 讀取端(0) 和 寫入端(1) 
    int prev_read_fd = STDIN_FILENO;
    pid_t pids[MAX_PIPES];

    if (num_cmds ==0) return 0;

    for (int i = 0; i < num_cmds; i++) {
        // 最後一個指令不需要建立pipe
        if (i < num_cmds-1) {
            if (pipe(pipes) < 0) {
                perror("Pipe failed");
                return -1;
            }
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            return -1;
        }

        if (pid == 0) { // child process
            // 關閉目前的read端,只需要保留write端,將結果輸出就好
            if (i < num_cmds-1) close(pipes[0]);
            
            // 重定向輸入
            if (i > 0) {
                // 將child process的stdin改變,指向上一個指令的讀取端 (不影響parent process的stdin指向的情況,依然是從鍵盤輸入)
                dup2(prev_read_fd, STDIN_FILENO);
                close(prev_read_fd);
            }

            // 重定向輸出
            if (i < num_cmds-1) {
                // 將child process的stdout改變,指向本次指令建立的write端
                dup2(pipes[1], STDOUT_FILENO);
                close(pipes[1]);
            }

            // 調整好輸入和輸出來源,執行本次的指令
            command_t* cmd = &pipeline->commands[i];
            execvp(cmd->args[0], cmd->args);

            // 看到底下訊息表示出現問題
            fprintf(stderr, "Command not found: %s\n", cmd->args[0]);
            exit(1);
        } else { // parent process
            pids[i] = pid;

            // parent process負責將上一個指令的pipe read端關掉, 以及parent process不需要用到write端,所以也是關掉本次pipe的write端
            if (i>0) close(prev_read_fd);
            if (i < num_cmds-1) {
                close(pipes[1]);
                prev_read_fd = pipes[0];
            }

        }
    }
    return 0;
}