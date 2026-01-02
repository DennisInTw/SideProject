#ifndef EXECUTOR_H
#define EXECUTOR_H

int execute_single_command(command_t* cmd);
int execute_pipeline(pipeline_t* pipeline);

#endif