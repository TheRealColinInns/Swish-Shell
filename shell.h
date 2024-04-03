/**
 * @file
 *
 * Contains shell history data structures and retrieval functions.
 */
#include <stddef.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#ifndef _SHELL_H_
#define _SHELL_H_

//struct containing info for a background process
struct process {
	char *command;
	pid_t background_pid;
};

//struct containing all info needed to execute a command
struct command_line {
    char **tokens;
    bool stdout_pipe;
    char *stdin_file;
    bool append;
    char *stdout_file;
};

int background_execute(char **, size_t);
void sigchld_handler(int);
void sigint_handler(int);
int construct_pipeline(char **, size_t);
int execute_pipeline(struct command_line *);
int seperate_args(char **, char *, size_t);
char *next_token(char **, const char *);
size_t get_size(size_t, char **);
int execute(char **);
int builtins(char **, size_t, bool);

#endif